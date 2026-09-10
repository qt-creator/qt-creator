#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Cross-build a Qt for HarmonyOS that Qt Creator can be built and run against.

This carries the configure line that was measured to work, because three of its
arguments are not the obvious ones:

- The node-api headers have to be given as NodeAddonApi_INCLUDE_DIR.
  NODE_ADDON_API_ROOT is only a hint to find_path, and the OHOS toolchain roots
  that search at the sysroot, so configure fails with "Qt for OHOS requires
  node-api-addon" however right the path is.
- The OHOS built third party libraries (fontconfig, freetype, png, jpeg, icu)
  have to be given as CMAKE_FIND_ROOT_PATH. CMAKE_PREFIX_PATH is ignored for
  the same rooting reason, and the OHOS platform plugin links fontconfig
  unconditionally, so it is not optional.
- qtbase does not build for OHOS with precompiled headers, because
  qohosimageconversions.cpp compiles with exceptions against a Gui header
  compiled without them.

A host Qt of the same version, with at least the modules built here, is
required and is not built here: cross-building a module takes its host tools
from there. An in source developer build of the same checkout will do, which is
what --host-path normally points at.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

# Beyond qtbase, the modules Qt Creator uses. Quick, Designer, Help and Svg come
# out of these, and without qtserialport the serial terminal is left out.
DEFAULT_MODULES = ["qtshadertools", "qtdeclarative", "qtsvg", "qttools", "qtserialport"]


def absolute(path: str) -> Path:
    return Path(path).expanduser().resolve()


def openharmony_path(sdk: Path) -> Path:
    """The "openharmony" directory inside whatever the SDK argument points at.

    The same candidates Qt Creator itself accepts, so that one path works for
    both.
    """
    for candidate in ["sdk/default/openharmony", "default/openharmony", "openharmony", ""]:
        root = sdk / candidate if candidate else sdk
        if (root / "native" / "sysroot").is_dir():
            return root
    raise SystemExit("No OpenHarmony SDK with a native/sysroot under {}".format(sdk))


def run(command: list[str], cwd: Path | None = None) -> None:
    print("+ {}".format(" ".join(command)))
    environment = dict(os.environ)
    # A CCACHE_PREFIX of icecc makes every compiler call go through a launcher
    # that cannot reach the cross compiler.
    environment.pop("CCACHE_PREFIX", None)
    subprocess.check_call(command, cwd=cwd, env=environment)


def build(build_path: Path, jobs: int | None) -> None:
    ninja = ["ninja", "-C", str(build_path)]
    if jobs:
        ninja += ["-j", str(jobs)]
    run(ninja)
    run(ninja + ["install"])


def configure_qtbase(args: argparse.Namespace, openharmony: Path, build_path: Path) -> None:
    command = [
        args.cmake,
        "-S", str(args.qt_source / "qtbase"),
        "-B", str(build_path),
        "-GNinja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX={}".format(args.prefix),
        "-DQT_HOST_PATH={}".format(args.host_path),
        "-DQT_HOST_PATH_CMAKE_DIR={}".format(args.host_path / "lib" / "cmake"),
        "-DOHOS_SDK_ROOT={}".format(openharmony),
        "-DBUILD_WITH_PCH=OFF",
        "-DQT_BUILD_TESTS=OFF",
        "-DQT_BUILD_EXAMPLES=OFF",
    ]
    if args.packages:
        command.append("-DCMAKE_FIND_ROOT_PATH={}".format(args.packages))
    if args.node_addon_api:
        command.append("-DNodeAddonApi_INCLUDE_DIR={}".format(args.node_addon_api))
    if args.openssl:
        command.append("-DOPENSSL_ROOT_DIR={}".format(args.openssl))
    run(command + args.cmake_arguments)


def configure_module(args: argparse.Namespace, module: str, build_path: Path) -> None:
    build_path.mkdir(parents=True, exist_ok=True)
    configure = args.prefix / "bin" / "qt-configure-module"
    if not configure.exists():
        raise SystemExit("No {}, so qtbase has not been installed yet".format(configure))
    run([str(configure), str(args.qt_source / module), "--",
         "-DQT_BUILD_TESTS=OFF", "-DQT_BUILD_EXAMPLES=OFF",
         "-DQT_FEATURE_warnings_are_errors=OFF", "-DBUILD_WITH_PCH=OFF"]
        + args.cmake_arguments, cwd=build_path)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--qt-source", type=absolute, required=True,
                        help="Qt source directory holding qtbase and the modules")
    parser.add_argument("--host-path", type=absolute, required=True,
                        help="host Qt of the same version with at least the same modules, an "
                             "in source build of the same checkout will do")
    parser.add_argument("--sdk", type=absolute, required=True,
                        help="HarmonyOS SDK, either a command-line-tools directory or the "
                             "\"openharmony\" one inside it")
    parser.add_argument("--prefix", type=absolute, required=True,
                        help="where to install the result, which is what a HarmonyOS Qt "
                             "version in Qt Creator then points at")
    parser.add_argument("--build-path", type=absolute,
                        help="where to build, \"<prefix>-build\" by default")
    parser.add_argument("--packages", type=absolute,
                        help="prefix holding the OHOS built fontconfig, freetype, png, jpeg "
                             "and icu, required by the OHOS platform plugin")
    parser.add_argument("--node-addon-api", type=absolute,
                        help="directory holding napi.h, required for the OHOS platform "
                             "plugin")
    parser.add_argument("--openssl", type=absolute, help="prefix holding an OHOS built OpenSSL")
    parser.add_argument("--modules", nargs="*", default=DEFAULT_MODULES,
                        help="modules to build on top of qtbase")
    parser.add_argument("--cmake", default="cmake",
                        help="CMake to configure with, taken from PATH by default")
    parser.add_argument("--jobs", type=int, help="parallel build jobs")
    parser.add_argument("cmake_arguments", nargs="*",
                        help="further arguments, passed to every configure. They start with a "
                             "dash, so put them after a \"--\"")
    args = parser.parse_args()

    openharmony = openharmony_path(args.sdk)
    build_root = args.build_path or Path(str(args.prefix) + "-build")

    print("== qtbase")
    qtbase_build = build_root / "qtbase"
    configure_qtbase(args, openharmony, qtbase_build)
    build(qtbase_build, args.jobs)

    for module in args.modules:
        print("== {}".format(module))
        module_build = build_root / module
        configure_module(args, module, module_build)
        build(module_build, args.jobs)

    print("\nInstalled Qt for HarmonyOS into {}".format(args.prefix))
    print("Add {}/bin/qmake as a Qt version in Qt Creator.".format(args.prefix))
    return 0


if __name__ == "__main__":
    sys.exit(main())
