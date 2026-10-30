#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Build the Qt tools that a Qt for HarmonyOS from the installer lacks for the device.

Qt Creator running on a HarmonyOS device builds projects against a Qt for
HarmonyOS, and that needs moc, rcc, uic, qmake and qtpaths built for the device,
and qmltyperegistrar, qmlcachegen and qmlimportscanner for a Qt Quick project,
and qmlls for editing QML. The installer's Qt for HarmonyOS has none: its qmake
and qt-cmake are scripts that forward to the desktop Qt next to it.

This builds them from the qtbase, qtlanguageserver and qtdeclarative commits
that Qt was built from, read from the sha1s.txt the installer puts beside it,
because tools and libraries have to come from the same sources. The QML tools
are built against the Qt for HarmonyOS itself and run with its libraries.
qmlls needs the Qt Language Server module, which the Qt for HarmonyOS lacks,
so that is built against it as well, as static libraries. The result is
librcc.so and friends rather than executables, since on HarmonyOS every
executable is a loadable module. The copies of qtchost in Qt Creator's package
map and run them.

The tools are built with QT_FORCE_BUILD_TOOLS, which does not configure for
HarmonyOS before qtbase 0107b66097d ("CMake: Record the Qt module deps of
plugins only for Android"). That change is applied to older sources.

Without the HarmonyOS platform plugin, QtCore asks a JS thread that does not
exist for the application and OS versions and the standard paths, and
crashes. The fixes for that, which are in qtbase 6.12.1 (QTBUG-150323), are
applied to older sources as well.

Building needs git, cmake, ninja and a host compiler, the desktop Qt of the
same version, and the node-addon-api headers, which install_third_party.py
installs together with the libraries a Qt for HarmonyOS needs at run time.

The tools come with a QtCore of their own, built without ICU, in libexec/lib.
Qt Creator loads it in place of the one of the Qt for HarmonyOS, which needs
the ICU of the additional packages.

The result is one archive, "qt-tools-<version>-<target>.tar.gz", whose
contents go into the Qt for HarmonyOS on the device, which Qt Creator does
with "Add Qt Tools" in its HarmonyOS preferences. Besides the tools, the
archive has the Qt6CoreTools, Qt6GuiTools, Qt6WidgetsTools and Qt6QmlTools
CMake packages of the desktop Qt, because the Qt for HarmonyOS has none. They
name the desktop's executables, which CMake insists exist, so the archive has
an empty file for each of them that the Qt for HarmonyOS lacks. Qt Creator runs
its copies of qtchost in place of moc, rcc, uic, cmake_automoc_parser, qlalr,
qtpaths, qmltyperegistrar, qmlcachegen, qmlimportscanner and qmlls. The others
are not needed for building Qt Widgets and Qt Quick applications.

The archive records the qtbase commit in sha1s-tools.txt, so that Qt Creator
can refuse it for a Qt built from other sources.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tarfile
from pathlib import Path

QTBASE_URL = "https://code.qt.io/qt/qtbase.git"
QTDECLARATIVE_URL = "https://code.qt.io/qt/qtdeclarative.git"
QTLANGUAGESERVER_URL = "https://code.qt.io/qt/qtlanguageserver.git"

BACKPORT = "0107b66097dda8bdd25d020cca1da3d4edf5c931"

# In the order they were integrated in the 6.12 branch.
NO_JS_THREAD = [
    "5001441b5a2ec6d5009208900841f4a45280c0b3",
    "032ca114a1bc24f9904f2b9273fb5c4596c63e78",
    "39d3b1523cdc1703a829d485ec2beedc6501ff67",
    "599c8f65fbfd870e31d3e6e2dfff6f9b2cff42b0",
]

# Build target, and where its library lands relative to the build directory.
TOOLS = {
    "moc_native": "libexec/libmoc.so",
    "rcc_native": "libexec/librcc.so",
    "uic_native": "libexec/libuic.so",
    "cmake_automoc_parser_native": "libexec/libcmake_automoc_parser.so",
    "qmake_native": "bin/libqmake.so",
    "qtpaths_native": "bin/libqtpaths.so",
}

# The same for the tools that qt_add_qml_module runs, which are built from qtdeclarative.
QML_TOOLS = {
    "qmltyperegistrar_native": "libexec/libqmltyperegistrar.so",
    "qmlcachegen_native": "libexec/libqmlcachegen.so",
    "qmlimportscanner_native": "libexec/libqmlimportscanner.so",
    "qmlls": "bin/libqmlls.so",
}

# The tools' own QtCore, where Qt Creator looks for it, and where it lands in the build.
TOOLS_QTCORE = ("libexec/lib/libQt6Core.so", "lib/libQt6Core.so")

# The packages that find_package(Qt6 COMPONENTS Widgets Quick) looks for in QT_HOST_PATH,
# which the Qt for HarmonyOS lacks.
TOOLS_PACKAGES = ["Qt6CoreTools", "Qt6GuiTools", "Qt6WidgetsTools", "Qt6QmlTools"]

# Where a package names the executables it imports.
IMPORTED_FILE = re.compile(r'"\$\{_IMPORT_PREFIX\}/((?:bin|libexec)/[^"]+)"')

# Only QtCore and Bootstrap are needed for the tools. ICU is left out so that the tools'
# own QtCore does not need it. zlib is the bundled one, which QtCore exports, because the
# QML tools run with the Qt for HarmonyOS's QtNetwork, and that takes zlib from QtCore
# (see unprefix_zlib()).
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
    "-DFEATURE_system_zlib=OFF",
]

