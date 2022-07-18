# Copyright (c) 2022 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import json
import os
import re
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
from difflib import unified_diff

if os.name == 'nt':
    sys.stdout.reconfigure(encoding='utf-8')

flds = ['Return code', 'Standard out', 'Standard err']
streams = ['stdin', 'stderr']

parser = argparse.ArgumentParser()
parser.add_argument("--target", required=True, metavar="EXE")
parser.add_argument("--tests", required=True, metavar="DIR")
parser.add_argument("--data-dir", required=True, metavar="DIR")
parser.add_argument("--version", required=True, metavar="SEMVER")
parser.add_argument("--install", metavar="DIR")
parser.add_argument("--install-with", metavar="TOOL",
                    type=lambda s: s.split(';'), action='append', default=[])
args = parser.parse_args()
args.install_with = [item for groups in args.install_with for item in groups]
args.data_dir = os.path.abspath(args.data_dir).replace('\\', '/')
args.data_dir_alt = None

target = args.target
target_name = os.path.basename(target)
version = args.version

TEMP = tempfile.gettempdir().replace('\\', '/')
TEMP_ALT = None

if os.sep != '/':
    TEMP_ALT = TEMP.replace('/', os.sep)
    args.data_dir_alt = args.data_dir.replace('/', os.sep)
print(TEMP_ALT, TEMP)


def expand(input):
    return input\
        .replace('$TMP', TEMP) \
        .replace('$DATA', args.data_dir) \
        .replace('$VERSION', args.version)


def alt_sep(input, value, var):
    split = input.split(value)
    first = split[0]
    split = split[1:]
    for index in range(len(split)):
        m = re.match(r'(\S+)(\s*.*)', split[index])
        g2 = m.group(2)
        if g2 is None:
            g2 = ''
        split[index] = '{}{}'.format(m.group(1).replace(os.sep, '/'), g2)
    return var.join([first, *split])


def fix(input, patches):
    if os.name == 'nt':
        input = input.replace(b'\r\n', b'\n')
    input = input.decode('UTF-8') \
        .replace(TEMP, '$TEMP') \
        .replace(args.data_dir, '$DATA') \
        .replace(args.version, '$VERSION')

    if TEMP_ALT is not None:
        input = alt_sep(input, TEMP_ALT, '$TEMP')
        input = alt_sep(input, args.data_dir_alt, '$DATA')

    if not len(patches):
        return input

    input = input.split('\n')
    for patch in patches:
        patched = patches[patch]
        ptrn = re.compile(patch)
        for lineno in range(len(input)):
            if ptrn.match(input[lineno]):
                input[lineno] = patched
    return '\n'.join(input)


def last_enter(s):
    if len(s) and s[-1] == '\n':
        s = s[:-1] + '\\n'
    return s + '\n'


def diff(expected, actual):
    expected = last_enter(expected).splitlines(keepends=True)
    actual = last_enter(actual).splitlines(keepends=True)
    return ''.join(list(unified_diff(expected, actual))[2:])


def touch(args):
    with open(args[0], "wb", encoding="utf-8") as f:
        pass


def git(args):
    subprocess.run(["git", *args], shell=False)


def cov(aditional):
    subprocess.run([args.target, *aditional], shell=False)

file_cache = {}
rw_mask = stat.S_IWRITE | stat.S_IWGRP | stat.S_IWOTH
ro_mask = 0o777 ^ rw_mask

def make_RO(args):
    print(f'{args}...')
    mode = os.stat(args[0]).st_mode
    print('{:03o} -> {:03o}'.format(mode, mode & ro_mask))
    file_cache[args[0]] = mode
    os.chmod(args[0], mode & ro_mask)
    print('{:03o} -> {:03o}'.format(mode, os.stat(args[0]).st_mode))

def make_RW(args):
    try:
        mode = file_cache[args[0]]
    except KeyError:
        mode = os.stat(args[0]).st_mode | rw_mask
    os.chmod(args[0], mode)

