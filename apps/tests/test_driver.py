# Copyright (c) 2022 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import concurrent.futures
import json
import os
import random
import re
import shlex
import shutil
import stat
import string
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass
from difflib import unified_diff
from typing import Tuple, Union

if os.name == "nt":
    from ctypes import create_unicode_buffer, windll  # type: ignore

    GetLongPathName = windll.kernel32.GetLongPathNameW

    sys.stdout.reconfigure(encoding="utf-8")  # type: ignore


def git_config(name, value):
    subprocess.run(["git", "config", "--global", name, value], capture_output=True)


def ensure_identity_exists():
    proc = subprocess.run(["git", "config", "user.name"], capture_output=True)
    if proc.stdout != b"":
        return
    git_config("user.email", "test_runner@example.com")
    git_config("user.name", "Test Runner")
    git_config("init.defaultBranch", "main")


ensure_identity_exists()

flds = ["Return code", "Standard out", "Standard err"]
streams = ["stdin", "stderr"]

parser = argparse.ArgumentParser()
parser.add_argument("--target", required=True, metavar="EXE")
parser.add_argument("--tests", required=True, metavar="DIR")
parser.add_argument("--data-dir", required=True, metavar="DIR")
parser.add_argument("--version", required=True, metavar="SEMVER")
parser.add_argument("--install", metavar="DIR")
parser.add_argument(
    "--install-with",
    metavar="TOOL",
    type=lambda s: s.split(";"),
    action="append",
    default=[],
)
parser.add_argument(
    "--run",
    metavar="TEST[,...]",
    type=lambda s: [int(x) for x in s.split(",")],
    action="append",
    default=[],
)
args = parser.parse_args()
args.install_with = [item for groups in args.install_with for item in groups]
args.run = [item for groups in args.run for item in groups]
args.data_dir = os.path.abspath(args.data_dir).replace("\\", "/")
args.data_dir_alt = None

target = args.target
target_name = os.path.basename(target)
version = args.version

TEMP = os.path.abspath(os.path.join(tempfile.gettempdir(), "test-driver")).replace(
    "\\", "/"
)
TEMP_ALT = None

os.makedirs(TEMP, exist_ok=True)

if os.name == "nt":
    BUFFER_SIZE = 2048
    buffer = create_unicode_buffer(BUFFER_SIZE)
    GetLongPathName(TEMP, buffer, BUFFER_SIZE)
    TEMP = buffer.value

if os.sep != "/":
    TEMP_ALT = TEMP.replace("/", os.sep)
    args.data_dir_alt = args.data_dir.replace("/", os.sep)
print(TEMP_ALT, TEMP)


def expand(input, tempdir):
    return (
        input.replace("$TMP", tempdir)
        .replace("$DATA", args.data_dir)
        .replace("$VERSION", args.version)
    )


def alt_sep(input, value, var):
    split = input.split(value)
    first = split[0]
    split = split[1:]
    for index in range(len(split)):
        m = re.match(r"(\S+)(\s*.*)", split[index])
        if m is None:
            continue
        g2 = m.group(2)
        if g2 is None:
            g2 = ""
        split[index] = "{}{}".format(m.group(1).replace(os.sep, "/"), g2)
    return var.join([first, *split])


def fix(input, patches, tempdir, tempdir_alt):
    if os.name == "nt":
        input = input.replace(b"\r\n", b"\n")
    input = input.decode("UTF-8")
    input = alt_sep(input, tempdir, "$TMP")
    input = alt_sep(input, args.data_dir, "$DATA")
    input = input.replace(args.version, "$VERSION")

    if tempdir_alt is not None:
        input = alt_sep(input, tempdir_alt, "$TMP")
        input = alt_sep(input, args.data_dir_alt, "$DATA")

    if not len(patches):
        return input

    input = input.split("\n")
    for patch in patches:
        patched = patches[patch]
        ptrn = re.compile(patch)
        for lineno in range(len(input)):
            if ptrn.match(input[lineno]):
                input[lineno] = patched
    return "\n".join(input)


def last_enter(s):
    if len(s) and s[-1] == "\n":
        s = s[:-1] + "\\n"
    return s + "\n"


