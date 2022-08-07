#!/usr/bin/env python

import json
import subprocess
import sys

data = json.load(sys.stdin)


def run(*args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return (out, err, p.returncode)


def output(*args):
    return run(*args)[0].strip().decode("utf-8")


data["git"]["head"] = output("git", "log", "-1", "--pretty=format:%H")
json.dump(data, sys.stdout)
