#!/usr/bin/env python
# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import os

from github.api import API
from github.cmake import get_version

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GITHUB_ORG = "mzdun"

parser = argparse.ArgumentParser(usage="Download artifacts from GitHub")
parser.add_argument(
    "--dry-run",
    action="store_true",
    required=False,
    help="print commands, change nothing",
)

version_tag = get_version().tag()
repo_name = get_version().name.value
api = API(GITHUB_ORG, repo_name)

filename = api.download_archive(version_tag, "Packages")
if filename is not None:
    print(">>>", filename)
