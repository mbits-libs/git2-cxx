# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import functools
import os
import re
import shutil
import shlex
import subprocess
import sys
from contextlib import contextmanager
from typing import Callable, List, Union


def copy_file(src, dst):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst, follow_symlinks=False)


@contextmanager
def _set_env(compiler: List[str]):
    orig_env = {}
    if sys.platform != "win32":
        for var, value in zip(["CC", "CXX"], compiler):
            if var in os.environ:
                orig_env[var] = os.environ[var]
            os.environ[var] = value
    try:
        yield
    finally:
        for var in ["CC", "CXX"]:
            if var in os.environ:
                del os.environ[var]
        for var, value in orig_env.items():
            os.environ[var] = value


def _prn1(value):
    if isinstance(value, str):
        return f'\033[2;31m"{value}"\033[m'
    return _prn2(value)


def _prn2(value):
    if isinstance(value, bool):
        return "\033[34mtrue\033[m" if value else "\033[34mfalse\033[m"
    if isinstance(value, str):
        return value
    if isinstance(value, dict):
        return "{{{}}}".format(", ".join(f"{key}: {_prn1(sub)}" for key, sub in value))
    if isinstance(value, (list, set, tuple)):
        return "[{}]".format(", ".join(_prn1(sub) for sub in value))
    return f"\033[2;34m{value}\033[m"


def _prn(config, indent=""):
    prefix = "- "
    for key, value in config.items():
        intro = f"{indent}{prefix}\033[2;37m{key}:\033[m"
        if key[:2] == "--":
            continue
        if key == "compiler":
            comp = config["--orig-compiler"]
            if len(value) == 1 and value[0] == comp:
                print(f"{intro} {_prn2(comp)}", file=sys.stderr)
            else:
                print(f"{intro} {_prn2(comp)}, {_prn2(value)}", file=sys.stderr)
            continue
        print(f"{intro} {_prn2(value)}", file=sys.stderr)
        prefix = "  "


def _sanitizer_str(value: bool):
    return "with-sanitizer" if value else "no-sanitizer"


def _build_name(config: dict, keys: list):
    name = []
    for key in keys:
        val = config[key]
        if key == "sanitizer":
            val = _sanitizer_str(val)
        elif key == "compiler":
            val = config[key][0]
        else:
            val = f"{val}"
        name.append(val)
    return ", ".join(name)


def _print_arg(arg: str):
    color = ""
    if arg[:1] == "-":
        color = "\033[2;37m"
    arg = shlex.join([arg])
    if color == "" and arg[:1] in ["'", '"']:
        color = "\033[2;34m"
    if color == "":
        return arg
    return f"{color}{arg}\033[m"


_print_prefix = ""


def print_args(*args: str):
    cmd = shlex.join([args[0]])
    args = " ".join([_print_arg(arg) for arg in args[1:]])
    print(f"{_print_prefix}\033[33m{cmd}\033[m {args}", file=sys.stderr)


def _ls(dirname):
    result = []
    for root, _, filenames in os.walk(dirname):
        result.extend(
            os.path.relpath(os.path.join(root, filename), start=dirname)
            for filename in filenames
        )
    return result


class runner:
    DRY_RUN = False
    CUTDOWN_OS = False

    @staticmethod
    def run_steps(config: dict, keys: list, steps: List[callable]):
        title = _build_name(config, keys)
        prefix = "\033[1;34m|\033[m "
        first_step = True

        print(f"\033[1;34m+--[BUILD] {title}\033[m", file=sys.stderr)
        if runner.DRY_RUN:
            _prn(config, prefix)
            first_step = False

        with _set_env(config["compiler"]):
            for step in steps:
                if step(config, prefix, first_step):
                    first_step = False

        print(f"\033[1;34m+--------- \033[2;34m{title}\033[m", file=sys.stderr)

    @staticmethod
    def run_step(
        step_name: str,
        step: Callable[[dict], None],
        visible: Union[Callable[[dict], bool], None],
        config: Union[dict, None],
        prefix: str,
        first_step: bool,
    ):
        global _print_prefix
        if config is None:
            return step_name

        if visible and not visible(config):
            return False

        if not first_step:
            print(prefix, file=sys.stderr)

        print(f"{prefix}\033[1;35m+-[STEP] {step_name}\033[m", file=sys.stderr)
        _print_prefix = f"{prefix}\033[1;35m|\033[m "
        step(config)
        print(
            f"{prefix}\033[1;35m+------- \033[2;35m{step_name}\033[m", file=sys.stderr
        )
        return True

    @staticmethod
    def refresh_build_dir(name):
        if runner.DRY_RUN:
            return

        fullname = os.path.join("build", name)
        shutil.rmtree(fullname, ignore_errors=True)
        os.makedirs(fullname, exist_ok=True)

    @staticmethod
    def call(*args):
        print_args(*args)
        if runner.DRY_RUN:
            return
        args = (shutil.which(args[0]), *args[1:])
        proc = subprocess.run(args, check=False)
        if proc.returncode != 0:
            sys.exit(1)

    @staticmethod
    def copy(src_dir: str, dst_dir: str, regex: str = ""):
        print_args("cp", "-r", src_dir, dst_dir)
        if runner.DRY_RUN:
            return
        files = _ls(src_dir)
        if regex:
            files = (name for name in files if re.match(regex, name))
        for name in files:
            copy_file(os.path.join(src_dir, name), os.path.join(dst_dir, name))


def step_call(step_name: str, visible: Union[Callable[[dict], bool], None] = None):
    def decorator(step: Callable[[dict], None]):
        @functools.wraps(step)
        def run_step(config: Union[dict, None], prefix: str = "", first_step=True):
            return runner.run_step(step_name, step, visible, config, prefix, first_step)

        return run_step

    return decorator
