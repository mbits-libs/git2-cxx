#!/usr/bin/env python

import os
import subprocess
import sys
import time

PROMPT = "\033[01;32mmarcin@nuc7\033[00m:\033[01;34m~/code/cov\033[00m$ "
LINES = [
    ("", "cov show v0.18.0..:libs/cov-api/include/cov"),
    ("cov show v0.18.0..:libs/cov-api/include/cov", "/format.hh"),
    ("clear", ""),
]

click = 0.1
gawk = 2.5


def write(text):
    sys.stdout.write(text)
    sys.stdout.flush()
    try:
        os.fsync(sys.stdout.fileno())
        os.fsync(sys.stderr.fileno())
    except OSError:
        pass


def prompt():
    write(PROMPT)


first = True
for prefix, line in LINES:
    prompt()
    time.sleep(click)
    if first:
        first = False
    else:
        time.sleep(gawk)
    if prefix:
        write(prefix)
        time.sleep(2 * click)

    for c in [line[idx : idx + 2] for idx in range(len(line)) if idx % 2 == 0]:
        write(c)
        time.sleep(click)
    write(f"\n")
    subprocess.run(prefix + line, shell=True)

write(f"\n")
