#!/usr/bin/env python

import hashlib
import os
import shlex
import shutil
import subprocess
import time
from dataclasses import dataclass, field
from pprint import pprint
from typing import Dict, List, Union

INITIAL = 1
CHANGES = 2
MORE_CHANGES = 3
DELETIONS = 4

EXACT = "exact match"
EOL_CHANGE = "partial match"
MISMATCH = "mismatch"
ABSENT = "absent"


def mk_name(*args):
    return ", ".join(" ".join(pair) for pair in zip(["tree", "file"], args))


@dataclass
class mod:
    op: str
    content: Union[str, None] = None


def edit(content: str):
    return mod("A", content)


@dataclass
class file_history:
    filename: str
    test: str
    changes: Dict[int, mod]
    hashed: int = DELETIONS
    tested: int = INITIAL
    tested_oid: str = None
    hashes: Dict[str, str] = field(default_factory=lambda: {})
    lines: int = 0


@dataclass
class history_op:
    filename: str
    op: str
    content: Union[str, None]


def make_content(which: str, eol: str, will_change: bool = False):
    return (
        eol.join(
            [
                f"this is a file with {which} line endings$description",
                f"those ending will{'' if will_change else ' not' } change in the future",
                "",
                "Mary had a little lamb,",
                "whose fleece was white as snow.",
                "And everywhere that Mary went,",
                "the lamb was sure to go.",
            ]
        )
        + eol
    )


def make_content2(which: str, eol: str, will_change: bool = False) -> bytes:
    return (
        eol.join(
            [
                f"this is a file with {which} line endings$description",
                f"those ending will{'' if will_change else ' not' } change in the future",
                "",
                "this file has changed, and so did digests",
            ]
        )
        + eol
    )


history: List[file_history] = [
    file_history(
        "is/unix",
        mk_name(EXACT),
        {INITIAL: edit(make_content("unix", "\n"))},
    ),
    file_history(
        "is/dos",
        mk_name(EXACT),
        {INITIAL: edit(make_content("dos", "\r\n"))},
    ),
    ################################################################################################
    file_history(
        "was/unix",
        mk_name(EOL_CHANGE),
        {
            INITIAL: edit(
                make_content("unix", "\n", will_change=True),
            ),
            CHANGES: edit(make_content("unix", "\r\n", will_change=True)),
        },
    ),
    file_history(
        "was/dos",
        mk_name(EOL_CHANGE),
        {
            INITIAL: edit(
                make_content("dos", "\r\n", will_change=True),
            ),
            CHANGES: edit(make_content("dos", "\n", will_change=True)),
        },
    ),
    ################################################################################################
    file_history(
        "is/unix_changed",
        mk_name(MISMATCH, EXACT),
        {
            INITIAL: edit(make_content("unix", "\n")),
            CHANGES: edit(make_content2("unix", "\n")),
        },
    ),
    file_history(
        "is/dos_changed",
        mk_name(MISMATCH, EXACT),
        {
            INITIAL: edit(make_content("dos", "\r\n")),
            CHANGES: edit(make_content2("dos", "\r\n")),
        },
    ),
    ################################################################################################
    file_history(
        "was/unix_changed",
        mk_name(MISMATCH, EOL_CHANGE),
        {
            INITIAL: edit(
                make_content("unix", "\n", will_change=True),
            ),
            CHANGES: edit(make_content2("unix", "\n", will_change=True)),
            MORE_CHANGES: edit(make_content2("unix", "\r\n", will_change=True)),
        },
        hashed=CHANGES,
    ),
    file_history(
        "was/dos_changed",
        mk_name(MISMATCH, EOL_CHANGE),
        {
            INITIAL: edit(
                make_content("dos", "\r\n", will_change=True),
            ),
            CHANGES: edit(make_content2("dos", "\r\n", will_change=True)),
            MORE_CHANGES: edit(make_content2("dos", "\n", will_change=True)),
        },
        hashed=CHANGES,
    ),
    ################################################################################################
    file_history(
        "changed",
        mk_name(MISMATCH, MISMATCH),
        {
            INITIAL: edit("this is first version of this file\n"),
            CHANGES: edit("this is second version of this file\n"),
            DELETIONS: edit("this is final version of this file\n"),
        },
        hashed=CHANGES,
    ),
    ################################################################################################
    file_history(
        "no_more",
        mk_name(MISMATCH, ABSENT),
        {
            INITIAL: edit(
                "Let us hurry to love files because they depart so quickly\n"
            ),
            CHANGES: edit(
                "Let us hurry to love people because they depart so quickly.\nks. Jan Twardowski\n",
            ),
            DELETIONS: mod("D"),
        },
        hashed=INITIAL,
        tested=CHANGES,
    ),
    ################################################################################################
    file_history(
        "is/unix_added",
        mk_name(ABSENT, EXACT),
        {CHANGES: edit(make_content("unix", "\n"))},
        tested=INITIAL,
        hashed=CHANGES,
    ),
    file_history(
        "is/dos_added",
        mk_name(ABSENT, EXACT),
        {CHANGES: edit(make_content("dos", "\r\n"))},
        tested=INITIAL,
        hashed=CHANGES,
    ),
    ################################################################################################
    file_history(
        "was/unix_added",
        mk_name(ABSENT, EOL_CHANGE),
        {
            CHANGES: edit(make_content("unix", "\n", will_change=True)),
            MORE_CHANGES: edit(make_content("unix", "\r\n", will_change=True)),
        },
        tested=INITIAL,
        hashed=CHANGES,
    ),
    file_history(
        "was/dos_added",
        mk_name(ABSENT, EOL_CHANGE),
        {
            CHANGES: edit(make_content("dos", "\r\n", will_change=True)),
            MORE_CHANGES: edit(make_content("dos", "\n", will_change=True)),
        },
        tested=INITIAL,
        hashed=CHANGES,
    ),
    ################################################################################################
    file_history(
        "changed_added",
        mk_name(ABSENT, MISMATCH),
        {
            CHANGES: edit(
                "(added)\nLet us hurry to love files because they depart so quickly\n",
            ),
            MORE_CHANGES: edit(
                "(added)\nLet us hurry to love people because they depart so quickly.\nks. Jan Twardowski\n",
            ),
        },
        tested=INITIAL,
        hashed=CHANGES,
    ),
    ################################################################################################
]

