#!/usr/bin/env python

import os
import sys
import shlex
import subprocess

from yaml import load

try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader  # type: ignore

result = subprocess.run(
    ["git", "rev-parse", "--show-toplevel"], shell=False, capture_output=True
)
if result.returncode:
    sys.stderr.write(result.stderr.decode("UTF-8"))
    sys.exit(result.returncode)

__dir__ = result.stdout.strip().decode("UTF-8")
WORKFLOWS = os.path.join(__dir__, ".github", "workflows")
ACTION = "github/super-linter"
PREFIX = f"{ACTION}/"

UNWANTED = [
    "VALIDATE_ALL_CODEBASE",  # local runs are always "true"
]

if "--verbose" in sys.argv[1:]:
    UNWANTED.append("LOG_LEVEL")


def get_linter_action(step):
    try:
        action = step["uses"].split("@", 1)[0]
    except KeyError:
        return
    if action == ACTION:
        version = "latest"
    elif action[: len(PREFIX)] == PREFIX:
        version = "{}-latest".format(action[len(PREFIX) :])
    else:
        return

    try:
        env = step["env"]
    except KeyError:
        env = []

    try:
        name = step["name"]
    except KeyError:
        name = step["uses"]

    _env = {}
    for key in env:
        value = env[key]
        if isinstance(value, bool):
            _env[key] = "true" if value else "false"
            continue
        if isinstance(value, str) and "${{" in value:
            continue
        _env[key] = value

    return (name, version, _env)


def get_linter_actions(data, yml_name):
    result = []
    for job_id in data["jobs"]:
        job = data["jobs"][job_id]
        actions = []
        for step in job["steps"]:
            action = get_linter_action(step)
            if action is None:
                continue
            actions.append(action)

        if not len(actions):
            continue

        try:
            name = job["name"]
        except KeyError:
            name = job_id

        if name != yml_name:
            name = f"{yml_name} » {name}"

        result.append((name, actions))

    if not len(result):
        return

    return result


def docker_args(version, env):
    _env = {key: env[key] for key in env if key not in UNWANTED}
    _env["RUN_LOCAL"] = "true"
    _env["USE_FIND_ALGORITHM"] = "true"

    args = [
        arg for pair in (["-e", f"{key}={_env[key]}"] for key in _env) for arg in pair
    ]
    return [
        "docker",
        "run",
        "--rm",
        *args,
        "-v" f"{__dir__}:/tmp/lint",
        f"github/super-linter:{version}",
    ]


JOBS = []
for root, dirs, files in os.walk(WORKFLOWS):
    for filename in files:
        if os.path.splitext(filename)[1] not in [".yml", ".yaml"]:
            continue

        with open(os.path.join(root, filename), encoding="UTF-8") as f:
            data = load(f, Loader=Loader)
            try:
                yml_name = data["name"]
            except KeyError:
                yml_name = os.path.splitext(filename)[0]
            data = get_linter_actions(data, yml_name)
            if data is not None:
                JOBS.extend(data)


for job_name, steps in JOBS:
    print(f"job:  \033[1;49;95m{job_name}\033[m")
    for name, version, env in steps:
        print(
            f"step: \033[1;49;96m{name}\033[m \033[2;49;32m(github/super-linter:"
            f"\033[0;49;32m{version}\033[2;49;32m)\033[m"
        )
        args = docker_args(version, env)
        print("\033[0;49;30m{}\033[m".format(shlex.join(args)))
        subprocess.run(args, shell=False)
        print()