op_types = {
    'mkdirs': (1, lambda args: os.makedirs(expand(args[0]), exist_ok=True)),
    'rm': (1, lambda args: shutil.rmtree(expand(args[0]))),
    'ro': (1, make_RO),
    'rw': (1, make_RW),
    'touch': (1, touch),
    'cd': (1, lambda args: os.chdir(expand(args[0]))),
    'git': (0, git),
    'cov': (0, cov),
}


class Test:
    def __init__(self, data, filename, count):
        self.data = data
        self.ok = True

        name = os.path.splitext(os.path.basename(filename))[0].split('-')
        if int(name[0]) == count:
            name = name[1:]
        else:
            name[0] = '({})'.format(name[0])
        self.name = ' '.join(name)

        try:
            call_args, expected = data['args'], data['expected']
            self.args = call_args
            self.expected = expected
        except KeyError:
            self.ok = False
            return

        if self.expected is not None:
            if not isinstance(self.expected[1], str):
                self.expected[1] = '\n'.join(self.expected[1])
            if not isinstance(self.expected[2], str):
                self.expected[2] = '\n'.join(self.expected[2])

        try:
            self.patches = data['patches']
        except KeyError:
            self.patches = {}

        self.check = ["all", "all"]
        for stream in range(len(streams)):
            try:
                self.check[stream] = data['check'][streams[stream]]
            except KeyError:
                pass

        try:
            self.prepare = data['prepare']
        except KeyError:
            self.prepare = []

        try:
            self.cleanup = data['cleanup']
        except KeyError:
            self.cleanup = []

        try:
            self.lang = data['lang']
        except KeyError:
            self.lang = 'en'

        try:
            self.env = data['env']
        except KeyError:
            self.env = []

    @staticmethod    
    def run_cmds(ops):
        for op in ops:
            orig = op
            if isinstance(op, str):
                op = shlex.split(op)
            is_safe = False
            try:
                name = op[0]
                if name[:5] == 'safe-':
                    name = name[5:]
                    is_safe = True
                min_args, cb = op_types[name]
                op = op[1:]
                if len(op) < min_args:
                    return None
                cb([expand(o) for o in op])
            except Exception as ex:
                print('Problem while handling', orig)
                print(ex)
                if is_safe:
                    continue
                return None
        return True

    def run(self):
        current_directory = os.getcwd()

        prep = Test.run_cmds(self.prepare)
        if prep is None:
            return None

        expanded = [expand(arg) for arg in self.args]

        env = {name: os.environ[name] for name in os.environ}
        env['LANGUAGE'] = self.lang
        for key in self.env:
            value = self.env[key]
            if value:
                env[key] = value
            elif key in env:
                del env[key]

        proc = subprocess.run([target, *expanded],
                              capture_output=True, env=env)

        clean = Test.run_cmds(self.cleanup)
        if clean is None:
            return None

        os.chdir(current_directory)

        return [proc.returncode,
                fix(proc.stdout, self.patches),
                fix(proc.stderr, self.patches)]

    def clip(self, actual):
        clipped = [*actual]
        for ndx in range(len(self.check)):
            check = self.check[ndx]
            if check != "all":
                if check == "begin":
                    clipped[ndx + 1] = clipped[ndx +
                                               1][:len(self.expected[ndx + 1])]
                elif check == "end":
                    clipped[ndx + 1] = clipped[ndx +
                                               1][-len(self.expected[ndx + 1]):]
                else:
                    return check
        return clipped

    def report(self, actual):
        for ndx in range(len(actual)):
            if actual[ndx] == self.expected[ndx]:
                continue
            if ndx:
                check = self.check[ndx - 1]
                pre_mark = '...' if check == "end" else ''
                post_mark = '...' if check == "begin" else ''
                print(f"""{flds[ndx]}
  Expected:
    {pre_mark}{repr(self.expected[ndx])}{post_mark}
  Actual:
    {pre_mark}{repr(actual[ndx])}{post_mark}

Diff:
{diff(self.expected[ndx], actual[ndx])}""")
            else:
                print(f"""{flds[ndx]}
  Expected:
    {repr(self.expected[ndx])}
  Actual:
    {repr(actual[ndx])}""")

        env = {}
        env['LANGUAGE'] = self.lang
        for key in self.env:
            value = self.env[key]
            if value:
                env[key] = value
            elif key in env:
                del env[key]

        expanded = [expand(arg) for arg in self.args]
        print(' '.join(shlex.quote(arg) for arg in [
              *['{}={}'.format(key, env[key]) for key in env], target, *expanded]))

    @staticmethod
    def load(filename, count):
        with open(filename) as f:
            return Test(json.load(f), filename, count)


