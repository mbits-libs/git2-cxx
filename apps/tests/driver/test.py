# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import json
import os
import random
import re
import shlex
import shutil
import string
import subprocess
import sys
from dataclasses import dataclass
from difflib import unified_diff
from typing import Callable, Dict, List, Optional, Tuple, Union

_flds = ["Return code", "Standard out", "Standard err"]
_streams = ["stdin", "stderr"]


def _last_enter(value: str):
    if len(value) and value[-1] == "\n":
        value = value[:-1] + "\\n"
    return value + "\n"


def _diff(expected, actual):
    expected = _last_enter(expected).splitlines(keepends=True)
    actual = _last_enter(actual).splitlines(keepends=True)
    return "".join(list(unified_diff(expected, actual))[2:])


def _alt_sep(input, value, var):
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


def to_lines(stream: str):
    lines = stream.split("\n")
    if len(lines) > 1 and lines[-1] == "":
        lines = lines[:-1]
        lines[-1] += "\n"
    if len(lines) == 1:
        return lines[0]
    return lines


@dataclass
class Env:
    target: str
    data_dir: str
    tempdir: str
    version: str
    counter_digits: int
    counter_total: int
    handlers: Dict[str, Tuple[int, Callable]]
    data_dir_alt: Optional[str] = None
    tempdir_alt: Optional[str] = None

    def expand(self, input: str, tempdir: str):
        return (
            input.replace("$TMP", tempdir)
            .replace("$DATA", self.data_dir)
            .replace("$VERSION", self.version)
        )

    def fix(self, raw_input: bytes, patches: Dict[str, str]):
        if os.name == "nt":
            raw_input = raw_input.replace(b"\r\n", b"\n")
        input = raw_input.decode("UTF-8")
        input = _alt_sep(input, self.tempdir, "$TMP")
        input = _alt_sep(input, self.data_dir, "$DATA")
        input = input.replace(self.version, "$VERSION")

        if self.tempdir_alt is not None:
            input = _alt_sep(input, self.tempdir_alt, "$TMP")
            input = _alt_sep(input, self.data_dir_alt, "$DATA")

        if not len(patches):
            return input

        lines = input.split("\n")
        for patch in patches:
            patched = patches[patch]
            pattern = re.compile(patch)
            for lineno in range(len(lines)):
                if pattern.match(lines[lineno]):
                    lines[lineno] = patched
        return "\n".join(lines)


class Test:
    def __init__(self, data: dict, filename: str, count: int):
        self.cwd = os.getcwd()
        self.data = data
        self.filename = filename
        self.ok = True
        self.post_args = []
        self.linear = data.get("linear", False)
        self.disabled = data.get("disabled")

        if isinstance(self.disabled, bool):
            self.ok = not self.disabled
        elif isinstance(self.disabled, str):
            self.ok = self.disabled != sys.platform

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
        for stream in range(len(_streams)):
            try:
                self.check[stream] = data["check"][_streams[stream]]
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

    def run_cmds(self, env: Env, ops: List[Union[str, List[str]]], tempdir: str):
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
                min_args, cb = env.handlers[name]
                op = op[1:]
                if len(op) < min_args:
                    return None
                cb(self, [env.expand(o, tempdir) for o in op])
            except Exception as ex:
                if op[0] != "safe-rm":
                    print("Problem while handling", orig)
                    print(ex)
                    raise
                if is_safe:
                    continue
                return None
        return True

    def run(self, environment: Env):
        root = os.path.join(
            "build",
            ".testing",
            "".join(random.choice(string.ascii_letters) for _ in range(16)),
        )
        root = self.cwd = os.path.join(self.cwd, root)
        os.makedirs(root, exist_ok=True)

        prep = self.run_cmds(environment, self.prepare, environment.tempdir)
        if prep is None:
            return None

        expanded = [environment.expand(arg, environment.tempdir) for arg in self.args]
        post_expanded = [
            [environment.expand(arg, environment.tempdir) for arg in cmd]
            for cmd in self.post_args
        ]

        _env = {name: os.environ[name] for name in os.environ}
        _env["LANGUAGE"] = self.lang
        for key in self.env:
            value = self.env[key]
            if value:
                _env[key] = environment.expand(value, environment.tempdir)
            elif key in _env:
                del _env[key]

        cwd = None if self.linear else self.cwd
        proc: subprocess.CompletedProcess = subprocess.run(
            [environment.target, *expanded], capture_output=True, env=_env, cwd=cwd
        )
        returncode: int = proc.returncode
        test_stdout: bytes = proc.stdout
        test_stderr: bytes = proc.stderr

        for sub_expanded in post_expanded:
            if returncode != 0:
                break
            proc_post: subprocess.CompletedProcess = subprocess.run(
                [environment.target, *sub_expanded],
                capture_output=True,
                env=_env,
                cwd=cwd,
            )
            returncode = proc_post.returncode
            if len(test_stdout) and len(proc_post.stdout):
                test_stdout += b"\n"
            if len(test_stderr) and len(proc_post.stderr):
                test_stderr += b"\n"
            test_stdout += proc_post.stdout
            test_stderr += proc_post.stderr

        clean = self.run_cmds(environment, self.cleanup, environment.tempdir)
        if clean is None:
            return None

        return [
            returncode,
            environment.fix(test_stdout, self.patches),
            environment.fix(test_stderr, self.patches),
        ]

    def clip(self, actual: List[str]):
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

    def report(self, env: Env, actual: List[str], tempdir: str):
        result = ""
        for ndx in range(len(actual)):
            if actual[ndx] == self.expected[ndx]:
                continue
            if ndx:
                check = self.check[ndx - 1]
                pre_mark = "..." if check == "end" else ""
                post_mark = "..." if check == "begin" else ""
                result += f"""{_flds[ndx]}
  Expected:
    {pre_mark}{repr(self.expected[ndx])}{post_mark}
  Actual:
    {pre_mark}{repr(actual[ndx])}{post_mark}

Diff:
{_diff(self.expected[ndx], actual[ndx])}
"""
            else:
                result += f"""{_flds[ndx]}
  Expected:
    {repr(self.expected[ndx])}
  Actual:
    {repr(actual[ndx])}
"""

        _env = {}
        _env["LANGUAGE"] = self.lang
        for key in self.env:
            value = self.env[key]
            if value:
                _env[key] = value
            elif key in _env:
                del _env[key]

        expanded = [env.expand(arg, tempdir) for arg in self.args]
        result += " ".join(
            shlex.quote(arg)
            for arg in [
                *["{}={}".format(key, _env[key]) for key in _env],
                env.target,
                *expanded,
            ]
        )

        return result

    def path(self, filename):
        return os.path.join(self.cwd, filename)

    def chdir(self, sub):
        self.cwd = os.path.abspath(os.path.join(self.cwd, sub))
        if self.linear:
            os.chdir(self.cwd)

    def rmtree(self, sub):
        shutil.rmtree(self.path(sub))

    def makedirs(self, sub):
        os.makedirs(self.path(sub), exist_ok=True)

    @staticmethod
    def load(filename, count):
        with open(filename) as f:
            return Test(json.load(f), filename, count)
