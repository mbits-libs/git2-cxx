#!/usr/bin/env python

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


def git_log_format(fmt):
    return output("git", "log", "-1", "--pretty=format:%" + fmt)


def cov_version():
    exec = os.environ["COV_EXE_PATH"]
    try:
        p = subprocess.Popen(
            [exec, "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        out, _ = p.communicate()
        if p.returncode == 0:
            return out.strip().decode("UTF-8").split(" ")[-1]
    except FileNotFoundError:
        pass
    return "0.25.0"


message = sys.stdin.read().split()

GIT_BRANCH = output("git", "rev-parse", "--abbrev-ref", "HEAD")
GIT_HEAD = git_log_format("H")
VERSION = cov_version()


json.dump(
    {
        "$schema": f"https://raw.githubusercontent.com/mzdun/cov/v{VERSION}/apps/report-schema.json",
        "git": {"branch": GIT_BRANCH, "head": GIT_HEAD},
        "files": [],
    },
    sys.stdout,
)
