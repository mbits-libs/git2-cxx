#!/usr/bin/env python

import os
from typing import Dict, List

import polib

from po_helpers import (
    Directories,
    MakeBin,
    MakeList,
    existing_catalogue,
    same_content,
)

root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DIRS = Directories.make(root)

# lang -> file -> POFile
split: Dict[str, Dict[str, List[polib.POEntry]]] = {}
srcs = []

for _, dirs, files in os.walk(DIRS.data_strings):
    dirs[:] = []
    for filename in sorted(files):
        name, ext = os.path.splitext(filename)
        if ext != ".lngs":
            continue
        srcs.append(name)

for _, dirs, files in os.walk(DIRS.data_translations):
    dirs[:] = []
    for filename in sorted(files):
        name, ext = os.path.splitext(filename)
        if ext != ".po":
            continue
        split[name] = {}
        input = polib.pofile(os.path.join(DIRS.data_translations, filename))
        for entry in input:
            # if len != 2 we want the exception here
            lngs, ctxt = entry.msgctxt.split(".")
            entry.msgctxt = ctxt

            if lngs not in split[name]:
                output = polib.POFile()
                output.header = input.header
                output.metadata = input.metadata
                output.metadata_is_fuzzy = input.metadata_is_fuzzy
                split[name][lngs] = output
            split[name][lngs].append(entry)

for lang_code in split:
    files = split[lang_code]
    for name in files:
        output = files[name]
        out_name = os.path.join(DIRS.build_translations, f"{lang_code}/{name}.po")
        os.makedirs(os.path.dirname(out_name), exist_ok=True)
        existing = existing_catalogue(out_name)
        if existing is None or not same_content(output, existing):
            print(f"[PO] {lang_code}/{name}.po")
            output.save(out_name)
            MakeBin.normalize_line_endings(out_name)


srcs = sorted(srcs)
languages = sorted(split.keys())
make = MakeList()

llcc = os.path.join(DIRS.data_translations, "llcc.txt")

for language in languages:
    for src in srcs:
        make.append(MakeBin.make(DIRS, language, src, llcc))

make.run()
