#!/usr/bin/env python

import os
import sys

name = sys.argv[1]
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
app = os.path.join(root, "libs", "app")

with open(os.path.join(root, "CMakeLists.txt"), "r") as f:
    for attr, value in [line.strip().split(' ', 1) for line in f.read().split(
        'project (cov', 1)[1].split(')', 1)[0].strip().split('\n')]:
        if attr == 'VERSION':
            VERSION = value
            break

def touch(filename, text = None):
    print(filename)
    if text is None:
        try:
            open(filename, "r")
        except FileNotFoundError:
            open(filename, "w")
        return
    with open(filename, "w", encoding='UTF-8') as f:
        print(text, file=f)


touch(os.path.join(app, "data", "strings", f"{name}.lngs"), f'''[
    project("cov"),
	namespace("cov::app::str::{name}"),
    version("{VERSION}"),
    serial(1)
] strings {{
}}''')

os.makedirs(os.path.join(app, "data", "translations", name), exist_ok=True)
touch(os.path.join(app, "include", "cov", "app", f"{name}.hh"))
touch(os.path.join(app, "src", f"{name}.cc"))
