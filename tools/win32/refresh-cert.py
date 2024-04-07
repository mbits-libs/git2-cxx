# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import base64
import json
import os
import random
import string
import subprocess
import sys

__dir_name__ = os.path.dirname(__file__)
__root_dir__ = os.path.dirname(os.path.dirname(__dir_name__))


def run(*args: str):
    proc = subprocess.run(args, shell=False, capture_output=False)
    if proc.returncode != 0:
        sys.exit(1)


letters = string.ascii_letters + string.digits + string.punctuation
weights = (
    ([7] * len(string.ascii_letters))
    + ([5] * len(string.digits))
    + ([2] * len(string.punctuation))
)
token = "".join(random.choices(letters, weights, k=50))

run(
    "pwsh",
    os.path.join(__dir_name__, "refresh-cert.ps1"),
    "-password",
    token,
    "-filename",
    "certificate.pfx",
)


def load_binary(filename: str):
    with open(filename, "rb") as cert:
        return base64.b64encode(cert.read()).decode("UTF-8")


data = {
    "token": base64.b64encode(token.encode("UTF-8")).decode("UTF-8"),
    "secret": load_binary("certificate.pfx"),
}

os.remove("certificate.pfx")

with open("signature.key", "w", encoding="UTF-8") as signature:
    json.dump(data, signature)
