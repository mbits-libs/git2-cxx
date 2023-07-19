# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import json
import os
import subprocess
import urllib.parse
from typing import Any, List, Optional, Union

from .changelog import ChangeLog, format_changelog, read_tag_date
from .runner import Environment, capture, checked, checked_capture, print_args


def locate_remote(owner: str, repo: str) -> Optional[str]:
    proc = capture("git", "remote", "-v")

    expected = [
        f"https://github.com/{owner}/{repo}.git",
        f"git@github.com:{owner}/{repo}.git",
    ]
    for remote_name, url in [
        line[:-7].split("\t")
        for line in proc.stdout.decode("UTF-8").split("\n")
        if line[-7:] == " (push)"
    ]:
        if url in expected:
            return remote_name

    return None


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


def gh(
    url: str,
    *args: str,
    method: Optional[str] = None,
    server: Optional[str] = None,
    capture_output: bool = True,
):
    cmd = [
        "gh",
        "api",
        "-H",
        "Accept: application/vnd.github+json",
        "-H",
        "X-GitHub-Api-Version: 2022-11-28",
    ]
    if method:
        cmd.extend(["--method", method.upper()])
    if server:
        url = f"{server}{url}"
    if Environment.DRY_RUN:
        print_args((*cmd, url, *args))
        return None
    return (
        checked_capture(*cmd, url, *args)
        if capture_output
        else checked(*cmd, url, *args)
    )


class API:
    owner: str
    repo: str
    remote: Optional[str]

    @property
    def root(self):
        return f"/repos/{self.owner}/{self.repo}"

    def gh(
        self,
        res: str,
        *args: str,
        method: Optional[str] = None,
        server: Optional[str] = None,
        capture_output: bool = True,
    ):
        return gh(
            f"{self.root}{res}",
            *args,
            method=method,
            server=server,
            capture_output=capture_output,
        )

    def json_from(
        self,
        res: str,
        *args: str,
        method: Optional[str] = None,
        server: Optional[str] = None,
        default: Any = {},
    ):
        proc = self.gh(res, *args, method=method, server=server)
        if proc is None:
            return default
        stdout = proc.stdout.decode("UTF-8")
        if Environment.DBG:
            print(stdout)
        return json.loads(stdout)

    def first_from(self, res: str, root_name: None = None, **filter):
        default = [] if root_name is None else {}
        data: Union[dict, list] = self.json_from(res, default=default)
        if root_name is not None:
            data = data.get(root_name, [])

        for item in data:
            sel = True
            for key in filter:
                if item.get(key) != filter[key]:
                    sel = False
                    break
            if sel:
                return item

        return {}

    def __init__(self, owner: str, repo: str):
        self.owner = owner
        self.repo = repo
        self.remote = locate_remote(owner, repo)

    def release(self, gh_release: dict) -> dict:
        checked(
            "git", "push", self.remote, "main", "--follow-tags", "--force-with-lease"
        )

        flags = []
        for name, value in gh_release.items():
            is_str = isinstance(value, str)
            if not is_str:
                value = json.dumps(value)
            flags.append("-f" if is_str else "-F")
            flags.append(f"{name}={value}")

        return self.json_from("/releases", *flags, method="POST", default={})

    def download_archive(self, ref: str, workflow: str, archive_name: str):
        ref_hash = (
            capture("git", "rev-list", "--no-walk", ref).stdout.decode("UTF-8").strip()
        )
        if Environment.DBG:
            print(ref_hash)

        run = self.first_from(
            f"/actions/runs?event=push&status=completed&head_sha={ref_hash}",
            root_name="workflow_runs",
            name=workflow,
        )

        run_id = run.get("id") if run else None
        if run_id is None:
            if Environment.DRY_RUN:
                run_id = ":RUN_ID"
                run["status"] = "completed"
            else:
                print(f"{workflow}: no 'push' runs for {ref}")
                return None

        print(
            f"{workflow} #{run.get('run_number', 0)}: {run.get('display_title', '-')}"
        )
        if run.get("status") != "completed":
            return None

        artifact = self.first_from(
            f"/actions/runs/{run_id}/artifacts",
            root_name="artifacts",
            name=archive_name,
        )

        artifact_id = artifact.get("id")
        if artifact_id is None:
            if Environment.DRY_RUN:
                artifact_id = ":ARTIFACT_ID"
            else:
                return None

        cmd = [
            "gh",
            "api",
            "-H",
            "Accept: application/vnd.github+json",
            "-H",
            "X-GitHub-Api-Version: 2022-11-28",
            f"{self.root}/actions/artifacts/{artifact_id}/zip",
        ]

        filename = f"build/downloads/{run.get('run_number', ':RUN_NUMBER' if Environment.DRY_RUN else 'unknown')}/{archive_name}.zip"
        print_args(cmd)
        if not Environment.DRY_RUN:
            os.makedirs(os.path.dirname(filename), exist_ok=True)
            with open(filename, "wb") as output:
                proc = subprocess.Popen(cmd, stdout=output)
                proc.communicate()

        return filename

    def get_unpublished_release(self, tag_name: str):
        return self.first_from(f"/releases", tag_name=tag_name)

    def asset_list(self, release_id: int):
        assets = self.json_from(f"/releases/{release_id}/assets")
        listing = ((asset.get("name"), asset.get("id")) for asset in assets)
        return {
            asset_name: asset_id
            for asset_name, asset_id in listing
            if asset_name is not None and asset_id is not None
        }

    def delete_asset(self, asset_id: int):
        return self.gh(f"/releases/assets/{asset_id}", method="DELETE")

    def upload_asset(self, release_id: int, path: str):
        quoted = urllib.parse.quote_plus(os.path.basename(path))
        upload_url = f"/releases/{release_id}/assets?name={quoted}"
        response = self.json_from(
            upload_url,
            "-F",
            f"data=@{path}",
            method="POST",
            server="https://uploads.github.com",
        )
        errors = response.get("errors")
        if errors is not None:
            raise RuntimeError(json.dumps(errors))

    def upload_assets(self, tag_name: int, paths: List[str]):
        checked("gh", "release", "upload", tag_name, *paths, "--clobber")

    def publish_release(self, release_id: int) -> Optional[str]:
        return self.json_from(
            f"/releases/{release_id}",
            "-f",
            "draft=false",
            "-F",
            "make_latest=legacy",
            method="PATCH",
        ).get("html_url")
