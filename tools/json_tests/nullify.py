#!/usr/bin/env python
# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import os
from typing import List

from driver.test import Test

__file_dir__ = os.path.dirname(__file__)
__root_dir__ = os.path.abspath(os.path.dirname(os.path.dirname(__file_dir__)))
__test_dir__ = os.path.join(__root_dir__, "apps", "tests")

parser = argparse.ArgumentParser()
parser.add_argument(
    "--tests", required=True, metavar="DIR", default=[], action="append", nargs="+"
)
parser.add_argument(
    "--lang",
    metavar="ID",
)
parser.add_argument(
    "--run",
    metavar="TEST[,...]",
    action="append",
    default=[],
)


def _enum_tests(args: argparse.Namespace):
    testsuite: List[str] = []
    for testdir in args.tests:
        for root, _, files in os.walk(testdir):
            for filename in files:
                testsuite.append(os.path.join(root, filename))
    return testsuite


def _load_tests(testsuite: List[str], run: List[str]):
    counter = 0
    tests: List[Test] = []

    for filename in sorted(testsuite):
        counter += 1
        if counter not in run:
            continue

        test = Test.load(filename, counter)
        if not test.ok:
            continue
        tests.append(test)

    return tests


def __main__():
    args = parser.parse_args()
    args.tests = [testdir for group in args.tests for testdir in group]
    for index in range(len(args.tests)):
        testdir = args.tests[index]
        if not os.path.isdir(f"{__test_dir__}/{testdir}") and os.path.isdir(
            f"{__test_dir__}/main-set/{testdir}"
        ):
            testdir = f"main-set/{testdir}"
        args.tests[index] = os.path.join(__test_dir__, testdir)
    testsuite = _enum_tests(args)
    print("tests:")
    for testdir in args.tests:
        print(f" - {testdir}")

    run = args.run

    if not len(run):
        run = list(range(1, len(testsuite) + 1))
    else:
        print("running:", ", ".join(str(x) for x in run))

    tests = _load_tests(testsuite, run)

    for test in tests:
        test.nullify(args.lang)


if __name__ == "__main__":
    __main__()
