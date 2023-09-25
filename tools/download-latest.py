#!/usr/bin/env python
# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import os
import sys
import subprocess

from archives import locate_unpack
from github.api import API
from github.cmake import get_version
from github.runner import Environment, print_args

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GITHUB_ORG = "mzdun"


parser = argparse.ArgumentParser(usage="Download the latest release from GitHub")
parser.add_argument(
    "--dry-run",
    action="store_true",
    required=False,
    help="print commands, change nothing",
)

args = parser.parse_args()
Environment.DRY_RUN = args.dry_run

version_tag = get_version().tag()
repo_name = get_version().name.value
api = API(GITHUB_ORG, repo_name)

proc = subprocess.run(
    [sys.executable, os.path.join(ROOT, "packages", "system.py"), "platform"],
    shell=False,
    capture_output=True,
)
platform = None if proc.returncode != 0 else proc.stdout.strip().decode("UTF-8")


release = api.get_release(platform=platform)

if release is None:
    sys.exit(0)

unpack, msg = locate_unpack(release)
print_args((*msg, release.replace(os.path.sep, "/"), "build/latest"))
if not Environment.DRY_RUN:
    unpack(release, "build/latest")