def diff(expected, actual):
    expected = last_enter(expected).splitlines(keepends=True)
    actual = last_enter(actual).splitlines(keepends=True)
    return "".join(list(unified_diff(expected, actual))[2:])


def touch(test: "Test", args):
    filename = test.path(args[0])
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "wb") as f:
        if len(args) > 1:
            f.write(args[1].encode("UTF-8"))


def git(test: "Test", args):
    subprocess.run(["git", *args], shell=False, cwd=test.cwd)


def cov(test: "Test", aditional):
    subprocess.run([target, *aditional], shell=False, cwd=test.cwd)


file_cache = {}
rw_mask = stat.S_IWRITE | stat.S_IWGRP | stat.S_IWOTH
ro_mask = 0o777 ^ rw_mask


def make_RO(test: "Test", args):
    mode = os.stat(args[0]).st_mode
    file_cache[args[0]] = mode
    os.chmod(test.path(args[0]), mode & ro_mask)


def make_RW(test: "Test", args):
    try:
        mode = file_cache[args[0]]
    except KeyError:
        mode = os.stat(args[0]).st_mode | rw_mask
    os.chmod(test.path(args[0]), mode)


def untar(test: "Test", src, dst):
    with tarfile.open(src) as TAR:

        def is_within_directory(directory, target):
            abs_directory = os.path.abspath(directory)
            abs_target = os.path.abspath(target)

            prefix = os.path.commonprefix([abs_directory, abs_target])

            return prefix == abs_directory

        def safe_extract(tar, path=".", members=None, *, numeric_owner=False):
            for member in tar.getmembers():
                member_path = os.path.join(path, member.name)
                if not is_within_directory(path, member_path):
                    raise Exception("Attempted Path Traversal in Tar File")

            tar.extractall(path, members, numeric_owner=numeric_owner)

        safe_extract(TAR, test.path(dst))


def unzip(test: "Test", src, dst):
    with zipfile.ZipFile(src) as ZIP:
        ZIP.extractall(test.path(dst))


ARCHIVES = {".tar": untar, ".zip": unzip}


def cat(test: "Test", args):
    filename = args[0]
    with open(test.path(filename)) as f:
        sys.stdout.write(f.read())


def unpack(test: "Test", args):
    archive = args[0]
    dst = args[1]
    reminder, ext = os.path.splitext(archive)
    _, mid = os.path.splitext(reminder)
    if mid == ".tar":
        ext = ".tar"
    ARCHIVES[ext](test, archive, dst)


def detach(test: "Test", args):
    dirname = test.path(args[0])
    with open(os.path.join(dirname, "HEAD")) as f:
        link = f.readline().strip()
    print(link)
    if link[:5] != "ref: ":
        return
    link = link[5:]
    with open(os.path.join(dirname, link)) as f:
        link = f.readline().strip()
    with open(os.path.join(dirname, "HEAD"), "w") as f:
        print(link, file=f)


op_types = {
    "mkdirs": (1, lambda test, args: test.makedirs(args[0], exist_ok=True)),
    "rm": (1, lambda test, args: test.rmtree(args[0])),
    "ro": (1, make_RO),
    "rw": (1, make_RW),
    "touch": (1, touch),
    "cd": (1, lambda test, args: test.chdir(args[0])),
    "unpack": (2, unpack),
    "git": (0, git),
    "cov": (0, cov),
    "detach": (1, detach),
    "cat": (1, cat),
}