all_files = {file.filename: file for file in history}


commit_description = {
    INITIAL: " created in initial commit",
    CHANGES: " added in second wave of files",
}

commits = []

for commit_no in range(DELETIONS):
    key = commit_no + 1
    changes: List[history_op] = []
    hashes: List[str] = []
    tests: List[str] = []
    try:
        description = commit_description[key]
    except KeyError:
        description = None
    for file in history:
        if file.hashed == key:
            hashes.append(file.filename)
        if file.tested == key:
            tests.append(file.filename)
        try:
            op = file.changes[key]
        except KeyError:
            continue
        if description is not None and op.op == "A":
            if "$description" in op.content:
                for all in file.changes:
                    file.changes[all].content = file.changes[all].content.replace(
                        "$description", description
                    )
        changes.append(history_op(file.filename, op.op, op.content))
    commits.append((changes, hashes, tests))

root = "verify"


def A(file: str, contents: Union[str, None]):
    filename = os.path.join(root, file)
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "wb") as f:
        if contents is not None:
            f.write(contents.encode("UTF-8"))
    print(shlex.join(["git", "add", file]))
    subprocess.run(["git", "-C", root, "add", file], shell=False, capture_output=True)


def D(file: str, _):
    print(shlex.join(["git", "rm", file]))
    subprocess.run(["git", "-C", root, "rm", file], shell=False, capture_output=True)


def git_commit(message: str):
    print(shlex.join(["git", "commit", "-m", message]))
    subprocess.run(
        ["git", "-C", root, "commit", "-m", message], shell=False, capture_output=True
    )


def config(key: str, value: str):
    # print(shlex.join(["git", "config", "--local", key, value]))
    subprocess.run(["git", "-C", root, "config", "--local", key, value], shell=False)


commands = {"A": A, "D": D}
hashes = {"md5": hashlib.md5, "sha1": hashlib.sha1}

shutil.rmtree("verify", ignore_errors=True)
os.makedirs("verify", exist_ok=True)
subprocess.run(["git", "-C", "verify", "init"], shell=False)
config("user.name", "Johnny Appleseed")
config("user.email", "johnny@appleseed.com")
config("core.autocrlf", "false")

