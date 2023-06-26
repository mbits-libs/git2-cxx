#!/usr/bin/env python3
# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import matrix
import runner
import os
from contextlib import contextmanager
from typing import List, Set


@contextmanager
def cd(path):
    current = os.getcwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(current)


_used_compilers = {}


def _compiler(value: str):
    global _used_compilers
    compiler, names = matrix.find_compiler(value)
    if compiler not in _used_compilers:
        _used_compilers[compiler] = []
    _used_compilers[compiler].append(names)
    return compiler


def _boolean(value: str):
    return value.lower() in _TRUE


_TRUE = {"true", "on", "yes", "1", "with-sanitizer"}
_TYPES = {"compiler": _compiler, "sanitizer": _boolean}


def _flatten(array: List[list]) -> list:
    return [item for sublist in array for item in sublist]


def _config(config: List[str]):
    args = {}
    for arg in config:
        if arg[:1] == "-":
            continue
        _arg = arg.split("=", 1)
        if len(_arg) == 1:
            continue

        name, vals = _arg
        name = name.strip()
        conv = _TYPES.get(name, lambda value: value)
        values = {conv(val.strip()) for val in vals.split(",")}
        if name in args:
            values.update(args[name])
        args[name] = list(values)
    if "os" not in args:
        args["os"] = [matrix.platform]
    return matrix.cartesian(args)


def _steps(steps: List[str], parser: argparse.ArgumentParser):
    result = [
        step.strip().lower() for step in _flatten(step.split(",") for step in steps)
    ]
    known = [step(None).name.lower() for step in matrix.steps.build_steps()]
    for step in result:
        if step not in known:
            names = ", ".join(step(None).name for step in matrix.steps.build_steps())
            parser.error(f"unrecognized step: {step}; known steps are: {names}")
    return result


def _extend(plus: List[str]):
    result: Set[str] = {
        info.name.lower()
        for info in (step(None) for step in matrix.steps.build_steps())
        if not info.only_verbose()
    }
    result.update(plus)
    return list(result)


def _name(step: runner.step_info):
    return f"*{step.name}" if step.only_verbose() else step.name


def _known_steps():
    steps = matrix.steps.build_steps()
    if not len(steps):
        return "<none>"
    infos = [step(None) for step in steps]
    verbose = [1 for info in infos if info.only_verbose()]
    result = _name(infos[-1])
    prev_steps = ", ".join(_name(info) for info in infos[:-1])
    if len(prev_steps):
        result = f"{prev_steps} and {result}"
    if len(verbose):
        result = (
            f"{result} (note: steps with asterisks can only be called through --step)"
        )
    return result


_default_compiler = {"ubuntu": "gcc", "windows": "msvc"}


def default_compiler():
    try:
        return os.environ["DEV_CXX"]
    except KeyError:
        return _default_compiler[matrix.platform]


parser = argparse.ArgumentParser(description="Unified action runner")
parser.add_argument(
    "--dry-run",
    action="store_true",
    required=False,
    help="print steps and commands, do nothing",
)
parser.add_argument(
    "--dev",
    required=False,
    action="store_true",
    help=f'shortcut for "-c os={matrix.platform} '
    f'compiler={default_compiler()} build_type=Debug sanitizer=OFF"',
)
parser.add_argument(
    "--rel",
    required=False,
    action="store_true",
    help=f'shortcut for "-c os={matrix.platform} '
    f'compiler={default_compiler()} build_type=Release sanitizer=OFF"',
)
parser.add_argument(
    "--cutdown-os",
    required=False,
    action="store_true",
    help="configure CMake with COV_CUTDOWN_OS=ON",
)
parser.add_argument(
    "--github",
    required=False,
    action="store_true",
    help="use ::group:: annotation in steps",
)
parser.add_argument(
    "-s",
    "--steps",
    metavar="step",
    nargs="*",
    action="append",
    default=[],
    help="run only listed steps; if missing, run all the steps; "
    f"known steps are: {_known_steps()}.",
)
parser.add_argument(
    "-S",
    dest="steps_plus",
    nargs="*",
    action="append",
    default=[],
    help=argparse.SUPPRESS,
)
parser.add_argument(
    "-c",
    dest="configs",
    metavar="config",
    nargs="*",
    action="append",
    default=[],
    help="run only build matching the config; "
    "each filter is a name of a matrix axis followed by comma-separated values to take; "
    f'if "os" is missing, it will default to additional "-c os={matrix.platform}"',
)


def main():
    args = parser.parse_args()
    args.steps = _steps(_flatten(args.steps), parser)
    args.steps_plus = _steps(_flatten(args.steps_plus), parser)
    if len(args.steps) and len(args.steps_plus):
        parser.error("-s/--step and -S are mutually exclusive")
    if len(args.steps_plus):
        args.steps = _extend(args.steps_plus)
    if args.dev or args.rel:
        if args.dev and args.rel:
            parser.error("--dev and --rel are mutually exclusive")
        args.configs.append(
            [
                f"os={matrix.platform}",
                f"compiler={default_compiler()}",
                f"build_type={'Release' if args.rel else 'Debug'}",
                "sanitizer=OFF",
            ]
        )
    args.configs = _config(_flatten(args.configs))
    runner.runner.DRY_RUN = args.dry_run
    runner.runner.CUTDOWN_OS = args.cutdown_os
    runner.runner.GITHUB_ANNOTATE = args.github
    path = os.path.join(os.path.dirname(__file__), "config.json")
    configs, keys = matrix.load_matrix(path)

    usable = [
        config
        for config in configs
        if len(args.configs) == 0 or matrix.matches_any(config, args.configs)
    ]

    first_build = True
    for conf in usable:
        try:
            used_compilers = _used_compilers[conf["compiler"]]
        except KeyError:
            used_compilers = [matrix.find_compiler(conf["compiler"])[1]]
        for compiler in used_compilers:
            if not first_build:
                print()

            matrix.steps.build_config(
                {
                    **conf,
                    "compiler": compiler,
                    "--orig-compiler": conf["compiler"],
                    "--first-build": first_build,
                },
                keys,
                args.steps,
            )
            first_build = False


if __name__ == "__main__":
    main()
