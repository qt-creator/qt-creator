#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Build the Qt tools that a Qt for HarmonyOS from the installer lacks for the device.

Qt Creator running on a HarmonyOS device builds projects against a Qt for
HarmonyOS, and that needs moc, rcc, uic and qmake built for the device. The
installer's Qt for HarmonyOS has none: its qmake and qt-cmake are scripts that
forward to the desktop Qt next to it.

This builds them from the qtbase commit that Qt was built from, read from the
sha1s.txt the installer puts beside it, because tools and libraries have to
come from the same sources. The result is librcc.so and friends rather than
executables, since on HarmonyOS every executable is a loadable module. The
copies of qtchost in Qt Creator's package map and run them.

The tools are built with QT_FORCE_BUILD_TOOLS, which does not configure for
HarmonyOS before qtbase 0107b66097d ("CMake: Record the Qt module deps of
plugins only for Android"). That change is applied to older sources.

Building needs git, cmake, ninja and a host compiler, the desktop Qt of the
same version, and the node-addon-api headers, which install_third_party.py
installs together with the libraries a Qt for HarmonyOS needs at run time.

The tools come with a QtCore of their own, built without ICU, in libexec/lib.
Qt Creator loads it in place of the one of the Qt for HarmonyOS, which needs
the ICU of the additional packages.

The output directory holds libexec and bin. Their files go into the libexec
and bin directories of the Qt for HarmonyOS on the device.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

QTBASE_URL = "https://code.qt.io/qt/qtbase.git"

BACKPORT = "0107b66097dda8bdd25d020cca1da3d4edf5c931"

# Build target, and where its library lands relative to the build directory.
TOOLS = {
    "moc_native": "libexec/libmoc.so",
    "rcc_native": "libexec/librcc.so",
    "uic_native": "libexec/libuic.so",
    "qmake_native": "bin/libqmake.so",
}

# The tools' own QtCore, where Qt Creator looks for it, and where it lands in the build.
TOOLS_QTCORE = ("libexec/lib/libQt6Core.so", "lib/libQt6Core.so")

# Only QtCore and Bootstrap are needed for the tools. ICU is left out so that the tools'
# own QtCore does not need it.
CONFIGURE_ARGUMENTS = [
    "-DCMAKE_BUILD_TYPE=Release",
    "-DQT_FORCE_BUILD_TOOLS=ON",
    "-DBUILD_WITH_PCH=OFF",
    "-DQT_BUILD_TESTS=OFF",
    "-DQT_BUILD_EXAMPLES=OFF",
    "-DFEATURE_gui=OFF",
    "-DFEATURE_network=OFF",
    "-DFEATURE_sql=OFF",
    "-DFEATURE_testlib=OFF",
    "-DFEATURE_dbus=OFF",
    "-DFEATURE_concurrent=OFF",
    "-DFEATURE_icu=OFF",
]


def absolute(path: str) -> Path:
    return Path(path).expanduser().resolve()


def run(command: list, cwd: Path | None = None, capture: bool = False) -> str:
    command = [str(part) for part in command]
    print("+ {}".format(" ".join(command)), flush=True)
    environment = dict(os.environ)
    # A CCACHE_PREFIX of icecc makes every compiler call go through a launcher
    # that cannot reach the cross compiler.
    environment.pop("CCACHE_PREFIX", None)
    result = subprocess.run(command, cwd=cwd, env=environment, text=True,
                            stdout=subprocess.PIPE if capture else None)
    if result.returncode != 0:
        raise SystemExit("{} failed".format(Path(command[0]).name))
    return result.stdout if capture else ""


def succeeds(command: list, cwd: Path) -> bool:
    return subprocess.run([str(part) for part in command], cwd=cwd,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0


def openharmony_path(sdk: Path) -> Path:
    """The "openharmony" directory inside whatever the SDK argument points at."""
    for candidate in ["sdk/default/openharmony", "default/openharmony", "openharmony", ""]:
        root = sdk / candidate if candidate else sdk
        if (root / "native" / "sysroot").is_dir():
            return root
    raise SystemExit("No OpenHarmony SDK with a native/sysroot under {}".format(sdk))


def recorded_qtbase(qt: Path) -> str | None:
    sha1s = qt.parent / "sha1s.txt"
    if not sha1s.is_file():
        return None
    for line in sha1s.read_text().splitlines():
        fields = line.split()
        if len(fields) == 2 and fields[0] == "qtbase":
            return fields[1]
    return None


def fetch(source: Path, sha: str) -> None:
    """Check qtbase out at sha, with the backport applied where it is missing."""
    if not (source / ".git").is_dir():
        source.mkdir(parents=True, exist_ok=True)
        run(["git", "init", "--quiet"], cwd=source)
    head = subprocess.run(["git", "rev-parse", "--verify", "--quiet", "HEAD"], cwd=source,
                          stdout=subprocess.PIPE, text=True).stdout.strip()
    if head != sha:
        # The parent comes along, so that the backport can be turned into a patch.
        run(["git", "fetch", "--quiet", "--depth", "2", QTBASE_URL, sha, BACKPORT], cwd=source)
        run(["git", "checkout", "--quiet", "--force", "--detach", sha], cwd=source)

    patch = source.parent / "backport.patch"
    patch.write_text(run(["git", "format-patch", "-1", "--stdout", BACKPORT], cwd=source,
                         capture=True))
    if succeeds(["git", "apply", "--reverse", "--check", patch], source):
        print("qtbase {} has {} already".format(sha[:11], BACKPORT[:11]))
    elif succeeds(["git", "apply", "--check", patch], source):
        run(["git", "apply", patch], cwd=source)
    else:
        raise SystemExit("{} applies to qtbase {} neither way".format(BACKPORT[:11], sha[:11]))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--qt", type=absolute, required=True,
                        help="the installer's Qt for HarmonyOS, the directory holding its "
                             "lib and mkspecs, such as ~/Qt/6.12.0/harmonyos_arm64_v8a")
    parser.add_argument("--host-path", type=absolute,
                        help="desktop Qt of the same version, \"gcc_64\" beside --qt by "
                             "default")
    parser.add_argument("--sdk", type=absolute, required=True,
                        help="HarmonyOS SDK, either a command-line-tools directory or the "
                             "\"openharmony\" one inside it")
    parser.add_argument("--packages", type=absolute, required=True,
                        help="prefix that install_third_party.py installed into, for the "
                             "node-addon-api headers in its include directory")
    parser.add_argument("--path", type=absolute, required=True,
                        help="directory to put libexec and bin with the tools into")
    parser.add_argument("--work-path", type=absolute,
                        help="where to keep the qtbase checkout and its build, "
                             "\"<path>-work\" by default. Reused by a later run")
    parser.add_argument("--qtbase-sha",
                        help="qtbase commit to build, the one sha1s.txt beside --qt records "
                             "by default")
    parser.add_argument("--cmake", default="cmake",
                        help="CMake to configure with, taken from PATH by default")
    parser.add_argument("--jobs", type=int, help="parallel build jobs")
    args = parser.parse_args()

    host_path = args.host_path or args.qt.parent / "gcc_64"
    if not (host_path / "lib" / "cmake").is_dir():
        raise SystemExit("No desktop Qt at {}".format(host_path))
    sha = args.qtbase_sha or recorded_qtbase(args.qt)
    if not sha:
        raise SystemExit("No sha1s.txt beside {} names qtbase, give --qtbase-sha"
                         .format(args.qt))
    host_sha = recorded_qtbase(host_path)
    if host_sha and host_sha != sha:
        raise SystemExit("The desktop Qt is qtbase {}, the tools would be {}"
                         .format(host_sha[:11], sha[:11]))
    if not (args.packages / "include" / "napi.h").is_file():
        raise SystemExit("No include/napi.h in {}".format(args.packages))
    openharmony = openharmony_path(args.sdk)

    work = args.work_path or Path(str(args.path) + "-work")
    source = work / "qtbase"
    build = work / "build"
    fetch(source, sha)

    run([args.cmake, "-S", source, "-B", build, "-GNinja",
         "-DQT_HOST_PATH={}".format(host_path),
         "-DQT_HOST_PATH_CMAKE_DIR={}".format(host_path / "lib" / "cmake"),
         "-DOHOS_SDK_ROOT={}".format(openharmony),
         "-DNodeAddonApi_INCLUDE_DIR={}".format(args.packages / "include")]
        + CONFIGURE_ARGUMENTS)
    command = [args.cmake, "--build", build, "--target"] + list(TOOLS)
    if args.jobs:
        command += ["--parallel", str(args.jobs)]
    run(command)

    copies = {library: library for library in TOOLS.values()}
    copies[TOOLS_QTCORE[0]] = TOOLS_QTCORE[1]
    for target, built in copies.items():
        target = args.path / target
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(build / built, target)

    print("\nBuilt the tools of qtbase {} into {}".format(sha[:11], args.path))
    print("Copy its libexec and bin into those of the Qt for HarmonyOS on the device.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