if args.install is not None:
    root_dir = os.path.dirname(os.path.dirname(target))

    os.makedirs(os.path.join(args.install, 'bin'), exist_ok=True)
    os.makedirs(os.path.join(args.install, 'libexec', 'cov'), exist_ok=True)

    shutil.copy2(target, os.path.join(args.install, 'bin'))
    if os.path.exists(os.path.join(root_dir, "libexec")):
        shutil.copytree(os.path.join(root_dir, "libexec"), os.path.join(
            args.install, 'libexec'), dirs_exist_ok=True)
    if os.path.exists(os.path.join(root_dir, "share")):
        shutil.copytree(os.path.join(root_dir, "share"), os.path.join(
            args.install, 'share'), dirs_exist_ok=True)

    for module in args.install_with:
        shutil.copy2(module, os.path.join(args.install, 'libexec', 'cov'))

    target = os.path.join(args.install, 'bin', target_name)


print(target)
print(args.data_dir)
print(args.tests)

testsuite = []
for root, dirs, files in os.walk(args.tests):
    dirs[:] = []
    for filename in files:
        testsuite.append(os.path.join(root, filename))

length = len(testsuite)
digits = 1
while length > 9:
    digits += 1
    length = length // 10


def to_lines(stream):
    lines = stream.split('\n')
    if len(lines) > 1 and lines[-1] == '':
        lines = lines[:-1]
        lines[-1] += '\n'
    if len(lines) == 1:
        return lines[0]
    return lines


had_errors = False
counter = 0
for filename in sorted(testsuite):
    counter += 1
    test = Test.load(filename, counter)
    if not test.ok:
        continue

    print(f"\033[2;49;92m[{counter:>{digits}}/{len(testsuite)}]\033[m \033[2;49;30m{test.name}\033[m")

    actual = test.run()
    if actual is None:
        print(f"\033[2;49;92m[{counter:>{digits}}/{len(testsuite)}]\033[m \033[2;49;30m{test.name}\033[m \033[0;49;34mSKIPPED\033[m")
        continue

    if test.expected is None:
        test.data['expected'] = [actual[0], *
                                 [to_lines(stream) for stream in actual[1:]]]
        with open(filename, "w") as f:
            json.dump(test.data, f, indent=4)
            print(file=f)
        print(f"\033[2;49;92m[{counter:>{digits}}/{len(testsuite)}]\033[m \033[2;49;30m{test.name}\033[m \033[0;49;34msaved\033[m")
        continue

    clipped = test.clip(actual)

    if isinstance(clipped, str):
        print(
            f"\033[2;49;92m[{counter:>{digits}}/{len(testsuite)}]\033[m \033[2;49;30m{test.name}\033[m \033[0;49;91mFAILED (unknown check '{clipped}')\033[m")
        had_errors = True
        continue

    if actual == test.expected or clipped == test.expected:
        print(f"\033[2;49;92m[{counter:>{digits}}/{len(testsuite)}]\033[m \033[2;49;30m{test.name}\033[m \033[2;49;92mPASSED\033[m")
        continue

    test.report(clipped)
    print(f"\033[2;49;92m[{counter:>{digits}}/{len(testsuite)}]\033[m \033[2;49;30m{test.name}\033[m \033[0;49;91mFAILED\033[m")
    had_errors = True


if args.install is not None:
    pass
    # shutil.rmtree(args.install)

if had_errors:
    sys.exit(1)
