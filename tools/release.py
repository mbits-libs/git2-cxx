#!/usr/bin/env python

import os
import subprocess
import sys
import time
from typing import Dict, List, NamedTuple, Tuple

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class Section(NamedTuple):
    key: str
    header: str


TYPES = [
    Section("BREAKING_CHANGES", "Breaking Changes"),
    Section("feat", "New Features"),
    Section("fix", "Bug Fixes"),
]
KNOWN_TYPES = [section.key for section in TYPES]

SCOPES = [
    Section("core", "Application"),
    Section("rt", "App Runtime"),
    Section("app", "Application Library"),
    Section("api", "API"),
    Section("lighter", "Lighter"),
    Section("webapi", "Web API"),
    Section("webapp", "Node Application"),
    Section("server", "HTTP Server"),
    Section("lang", "Language Strings"),
    Section("cmake", "CMake"),
    Section("conan", "Conan"),
    Section("yarn", "yarn"),
]


SCOPE_FIX = {"ws": "webapi", "rest": "webapi", "lngs": "lang"}


def get_version():
    with open(os.path.join(ROOT, "CMakeLists.txt"), "r") as f:
        attr, rest = f.read().split("project (cov", 1)[1].split(")", 1)

    for attr, value in [
        line.strip().split(" ", 1) for line in attr.strip().split("\n")
    ]:
        if attr == "VERSION":
            version = value
            break
    rest = rest.split('set(PROJECT_VERSION_STABILITY "', 1)[1].split('"')[0]
    return (version, rest)


def set_version(version: str):
    with open(os.path.join(ROOT, "CMakeLists.txt"), "r") as f:
        text = f.read()
    preamble, text = text.split("project (cov", 1)
    attrs, text = text.split(")", 1)
    before, rest = attrs.split("VERSION ")
    _, after = rest.split("\n", 1)
    with open(os.path.join(ROOT, "CMakeLists.txt"), "wb") as f:
        f.write(
            f"{preamble}project (cov{before}VERSION {version}\n{after}){text}".encode(
                "UTF-8"
            )
        )


class Commit(NamedTuple):
    type: str
    scope: str
    summary: str
    hash: str
    short_hash: str
    breaking: bool


class CommitLink(NamedTuple):
    summary: str
    hash: str
    short_hash: str
    breaking: bool


ChangeLog = Dict[str, Dict[str, List[CommitLink]]]

LEVEL_BENIGN = 0
LEVEL_PATCH = 1
LEVEL_FEATURE = 2
LEVEL_BREAKING = 3


def level_from_commit(commit: Commit) -> int:
    if commit.breaking:
        return LEVEL_BREAKING
    try:
        return {"feat": LEVEL_FEATURE, "fix": LEVEL_PATCH}[commit.type]
    except KeyError:
        return LEVEL_BENIGN


def get_commit(message: str, hash: str, short_hash: str) -> Commit:
    split = message.split(": ", 1)
    if len(split) != 2:
        return None
    encoded, summary = split
    encoded = encoded.strip()
    breaking = len(encoded) and encoded[-1] == "!"
    if breaking:
        encoded = encoded[:-1].rstrip()
    type_scope = encoded.split("(", 1)
    if not len(type_scope[0]):
        return None
    scope = ""
    if len(type_scope) == 2:
        scope = ")".join(type_scope[1].split(")")[:-1]).strip()
    return Commit(type_scope[0].strip(), scope, summary, hash, short_hash, breaking)


def sem_ver(tag):
    split = tag.split("-", 1)
    if len(split) == 2:
        stability = split[1]
    else:
        stability = ""
    ver = [int(s) for s in split[0].split(".")]
    while len(ver) < 3:
        ver.append(0)
    return (*ver, stability)


def get_tags(version: str, stability: str) -> str:
    tags = (
        subprocess.run(
            ["git", "tag"],
            shell=False,
            capture_output=True,
        )
        .stdout.decode("UTF-8")
        .split("\n")
    )
    versions = []
    for tag in tags:
        if tag[:1] != "v":
            continue
        ver = [*sem_ver(tag[1:])]
        if ver[3] == "":
            ver[3] = "z"
        versions.append([(*ver,), tag])
    versions = list(reversed(sorted(versions)))

    ver = [*sem_ver(f"{version}{stability}")]
    if ver[3] == "":
        ver[3] = "z"
    ver = (*ver,)
    for index in range(len(versions)):
        if versions[index][0] > ver:
            continue
        if index > 0:
            return [versions[index][1], versions[index - 1][1]]
        return [versions[index][1], "HEAD"]
    return []


def get_log(range: List[str]) -> Tuple[ChangeLog, int]:
    args = ["git", "log", "--format=%s%n%H%n%h"]
    if len(range):
        if len(range) == 1:
            args.append(f"{range[0]}..HEAD")
        else:
            args.append("..".join(range[:2]))
    print(" ".join(args))
    proc = subprocess.run(
        args,
        shell=False,
        capture_output=True,
    )
    log = proc.stdout.decode("UTF-8").split("\n")
    changes: ChangeLog = {}
    level = LEVEL_BENIGN
    for triple in zip(log[::3], log[1::3], log[2::3]):
        commit = get_commit(*triple)
        if commit is None:
            continue
        if commit.type == "chore" and commit.summary[:8] == "release ":
            continue
        current_level = level_from_commit(commit)
        if current_level > level:
            level = current_level
        hidden = commit.type not in KNOWN_TYPES
        current_type = (
            "BREAKING_CHANGE"
            if hidden and commit.breaking
            else commit.type
            if not hidden
            else "other"
        )
        try:
            current_scope = SCOPE_FIX[commit.scope]
        except KeyError:
            current_scope = commit.scope
        if current_type not in changes:
            changes[current_type] = {}
        if current_scope not in changes[current_type]:
            changes[current_type][current_scope] = []
        changes[current_type][current_scope].append(
            CommitLink(commit.summary, commit.hash, commit.short_hash, commit.breaking)
        )

    return changes, level


