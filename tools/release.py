#!/usr/bin/env python
# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import argparse
import os
import sys
from pprint import pprint
from typing import Optional

from github.api import format_release, locate_remote
from github.api import release as github_release
from github.changelog import FORCED_LEVEL, update_changelog
from github.cmake import get_version, set_version
from github.git import add_files, annotated_tag, bump_version, commit, get_log, get_tags
from github.runner import Environment

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GITHUB_ORG = "mzdun"
SCOPE_FIX = {"ws": "webapi", "rest": "webapi", "lngs": "lang"}

#####################################################################


def release(take_all: bool, forced_level: Optional[int]):
    project = get_version()
    github_link = f"https://github.com/{GITHUB_ORG}/{project.name.value}"
    tags = get_tags(project)
    log, level = get_log(tags, SCOPE_FIX, take_all)

    if forced_level is not None:
        level = forced_level

    next_stability = project.stability.value
    if next_stability == "":
        next_stability = "-test"
    next_tag = f"v{bump_version(project.ver(), level)}{next_stability}"
    if not Environment.DRY_RUN:
        update_changelog(log, next_tag, project.tag(), github_link)
        set_version(next_tag[1:])

    commit_message = f"release {next_tag[1:]}"

    add_files(os.path.join(ROOT, "CMakeLists.txt"), os.path.join(ROOT, "CHANGELOG.md"))
    commit(f"chore: {commit_message}")
    annotated_tag(next_tag, commit_message)

    github_remote = locate_remote(GITHUB_ORG, project.name.value)

    if github_remote is not None:
        gh_release = format_release(log, next_tag, project.tag(), github_link)
        response = github_release(
            GITHUB_ORG, project.name.value, github_remote, gh_release
        )
        html_url = response.get("html_url")
        if html_url is not None:
            print(f"Visit draft at {html_url}")


parser = argparse.ArgumentParser(usage="Creates a release draft in GitHub")
parser.add_argument(
    "--dry-run",
    action="store_true",
    required=False,
    help="print commands, change nothing",
)
parser.add_argument(
    "--all",
    required=False,
    action="store_true",
    help="create a changelog with all sections, not only the 'feat', 'fix' and 'breaking'",
)
parser.add_argument(
    "--force",
    required=False,
    help="ignore the version change from changelog and instead use this settings' change",
    choices=FORCED_LEVEL.keys(),
)


def __main__():
    args = parser.parse_args()
    Environment.DRY_RUN = args.dry_run
    release(args.all, FORCED_LEVEL.get(args.force))


if __name__ == "__main__":
    __main__()