class Test:
    def __init__(self, data, filename, count):
        self.cwd = os.getcwd()
        self.data = data
        self.ok = True
        self.post_args = []

        renovate = False

        name = os.path.splitext(os.path.basename(filename))[0].split("-")
        if int(name[0]) == count:
            name = name[1:]
        else:
            name[0] = "({})".format(name[0])
        self.name = " ".join(name)

        try:
            call_args, expected = data["args"], data["expected"]
            if isinstance(call_args, str):
                call_args = shlex.split(call_args)
            else:
                data["args"] = shlex.join(call_args)
                renovate = True
            self.args = call_args
            self.expected = expected
        except KeyError:
            self.ok = False
            return

        try:
            post_args = data["post"]
            if isinstance(post_args, str):
                post_args = [post_args]
            for index in range(len(post_args)):
                cmd = post_args[index]
                if isinstance(cmd, str):
                    cmd = shlex.split(cmd)
                else:
                    data["post"][index] = shlex.join(cmd)
                    renovate = True
                self.post_args.append(cmd)

        except KeyError:
            pass

        if self.expected is not None:
            if not isinstance(self.expected[1], str):
                self.expected[1] = "\n".join(self.expected[1])
            if not isinstance(self.expected[2], str):
                self.expected[2] = "\n".join(self.expected[2])

        try:
            self.patches = data["patches"]
        except KeyError:
            self.patches = {}

        self.check = ["all", "all"]
        for stream in range(len(streams)):
            try:
                self.check[stream] = data["check"][streams[stream]]
            except KeyError:
                pass

        try:
            self.prepare = data["prepare"]
            for cmd in self.prepare:
                if not isinstance(cmd, str):
                    for index in range(len(data["prepare"])):
                        if not isinstance(data["prepare"][index], str):
                            data["prepare"][index] = shlex.join(data["prepare"][index])
                    renovate = True
                    break
        except KeyError:
            self.prepare = []

        try:
            self.cleanup = data["cleanup"]
            for cmd in self.cleanup:
                if not isinstance(cmd, str):
                    for index in range(len(data["cleanup"])):
                        if not isinstance(data["cleanup"][index], str):
                            data["cleanup"][index] = shlex.join(data["cleanup"][index])
                    renovate = True
                    break
        except KeyError:
            self.cleanup = []

        try:
            self.lang = data["lang"]
        except KeyError:
            self.lang = "en"

        try:
            self.env = data["env"]
        except KeyError:
            self.env = []

        if renovate:
            data["expected"] = [
                self.expected[0],
                *[to_lines(stream) for stream in self.expected[1:]],
            ]
            with open(filename, "w") as f:
                json.dump(data, f, indent=4)
                print(file=f)

    def run_cmds(self, ops, tempdir):
        for op in ops:
            orig = op
            if isinstance(op, str):
                op = shlex.split(op)
            is_safe = False
            try:
                name = op[0]
                if name[:5] == "safe-":
                    name = name[5:]
                    is_safe = True
                min_args, cb = op_types[name]
                op = op[1:]
                if len(op) < min_args:
                    return None
                cb(self, [expand(o, tempdir) for o in op])
            except Exception as ex:
                if op[0] != "safe-rm":
                    print("Problem while handling", orig)
                    print(ex)
                if is_safe:
                    continue
                return None
        return True

    def run(self, tempdir, tempdir_alt):
        root = os.path.join(
            "build",
            ".testing",
            "".join(random.choice(string.ascii_letters) for _ in range(16)),
        )
        root = self.cwd = os.path.join(self.cwd, root)
        os.makedirs(root, exist_ok=True)

        prep = self.run_cmds(self.prepare, tempdir)
        if prep is None:
            return None

        expanded = [expand(arg, tempdir) for arg in self.args]
        post_expanded = [
            [expand(arg, tempdir) for arg in cmd] for cmd in self.post_args
        ]

        env = {name: os.environ[name] for name in os.environ}
        env["LANGUAGE"] = self.lang
        for key in self.env:
            value = self.env[key]
            if value:
                env[key] = expand(value, tempdir)
            elif key in env:
                del env[key]

        proc = subprocess.run(
            [target, *expanded], capture_output=True, env=env, cwd=self.cwd
        )
        returncode = proc.returncode
        test_stdout = proc.stdout
        test_stderr = proc.stderr

        for sub_expanded in post_expanded:
            if returncode != 0:
                break
            proc = subprocess.run(
                [target, *sub_expanded], capture_output=True, env=env, cwd=self.cwd
            )
            returncode = proc.returncode
            if len(test_stdout) and len(proc.stdout):
                test_stdout += b"\n"
            if len(test_stderr) and len(proc.stderr):
                test_stderr += b"\n"
            test_stdout += proc.stdout
            test_stderr += proc.stderr

        clean = self.run_cmds(self.cleanup, tempdir)
        if clean is None:
            return None

        return [
            returncode,
            fix(test_stdout, self.patches, tempdir, tempdir_alt),
            fix(test_stderr, self.patches, tempdir, tempdir_alt),
        ]

    def clip(self, actual):
        clipped = [*actual]
        for ndx in range(len(self.check)):
            check = self.check[ndx]
            if check != "all":
                if check == "begin":
                    clipped[ndx + 1] = clipped[ndx + 1][: len(self.expected[ndx + 1])]
                elif check == "end":
                    clipped[ndx + 1] = clipped[ndx + 1][-len(self.expected[ndx + 1]) :]
                else:
                    return check
        return clipped

    def report(self, actual, tempdir):
        result = ""
        for ndx in range(len(actual)):
            if actual[ndx] == self.expected[ndx]:
                continue
            if ndx:
                check = self.check[ndx - 1]
                pre_mark = "..." if check == "end" else ""
                post_mark = "..." if check == "begin" else ""
                result += f"""{flds[ndx]}
  Expected:
    {pre_mark}{repr(self.expected[ndx])}{post_mark}
  Actual:
    {pre_mark}{repr(actual[ndx])}{post_mark}

Diff:
{diff(self.expected[ndx], actual[ndx])}
"""
            else:
                result += f"""{flds[ndx]}
  Expected:
    {repr(self.expected[ndx])}
  Actual:
    {repr(actual[ndx])}
"""

        env = {}
        env["LANGUAGE"] = self.lang
        for key in self.env:
            value = self.env[key]
            if value:
                env[key] = value
            elif key in env:
                del env[key]

        expanded = [expand(arg, tempdir) for arg in self.args]
        result += " ".join(
            shlex.quote(arg)
            for arg in [
                *["{}={}".format(key, env[key]) for key in env],
                target,
                *expanded,
            ]
        )

        return result

    def path(self, filename):
        return os.path.join(self.cwd, filename)

    def chdir(self, sub):
        self.cwd = os.path.abspath(os.path.join(self.cwd, sub))

    @staticmethod
    def load(filename, count):
        with open(filename) as f:
            return Test(json.load(f), filename, count)


