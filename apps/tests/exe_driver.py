# Copyright (c) 2022 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import shutil
from difflib import unified_diff

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
args.data_dir = os.path.abspath(args.data_dir)

target = args.target
target_name = os.path.basename(target)
version = args.version


def expand(input):
    return input\
        .replace('$DATA', args.data_dir) \
        .replace('$VERSION', args.version)


def fix(input, patches):
    if os.name == 'nt':
        input = input.replace(b'\r\n', b'\n')
    input = input.decode('UTF-8') \
        .replace(args.data_dir, '$DATA') \
        .replace(args.version, '$VERSION')

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
            self.working_directory = data['working-directory']
        except KeyError:
            self.working_directory = None

        try:
            self.mkdirs = data['mkdirs']
        except KeyError:
            self.mkdirs = []

        try:
            self.lang = data['lang']
        except KeyError:
            self.lang = 'en'

        try:
            self.env = data['env']
        except KeyError:
            self.env = []

    def run(self):
        current_directory = None
        if self.working_directory is not None:
            working_directory = expand(self.working_directory)
            current_directory = os.getcwd()
            os.chdir(working_directory)

        for dirname in self.mkdirs:
            os.makedirs(expand(dirname), exist_ok=True)

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

        if current_directory is not None:
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
        expanded = [expand(arg) for arg in self.args]
        print(' '.join([shlex.quote(arg) for arg in [target, *expanded]]))

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
    if os.path.exists(os.path.join(root_dir, "shared")):
        shutil.copytree(os.path.join(root_dir, "shared"), os.path.join(
            args.install, 'shared'), dirs_exist_ok=True)

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

had_errors = False
counter = 0
for filename in sorted(testsuite):
    counter += 1
    test = Test.load(filename, counter)
    if not test.ok:
        continue

    print(f"[{counter:>{digits}}/{len(testsuite)}] {test.name}")

    actual = test.run()
    if test.expected is None:
        test.data['expected'] = actual
        with open(filename, "w") as f:
            json.dump(test.data, f, separators=(", ", ": "))
        print(f"[{counter:>{digits}}/{len(testsuite)}] {test.name} saved")
        continue

    clipped = test.clip(actual)

    if isinstance(clipped, str):
        print(f"[{counter:>{digits}}/{len(testsuite)}] {test.name} **FAILED** (unknown check '{clipped}')")
        had_errors = True
        continue

    if actual == test.expected or clipped == test.expected:
        print(f"[{counter:>{digits}}/{len(testsuite)}] {test.name} PASSED")
        continue

    test.report(clipped)
    print(f"[{counter:>{digits}}/{len(testsuite)}] {test.name} **FAILED**")
    had_errors = True


if args.install is not None:
    shutil.rmtree(args.install)

if had_errors:
    sys.exit(1)
