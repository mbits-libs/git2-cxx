#!/usr/bin/env python

import hashlib
import json
import os
import subprocess
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Dict, List


def query_impl(elem: ET.Element, path: List[str]):
    here = path[0]
    rest = path[1:]
    result: List[ET.Element] = []
    for child in elem:
        if child.tag != here:
            continue
        if rest == []:
            result.append(child)
            continue
        result.extend(query_impl(child, rest))
    return result


def query(elem: ET.Element, path: str):
    return query_impl(elem, path.split("/"))


def run(*args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return (out, err, p.returncode)


def output(*args):
    return run(*args)[0].strip().decode("utf-8")


def git_log_format(fmt):
    return output("git", "log", "-1", "--pretty=format:%" + fmt)


root = ET.parse(sys.stdin).getroot()

source = query(root, "sources/source")
if len(source) != 1:
    print("Not a Cobertura XML", file=sys.stderr)
    sys.exit(1)

ROOT_DIR = source[0].text.replace(os.sep, "/")
if len(ROOT_DIR) == 0 or ROOT_DIR[-1] != "/":
    ROOT_DIR += "/"
CLASSES = query(root, "packages/package/classes/class")

GIT_BRANCH = output("git", "rev-parse", "--abbrev-ref", "HEAD")
GIT_HEAD = git_log_format("H")


@dataclass
class LineInfo:
    line: int
    hits: int


@dataclass
class ClassInfo:
    hash: str
    name: str
    lines: List[LineInfo]


collected: Dict[str, ClassInfo] = {}

for class_element in CLASSES:
    try:
        filename = os.path.abspath(
            os.path.join(ROOT_DIR, class_element.attrib["filename"])
        )
    except KeyError:
        continue
    name = os.path.relpath(filename).replace(os.sep, "/")
    # print(name, filename, class_element.attrib["filename"])
    line_elements = query(class_element, "lines/line")
    lines = [
        LineInfo(int(line.attrib["number"]), int(line.attrib["hits"]))
        for line in line_elements
    ]
    if name in collected:
        merged = {line.line: line.hits for line in collected[name].lines}
        for line in lines:
            if line.line not in merged:
                merged[line.line] = 0
            merged[line.line] += line.hits
        collected[name].lines = [LineInfo(line, merged[line]) for line in merged]
        continue
    m = hashlib.sha1()
    with open(filename, "rb") as source:
        read = 8192
        while read > 0:
            buf = source.read(read)
            read = len(buf)
            m.update(buf)

    collected[name] = ClassInfo(m.hexdigest(), name, lines)


def cov_from(info: ClassInfo) -> dict:
    return {
        "name": info.name,
        "digest": f"sha1:{info.hash}",
        "line_coverage": {line.line: line.hits for line in info.lines},
    }


json.dump(
    {
        "$schema": "https://raw.githubusercontent.com/mzdun/cov/v0.23.0/apps/report-schema.json",
        "git": {"branch": GIT_BRANCH, "head": GIT_HEAD},
        "files": [cov_from(collected[name]) for name in sorted(collected.keys())],
    },
    sys.stdout,
)
