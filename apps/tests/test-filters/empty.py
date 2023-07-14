#!/usr/bin/env python

import json
import subprocess
import sys


def run(*args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return (out, err, p.returncode)


def output(*args):
    return run(*args)[0].strip().decode("utf-8")


def git_log_format(fmt):
    return output("git", "log", "-1", "--pretty=format:%" + fmt)


message = sys.stdin.read().split()

GIT_BRANCH = output("git", "rev-parse", "--abbrev-ref", "HEAD")
GIT_HEAD = git_log_format("H")


json.dump(
    {
        "$schema": "https://raw.githubusercontent.com/mzdun/cov/v0.18.0/apps/report-schema.json",
        "git": {"branch": GIT_BRANCH, "head": GIT_HEAD},
        "files": [],
    },
    sys.stdout,
)
