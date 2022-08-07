#!/usr/bin/env python

import hashlib
import json
import os
import subprocess
import sys


def run(*args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return (out, err, p.returncode)


def output(*args):
    return run(*args)[0].strip().decode("utf-8")


data = json.load(sys.stdin)


files = []
for filename in data["files"]:
    file_info = data["files"][filename]
    print(f"[ADD] {filename}", file=sys.stderr)
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "wb") as f:
        text = b""
        try:
            text = "\n".join(file_info["text"]).encode("UTF-8")
        except KeyError:
            pass
        try:
            hashed = "\n".join(file_info["hashed"]).encode("UTF-8")
        except KeyError:
            hashed = text
        if not len(text) or text[-1] != b"\n":
            text += b"\n"
        if not len(hashed) or hashed[-1] != b"\n":
            hashed += b"\n"

        m = hashlib.sha1()
        m.update(hashed)
        hex = m.hexdigest()
        f.write(text)
        files.append(
            {
                "name": filename,
                "digest": f"sha1:{hex}",
                "line_coverage": file_info["lines"],
            }
        )

try:
    commit_list = data["commits"]
except KeyError:
    commit_list = []

if commit_list:
    run("git", "config", "core.fsmonitor", "false")

for commit in commit_list:
    message = commit["message"]
    commit_files = commit["files"]
    run("git", "add", *commit_files)
    run("git", "commit", "-m", message)

report = {
    "git": {
        "branch": "main",
        "head": output("git", "log", "-1", "--pretty=format:%H"),
    },
    "files": files,
}
json.dump(report, sys.stdout)
