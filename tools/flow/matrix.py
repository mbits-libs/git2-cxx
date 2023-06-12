# Copyright (c) 2023 Marcin Zdun
# This code is licensed under MIT license (see LICENSE for details)

import json
import os
import sys
from typing import Dict, List, Tuple

from runner import runner, step_call, copy_file, print_args

platform = {
    "linux": "ubuntu",
    "win32": "windows",
}[sys.platform]

_names = {
    "clang": ["clang", "clang++"],
    "stdclang": ["clang", "stdclang"],
    "gcc": ["gcc", "g++"],
}


def matches(tested: dict, test: dict):
    for key, value in test.items():
        val = tested.get(key)
        if val != value:
            return False
    return True


def matches_any(tested: dict, tests: list):
    for test in tests:
        if matches(tested, test):
            return True
    return False


def _split_keys(includes: List[dict], keys: List[str]) -> List[Tuple[dict, dict]]:
    result = []
    for include in includes:
        expand_key = {}
        expand_value = {}
        for key, value in include.items():
            if key in keys:
                expand_key[key] = value
            else:
                expand_value[key] = value
        result.append((expand_key, expand_value))
    return result


def cartesian(input: Dict[str, list]) -> List[dict]:
    product = [{}]

    for key, values in input.items():
        next_level = []
        for value in values:
            for obj in product:
                next_level.append({**obj, key: value})
        product = next_level

    return product


def load_matrix(json_path: str) -> Tuple[List[dict], List[str]]:
    with open(json_path, encoding="UTF-8") as f:
        setup: dict = json.load(f)

    full = cartesian(setup.get("matrix", {}))

    excludes = setup.get("exclude", [])
    matrix = [obj for obj in full if not matches_any(obj, excludes)]
    keys = list(setup.get("matrix", {}).keys())

    includes = _split_keys(setup.get("include", []), keys)
    for obj in matrix:
        for include_key, include_value in includes:
            if not matches(obj, include_key):
                continue
            for key, value in include_value.items():
                obj[key] = value

    return matrix, keys


def find_compiler(compiler: str) -> Tuple[str, List[str]]:
    dirname = os.path.dirname(compiler)
    filename = os.path.basename(compiler)
    if sys.platform == "win32":
        filename = os.path.splitext(filename)[0]
    chunks = filename.split("-", 1)
    if len(chunks) == 1:
        version = None
    else:
        version = chunks[1]
    filename = chunks[0].lower()

    try:
        compiler_names = _names[filename]
    except:
        compiler_names = [filename]

    compilers = [
        os.path.join(dirname, name if version is None else f"{name}-{version}")
        for name in compiler_names
    ]

    if filename == "stdclang":
        filename = "clang"

    return filename, compilers


class steps:
    @staticmethod
    @step_call("Conan")
    def configure_conan(config: dict):
        CONAN_DIR = "build/conan"
        CONAN_PROFILE = "_profile-compiler"
        CONAN_PROFILE_GEN = "_profile-build_type"
        profile_gen = f"./{CONAN_DIR}/{CONAN_PROFILE_GEN}-{config['preset']}"

        if config.get("--first-build", False):
            runner.refresh_build_dir("conan")

        if not runner.DRY_RUN:
            with open(profile_gen, "w", encoding="UTF-8") as profile:
                print(
                    f"""include({CONAN_PROFILE})

[settings]""",
                    file=profile,
                )
                for setting in [
                    *config.get("conan_settings", []),
                    f"build_type={config['build_type']}",
                ]:
                    print(setting, file=profile)

        runner.call(
            "conan",
            "profile",
            "new",
            "--detect",
            "--force",
            f"./{CONAN_DIR}/{CONAN_PROFILE}",
        )

        runner.call(
            "conan",
            "install",
            "-if",
            CONAN_DIR,
            "-of",
            CONAN_DIR,
            "--build",
            "missing",
            "-pr:b",
            profile_gen,
            "-pr:h",
            profile_gen,
            ".",
        )

        if not runner.DRY_RUN:
            os.remove("CMakeUserPresets.json")

    @staticmethod
    @step_call("CMake")
    def configure_cmake(config: dict):
        runner.refresh_build_dir(config["preset"])
        cov_COVERALLS = "ON" if config.get("coverage", False) else "OFF"
        COV_SANITIZE = "ON" if config.get("sanitizer", False) else "OFF"
        COV_CUTDOWN_OS = "ON" if runner.CUTDOWN_OS else "OFF"
        runner.call(
            "cmake",
            "--preset",
            f"{config['preset']}-{config['build_generator']}",
            f"-Dcov_COVERALLS:BOOL={cov_COVERALLS}",
            f"-DCOV_SANITIZE:BOOL={COV_SANITIZE}",
            f"-DCOV_CUTDOWN_OS:BOOL={COV_CUTDOWN_OS}",
        )

    @staticmethod
    @step_call("Build")
    def build(config: dict):
        runner.call("cmake", "--build", "--preset", config["preset"], "--parallel")

    @staticmethod
    @step_call("Test")
    def test(config: dict):
        if config.get("coverage", False):
            runner.call(
                "cmake",
                "--build",
                "--preset",
                config["preset"],
                "--target",
                "cov_coveralls",
            )
        else:
            runner.call("ctest", "--preset", config["preset"])

    @staticmethod
    @step_call("Pack", lambda config: len(config.get("cpack_generator", [])) > 0)
    def pack(config: dict):
        for generator in config.get("cpack_generator", []):
            runner.call("cpack", "--preset", config["preset"], "-G", generator)

    @staticmethod
    @step_call("Artifacts")
    def store(config: dict):
        if not runner.DRY_RUN:
            os.makedirs("build/artifacts", exist_ok=True)
        preset = config["preset"]
        runner.copy(
            f"build/{preset}/packages",
            "build/artifacts/packages",
            r"^cov-.*-(devel|apps)\..*$",
        )
        runner.copy(f"build/{preset}/test-results", "build/artifacts/test-results")
        if config.get("coverage", False):
            output = f"build/artifacts/coveralls/{config['report_os']}-{config['report_compiler']}-{config['build_type']}.json"
            print_args("cp", f"build/{preset}/coveralls.json", output)
            if not runner.DRY_RUN:
                copy_file(f"build/{preset}/coveralls.json", output)

    @staticmethod
    def build_steps():
        return [
            steps.configure_conan,
            steps.configure_cmake,
            steps.build,
            steps.test,
            steps.pack,
            steps.store,
        ]

    @staticmethod
    def build_config(config: dict, keys: list, wanted_steps: list):
        program = steps.build_steps()
        if len(wanted_steps):
            program = [step for step in program if step(None).lower() in wanted_steps]
        runner.run_steps(config, keys, program)
