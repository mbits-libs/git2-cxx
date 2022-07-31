#!/usr/bin/env python

import os
import secrets
import string
import subprocess
import sys
import time
from typing import Dict, List, NamedTuple, Tuple

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

GITHUB_ORG = "mzdun"
GITHUB_PROJ = "cov"
GITHUB_LINK = f"https://github.com/{GITHUB_ORG}/{GITHUB_PROJ}"
SEPARATOR = "--------------------------------"

COMMIT_SEP = "--{}".format(
    "".join(secrets.choice(string.ascii_letters + string.digits) for i in range(20))
)

LEVEL_BENIGN = 0
LEVEL_PATCH = 1
LEVEL_FEATURE = 2
LEVEL_BREAKING = 3

ARGS = sys.argv[1:]

flags = ["all", "dry-run"]

all = False
dry_run = False
forced_level = None

for flag in flags:
    value = False
    for index in range(len(ARGS)):
        if ARGS[index] == f"--{flag}":
            value = True
            del ARGS[index]
            break
    globals()[flag.replace("-", "_")] = value

for index in range(len(ARGS)):
    arg = ARGS[index]
    force = "--force="
    if arg[: len(force)] == force:
        lvl = arg[8:].lower()
        try:
            forced_level = {
                "patch": LEVEL_PATCH,
                "fix": LEVEL_PATCH,
                "minor": LEVEL_FEATURE,
                "feat": LEVEL_FEATURE,
                "feature": LEVEL_FEATURE,
                "major": LEVEL_BREAKING,
                "breaking": LEVEL_BREAKING,
                "release": LEVEL_BREAKING,
            }[lvl]
        except KeyError:
            print(f"Unknown level {ARGS[index][8:]}")
            sys.exit(1)
        del ARGS[index]
        break


class Section(NamedTuple):
    key: str
    header: str


BREAKING_CHANGE = "BREAKING_CHANGE"
TYPES = [
    Section(BREAKING_CHANGE, "Breaking"),
    Section("feat", "New Features"),
    Section("fix", "Bug Fixes"),
]
KNOWN_TYPES = [section.key for section in TYPES]
TYPE_FIX = {"docs": "fix"}

ALL_TYPES = {
    "assets": "Assets",
    "build": "Build System",
    "chore": "Chores",
    "ci": "Continuos Integration",
    "perf": "Performance Improvements",
    "refactor": "Code Refactoring",
    "revert": "Reverts",
    "style": "Code Style",
    "test": "Tests",
}


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
    is_breaking: bool
    breaking_message: List[str] = []
    references: Dict[str, List[str]] = {}


class CommitLink(NamedTuple):
    scope: str
    summary: str
    hash: str
    short_hash: str
    is_breaking: bool
    breaking_message: List[str]
    references: Dict[str, List[str]]


ChangeLog = Dict[str, List[CommitLink]]


def level_from_commit(commit: Commit) -> int:
    if commit.is_breaking:
        return LEVEL_BREAKING
    try:
        return {"feat": LEVEL_FEATURE, "fix": LEVEL_PATCH}[commit.type]
    except KeyError:
        return LEVEL_BENIGN


def get_commit(hash: str, short_hash: str, message: str) -> Commit:
    subject, body = (message + "\n\n").split("\n\n", 1)
    split = subject.split(": ", 1)
    if len(split) != 2:
        return None

    encoded, summary = split
    encoded = encoded.strip()
    is_breaking = len(encoded) and encoded[-1] == "!"
    if is_breaking:
        encoded = encoded[:-1].rstrip()
    type_scope = encoded.split("(", 1)
    if not len(type_scope[0]):
        return None
    scope = ""
    if len(type_scope) == 2:
        scope = ")".join(type_scope[1].split(")")[:-1]).strip()

    breaking_change = None
    refs = []
    closes = []
    body = body.strip().split("BREAKING CHANGE:", 1)
    if len(body) > 1:
        breaking_change = [para.strip() for para in body[1].strip().split("\n\n")]

    lines = body[0].strip().split("\n")
    for index_plus_1 in range(len(lines), 0, -1):
        index = index_plus_1 - 1
        footer_line = lines[index].strip()
        if footer_line == "":
            continue
        footer = footer_line.split(": ", 1)
        if len(footer) == 1:
            break
        name = footer[0].strip().lower()
        if name in ["refs", "closes"]:
            items = [v.strip() for v in footer[1].split(",")]
            if name == "refs":
                refs = items + refs
            else:
                closes = items + closes
            continue

    references = {}

    if len(refs):
        references["references"] = refs
    if len(closes):
        references["closes"] = closes

    return Commit(
        type_scope[0].strip(),
        scope,
        summary,
        hash,
        short_hash,
        is_breaking,
        breaking_change,
        references,
    )


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


