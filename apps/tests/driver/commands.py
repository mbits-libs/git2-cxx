# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import os
import stat
import sys
import tarfile
import zipfile
from typing import Callable, Dict, List, Tuple

from . import test

_file_cache = {}
_rw_mask = stat.S_IWRITE | stat.S_IWGRP | stat.S_IWOTH
_ro_mask = 0o777 ^ _rw_mask


def _touch(test: test.Test, args: List[str]):
    filename = test.path(args[0])
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "wb") as f:
        if len(args) > 1:
            f.write(args[1].encode("UTF-8"))


def _make_RO(test: test.Test, args: List[str]):
    filename = test.path(args[0])
    mode = os.stat(filename).st_mode
    _file_cache[filename] = mode
    os.chmod(filename, mode & _ro_mask)


def _make_RW(test: test.Test, args: List[str]):
    filename = test.path(args[0])
    try:
        mode = _file_cache[filename]
    except KeyError:
        mode = os.stat(filename).st_mode | _rw_mask
    os.chmod(filename, mode)


def _untar(test: test.Test, src, dst):
    with tarfile.open(src) as TAR:

        def is_within_directory(directory, target):
            abs_directory = os.path.abspath(directory)
            abs_target = os.path.abspath(target)

            prefix = os.path.commonprefix([abs_directory, abs_target])

            return prefix == abs_directory

        def safe_extract(tar, path=".", members=None, *, numeric_owner=False):
            for member in tar.getmembers():
                member_path = os.path.join(path, member.name)
                if not is_within_directory(path, member_path):
                    raise Exception("Attempted Path Traversal in Tar File")

            tar.extractall(path, members, numeric_owner=numeric_owner)

        safe_extract(TAR, test.path(dst))


def _unzip(test: test.Test, src, dst):
    with zipfile.ZipFile(src) as ZIP:
        ZIP.extractall(test.path(dst))


_ARCHIVES = {".tar": _untar, ".zip": _unzip}


def _unpack(test: test.Test, args: List[str]):
    archive = args[0]
    dst = args[1]
    reminder, ext = os.path.splitext(archive)
    _, mid = os.path.splitext(reminder)
    if mid == ".tar":
        ext = ".tar"
    _ARCHIVES[ext](test, archive, dst)


def _cat(test: test.Test, args: List[str]):
    filename = args[0]
    with open(test.path(filename)) as f:
        sys.stdout.write(f.read())


HANDLERS: Dict[str, Tuple[int, Callable]] = {
    "mkdirs": (1, lambda test, args: test.makedirs(args[0])),
    "rm": (1, lambda test, args: test.rmtree(args[0])),
    "ro": (1, _make_RO),
    "rw": (1, _make_RW),
    "touch": (1, _touch),
    "cd": (1, lambda test, args: test.chdir(args[0])),
    "unpack": (2, _unpack),
    "cat": (1, _cat),
}