if args.install is not None:
    root_dir = os.path.dirname(os.path.dirname(target))

    os.makedirs(os.path.join(args.install, "bin"), exist_ok=True)
    os.makedirs(os.path.join(args.install, "libexec", "cov"), exist_ok=True)

    shutil.copy2(target, os.path.join(args.install, "bin"))
    if os.path.exists(os.path.join(root_dir, "libexec")):
        shutil.copytree(
            os.path.join(root_dir, "libexec"),
            os.path.join(args.install, "libexec"),
            dirs_exist_ok=True,
        )
    if os.path.exists(os.path.join(root_dir, "share")):
        shutil.copytree(
            os.path.join(root_dir, "share"),
            os.path.join(args.install, "share"),
            dirs_exist_ok=True,
        )

    filters_target = "share/cov-{}/filters".format(
        ".".join(args.version.split(".", 2)[:2])
    )
    filters_source = os.path.join(os.path.dirname(__file__), "test-filters")
    for _, dirs, files in os.walk(filters_source):
        dirs[:] = []
        for filename in files:
            name, ext = os.path.splitext(filename)
            if ext != ".py":
                continue
            shutil.copy2(
                os.path.join(filters_source, filename),
                os.path.join(args.install, filters_target, name),
            )

    for module in args.install_with:
        shutil.copy2(module, os.path.join(args.install, "libexec", "cov"))

    target = os.path.join(args.install, "bin", target_name)


print(target)
print(args.data_dir)
print(args.tests)
print("version:", args.version)

testsuite = []
for root, dirs, files in os.walk(args.tests):
    for filename in files:
        testsuite.append(os.path.join(root, filename))

length = len(testsuite)
digits = 1
while length > 9:
    digits += 1
    length = length // 10


def to_lines(stream):
    lines = stream.split("\n")
    if len(lines) > 1 and lines[-1] == "":
        lines = lines[:-1]
        lines[-1] += "\n"
    if len(lines) == 1:
        return lines[0]
    return lines


