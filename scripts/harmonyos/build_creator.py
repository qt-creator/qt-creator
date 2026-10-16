#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Cross-build Qt Creator for a HarmonyOS device, against the installer's Qt.

The SDK's clang 15 miscompiles the coroutines of Utils::Result, so this builds
with an upstream clang instead, 21 by default. That one knows the
aarch64-linux-ohos target, and takes everything else from the SDK: the sysroot,
libc++ and the compiler runtime. Three things are not obvious:

- clang needs a resource directory of its own version for its headers, or the
  NEON intrinsics fail, but the OHOS compiler runtime exists only in the SDK's.
  The build gets a merged one.
- Qt derives the SDK location from the compiler path, which no longer points
  into the SDK, so the toolchain file sets OHOS_NDK_ROOT.
- QmlDesigner and what only it uses need std::ranges, which the SDK's
  libc++ 15 lacks.

Packaging and signing are not done here.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

CONFIGURE_ARGUMENTS = [
    "-DCMAKE_BUILD_TYPE=Release",
    "-DWITH_TESTS=OFF",
    "-DBUILD_WITH_PCH=OFF",
    "-DBUILD_PLUGIN_QMLDESIGNER=OFF",
    "-DBUILD_LIBRARY_QMLDESIGNERCORE=OFF",
    "-DBUILD_LIBRARY_QMLDESIGNERUTILS=OFF",
    "-DBUILD_LIBRARY_QMLPUPPETCOMMUNICATION=OFF",
    "-DBUILD_EXECUTABLE_QMLPUPPET=OFF",
    "-DBUILD_LIBRARY_SQLITE=OFF",
    "-DBUILD_LIBRARY_SQLITEINTERNAL=OFF",
    "-DBUILD_LIBRARY_ASTCHECK=OFF",
]

TOOLCHAIN = """\
include("{openharmony}/native/build/cmake/ohos.toolchain.cmake")

set(OHOS_NDK_ROOT "{openharmony}/native")

set(CMAKE_C_COMPILER "{llvm}/bin/clang")
set(CMAKE_CXX_COMPILER "{llvm}/bin/clang++")

string(APPEND CMAKE_C_FLAGS " -resource-dir={resource_dir}")
string(APPEND CMAKE_CXX_FLAGS
       " -nostdinc++ -isystem {libcxx} -resource-dir={resource_dir}")
foreach(_kind EXE SHARED MODULE)
  string(APPEND CMAKE_${{_kind}}_LINKER_FLAGS
         " -B{llvm}/bin -fuse-ld=lld -L{libs} -rtlib=compiler-rt -unwindlib=libunwind")
endforeach()
"""


def absolute(path: str) -> Path:
    return Path(path).expanduser().resolve()


def run(command: list) -> None:
    command = [str(part) for part in command]
    print("+ {}".format(" ".join(command)), flush=True)
    environment = dict(os.environ)
    # A CCACHE_PREFIX of icecc makes every compiler call go through a launcher
    # that cannot reach the cross compiler.
    environment.pop("CCACHE_PREFIX", None)
    if subprocess.run(command, env=environment).returncode != 0:
        raise SystemExit("{} failed".format(Path(command[0]).name))


def openharmony_path(sdk: Path) -> Path:
    """The "openharmony" directory inside whatever the SDK argument points at."""
    for candidate in ["sdk/default/openharmony", "default/openharmony", "openharmony", ""]:
        root = sdk / candidate if candidate else sdk
        if (root / "native" / "sysroot").is_dir():
            return root
    raise SystemExit("No OpenHarmony SDK with a native/sysroot under {}".format(sdk))


def only_directory(pattern: str, parent: Path) -> Path:
    found = sorted(parent.glob(pattern))
    if len(found) != 1:
        raise SystemExit("Expected one {} in {}, found {}".format(pattern, parent, len(found)))
    return found[0]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--qt", type=absolute, required=True,
                        help="the installer's Qt for HarmonyOS, such as "
                             "~/Qt/6.12.0/harmonyos_arm64_v8a")
    parser.add_argument("--host-path", type=absolute,
                        help="desktop Qt of the same version, \"gcc_64\" beside --qt by "
                             "default")
    parser.add_argument("--sdk", type=absolute, required=True,
                        help="HarmonyOS SDK, either a command-line-tools directory or the "
                             "\"openharmony\" one inside it")
    parser.add_argument("--packages", type=absolute,
                        help="prefix that install_third_party.py installed into, for "
                             "packaging the libraries Qt needs at run time")
    parser.add_argument("--llvm", type=absolute, default=Path("/usr/lib/llvm-21"),
                        help="LLVM installation with clang and lld, %(default)s by default")
    parser.add_argument("--source", type=absolute,
                        help="Qt Creator sources, this checkout by default")
    parser.add_argument("--path", type=absolute, required=True,
                        help="build directory")
    parser.add_argument("--cmake", default="cmake",
                        help="CMake to configure with, taken from PATH by default")
    parser.add_argument("--jobs", type=int, help="parallel build jobs")
    parser.add_argument("cmake_arguments", nargs="*",
                        help="further CMake arguments, after a \"--\"")
    args = parser.parse_args()

    source = args.source or Path(__file__).resolve().parents[2]
    host_path = args.host_path or args.qt.parent / "gcc_64"
    if not (host_path / "lib" / "cmake").is_dir():
        raise SystemExit("No desktop Qt at {}".format(host_path))
    if not (args.llvm / "bin" / "clang++").is_file():
        raise SystemExit("No clang++ in {}/bin".format(args.llvm))
    openharmony = openharmony_path(args.sdk)
    native = openharmony / "native"

    # The resource directory and the toolchain file depend on nothing in the
    # build directory, but live there so that one directory is all there is.
    resource_dir = args.path / "ohos-resource-dir"
    shutil.rmtree(resource_dir, ignore_errors=True)
    (resource_dir / "lib").mkdir(parents=True)
    (resource_dir / "include").symlink_to(only_directory("lib/clang/*/include", args.llvm))
    (resource_dir / "lib" / "aarch64-linux-ohos").symlink_to(
        only_directory("llvm/lib/clang/*/lib/aarch64-linux-ohos", native))

    toolchain = args.path / "ohos-upstream-clang.toolchain.cmake"
    toolchain.write_text(TOOLCHAIN.format(
        openharmony=openharmony, llvm=args.llvm, resource_dir=resource_dir,
        libcxx=native / "llvm/include/libcxx-ohos/include/c++/v1",
        libs=native / "llvm/lib/aarch64-linux-ohos"))

    command = [args.cmake, "-S", source, "-B", args.path, "-GNinja",
               "-DCMAKE_TOOLCHAIN_FILE={}".format(
                   args.qt / "lib" / "cmake" / "Qt6" / "qt.toolchain.cmake"),
               "-DQT_HOST_PATH={}".format(host_path),
               "-DQT_CHAINLOAD_TOOLCHAIN_FILE={}".format(toolchain),
               "-DOHOS_SDK_ROOT={}".format(openharmony)] + CONFIGURE_ARGUMENTS
    if args.packages:
        command.append("-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH={}".format(args.packages))
    run(command + args.cmake_arguments)

    command = [args.cmake, "--build", args.path]
    if args.jobs:
        command += ["--parallel", str(args.jobs)]
    run(command)

    print("\nBuilt Qt Creator for HarmonyOS in {}".format(args.path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