def link_str(link: CommitLink, show_breaking: bool, for_github: bool) -> str:
    breaking = ""
    if show_breaking:
        if link.breaking:
            breaking = "💥 "
    hash_link = f"https://github.com/mzdun/cov/commit/{link.hash}"
    if for_github:
        hash_link = link.short_hash
    return f"- {breaking}{link.summary} ([{link.short_hash}]({hash_link}))"


def show_links(
    links: List[CommitLink], show_breaking: bool, for_github: bool
) -> List[str]:
    result = [link_str(link, show_breaking, for_github) for link in links]
    if len(result):
        result.append("")
    return result


def show_section(
    input: Dict[str, List[CommitLink]], show_breaking: bool, for_github: bool
):
    section = {key: input[key] for key in input}
    try:
        items = section[""]
        del section[""]
    except KeyError:
        items = []

    lines = show_links(items, show_breaking, for_github)

    for scope in SCOPES:
        try:
            items = section[scope.key]
        except KeyError:
            continue

        del section[scope.key]

        lines.extend([f"#### {scope.header}", ""])
        lines.extend(show_links(items, show_breaking, for_github))

    for scope in sorted(section.keys()):
        lines.extend([f"#### {scope}", ""])
        lines.extend(show_links(section[scope], show_breaking, for_github))

    return lines


def show_changelog(
    cur_tag: str, prev_tag: str, today: str, for_github: bool
) -> List[str]:
    compare = f"https://github.com/mzdun/cov/compare/{prev_tag}..{cur_tag}"
    lines = []
    if not for_github:
        lines = [
            f"## [{cur_tag[1:]}]({compare}) ({today})",
            "",
        ]

    for section in TYPES:
        try:
            type_section = LOG[section.key]
        except KeyError:
            continue

        show_breaking = section.key != "BREAKING_CHANGES"

        lines.extend([f"### {section.header}", ""])
        lines.extend(show_section(type_section, show_breaking, for_github))

    if "other" in LOG:
        type_section = LOG["other"]
        lines.extend([f"### Other Changes", ""])
        lines.extend(show_section(type_section, True, for_github))

    if for_github:
        lines.append(f"**Full Changelog**: {compare}")

    return lines


def read_tag_date(tag: str):
    proc = subprocess.run(
        ["git", "log", "-n1", "--format=%aI", tag], shell=False, capture_output=True
    )
    if proc.returncode != 0:
        return time.strftime("%Y-%m-%d")
    return proc.stdout.decode("UTF-8").split("T", 1)[0]


def update_changelog(cur_tag: str, prev_tag: str):
    today = read_tag_date(cur_tag)
    changelog = show_changelog(cur_tag, prev_tag, today, False)
    github = show_changelog(cur_tag, prev_tag, today, True)

    print("\n".join(github))
    print("--------------------------------")

    with open(os.path.join(ROOT, "CHANGELOG.md")) as f:
        current = f.read().split("\n## ", 1)
    new_text = current[0] + "\n" + "\n".join(changelog)
    if len(current) > 1:
        new_text += "\n## " + current[1]
    with open(os.path.join(ROOT, "CHANGELOG.md"), "wb") as f:
        f.write(new_text.encode("UTF-8"))


def update_cmake(cur_tag: str):
    version = cur_tag[1:].split("-")[0]
    set_version(version)


if len(sys.argv) > 1:
    arg = sys.argv[1]
    if arg[:1] == "v":
        arg = arg[1:]
    if "-" not in arg:
        VERSION = arg
        STABILITY = ""
    else:
        VERSION, stability = arg.split("-", 1)
        STABILITY = f"-{stability}"
else:
    VERSION, STABILITY = get_version()

PREV_TAGS = get_tags(VERSION, STABILITY)
LOG, LEVEL = get_log(PREV_TAGS)
SEMVER = [*sem_ver(VERSION)]

if LEVEL > LEVEL_BENIGN:
    lvl = LEVEL_BREAKING - LEVEL
    SEMVER[lvl] += 1
    for index in range(lvl + 1, len(SEMVER)):
        SEMVER[index] = 0

NEW_TAG = "v{VERSION}{STABILITY}".format(
    VERSION=".".join(str(v) for v in SEMVER[:-1]), STABILITY=STABILITY
)

update_changelog(NEW_TAG, PREV_TAGS[0])
update_cmake(NEW_TAG)

MESSAGE = f"release {NEW_TAG[1:]}"

subprocess.check_call(
    [
        "git",
        "add",
        os.path.join(ROOT, "CMakeLists.txt"),
        os.path.join(ROOT, "CHANGELOG.md"),
    ],
    shell=False,
)
subprocess.check_call(["git", "commit", "-m", f"chore: {MESSAGE}"], shell=False)
subprocess.check_call(["git", "tag", "-am", MESSAGE, NEW_TAG], shell=False)
