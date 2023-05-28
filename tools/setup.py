#!/usr/bin/env python3

import sys
import os
import shutil
import shlex
import subprocess
from contextlib import contextmanager

CONAN_PROFILE = "workflow"
COV_SANITIZE = "OFF"


def which(name: str) -> str:
    pathext_var = os.environ.get("PATHEXT")
    if pathext_var is None:
        return name

    pathext = pathext_var.lower().split(os.path.pathsep)
    if os.path.sep in name:
        split = os.path.splitext(name)
        if len(split) < 2 or split[1].lower() not in pathext:
            for ext in pathext:
                result = f"{name}{ext}"
                if os.path.exists(result):
                    return result
        if os.path.exists(name):
            return name
        return None
    for dirname in os.environ["PATH"].split(os.path.pathsep):
        for ext in pathext:
            result = os.path.join(dirname, f"{name}{ext}")
            if os.path.exists(result):
                return result
    return None


CONAN = which("conan")


@contextmanager
def cd(path):
    current = os.getcwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(current)


def call(*args, stdout=subprocess.PIPE):
    print(shlex.join(args))
    proc = subprocess.run(args, stdout=stdout, check=False)
    if proc.returncode != 0:
        sys.exit(1)
    return proc.stdout.decode().strip() if stdout is not None else None


def conan(build_type):
    call(CONAN, "profile", "new", "--detect", "--force", CONAN_PROFILE)
    if sys.platform == "linux":
        call(
            CONAN,
            "profile",
            "update",
            "settings.compiler.libcxx=libstdc++11",
            CONAN_PROFILE,
        )

    call(CONAN, "profile", "update", f"settings.build_type={build_type}", CONAN_PROFILE)

    call(
        CONAN,
        "install",
        "../..",
        "--build",
        "missing",
        "-pr:b",
        CONAN_PROFILE,
        "-pr:h",
        CONAN_PROFILE,
        stdout=None,
    )


def refresh_dir(name):
    fullname = os.path.join("build", name)
    shutil.rmtree(fullname, ignore_errors=True)
    os.makedirs(fullname, exist_ok=True)


if len(sys.argv) < 3:
    print(
        "setup.py <compiler> <build_type>[,<build_type>...] [<sanitize:ON/OFF>]",
        file=sys.stderr,
    )
    sys.exit(1)

compilers = {
    "gcc": ["linux"],
    "clang": ["linux"],
    "msvc": ["win32"],
}


normalized = {"clang++": "clang", "g++": "gcc"}
names = {"clang": ["clang", "clang++"], "gcc": ["gcc", "g++"]}


def normalize_compiler(compiler: str):
    dirname = os.path.dirname(compiler)
    filename = os.path.basename(compiler)
    if sys.platform == "win32":
        filename = os.path.splitext(filename)[0]
    chunks = filename.split("-", 1)
    if len(chunks) == 1:
        version = None
    else:
        version = chunks[1]
    filename = chunks[0].lower()
    try:
        filename = normalized[filename]
    except KeyError:
        pass

    try:
        compiler_names = names[filename]
    except:
        compiler_names = [filename]

    return filename, [
        os.path.join(dirname, name if version is None else f"{name}-{version}")
        for name in compiler_names
    ]


ON_OFF = {"ON": "ON", "OFF": "OFF"}
compiler_name, compilers_used = normalize_compiler(sys.argv[1])
BUILD_TYPE = sys.argv[2].split(",")
if len(sys.argv) > 3:
    COV_SANITIZE = ON_OFF[sys.argv[3]]

try:
    if sys.platform not in compilers[compiler_name]:
        print(
            f"Cannot use {compiler_name} on {sys.platform}",
            file=sys.stderr,
        )
        sys.exit(1)
except KeyError:
    print(f"Unknown compiler: {compiler_name}", file=sys.stderr)
    sys.exit(1)

if sys.platform != "win32":
    for var, value in zip(["CC", "CXX"], compilers_used):
        os.environ[var] = value

refresh_dir("conan")
with cd("build/conan"):
    print("cd build/conan")

    for build_type in BUILD_TYPE:
        build_type = build_type.strip()
        conan(build_type)

    print("cd ../..")

generator = "msbuild" if sys.platform == "win32" else "ninja"

for build_type in BUILD_TYPE:
    build_type = build_type.strip().lower()
    preset = f"{build_type}-{generator}"
    refresh_dir(build_type)
    call("cmake", "--preset", preset, f"-DCOV_SANITIZE={COV_SANITIZE}", stdout=None)
    call("cmake", "--build", "--preset", build_type, "--parallel", stdout=None)
    call("ctest", "--preset", build_type, stdout=None)
