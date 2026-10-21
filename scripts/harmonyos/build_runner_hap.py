#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Package the runner that Qt Creator on a HarmonyOS device runs what it built with.

What Qt Creator builds on the device is an application module, not a package,
and from its sandbox it can neither install nor start one. It offers the module
over a socket on the device's loopback instead, to an installed package that
holds the runner and declares the "qtcrun" scheme, which is the only way an
application may start another one.

This builds that package the way the deploy step does for "Run without
installing": a minimal application goes through harmonydeployqt, and then

- the Qt is taken out, and libqohos.so replaced by a shim that maps the Qt the
  IDE hands over, so that one package runs applications built against any Qt,
- the application library is replaced by the runner, and libqtcwait.so, which
  holds a debugged application until the debugger has it, goes beside it,
- the module declares the scheme and the permission the socket needs,
- the native package qtctools.hnp carries the SDK's lldb-server.

The three libraries are built from share/qtcreator/harmonyos, with the ports
and paths harmonyosconstants.h has. What is left of harmonydeployqt's staging
is the C++ runtime and the third-party libraries a Qt needs, as the device may
not have those.

Signing needs a profile for the bundle name, which is not Qt Creator's own.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
import zipfile
from pathlib import Path

from build_hap import (absolute, openharmony_path, run, sdk_version, sign,
                       without_daemon)

APPLICATION = "runnerapp"
PROJECT = """\
cmake_minimum_required(VERSION 3.22)
project({name} LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_add_executable({name} main.cpp)
target_link_libraries({name} PRIVATE Qt6::Widgets)
"""
MAIN = """\
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    return 0;
}
"""
SCHEME = "qtcrun"
SCHEME_SKILL = """
          {{
            "actions": ["ohos.want.action.viewData"],
            "entities": ["entity.system.browsable"],
            "uris": [{{ "scheme": "{}" }}]
          }},"""


def constants(source: Path) -> dict[str, str]:
    """The ports and the server path, as Qt Creator itself has them."""
    header = (source / "src/plugins/harmonyos/harmonyosconstants.h").read_text()
    values = {}
    for name in ["CHANNEL_PORT", "DEBUG_PORT", "GATE_PORT"]:
        match = re.search(r"HARMONYOS_{}\s*=\s*(\d+);".format(name), header)
        if not match:
            raise SystemExit("No HARMONYOS_{} in harmonyosconstants.h".format(name))
        values[name] = match.group(1)
    match = re.search(r'HARMONYOS_DEBUG_SERVER_PATH\[\]\s*=\s*("[^"]*");', header)
    if not match:
        raise SystemExit("No HARMONYOS_DEBUG_SERVER_PATH in harmonyosconstants.h")
    values["DEBUG_SERVER_PATH"] = match.group(1)
    return values


def compile_library(compiler: Path, sysroot: Path, sources: Path, names: list[str],
                    libraries: list[str], output: Path, values: dict[str, str]) -> None:
    run([compiler, "--target=aarch64-linux-ohos", "--sysroot={}".format(sysroot),
         "-fPIC", "-shared", "-O1", "-std=c++17",
         # The template loads them by these names.
         "-Wl,-soname,{}".format(output.name), "-I{}".format(sources),
         "-DQTC_CHANNEL_PORT={}".format(values["CHANNEL_PORT"]),
         "-DQTC_GATE_PORT={}".format(values["GATE_PORT"]),
         "-DQTC_SERVER_PORT={}".format(values["DEBUG_PORT"]),
         "-DQTC_SERVER_PATH={}".format(values["DEBUG_SERVER_PATH"])]
        + [sources / name for name in names] + libraries + ["-o", output])


def edit(path: Path, pattern: str, insert: str, what: str) -> None:
    text = path.read_text()
    match = re.search(pattern, text)
    if not match:
        raise SystemExit("No {} in {}".format(what, path))
    path.write_text(text[:match.end()] + insert + text[match.end():])


