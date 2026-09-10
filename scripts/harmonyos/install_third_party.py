#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Build the third-party libraries a Qt for HarmonyOS needs at run time.

Qt links against ICU, fontconfig, freetype, libpng and libjpeg, and neither the
OpenHarmony SDK nor a Qt for HarmonyOS carries them: the Qt from the online
installer has libicuuc, libfontconfig, libfreetype and libpng16 as undefined
dependencies. Without them an application installs and starts but cannot load
the platform plugin.

Qt's own CI builds them with vcpkg, using the OpenHarmony triplet from The Qt
Company's vcpkg fork. This does the same, so what comes out is what Qt was
built and tested against.

The prefix it prints goes into Preferences > SDKs > HarmonyOS > Additional
packages, and from there into the build as QT_ADDITIONAL_PACKAGES_PREFIX_PATH.

Building the ports needs git, cmake, ninja, a host compiler and the autotools,
including the macros from autoconf-archive, which several of them configure
with. On Debian and Ubuntu that is

    sudo apt install git cmake ninja-build autoconf autoconf-archive automake libtool
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path

# The fork of the vcpkg registry that carries the OpenHarmony triplets and
# toolchain, and the fork of the vcpkg tool that knows "ohos" as a platform.
# Both are pinned to what Qt's CI provisions, in
# qt5/coin/provisioning/common/shared/.
REGISTRY_URL = "https://git.qt.io/qtbuildsystem/vcpkg.git"
REGISTRY_TAG = "2026.05.05-ohos"
TOOL_URL = "https://github.com/jobor/vcpkg-tool/archive/refs/tags/{}.tar.gz"
TOOL_TAG = "ohos-20260505-4dc8719"
TOOL_SHA256 = "96a94016cfff8a46bb7abc98917df0b39477ec2da8468daf790699b1ad66a026"

DEFAULT_TRIPLET = "arm64-ohos"

# The ports Qt is built against, as in qt5/coin/provisioning/common/shared/
# vcpkg/vcpkg.json. node-addon-api is not a run-time dependency, it is what
# configuring qtbase for HarmonyOS needs.
DEPENDENCIES = [
    "brotli",
    "expat",
    "fontconfig",
    {"name": "freetype", "default-features": False},
    "icu",
    "libjpeg-turbo",
    "libpng",
    "node-addon-api",
]

# Where the SDK keeps its "native" folder, relative to what the user configures.
# The same candidates that Qt Creator itself tries.
SDK_CANDIDATES = [
    "sdk/default/openharmony",
    "default/openharmony",
    "openharmony",
    "",
]


# What building the ports takes, beside a host compiler. autoconf-archive is a
# set of m4 macros rather than a program, so it is looked for where aclocal does.
HOST_TOOLS = ["git", "cmake", "ninja", "autoconf", "aclocal", "automake", "libtoolize"]


def absolute(path: str) -> Path:
    return Path(path).expanduser().resolve()


def run(command: list[str], **kwargs) -> None:
    print("+ {}".format(" ".join(str(part) for part in command)), flush=True)
    if subprocess.run([str(part) for part in command], **kwargs).returncode != 0:
        raise SystemExit("{} failed".format(Path(str(command[0])).name))


def check_host_tools() -> None:
    missing = [tool for tool in HOST_TOOLS if not shutil.which(tool)]
    if not missing:
        macros = subprocess.run(["aclocal", "--print-ac-dir"], capture_output=True, text=True)
        if not list(Path(macros.stdout.strip()).glob("ax_*.m4")):
            missing.append("autoconf-archive")
    if missing:
        raise SystemExit("Missing on this host: {}\nOn Debian and Ubuntu: sudo apt install "
                         "git cmake ninja-build autoconf autoconf-archive automake libtool"
                         .format(" ".join(missing)))


def sdk_root(given: Path) -> Path:
    """The directory the OHOS triplet wants, the one with "native" in it."""
    for candidate in SDK_CANDIDATES:
        root = given / candidate if candidate else given
        if (root / "native/build/cmake/ohos.toolchain.cmake").is_file():
            return root
    raise SystemExit("No OpenHarmony SDK with a CMake toolchain file under {}".format(given))


