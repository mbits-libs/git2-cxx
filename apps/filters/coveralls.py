#!/usr/bin/env python

import json
import sys
import os
import subprocess
from typing import Dict, List, Optional

def cov_version():
    exec = os.environ['COV_EXE_PATH']
    try:
        p = subprocess.Popen([exec, "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, _ = p.communicate()
        if p.returncode == 0:
            return out.strip().decode('UTF-8').split(' ')[-1]
    except FileNotFoundError:
        pass
    return '0.23.0'

def lines_from(lines: List[Optional[int]]) -> Dict[str, int]:
    result: Dict[int, int] = {}
    for index in range(len(lines)):
        count = lines[index]
        if count is not None:
            result[index + 1] = count
    return {str(line): count for line, count in result.items()}


def cov_from(coveralls: dict) -> dict:
    result = {
        "name": coveralls["name"],
        "digest": f"md5:{coveralls['source_digest']}",
        "line_coverage": lines_from(coveralls["coverage"]),
    }
    functions = coveralls.get("functions")
    if functions is not None and len(functions) > 0:
        result["functions"] = functions
    return result


data = json.load(sys.stdin)
git = data["git"]
git_head = git["head"]
VERSION = cov_version()

json.dump(
    {
        "$schema": f"https://raw.githubusercontent.com/mzdun/cov/v${VERSION}/apps/report-schema.json",
        "git": {"branch": git["branch"], "head": git_head["id"]},
        "files": [cov_from(source_file) for source_file in data["source_files"]],
    },
    sys.stdout,
    sort_keys=True,
)
