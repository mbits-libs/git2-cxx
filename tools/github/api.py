# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import json
import os
import urllib.parse
from pathlib import Path
from typing import Any, Optional

from .changelog import ChangeLog, format_changelog, read_tag_date
from .runner import Environment, capture, checked, checked_capture, print_args

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


def curl(
    url: str, *args: str, method: Optional[str] = None, capture_output: bool = True
):
    cmd = [
        "curl",
        "-H",
        "Accept: application/vnd.github+json",
        "-H",
        f"Authorization: token {_api_token}",
        "-H",
        "X-GitHub-Api-Version: 2022-11-28",
    ]
    if method:
        cmd.extend(["-X", method.upper()])
    if Environment.DRY_RUN:
        print_args((*cmd, url, *args))
        return None
    return (
        checked_capture(*cmd, url, *args)
        if capture_output
        else checked(*cmd, url, *args)
    )


def json_from(url: str, *args: str, method: Optional[str] = None, default: Any = {}):
    proc = curl(url, *args, method=method)
    if proc is None:
        return default
    return json.loads(proc.stdout.decode("UTF-8"))


def release(owner: str, project: str, github_remote: str, gh_release: dict) -> dict:
    checked("git", "push", github_remote, "main", "--follow-tags")
    result = curl(
        f"https://api.github.com/repos/{owner}/{project}/releases",
        "-d",
        json.dumps(gh_release),
        method="POST",
    )

    if result is None:
        return {}
    return json.loads(result.stdout.decode("UTF-8"))


def runs_for_commit(owner: str, project: str, ref: str):
    ref_hash = (
        capture("git", "rev-list", "--no-walk", ref).stdout.decode("UTF-8").strip()
    )
    runs = json_from(
        f"https://api.github.com/repos/{owner}/{project}/actions/runs",
        "-L",
    ).get("workflow_runs", [])

    return [
        item
        for item in runs
        if item.get("head_sha") == ref_hash and item.get("head_sha") == ref_hash
    ]


def list_artifacts(url: str, name: str):
    return [
        art
        for art in json_from(
            url,
            "-L",
        ).get("artifacts", [])
        if art.get("name") == name
    ]


def download_archive_from_url(url: str, filename: str):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    curl(url, "-L", "--output", filename, capture_output=False)


def download_archive(owner: str, project: str, ref: str, archive_name: str):
    runs = runs_for_commit(owner, project, ref)
    if len(runs) > 1:
        with open("dump.json", "w", encoding="UTF-8") as dump:
            json.dump(runs, dump)
    name: str = runs[0].get("name", "<unnamed>")
    run_number: int = runs[0].get("run_number", 0)
    display_title: str = runs[0].get("display_title", 0)
    status: str = runs[0].get("status", "")

    print(f"{name} #{run_number} {display_title} -- {status}")
    if status != "completed":
        return None

    artifacts_url: Optional[str] = runs[0].get("artifacts_url") if len(runs) else None
    if artifacts_url is None:
        return None

    artifacts = list_artifacts(artifacts_url, archive_name)
    archive_download_url: Optional[str] = (
        artifacts[0].get("archive_download_url") if len(artifacts) else None
    )

    if archive_download_url is None:
        return None

    filename = (
        f"build/downloads/{runs[0].get('run_number', 'unknown')}/{archive_name}.zip"
    )
    download_archive_from_url(archive_download_url, filename)

    return filename


class API:
    owner: str
    repo: str
    remote: Optional[str]

    def __init__(self, owner: str, repo: str):
        self.owner = owner
        self.repo = repo
        self.remote = locate_remote(owner, repo)

    def release(self, gh_release: dict) -> dict:
        return release(self.owner, self.repo, self.remote, gh_release)

    def download_archive(self, ref: str, archive_name: str):
        return download_archive(self.owner, self.repo, ref, archive_name)

    def unpublished_releases(self, tag_name: str):
        return [
            release
            for release in json_from(
                f"https://api.github.com/repos/{self.owner}/{self.repo}/releases",
                default=[],
            )
            if release.get("tag_name") == tag_name
        ]

    def upload_asset(self, release_id: int, path: str):
        quoted = urllib.parse.quote_plus(os.path.basename(path))
        upload_url = (
            "https://uploads.github.com/repos/"
            f"{self.owner}/{self.repo}/releases/{release_id}/assets?name={quoted}"
        )
        response = json_from(
            upload_url,
            "-H",
            "Content-Type: application/octet-stream",
            "--data-binary",
            f"@{path}",
            method="POST",
        )
        errors = response.get("errors")
        if errors is not None:
            raise RuntimeError(json.dumps(errors))

    def publish_release(self, release_id: int) -> Optional[str]:
        return json_from(
            f"https://api.github.com/repos/{self.owner}/{self.repo}/releases/{release_id}",
            "-d",
            '{"draft": false}',
            method="PATCH",
        ).get("html_url")
