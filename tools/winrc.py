#!/usr/bin/env python3

import os
import sys
import polib

from typing import Dict, NamedTuple, Optional, Tuple
from winrc_winnt import *

template = """
LANGUAGE {lang}, {sublang}

VS_VERSION_INFO VERSIONINFO
	FILEVERSION    RC_VERSION
	PRODUCTVERSION RC_VERSION
	FILETYPE       VFT_APP
{{
	BLOCK "StringFileInfo"
	{{
		BLOCK "{langid:04x}04b0"
		{{
			VALUE "CompanyName", RC_ORGANISATION
			VALUE "FileDescription", L"{fileName}{fileModule}"
			VALUE "FileVersion", RC_VERSION_STRING
			VALUE "InternalName", RC_MODULE
			VALUE "OriginalFilename", RC_MODULE ".{fileExt}"
			VALUE "ProductName", L"Cov"
			VALUE "ProductVersion", RC_VERSION_STRING
			VALUE "LegalCopyright", "Copyright (c) 2022 by " RC_ORGANISATION
		}}
	}}
	BLOCK "VarFileInfo"
	{{
		VALUE "Translation", 0x{langid:x}, 1200
	}}
}}
"""


class Translation(NamedTuple):
    context: str
    message: str


APP = {
    "fileName": Translation("DESCRIPTION", "Cov for Windows"),
}


class LocalInfo(NamedTuple):
    fileName: str


class Language(NamedTuple):
    lang: str
    sublang: str
    res_sublang: str


root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
translations_dir = os.path.join(root, "data", "translations")
dest = os.path.join(sys.argv[1] if len(sys.argv) > 1 else ".", "versioninfo.rc")

# pl_PL -> 0x415, but LANGUAGE uses SUBLANG_DEFAULT
# en_US -> 0x409, but LANGUAGE uses SUBLANG_ENGLISH_US
# en_UK -> 0x809, but LANGUAGE uses SUBLANG_ENGLISH_UK
#                 (sidenote, 0x08 is, for instance, SUBLANG_ENGLISH_JAMAICA)
# The file-level LANGUAGE has nothing to do with StringFileInfo langid. The relation is magical.
# Better force Visual Studio to do your bidding. Create a dummy RC with VERSIONINFO and tweak
# language of the resource and of StringFileInfo...


def mklang(lang, sublang, override=None):
    if override == None:
        override = sublang
    return Language(lang, sublang, override)


langs = {
    "pl": mklang(LANG_POLISH, SUBLANG_NEUTRAL),
    "pl_PL": mklang(LANG_POLISH, SUBLANG_DEFAULT, 0x04),
    "en": mklang(LANG_ENGLISH, SUBLANG_NEUTRAL),
    "en_UK": mklang(LANG_ENGLISH, SUBLANG_ENGLISH_UK, 0x08),
    "en_US": mklang(LANG_ENGLISH, SUBLANG_ENGLISH_US, 0x04),
}


def tr(string: Translation, lang: dict = {}) -> str:
    try:
        return lang[string.context]
    except KeyError:
        return string.message


def info(lang: dict = {}) -> Optional[LocalInfo]:
    strings = {prop: tr(APP[prop], lang) for prop in APP}
    for prop in strings:
        if strings[prop] != APP[prop].message:
            return LocalInfo(**strings)


def info_from_file(path: str) -> Tuple[str, Optional[LocalInfo]]:
    global langs

    po = polib.pofile(path)
    lang = po.metadata["Language"]

    ts = {entry.msgctxt: entry.msgstr for entry in po if entry.msgid != ""}
    return (lang, info(ts))


def create_translations() -> Dict[str, LocalInfo]:
    needed = {}

    for here, dirs, files in os.walk(translations_dir):
        dirs[:] = []
        for filename in files:
            name, ext = os.path.splitext(filename)
            if ext != ".po":
                continue
            if "-".join(name.split("-")[:-1]) != "cov":
                continue

            lang, nfo = info_from_file(os.path.join(here, filename))
            if nfo is not None:
                needed[lang] = nfo

    # RC defaults to en_US, so will we
    try:
        needed["en_US"]
    except KeyError:
        needed["en_US"] = LocalInfo(**{prop: APP[prop].message for prop in APP})

    return needed


escapes = {
    "\\": "\\\\",
    '"': '""',
    "\a": "\\a",
    "\b": "\\b",
    "\f": "\\f",
    "\n": "\\n",
    "\r": "\\r",
    "\t": "\\t",
    "\v": "\\v",
}


def res_str(s: str) -> str:
    result = ""
    for c in s:
        try:
            result += escapes[c]
        except KeyError:
            if ord(c) > 127:
                result += "\\x{:04x}".format(ord(c))
            else:
                result += c
    return result


translations = create_translations()
with open(dest, "w") as output:
    with open(
        sys.argv[2] if len(sys.argv) > 1 else "./apps/win32/versioninfo.h", "r"
    ) as preamble:
        output.write(preamble.read())

    fileModule = ""
    fileExt = "exe"
    if len(sys.argv) > 3:
        fileModule = " ({})".format(res_str(sys.argv[3]))
        fileExt = "dll"
    for lang in translations:
        spec = langs[lang]
        strings = translations[lang]._asdict()
        strings = {prop: res_str(strings[prop]) for prop in strings}

        print(
            template.format(
                langid=spec.lang | (spec.res_sublang << 8),
                lang=spec.lang,
                sublang=spec.sublang,
                fileModule=fileModule,
                fileExt=fileExt,
                **strings
            ),
            file=output,
        )