def lldb_server(openharmony: Path) -> Path:
    found = sorted((openharmony / "native/llvm/lib/clang").glob(
        "*/bin/aarch64-linux-ohos/lldb-server"))
    if not found:
        raise SystemExit("No lldb-server for the device in {}".format(openharmony))
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
                        help="HarmonyOS command-line-tools directory")
    parser.add_argument("--packages", type=absolute,
                        help="prefix of the libraries a Qt for HarmonyOS needs at run time")
    parser.add_argument("--source", type=absolute,
                        help="Qt Creator sources, this checkout by default")
    parser.add_argument("--path", type=absolute, required=True,
                        help="directory to build in")
    parser.add_argument("--bundle", default="org.qtproject.qtcrunner",
                        help="bundle name, the one the signing profile is for, %(default)s "
                             "by default")
    parser.add_argument("--output", type=absolute,
                        help="the package to write, qtcrunner.hap in --path by default")
    parser.add_argument("--profile", type=absolute, help="signing profile, .p7b")
    parser.add_argument("--certificate", type=absolute, help="signing certificate, .cer")
    parser.add_argument("--keystore", type=absolute, help="keystore with the key, .p12")
    parser.add_argument("--key-alias", help="alias of the key in the keystore")
    parser.add_argument("--keystore-password-file", type=absolute,
                        help="file holding the passphrase of the keystore and the key")
    parser.add_argument("--cmake", default="cmake",
                        help="CMake to configure with, taken from PATH by default")
    args = parser.parse_args()

    signing = [args.profile, args.certificate, args.keystore, args.key_alias,
               args.keystore_password_file]
    if any(signing) and not all(signing):
        raise SystemExit("Signing needs --profile, --certificate, --keystore, --key-alias "
                         "and --keystore-password-file")
    source = args.source or Path(__file__).resolve().parents[2]
    host_path = args.host_path or args.qt.parent / "gcc_64"
    openharmony = openharmony_path(args.sdk)
    native = openharmony / "native"
    hvigor = args.sdk / "bin/hvigorw"
    environment = {"PATH": "{}:{}".format(args.sdk / "bin", os.environ["PATH"])}
    if os.environ.get("JAVA_HOME"):
        environment["PATH"] = "{}/bin:{}".format(os.environ["JAVA_HOME"], environment["PATH"])

    version = sdk_version(args.sdk)
    template = (args.qt / "src/harmonyos/templates/build-profile.json5").read_text()
    wanted = re.search(r'"compatibleSdkVersion"\s*:\s*"([^"]*)"', template)
    if wanted and wanted.group(1) != version:
        raise SystemExit("{} is for SDK {}, {} is {}".format(args.qt, wanted.group(1),
                                                             args.sdk, version))

    # What an earlier run left would be taken over as it is.
    shutil.rmtree(args.path, ignore_errors=True)
    application = args.path / "src"
    build = args.path / "build"
    application.mkdir(parents=True)
    (application / "CMakeLists.txt").write_text(PROJECT.format(name=APPLICATION))
    (application / "main.cpp").write_text(MAIN)

    command = [args.cmake, "-S", application, "-B", build, "-GNinja",
               "-DCMAKE_BUILD_TYPE=Release",
               "-DCMAKE_TOOLCHAIN_FILE={}".format(
                   args.qt / "lib/cmake/Qt6/qt.toolchain.cmake"),
               "-DQT_HOST_PATH={}".format(host_path),
               "-DQT_CHAINLOAD_TOOLCHAIN_FILE={}".format(
                   native / "build/cmake/ohos.toolchain.cmake"),
               "-DOHOS_SDK_ROOT={}".format(openharmony)]
    if args.packages:
        command.append("-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH={}".format(args.packages))
    run(command, env=environment)
    run([args.cmake, "--build", build, "--target", APPLICATION + "_make_hap"],
        env=dict(environment,
                 QT_HARMONYOS_HVIGOR=str(without_daemon(hvigor, args.path / "hvigor"))),
        log=args.path / "make-hap.log")
    project = build / "harmonyos-build"
    libraries = project / "entry/libs/arm64-v8a"
    for entry in list(libraries.iterdir()):
        if entry.is_dir():
            shutil.rmtree(entry)
        elif entry.name.startswith("libQt6") or entry.name in ["libqohos.so",
                                                                 "lib{}.so".format(APPLICATION)]:
            entry.unlink()

    compiler = native / "llvm/bin/clang++"
    sources = source / "share/qtcreator/harmonyos"
    values = constants(source)
    sysroot = native / "sysroot"
    compile_library(compiler, sysroot, sources, ["qtcrunner.cpp"], ["-ldl"],
                    libraries / "libqtcrunner.so", values)
    compile_library(compiler, sysroot, sources, ["qtcshim.cpp", "qtcload.cpp"],
                    ["-lace_napi.z", "-ldl"], libraries / "libqohos.so", values)
    compile_library(compiler, sysroot, sources, ["qtcwait.cpp"], [],
                    libraries / "libqtcwait.so", values)

    constants_file = project / "entry/src/main/ets/common/QtAppConstants.ets"
    text, count = re.subn(r"(APP_LIBRARY_NAME\s*=\s*')[^']*(')",
                          lambda match: match.group(1) + "libqtcrunner.so" + match.group(2),
                          constants_file.read_text(), count=1)
    if count != 1:
        raise SystemExit("No APP_LIBRARY_NAME in {}".format(constants_file))
    constants_file.write_text(text)

    app = project / "AppScope/app.json5"
    app.write_text(re.sub(r'("bundleName"\s*:\s*")[^"]*(")',
                          lambda match: match.group(1) + args.bundle + match.group(2),
                          app.read_text(), count=1))

    module = project / "entry/src/main/module.json5"
    edit(module, r'"requestPermissions"\s*:\s*\[',
         '\n        { "name": "ohos.permission.INTERNET" },', "permission list")
    edit(module, r'"skills"\s*:\s*\[', SCHEME_SKILL.format(SCHEME), "skill list")
    edit(module, r'"module"\s*:\s*\{',
         '\n    "hnpPackages": [\n      { "package": "qtctools.hnp", "type": "private" }\n'
         '    ],', "module object")

    run([hvigor, "assembleHap", "--no-daemon"], cwd=project, env=environment,
        log=args.path / "hvigor.log")
    hap = sorted(project.glob("entry/build/default/outputs/default/*.hap"))[0]

    # The debug server is the only thing in it: where the application may execute from.
    tree = args.path / "hnp/src"
    packed = args.path / "hnp/package"
    (tree / "bin").mkdir(parents=True)
    packed.mkdir(parents=True)
    server = tree / "bin/lldb-server"
    shutil.copy2(lldb_server(openharmony), server)
    server.chmod(0o755)
    run([openharmony / "toolchains/hnpcli", "pack", "-i", tree, "-o", packed,
         "-n", "qtctools", "-v", "1.0"])
    with zipfile.ZipFile(hap, "a", compression=zipfile.ZIP_STORED) as archive:
        present = set(archive.namelist())
        for directory in ["hnp/", "hnp/arm64-v8a/"]:
            if directory not in present:
                archive.writestr(zipfile.ZipInfo(directory), b"")
        for package in sorted(packed.iterdir()):
            archive.write(package, "hnp/arm64-v8a/" + package.name)

    if not args.keystore:
        output = args.output or args.path / "qtcrunner-unsigned.hap"
        shutil.copy2(hap, output)
        print("\nPackaged the runner unsigned as {}".format(output))
        return 0
    output = args.output or args.path / "qtcrunner.hap"
    sign(hap, output, openharmony / "toolchains", args, environment)
    print("\nPackaged and signed the runner as {}".format(output))
    return 0


if __name__ == "__main__":
    sys.exit(main())