counter = 0
for ops, hashed_files, _ in commits:
    counter += 1
    for op in ops:
        cmd = commands[op.op]
        cmd(op.filename, op.content)
    git_commit(f"commit #{counter}")
    time.sleep(2.5)

    for filename in hashed_files:
        with open(os.path.join(root, filename), "rb") as f:
            content = f.read()
            lines = content.split(b"\n")
            if len(lines) and lines[-1] == b"":
                lines = lines[:-1]
            all_files[filename].lines = len(lines)
        for hash in hashes:
            all_files[filename].hashes[hash] = hashes[hash](content).hexdigest()

out = subprocess.run(
    ["git", "-C", root, "log", "--format=%H %ct %s"], shell=False, capture_output=True
)

commit_info = {}
for line in out.stdout.decode("utf-8").rstrip().split("\n"):
    hash, ts, commit = line.split(" ", 2)
    commit_info[int(commit.split("#")[1])] = [hash, ts]

counter = 0
for _, _, tested_files in commits:
    counter += 1
    if not len(tested_files):
        continue

    commit_id = commit_info[counter][0]
    ids = [f"{commit_id}:{filename}" for filename in tested_files]
    out = subprocess.run(
        ["git", "-C", root, "rev-parse", *ids],
        shell=False,
        capture_output=True,
    )
    for filename, oid in zip(
        tested_files, out.stdout.decode("utf-8").rstrip().split("\n")
    ):
        all_files[filename].tested_oid = oid


results = {
    mk_name(EXACT): ("text::in_repo", False, False),
    mk_name(EOL_CHANGE): ("text::in_repo | text::different_newline", False, False),
    mk_name(MISMATCH, EXACT): ("text::in_fs", True, False),
    mk_name(MISMATCH, EOL_CHANGE): (
        "text::in_fs | text::different_newline",
        True,
        False,
    ),
    mk_name(MISMATCH, MISMATCH): (
        "text::in_repo | text::in_fs | text::mismatched",
        True,
        True,
    ),
    mk_name(MISMATCH, ABSENT): ("text::missing", True, True),
    mk_name(ABSENT, EXACT): ("text::in_fs", True, False),
    mk_name(ABSENT, EOL_CHANGE): ("text::in_fs | text::different_newline", True, False),
    mk_name(ABSENT, MISMATCH): ("text::in_fs | text::mismatched", True, True),
    mk_name(ABSENT, ABSENT): ("text::missing", True, True),
}


def make_test(
    title: str,
    filename: str,
    commit_id: List[str],
    hashes: Dict[str, str],
    oid: Union[str, None],
    lines: int,
):
    test_result, remove_oid, remove_lines = results[title]
    if remove_oid:
        oid = None
    if remove_lines:
        lines = 0
    oid_list = [f'    .oid="{oid}"sv,'] if oid is not None else []
    lines_list = [f"    .lines = {lines},"] if lines != 0 else []
    return [
        f'.title = "{title}"sv,',
        f'.filename = "{filename}"sv,',
        f'.commit = "{commit_info[commit_id][0]}"sv,',
        *[f'.{hash} = "{hashes[hash]}"sv,' for hash in sorted(hashes.keys())],
        f".expected = {{",
        f"    .result = {test_result},",
        f"    .committed = {commit_info[commit_id][1]}s,",
        f'    .message = "commit #{commit_id}"sv,',
        *oid_list,
        *lines_list,
        "},",
    ]


reorg = {}
for file in history:
    if file.test not in reorg:
        reorg[file.test] = []
    reorg[file.test].append(
        make_test(
            file.test,
            file.filename,
            file.tested,
            file.hashes,
            file.tested_oid,
            file.lines,
        )
    )

no_such = b"This file was never in the repo\n"
no_such_hashes = {}
for hash in hashes:
    no_such_hashes[hash] = hashes[hash](no_such).hexdigest()

reorg[mk_name(ABSENT, ABSENT)] = [
    make_test(mk_name(ABSENT, ABSENT), "no_such", 1, no_such_hashes, None, 0)
]

with open("verify-test.inc", "wb") as output:
    for test_name in sorted(reorg.keys()):
        for test in reorg[test_name]:
            text = "\n    ".join(test)
            tests = f"{{\n    {text}\n}},\n"
            output.write(tests.encode("utf-8"))
