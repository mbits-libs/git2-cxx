# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import json
from typing import Optional
from pathlib import Path

from .runner import Environment, capture, checked, checked_capture
from .changelog import ChangeLog, read_tag_date, format_changelog

_api_token = ""


def _load_secret(owner: str, project: str) -> Optional[str]:
    try:
        with open(str(Path("~/.github_secrets").expanduser())) as f:
            github_secrets = json.load(f)
        for key in [
            f"{owner}/{project}:releases",
            f"{owner}:releases",
            f"{owner}/{project}",
            f"{owner}",
        ]:
            try:
                return github_secrets[key]
            except KeyError:
                continue
    except FileNotFoundError:
        pass
    return None


def _locate_remote(owner: str, project: str) -> Optional[str]:
    proc = capture("git", "remote", "-v")

    expected = [
        f"https://github.com/{owner}/{project}.git",
        f"git@github.com:{owner}/{project}.git",
    ]
    for remote_name, url in [
        line[:-7].split("\t")
        for line in proc.stdout.decode("UTF-8").split("\n")
        if line[-7:] == " (push)"
    ]:
        if url in expected:
            return remote_name

    return None


def locate_remote(owner: str, project: str) -> Optional[str]:
    global _api_token
    secret = _load_secret(owner, project)
    if secret is None:
        return None
    remote = _locate_remote(owner, project)
    if remote is None:
        return None
    _api_token = secret
    Environment.SECRETS.append(secret)
    return remote


def format_release(log: ChangeLog, cur_tag: str, prev_tag: str, github_link: str):
    lines = format_changelog(
        log, cur_tag, prev_tag, read_tag_date(cur_tag), True, github_link
    )

    return {
        "tag_name": cur_tag,
        "name": cur_tag,
        "body": "\n".join(lines),
        "draft": True,
        "prerelease": len(cur_tag.split("-", 1)) > 1,
    }


def release(owner: str, project: str, github_remote: str, gh_release: dict) -> dict:
    print("ping")
    checked("git", "push", github_remote, "main", "--follow-tags")
    result = checked_capture(
        "curl",
        "-X",
        "POST",
        "-H",
        "Accept: application/vnd.github+json",
        "-H",
        f"Authorization: token {_api_token}",
        f"https://api.github.com/repos/{owner}/{project}/releases",
        "-d",
        json.dumps(gh_release),
    )

    if result is None:
        return {}
    return json.loads(result.stdout.decode("UTF-8"))