def get_log(commit_range: List[str]) -> Tuple[ChangeLog, int]:
    args = ["git", "log", f"--format=%h %H%n%B%n{COMMIT_SEP}"]
    if len(commit_range):
        if len(commit_range) == 1:
            args.append(f"{commit_range[0]}..HEAD")
        else:
            args.append("..".join(commit_range[:2]))
    print(" ".join(args))
    print(SEPARATOR)
    proc = subprocess.run(
        args,
        shell=False,
        capture_output=True,
    )
    commit_log = []
    amassed = []
    for line in proc.stdout.decode("UTF-8").split("\n"):
        if line == COMMIT_SEP:
            if len(amassed):
                short_hash, hash = amassed[0].split(" ")
                commit = get_commit(hash, short_hash, "\n".join(amassed[1:]).strip())
                amassed = []

                if commit is None:
                    continue

                commit_log.append(commit)
            continue
        amassed.append(line)

    changes: ChangeLog = {}
    level = LEVEL_BENIGN

    for commit in commit_log:
        # Hide even from --all
        if commit.type == "chore" and commit.summary[:8] == "release ":
            continue
        current_level = level_from_commit(commit)
        if current_level > level:
            level = current_level
        try:
            current_type = TYPE_FIX[commit.type]
        except KeyError:
            current_type = commit.type
        hidden = current_type not in KNOWN_TYPES

        if hidden and not commit.is_breaking and not all:
            continue
        if hidden and commit.is_breaking:
            current_type = BREAKING_CHANGE

        try:
            current_scope = SCOPE_FIX[commit.scope]
        except KeyError:
            current_scope = commit.scope
        if current_type not in changes:
            changes[current_type] = []
        changes[current_type].append(
            CommitLink(
                current_scope,
                commit.summary,
                commit.hash,
                commit.short_hash,
                commit.is_breaking,
                commit.breaking_message,
                commit.references,
            )
        )

    return changes, level


def link_str(link: CommitLink, show_breaking: bool) -> str:
    scope = ""
    if len(link.scope):
        scope = link.scope

    if show_breaking:
        if link.is_breaking:
            if len(scope):
                scope = f"breaking, {scope}"
            else:
                scope = f"breaking"

    if len(scope):
        scope = f"**{scope}**: "

    hash_link = f"{GITHUB_LINK}/commit/{link.hash}"
    return f"- {scope}{link.summary} ([{link.short_hash}]({hash_link}))"


def show_links(links: List[CommitLink], show_breaking: bool) -> List[str]:
    result = [link_str(link, show_breaking) for link in links]
    if len(result):
        result.append("")
    return result


def show_changelog(
    cur_tag: str, prev_tag: str, today: str, for_github: bool
) -> List[str]:
    compare = f"{GITHUB_LINK}/compare/{prev_tag}...{cur_tag}"
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

        show_breaking = section.key != BREAKING_CHANGE

        lines.extend([f"### {section.header}", ""])
        lines.extend(show_links(type_section, show_breaking))

    for section in sorted(LOG.keys()):
        if section in KNOWN_TYPES:
            continue
        type_section = LOG[section]
        try:
            section_header = ALL_TYPES[section]
        except KeyError:
            section_header = section

        lines.extend([f"### {section_header}", ""])
        lines.extend(show_links(type_section, True))

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
    print(SEPARATOR)

    with open(os.path.join(ROOT, "CHANGELOG.md")) as f:
        current = f.read().split("\n## ", 1)
    new_text = current[0] + "\n" + "\n".join(changelog)
    if len(current) > 1:
        new_text += "\n## " + current[1]
    with open(os.path.join(ROOT, "CHANGELOG.md"), "wb") as f:
        f.write(new_text.encode("UTF-8"))


def update_cmake(cur_tag: str):
    if dry_run:
        return
    version = cur_tag[1:].split("-")[0]
    set_version(version)


if len(ARGS) > 0:
    arg = ARGS[0]
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

if forced_level is not None:
    LEVEL = forced_level

if LEVEL == LEVEL_BENIGN:
    print("Cowardly refusing to make an empty release")
    if dry_run:
        print(SEPARATOR)
    else:
        sys.exit(0)

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

if dry_run:
    print(f'Would commit "chore: {MESSAGE}"')
    sys.exit(0)

print(f'Committing "chore: {MESSAGE}"')
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
