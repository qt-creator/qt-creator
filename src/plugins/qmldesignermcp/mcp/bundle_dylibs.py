# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Helpers for collecting @rpath dylibs into PyInstaller bundles on macOS."""

from __future__ import annotations

import importlib
import re
import subprocess
import sys
from pathlib import Path


def run_otool(*args: str) -> str:
    return subprocess.check_output(["otool", *args], text=True)


def resolve_dynamic_path(raw_path: str, binary: Path) -> Path:
    executable_dir = Path(sys.executable).resolve().parent
    resolved = raw_path.replace("@loader_path", str(binary.parent))
    resolved = resolved.replace("@executable_path", str(executable_dir))
    return Path(resolved).resolve()


def rpaths_for(binary: Path) -> list[Path]:
    output = run_otool("-l", str(binary))
    rpaths: list[Path] = []
    in_rpath = False

    for line in output.splitlines():
        if "cmd LC_RPATH" in line:
            in_rpath = True
            continue
        if not in_rpath:
            continue

        match = re.match(r"\s*path (\S+)", line)
        if not match:
            continue

        rpaths.append(resolve_dynamic_path(match.group(1), binary))
        in_rpath = False

    return rpaths


# macOS ships system libraries via the dyld shared cache; they are not on
# disk but are always available at runtime, so we must never try to bundle them.
SYSTEM_DYLIB_PREFIXES = ("/usr/lib/", "/System/")


def dylib_dependencies_for(binary: Path) -> list[str]:
    output = run_otool("-L", str(binary))
    dependencies: list[str] = []

    for line in output.splitlines():
        match = re.match(r"\s+(@rpath/\S+\.dylib|/\S+\.dylib)", line)
        if not match:
            continue
        dep = match.group(1)
        if dep.startswith(SYSTEM_DYLIB_PREFIXES):
            continue
        dependencies.append(dep)

    return dependencies


def resolve_dependency(dependency: str, rpaths: list[Path]) -> Path:
    if dependency.startswith("/"):
        resolved = Path(dependency).resolve()
        if not resolved.exists():
            raise FileNotFoundError(f"Could not resolve {dependency}")
        return resolved

    name = dependency.removeprefix("@rpath/")
    for rpath in rpaths:
        candidate = (rpath / name).resolve()
        if candidate.exists():
            return candidate

    searched = ", ".join(str(path) for path in rpaths) or "<none>"
    raise FileNotFoundError(
        f"Could not resolve {dependency}; searched: {searched}"
    )


def collected_dylibs(module_names: list[str]) -> set[Path]:
    dylibs: set[Path] = set()

    for module_name in module_names:
        module = importlib.import_module(module_name)
        module_file = Path(module.__file__).resolve()
        rpaths = rpaths_for(module_file)

        for dependency in dylib_dependencies_for(module_file):
            dylibs.add(resolve_dependency(dependency, rpaths))

    return dylibs


def collected_binaries(
    module_names: list[str], destination_dir: str = "."
) -> list[tuple[str, str]]:
    if sys.platform != "darwin":
        return []

    return [
        (str(source), destination_dir)
        for source in sorted(collected_dylibs(module_names))
    ]