class color:
    reset = "\033[m"
    counter = "\033[2;49;92m"
    name = "\033[0;49;90m"
    failed = "\033[0;49;91m"
    passed = "\033[2;49;92m"
    skipped = "\033[0;49;34m"


counter = 0
run = args.run

if not len(run):
    run = list(range(1, len(testsuite) + 1))
else:
    print("running:", ", ".join(str(x) for x in run))


class TaskResult:
    OK = 0
    SKIPPED = 1
    SAVED = 2
    FAILED = 3
    CLIP_FAILED = 4


def task(test: Test, current_counter: int) -> Tuple[int, str, Union[str, None]]:
    temp_instance = "".join(random.choice(string.ascii_letters) for _ in range(16))
    tempdir = f"{TEMP}/{temp_instance}"
    tempdir_alt = None

    if TEMP_ALT is not None:
        tempdir_alt = f"{TEMP_ALT}{os.sep}{temp_instance}"

    test_counter = (
        f"{color.counter}[{current_counter:>{digits}}/{len(testsuite)}]{color.reset}"
    )
    test_name = f"{color.name}{test.name}{color.reset}"
    test_id = f"{test_counter} {test_name}"

    print(test_id)
    os.makedirs(tempdir, exist_ok=True)

    actual = test.run(tempdir, tempdir_alt)
    if actual is None:
        return (TaskResult.SKIPPED, test_id, None)

    if test.expected is None:
        test.data["expected"] = [
            actual[0],
            *[to_lines(stream) for stream in actual[1:]],
        ]
        with open(filename, "w") as f:
            json.dump(test.data, f, indent=4)
            print(file=f)
        return (TaskResult.SAVED, test_id, None)

    clipped = test.clip(actual)

    if isinstance(clipped, str):
        return (TaskResult.CLIP_FAILED, test_id, clipped)

    if actual == test.expected or clipped == test.expected:
        return (TaskResult.OK, test_id, None)

    report = test.report(clipped, tempdir)
    return (TaskResult.FAILED, test_id, report)


@dataclass
class Counters:
    error_counter: int = 0
    skip_counter: int = 0
    save_counter: int = 0

    def report(self, outcome: int, test_id: str, message: Union[str, None]):
        if outcome == TaskResult.SKIPPED:
            print(f"{test_id} {color.skipped}SKIPPED{color.reset}")
            self.skip_counter += 1
            return

        if outcome == TaskResult.SAVED:
            print(f"{test_id} {color.skipped}saved{color.reset}")
            self.skip_counter += 1
            self.save_counter += 1
            return

        if outcome == TaskResult.CLIP_FAILED:
            print(
                f"{test_id} {color.failed}FAILED (unknown check '{message}'){color.reset}"
            )
            self.error_counter += 1
            return

        if outcome == TaskResult.OK:
            print(f"{test_id} {color.passed}PASSED{color.reset}")
            return

        if message is not None:
            print(message)
        print(f"{test_id} {color.failed}FAILED{color.reset}")
        self.error_counter += 1

    def summary(self, counter: int):
        print(f"Failed {self.error_counter}/{counter}")
        if self.skip_counter > 0:
            skip_test = "test" if self.skip_counter == 1 else "tests"
            if self.save_counter > 0:
                print(
                    f"Skipped {self.skip_counter} {skip_test} (including {self.save_counter} due to saving)"
                )
            else:
                print(f"Skipped {self.skip_counter} {skip_test}")

        return self.error_counter == 0


counters = Counters()

with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
    futures = []
    for filename in sorted(testsuite):
        counter += 1
        if counter not in run:
            continue

        test = Test.load(filename, counter)
        if not test.ok:
            continue
        futures.append(executor.submit(task, test, counter))

    for future in concurrent.futures.as_completed(futures):
        outcome, test_id, message = future.result()
        # outcome, test_id, message = task(test, counter)
        counters.report(outcome, test_id, message)

if args.install is not None:
    shutil.rmtree(args.install)
# shutil.rmtree(TEMP, ignore_errors=True)
shutil.rmtree("build/.testing", ignore_errors=True)

if not counters.summary(counter):
    sys.exit(1)
