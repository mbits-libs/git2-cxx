#!/usr/bin/env python

import zlib
import sys
import os

with open(sys.argv[1], "rb") as input:
    blob = zlib.decompress(input.read())

with os.fdopen(sys.stdout.fileno(), "wb", closefd=False) as stdout:
    stdout.write(blob)
    stdout.flush()
