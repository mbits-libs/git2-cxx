#!/usr/bin/env python3

import json
import os
import subprocess
import sys
import shlex
from pprint import pprint
from typing import Dict, List

from github.cmake import _cmake


def _test_directory(preset_name: str):
    configs, presets = _load_presets("CMakePresets.json")
    stack = [preset_name]
    while len(stack):
        next = stack[0]
        stack = stack[1:]
        preset = presets.get(next)
        if preset is None:
            continue

        if preset[0] is not None:
            bin_dir = _get_bin_dir(configs, preset[0])
            if bin_dir is not None:
                return bin_dir
        stack.extend(reversed(preset[1]))

    return None


def _get_bin_dir(configs: Dict[str, tuple], preset_name: str):
    stack = [preset_name]
    while len(stack):
        next = stack[0]
        stack = stack[1:]
        preset = configs.get(next)
        if preset is None:
            continue

        if preset[0] is not None:
            return preset[0]

        stack.extend(reversed(preset[1]))

    return None


def _load_presets(path: str, known: List[str] = []):
    path = os.path.abspath(path)
    dirname = os.path.dirname(path)
    if path in known:
        return ({}, {})
    known.append(path)

    with open(path, encoding="UTF-8") as root_json:
        presets = json.load(root_json)

    configures: Dict[str, tuple] = {}
    for configurePreset in presets.get("configurePresets", []):
        name = configurePreset.get("name")
        if name is None:
            continue
        binaryDir = configurePreset.get("binaryDir")
        inherits = configurePreset.get("inherits", [])
        configures[name] = (binaryDir, inherits)

    tests: Dict[str, tuple] = {}
    for testPreset in presets.get("testPresets", []):
        name = testPreset.get("name")
        if name is None:
            continue
        configurePreset = testPreset.get("configurePreset")
        inherits = testPreset.get("inherits", [])
        tests[name] = (configurePreset, inherits)

    includes = presets.get("include", [])
    for include in includes:
        sub_cfgs, sub_tests = _load_presets(os.path.join(dirname, include), known)
        for name, cfg in sub_cfgs.items():
            configures[name] = cfg
        for name, test in sub_tests.items():
            tests[name] = test

    return (configures, tests)


def _load_tests(dirname: str):
    commands = _cmake(os.path.join(dirname, "CTestTestfile.cmake"))

    tests: Dict[str, List[str]] = {}

    for command in commands:
        if command.name == "set_tests_properties":
            continue
        if command.name == "subdirs":
            if len(command.args):
                sub = _load_tests(os.path.join(dirname, command.args[0].value))
                for name, cmd in sub.items():
                    tests[name] = cmd
            continue

        if command.name == "add_test":
            if len(command.args) > 1:
                name = command.args[0].value[3:-3]
                cmd = [arg.value for arg in command.args[1:]]
                if not len(cmd) or cmd[0] == "NOT_AVAILABLE":
                    continue
                tests[name] = cmd
            continue

        if command.name in ["if", "elseif", "else", "endif"]:
            continue

        pprint((dirname, command))

    return tests


def _print_arg(arg: str):
    cwd = os.getcwd().replace("\\", "/")
    cwd += "/"
    if arg[: len(cwd)] == cwd:
        arg = arg[len(cwd) :]
    color = ""
    if arg[:1] == "-":
        color = "\033[2;37m"
    arg = shlex.join([arg.replace("\\", "/")])
    if color == "" and arg[:1] in ["'", '"']:
        color = "\033[2;34m"
    if color == "":
        return arg
    return f"{color}{arg}\033[m"


def print_args(*args: str):
    cmd = shlex.join([args[0]])
    args = " ".join([_print_arg(arg) for arg in args[1:]])
    print(f"\033[33m{cmd}\033[m {args}", file=sys.stderr)


bin_dir = _test_directory(sys.argv[1])
if bin_dir is None:
    sys.exit(1)

bin_dir = bin_dir.replace("${sourceDir}/", "./")
tests = _load_tests(bin_dir)
if len(sys.argv) < 3:
    print("known test:", " ".join(tests.keys()))
    sys.exit(0)
try:
    test = [*tests[sys.argv[2]], *sys.argv[3:]]
except KeyError:
    print("known test:", " ".join(tests.keys()))
    sys.exit(1)
print_args(*test)
subprocess.run(test, shell=False)
