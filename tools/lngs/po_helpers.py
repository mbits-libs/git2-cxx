import os
import subprocess
from abc import ABC, abstractmethod
from dataclasses import dataclass
import shlex
from typing import List

import polib


def existing_catalogue(path):
    try:
        with open(path):
            pass
    except FileNotFoundError:
        return None
    return polib.pofile(path)


def entries_to_dict(cat: polib.POFile):
    result = {}
    for entry in cat:
        result[entry.msgctxt] = (
            entry.msgid,
            entry.msgid_plural,
            entry.msgstr,
            entry.msgstr_plural,
        )
    return result


def meta_of(cat: polib.POFile):
    return {
        key: cat.metadata[key]
        for key in cat.metadata
        if key not in ["POT-Creation-Date", "PO-Revision-Date"]
    }


def same_content(left: polib.POFile, right: polib.POFile):
    if meta_of(left) != meta_of(right):
        return False
    return entries_to_dict(left) == entries_to_dict(right)


@dataclass
class Directories:
    data_strings: str
    data_translations: str
    data_intermediate: str
    build_translations: str
    libs_app: str

    @staticmethod
    def make(root: str) -> "Directories":
        dirs = Directories(
            data_strings=os.path.join(root, "data", "strings"),
            data_translations=os.path.join(root, "data", "translations"),
            data_intermediate=os.path.join(root, "data", "intermediate").replace(
                "\\", "/"
            ),
            build_translations=os.path.join(root, "build", "data", "translation"),
            libs_app=os.path.join(root, "libs", "app").replace("\\", "/"),
        )

        for dirname in [
            dirs.data_strings,
            dirs.data_translations,
            dirs.data_intermediate,
            dirs.build_translations,
            os.path.join(dirs.libs_app, "src/strings"),
            os.path.join(dirs.libs_app, "include/cov/app/strings"),
        ]:
            os.makedirs(dirname, exist_ok=True)

        return dirs


class MakeRule(ABC):
    def __init__(self, tag_value: str, dst: str, inputs: List[str]):
        self.tag_value = tag_value
        self.dst = dst
        self.inputs = inputs

    def should_run(self):
        try:
            dst_mtime = os.stat(self.dst).st_mtime
            for input_path in self.inputs:
                in_mtime = os.stat(input_path).st_mtime
                if dst_mtime < in_mtime:
                    return True
        except FileNotFoundError:
            return True
        return False

    def call(self, *command):
        try:
            subprocess.check_call(command, shell=False)
        except subprocess.CalledProcessError as ex:
            print(shlex.join(command))
            raise ex

    def mark_generated(self, filename):
        with open(filename, encoding="UTF-8") as infile:
            contents = infile.readlines()

        for index in range(len(contents)):
            if contents[index][:3] == "// ":
                continue
            contents.insert(index, "// @generated\n")
            break

        with open(filename, "wb") as outfile:
            outfile.write(("".join(contents)).encode("UTF-8"))

    @abstractmethod
    def run(self):
        pass


class MakePot(MakeRule):
    @staticmethod
    def make(dirs: Directories, src: str) -> "MakePot":
        return MakePot(
            f"{src}.pot",
            os.path.join(dirs.build_translations, f"{src}.pot"),
            [os.path.join(dirs.data_strings, f"{src}.lngs")],
        )

    def run(self):
        print(f"[POT] {self.tag_value}")
        self.call(
            "lngs",
            "pot",
            self.inputs[0],
            "-o",
            self.dst,
            "-a",
            "Marcin Zdun <mzdun@midnightbits.com>",
            "-c",
            "midnightBITS",
            "-t",
            "Code coverage presented locally",
        )


class MakeCombined(MakeRule):
    @staticmethod
    def make(dirs: Directories, sources: List[str]) -> "MakeCombined":
        dst = os.path.join(dirs.data_translations, "cov.pot")
        inputs = [
            os.path.join(dirs.build_translations, f"{src}.pot")
            for src in sorted(sources)
        ]
        return MakeCombined("translations/cov.pot", dst, inputs)

    def run(self):
        output = None
        for filename in sorted(self.inputs):
            key = os.path.splitext(os.path.basename(filename))[0]
            input = polib.pofile(filename)
            if output is None:
                output = polib.POFile()
                output.header = input.header
                output.metadata = input.metadata
                output.metadata_is_fuzzy = input.metadata_is_fuzzy
            for entry in input:
                entry.msgctxt = f"{key}.{entry.msgctxt}"
                output.append(entry)
        existing = existing_catalogue(self.dst)
        if existing is None or not same_content(output, existing):
            print(f"[POT] {self.tag_value}")
            output.save(self.dst)
        else:
            with open(self.dst, "r+"):
                pass


class MakeRes(MakeRule):
    def __init__(self, tag_value: str, dst: str, inputs: List[str], include: str):
        super().__init__(tag_value, dst, inputs)
        self.include = include

    @staticmethod
    def make(dirs: Directories, src: str) -> "MakeRes":
        short_name = f"src/strings/{src}.cc"
        inc_name = f"cov/app/strings/{src}.hh"
        return MakeRes(
            short_name,
            f"{dirs.libs_app}/{short_name}",
            [os.path.join(dirs.data_strings, f"{src}.lngs")],
            inc_name,
        )

    def run(self):
        print(f"[CC] {self.tag_value}")
        self.call(
            "lngs",
            "res",
            self.inputs[0],
            "-o",
            self.dst,
            "--include",
            self.include,
            "--warp",
        )
        self.mark_generated(self.dst)


class MakeEnums(MakeRule):
    @staticmethod
    def make(dirs: Directories, src: str) -> "MakeEnums":
        short_name = f"cov/app/strings/{src}.hh"
        return MakeEnums(
            short_name,
            f"{dirs.libs_app}/include/{short_name}",
            [os.path.join(dirs.data_strings, f"{src}.lngs")],
        )

    def run(self):
        print(f"[HH] {self.tag_value}")
        self.call(
            "lngs",
            "enums",
            self.inputs[0],
            "-o",
            self.dst,
            "--resource",
        )
        self.mark_generated(self.dst)


class MakeBin(MakeRule):
    @staticmethod
    def make(dirs: Directories, language: str, src: str, llcc: str) -> "MakeBin":
        short_name = f"{language}/{src}"
        local_name = f"{language}/{src}.po"
        return MakeBin(
            short_name,
            f"{dirs.data_intermediate}/{short_name}",
            [
                os.path.join(dirs.data_strings, f"{src}.lngs"),
                os.path.join(dirs.build_translations, local_name),
                llcc,
            ],
        )

    def run(self):
        print(f"[INT] {self.tag_value}")
        self.call(
            "lngs",
            "make",
            self.inputs[0],
            "-o",
            self.dst,
            "-m",
            self.inputs[1],
            "-l",
            self.inputs[2],
        )


class MakeList(List[MakeRule]):
    def run(self):
        for rule in self:
            if rule.should_run():
                rule.run()
