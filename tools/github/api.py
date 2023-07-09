# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import json
import os
import sys
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
    GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")  # fall back to bot, if available
    if GITHUB_TOKEN:
        print(
            f"falling back to GITHUB_TOKEN (len: {len(GITHUB_TOKEN)})", file=sys.stderr
        )
        return GITHUB_TOKEN
    print("no secrets to be used...", file=sys.stderr)
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
    stdout = proc.stdout.decode("UTF-8")
    if Environment.DBG:
        print(stdout)
    return json.loads(stdout)


class API:
    owner: str
    repo: str
    remote: Optional[str]

    def __init__(self, owner: str, repo: str):
        self.owner = owner
        self.repo = repo
        self.remote = locate_remote(owner, repo)

    def release(self, gh_release: dict) -> dict:
        checked(
            "git", "push", self.remote, "main", "--follow-tags", "--force-with-lease"
        )

        return json_from(
            f"https://api.github.com/repos/{self.owner}/{self.repo}/releases",
            "-d",
            json.dumps(gh_release),
            method="POST",
            default={},
        )

    def download_archive(self, ref: str, archive_name: str):
        ref_hash = (
            capture("git", "rev-list", "--no-walk", ref).stdout.decode("UTF-8").strip()
        )
        if Environment.DBG:
            print(ref_hash)

        workflow_runs = json_from(
            f"https://api.github.com/repos/{self.owner}/{self.repo}/actions/runs",
            "-L",
        ).get("workflow_runs", [])

        runs = [
            run
            for run in workflow_runs
            if run.get("head_sha") == ref_hash and run.get("head_sha") == ref_hash
        ]

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

        artifacts_url: Optional[str] = (
            runs[0].get("artifacts_url") if len(runs) else None
        )
        if artifacts_url is None:
            return None

        artifacts = [
            art
            for art in json_from(
                artifacts_url,
                "-L",
            ).get("artifacts", [])
            if art.get("name") == name
        ]
        archive_download_url: Optional[str] = (
            artifacts[0].get("archive_download_url") if len(artifacts) else None
        )

        if archive_download_url is None:
            return None

        filename = (
            f"build/downloads/{runs[0].get('run_number', 'unknown')}/{archive_name}.zip"
        )
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        curl(archive_download_url, "-L", "--output", filename, capture_output=False)

        return filename

    def get_unpublished_release_id(self, tag_name: str):
        releases = json_from(
            f"https://api.github.com/repos/{self.owner}/{self.repo}/releases",
            default=[],
        )
        limited = [
            release for release in releases if release.get("tag_name") == tag_name
        ]
        return limited[0].get("id") if len(limited) else None

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