def registry(path: Path) -> Path:
    """Check the pinned tag of the vcpkg registry out, once."""
    if (path / ".git").is_dir():
        print("Have the vcpkg registry in {}".format(path))
        return path
    run(["git", "clone", "--quiet", "--depth", "1", "--branch", REGISTRY_TAG,
         REGISTRY_URL, path])
    return path


def tool(path: Path, work: Path) -> Path:
    """Build the vcpkg tool into the registry checkout, once.

    The published binary knows nothing about OpenHarmony, so the tool is built
    from the fork the pinned triplets come with.
    """
    vcpkg = path / "vcpkg"
    if vcpkg.is_file():
        print("Have the vcpkg tool at {}".format(vcpkg))
        return vcpkg

    source = work / "vcpkg-tool-{}".format(TOOL_TAG)
    if not source.is_dir():
        url = TOOL_URL.format(TOOL_TAG)
        print("Downloading {}".format(url), flush=True)
        with tempfile.TemporaryDirectory() as temp:
            archive = Path(temp) / "vcpkg-tool.tar.gz"
            urllib.request.urlretrieve(url, str(archive))
            if hashlib.sha256(archive.read_bytes()).hexdigest() != TOOL_SHA256:
                raise SystemExit("Checksum mismatch for {}".format(url))
            with tarfile.open(archive) as tar:
                tar.extractall(work, filter="data")

    build = source / "build"
    run(["cmake", "-S", source, "-B", build, "-GNinja", "-DCMAKE_BUILD_TYPE=Release",
         "-DBUILD_TESTING=OFF", "-DVCPKG_DEVELOPMENT_WARNINGS=OFF"])
    run(["cmake", "--build", build, "--parallel"])
    shutil.copy2(build / "vcpkg", vcpkg)
    vcpkg.chmod(0o755)
    # vcpkg asks for permission to send telemetry unless this file is there.
    (path / "vcpkg.disable-metrics").touch()
    return vcpkg


def manifest(directory: Path) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "vcpkg.json").write_text(
        json.dumps({"dependencies": DEPENDENCIES}, indent=2) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sdk", type=absolute, required=True,
                        help="the HarmonyOS SDK to build against, the same directory that "
                             "Preferences > SDKs > HarmonyOS is set to")
    parser.add_argument("--path", type=absolute, required=True,
                        help="directory to install the packages into. One prefix per triplet "
                             "is created in it, and that prefix is what to configure")
    parser.add_argument("--triplet", default=DEFAULT_TRIPLET,
                        help="vcpkg triplet to build, {} by default".format(DEFAULT_TRIPLET))
    parser.add_argument("--work-path", type=absolute,
                        help="where to keep the vcpkg checkout and its build trees, "
                             "\"vcpkg\" beside --path by default. Reused by a later run")
    args = parser.parse_args()

    check_host_tools()

    work = args.work_path or args.path.parent / "vcpkg"
    work.mkdir(parents=True, exist_ok=True)
    checkout = registry(work / "registry-{}".format(REGISTRY_TAG))
    vcpkg = tool(checkout, work)

    environment = dict(os.environ)
    environment["OHOS_SDK_ROOT"] = str(sdk_root(args.sdk))
    environment["VCPKG_ROOT"] = str(checkout)
    # ICU's Makefile defines a TARGET variable of its own, and make imports the
    # environment as make variables, which breaks the build.
    environment.pop("TARGET", None)

    manifest(work / "manifest")
    run([vcpkg, "install", "--triplet", args.triplet, "--x-install-root", args.path],
        cwd=work / "manifest", env=environment)

    prefix = args.path / args.triplet
    print("\nBuilt {} into {}".format(", ".join(
        entry if isinstance(entry, str) else entry["name"] for entry in DEPENDENCIES), prefix))
    print("Set it as Additional packages in Preferences > SDKs > HarmonyOS.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
