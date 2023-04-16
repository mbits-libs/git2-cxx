#!/usr/bin/env python

import hashlib
import json
import os
import shlex
import subprocess
import sys


def run(*args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return (out, err, p.returncode)


def print_lines(bytes, color):
    lines = bytes.rstrip().decode("utf-8").split("\n")
    if len(lines) > 0 and lines[-1] == "":
        lines = lines[-1]
    if len(lines):
        print("\n".join(f"{color}{line}\033[m" for line in lines), file=sys.stderr)


def print_run(ran):
    out, err, returncode = ran
    if os.name == "nt":
        out = out.replace(b"\r\n", b"\n")
        err = err.replace(b"\r\n", b"\n")
    print_lines(out, "\033[0;49;90m")
    if returncode:
        print_lines(err, "\033[0;49;91m")
        print(f"\033[2;49;91m> program ended with {returncode}\033[m", file=sys.stderr)


def printed_run(*args):
    result = run(*args)
    print(shlex.join(args), file=sys.stderr)
    print_run(result)


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
    printed_run("git", "config", "core.fsmonitor", "false")

for commit in commit_list:
    message = commit["message"]
    commit_files = commit["files"]
    printed_run("git", "add", *commit_files)
    printed_run("git", "commit", "-m", message)

report = {
    "git": {
        "branch": "main",
        "head": output("git", "log", "-1", "--pretty=format:%H"),
    },
    "files": files,
}
json.dump(report, sys.stdout)
