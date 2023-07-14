#!/usr/bin/env python

import json
import sys
from typing import Dict, List, Optional


def lines_from(lines: List[Optional[int]]) -> Dict[int, int]:
    result: Dict[int, int] = {}
    for index in range(len(lines)):
        count = lines[index]
        if count is not None:
            result[index] = count
    return result


def cov_from(coveralls: dict) -> dict:
    return {
        "name": coveralls["name"],
        "digest": f"md5:{coveralls['source_digest']}",
        "line_coverage": lines_from(coveralls["coverage"]),
    }


data = json.load(sys.stdin)
git = data["git"]
git_head = git["head"]
json.dump(
    {
        "$schema": "https://raw.githubusercontent.com/mzdun/cov/v0.18.0/apps/report-schema.json",
        "git": {"branch": git["branch"], "head": git_head["id"]},
        "files": [cov_from(source_file) for source_file in data["source_files"]],
    },
    sys.stdout,
)
