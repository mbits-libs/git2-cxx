#!/usr/bin/env python
# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import os
from typing import List

from driver.test import Test

__dir__ = os.path.dirname(__file__)

parser = argparse.ArgumentParser()
parser.add_argument("--tests", required=True, metavar="DIR")
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
    for root, _, files in os.walk(args.tests):
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
    args.tests = os.path.join(__dir__, args.tests)
    testsuite = _enum_tests(args)
    print("tests:  ", args.tests)

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
