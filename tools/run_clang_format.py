#!/usr/bin/env python

import sys
import subprocess

result = subprocess.run(
    ["git", "rev-parse", "--show-toplevel"], shell=False, capture_output=True
)
if result.returncode:
    sys.stderr.write(result.stderr.decode("UTF-8"))
    sys.exit(result.returncode)

__dir__ = result.stdout.strip().decode("UTF-8")
subprocess.run(
    [
        "docker",
        "run",
        "-it",
        "--workdir",
        "/src",
        "-v",
        f"{__dir__}:/src",
        "clang-format-lint",
        "--clang-format-executable",
        "/clang-format/clang-format12",
        "-r",
        "--style",
        "file",
        "--inplace",
        "true",
        "--extensions",
        "c,h,C,H,cpp,hpp,cc,hh,c++,h++,cxx,hxx",
        "--exclude",
        "./build ./external ./tools",
        ".",
    ],
    shell=False,
)
