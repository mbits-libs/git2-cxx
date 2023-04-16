#!/usr/bin/env python

import os
from po_helpers import (
    Directories,
    MakeCombined,
    MakeEnums,
    MakeList,
    MakePot,
    MakeRes,
)

root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DIRS = Directories.make(root)

make = MakeList()
srcs = []

for _, dirs, files in os.walk(DIRS.data_strings):
    dirs[:] = []
    for filename in sorted(files):
        src, ext = os.path.splitext(filename)
        if ext != ".lngs":
            continue
        make.append(MakePot.make(DIRS, src))
        srcs.append(src)

make.append(MakeCombined.make(DIRS, srcs))

for src in srcs:
    make.append(MakeRes.make(DIRS, src))

for src in srcs:
    make.append(MakeEnums.make(DIRS, src))

make.run()
