#!/usr/bin/env python

import os
import sys

name = sys.argv[1]
root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
app = os.path.join(root, "libs", "app")
cov_rt = os.path.join(root, "libs", "cov-rt", "include", "cov", "app")


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
    version("latest"),
    serial(1)
] strings {{
}}""",
)

short = name
if short[:4] == "cov_":
    short = short[4:]
Short = short.capitalize()
touch(
    os.path.join(cov_rt, f"{name}_tr.hh"),
    f"""// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/{name}.hh>
#include <cov/app/tr.hh>

namespace cov::app {{
	using {short}lng = str::{name}::faulty;
	using {Short}Strings = str::{name}::Strings;

	template <>
	struct lngs_traits<{short}lng>
	    : base_lngs_traits<{short}lng, "{name}", {Short}Strings> {{}};
}}  // namespace cov::app""",
)