# qtdeclarative is configured against the Qt for HarmonyOS, whose own toolchain file
# chain-loads the SDK's.
QML_CONFIGURE_ARGUMENTS = [
    "-DCMAKE_BUILD_TYPE=Release",
    "-DQT_FORCE_BUILD_TOOLS=ON",
    "-DBUILD_WITH_PCH=OFF",
    "-DQT_BUILD_TESTS=OFF",
    "-DQT_BUILD_EXAMPLES=OFF",
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


def recorded(qt: Path, module: str) -> str | None:
    """The commit of module that the sha1s.txt beside qt names."""
    sha1s = qt.parent / "sha1s.txt"
    if not sha1s.is_file():
        return None
    for line in sha1s.read_text().splitlines():
        fields = line.split()
        if len(fields) == 2 and fields[0] == module:
            return fields[1]
    return None


def apply(source: Path, patch: Path, name: str) -> None:
    """Apply patch to source unless it is there already."""
    if succeeds(["git", "apply", "--reverse", "--check", patch], source):
        print("{} has {} already".format(source.name, name))
    elif succeeds(["git", "apply", "--check", patch], source):
        run(["git", "apply", patch], cwd=source)
    else:
        raise SystemExit("{} applies to {} neither way".format(name, source.name))


def fetch(source: Path, url: str, sha: str, commits: list[str]) -> None:
    """Check the repository at url out at sha, with commits applied where they are
    missing."""
    if not (source / ".git").is_dir():
        source.mkdir(parents=True, exist_ok=True)
        run(["git", "init", "--quiet"], cwd=source)
    head = subprocess.run(["git", "rev-parse", "--verify", "--quiet", "HEAD"], cwd=source,
                          stdout=subprocess.PIPE, text=True).stdout.strip()
    missing = [commit for commit in commits
               if not succeeds(["git", "cat-file", "-e", commit + "^{commit}"], source)]
    if head != sha or missing:
        # The parents come along, so that the commits can be turned into patches.
        run(["git", "fetch", "--quiet", "--depth", "2", url, sha] + commits,
            cwd=source)
    if head != sha:
        run(["git", "checkout", "--quiet", "--force", "--detach", sha], cwd=source)

    for commit in commits:
        patch = source.parent / "{}.patch".format(commit[:11])
        patch.write_text(run(["git", "format-patch", "-1", "--stdout", commit], cwd=source,
                             capture=True))
        apply(source, patch, commit[:11])


def unprefix_zlib(source: Path) -> None:
    """Let the bundled zlib keep the names of the system one, which the QtCore of the Qt
    for HarmonyOS exports, and which its other libraries therefore import from QtCore."""
    zconf = source / "src" / "3rdparty" / "zlib" / "src" / "zconf.h"
    text = zconf.read_text()
    if "\n#define Z_PREFIX\n" in text:
        zconf.write_text(text.replace("\n#define Z_PREFIX\n", "\n"))


def pack(archive: Path, qt: Path, host_path: Path, entries: dict, sha: str) -> None:
    """Write the archive whose contents go into the Qt for HarmonyOS qt."""
    placeholders = set()
    for package in TOOLS_PACKAGES:
        directory = Path("lib") / "cmake" / package
        if (qt / directory).exists():
            raise SystemExit("{} has a {} already".format(qt, package))
        for file in sorted((host_path / directory).iterdir()):
            entries[str(directory / file.name)] = file
            placeholders.update(IMPORTED_FILE.findall(file.read_text()))
    placeholders = sorted(name for name in placeholders
                          if name not in entries and not (qt / name).exists())

    record = archive.parent / "sha1s-tools.txt"
    record.write_text("qtbase {}\n".format(sha))
    empty = archive.parent / "placeholder"
    empty.write_bytes(b"")
    def anonymous(info: tarfile.TarInfo) -> tarfile.TarInfo:
        info.uid = info.gid = 0
        info.uname = info.gname = ""
        return info

    with tarfile.open(archive, "w:gz") as tar:
        tar.add(record, "sha1s-tools.txt", filter=anonymous)
        for name, file in sorted(entries.items()):
            tar.add(file, name, filter=anonymous)
        for name in placeholders:
            tar.add(empty, name, filter=anonymous)
    record.unlink()
    empty.unlink()


def build_targets(args: argparse.Namespace, build: Path, targets: dict) -> None:
    command = [args.cmake, "--build", build, "--target"] + list(targets)
    if args.jobs:
        command += ["--parallel", str(args.jobs)]
    run(command)


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
                        help="directory to put the archive with the tools into")
    parser.add_argument("--work-path", type=absolute,
                        help="where to keep the checkouts and their builds, "
                             "\"<path>-work\" by default. Reused by a later run")
    parser.add_argument("--qtbase-sha",
                        help="qtbase commit to build, the one sha1s.txt beside --qt records "
                             "by default")
    parser.add_argument("--qtlanguageserver-sha",
                        help="qtlanguageserver commit to build, the one sha1s.txt beside "
                             "--qt records by default")
    parser.add_argument("--qtdeclarative-sha",
                        help="qtdeclarative commit to build, the one sha1s.txt beside --qt "
                             "records by default")
    parser.add_argument("--cmake", default="cmake",
                        help="CMake to configure with, taken from PATH by default")
    parser.add_argument("--jobs", type=int, help="parallel build jobs")
    args = parser.parse_args()

    host_path = args.host_path or args.qt.parent / "gcc_64"
    if not (host_path / "lib" / "cmake").is_dir():
        raise SystemExit("No desktop Qt at {}".format(host_path))
    sha = args.qtbase_sha or recorded(args.qt, "qtbase")
    if not sha:
        raise SystemExit("No sha1s.txt beside {} names qtbase, give --qtbase-sha"
                         .format(args.qt))
    host_sha = recorded(host_path, "qtbase")
    if host_sha and host_sha != sha:
        raise SystemExit("The desktop Qt is qtbase {}, the tools would be {}"
                         .format(host_sha[:11], sha[:11]))
    languageserver_sha = args.qtlanguageserver_sha or recorded(args.qt, "qtlanguageserver")
    if not languageserver_sha:
        raise SystemExit("No sha1s.txt beside {} names qtlanguageserver, give "
                         "--qtlanguageserver-sha".format(args.qt))
    declarative_sha = args.qtdeclarative_sha or recorded(args.qt, "qtdeclarative")
    if not declarative_sha:
        raise SystemExit("No sha1s.txt beside {} names qtdeclarative, give "
                         "--qtdeclarative-sha".format(args.qt))
    if not (args.packages / "include" / "napi.h").is_file():
        raise SystemExit("No include/napi.h in {}".format(args.packages))
    openharmony = openharmony_path(args.sdk)

    work = args.work_path or Path(str(args.path) + "-work")
    source = work / "qtbase"
    build = work / "build"
    fetch(source, QTBASE_URL, sha, [BACKPORT] + NO_JS_THREAD)
    unprefix_zlib(source)

    run([args.cmake, "-S", source, "-B", build, "-GNinja",
         "-DQT_HOST_PATH={}".format(host_path),
         "-DQT_HOST_PATH_CMAKE_DIR={}".format(host_path / "lib" / "cmake"),
         "-DOHOS_SDK_ROOT={}".format(openharmony),
         "-DNodeAddonApi_INCLUDE_DIR={}".format(args.packages / "include")]
        + CONFIGURE_ARGUMENTS)
    build_targets(args, build, TOOLS)

    def configure_against_qt(source: Path, build: Path, prefixes: list[Path]) -> None:
        run([args.cmake, "-S", source, "-B", build, "-GNinja",
             "-DCMAKE_TOOLCHAIN_FILE={}".format(
                 args.qt / "lib" / "cmake" / "Qt6" / "qt.toolchain.cmake"),
             "-DQT_CHAINLOAD_TOOLCHAIN_FILE={}".format(
                 openharmony / "native" / "build" / "cmake" / "ohos.toolchain.cmake"),
             "-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH={}".format(
                 ";".join(str(prefix) for prefix in prefixes)),
             "-DQT_HOST_PATH={}".format(host_path),
             "-DQT_HOST_PATH_CMAKE_DIR={}".format(host_path / "lib" / "cmake")]
            + QML_CONFIGURE_ARGUMENTS)

    # Installed, because a Qt module is found only in an installation, and into a prefix of
    # its own, because that of the Qt for HarmonyOS is not to be touched.
    languageserver_source = work / "qtlanguageserver"
    languageserver_build = work / "build-qtlanguageserver"
    languageserver = work / "qtlanguageserver-install"
    fetch(languageserver_source, QTLANGUAGESERVER_URL, languageserver_sha, [])
    configure_against_qt(languageserver_source, languageserver_build, [args.packages])
    run([args.cmake, "--build", languageserver_build]
        + (["--parallel", str(args.jobs)] if args.jobs else []))
    run([args.cmake, "--install", languageserver_build, "--prefix", languageserver])

    declarative_source = work / "qtdeclarative"
    declarative_build = work / "build-qtdeclarative"
    fetch(declarative_source, QTDECLARATIVE_URL, declarative_sha, [])
    configure_against_qt(declarative_source, declarative_build,
                         [args.packages, languageserver])
    build_targets(args, declarative_build, QML_TOOLS)

    args.path.mkdir(parents=True, exist_ok=True)
    version = re.search(r"^QT_VERSION = (\S+)$",
                        (args.qt / "mkspecs" / "qconfig.pri").read_text(), re.M)
    if not version:
        raise SystemExit("No QT_VERSION in the qconfig.pri of {}".format(args.qt))
    archive = args.path / "qt-tools-{}-{}.tar.gz".format(version.group(1), args.qt.name)
    entries = {library: build / library for library in TOOLS.values()}
    entries.update({library: declarative_build / library for library in QML_TOOLS.values()})
    entries[TOOLS_QTCORE[0]] = build / TOOLS_QTCORE[1]
    pack(archive, args.qt, host_path, entries, sha)

    print("\nPacked the tools of qtbase {} and qtdeclarative {} as {}"
          .format(sha[:11], declarative_sha[:11], archive))
    print("Add them to the Qt for HarmonyOS on the device with \"Add Qt Tools\" in the "
          "HarmonyOS preferences of Qt Creator.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
