#!/usr/bin/env python

import os
import sys

name = sys.argv[1]
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
app = os.path.join(root, "libs", "app")
cov_rt = os.path.join(root, "libs", "cov-rt", "include", "cov", "app")

with open(os.path.join(root, "CMakeLists.txt"), "r") as f:
    for attr, value in [
        line.strip().split(" ", 1)
        for line in f.read()
        .split("project (cov", 1)[1]
        .split(")", 1)[0]
        .strip()
        .split("\n")
    ]:
        if attr == "VERSION":
            VERSION = value
            break


def touch(filename, text=None):
    print(filename)
    if text is None:
        try:
            open(filename, "r")
        except FileNotFoundError:
            open(filename, "w")
        return
    with open(filename, "w", encoding="UTF-8") as f:
        print(text, file=f)


touch(
    os.path.join(root, "data", "strings", f"{name}.lngs"),
    f"""[
    project("cov"),
    namespace("cov::app::str::{name}"),
    version("{VERSION}"),
    serial(1)
] strings {{
}}""",
)

touch(os.path.join(app, "include", "cov", "app", "strings", f"{name}.hh"))
touch(os.path.join(app, "src", "strings", f"{name}.cc"))
short = name
if short[:4] == "cov_":
    short = short[4:]
Short = short.capitalize()
touch(
    os.path.join(cov_rt, f"{name}_tr.hh"),
    f"""// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/{name}.hh>
#include <cov/app/tr.hh>

namespace cov::app {{
	using {short}lng = str::{name}::lng;
	using {Short}Strings = str::{name}::Strings;

	template <>
	struct lngs_traits<{short}lng>
	    : base_lngs_traits<{short}lng, "{name}", {Short}Strings> {{}};
}}  // namespace cov::app
""",
)
