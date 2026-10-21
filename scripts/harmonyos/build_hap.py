#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Package a build of Qt Creator for HarmonyOS as an application, and sign it.

Qt's qtcreator_make_hap target runs harmonydeployqt, which generates the HAP
project and stages the application, the libraries it links and Qt. What is
left for here is what harmonydeployqt cannot know:

- Qt Creator's resources, which go into the bundle as a resource file
  directory, and its plugins, which are loaded rather than linked.
- The libraries those plugins need in turn, from the build, from Qt and from
  the additional packages.
- The launch arguments that reconcile the layout Qt Creator expects with the
  flat one of a HAP, the bundle name the signing profile is for, the SDK
  version, and the reason a user_grant permission has to give.
- The native package qtctools.hnp, which is the only place the application
  may execute from. It carries the files the build names in its extras file
  and the toolchain build_device_toolchain.py lays out.

Signing needs a profile, a certificate and a keystore, and the passphrase of
the keystore in a file. Without a keystore the package is left unsigned.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ABILITY_STAGE = "entry/src/main/ets/qabilitystage/QAbilityStage.ets"
DEVICE_PLUGINS = "/data/storage/el1/bundle/libs/arm64"
DEVICE_RESOURCES = "/data/storage/el1/bundle/entry/resources/resfile/qtcreator"


def absolute(path: str) -> Path:
    return Path(path).expanduser().resolve()


def run(command: list, cwd: Path | None = None, env: dict | None = None,
        shown: list | None = None, log: Path | None = None) -> None:
    command = [str(part) for part in command]
    print("+ {}".format(" ".join(str(part) for part in (shown or command))), flush=True)
    environment = dict(os.environ)
    # A CCACHE_PREFIX of icecc makes every compiler call go through a launcher
    # that cannot reach the cross compiler.
    environment.pop("CCACHE_PREFIX", None)
    environment.update(env or {})
    if log:
        with open(log, "w") as output:
            result = subprocess.run(command, cwd=cwd, env=environment, stdout=output,
                                    stderr=subprocess.STDOUT)
    else:
        result = subprocess.run(command, cwd=cwd, env=environment)
    if result.returncode != 0:
        raise SystemExit("{} failed{}".format(Path(command[0]).name,
                                              ", see {}".format(log) if log else ""))


def openharmony_path(sdk: Path) -> Path:
    for candidate in ["sdk/default/openharmony", "default/openharmony", "openharmony", ""]:
        root = sdk / candidate if candidate else sdk
        if (root / "native" / "sysroot").is_dir():
            return root
    raise SystemExit("No OpenHarmony SDK with a native/sysroot under {}".format(sdk))


def sdk_version(sdk: Path) -> str:
    """The SDK version as build-profile.json5 names it, such as "6.1.0(23)"."""
    package = json.loads((sdk / "sdk/default/sdk-pkg.json").read_text())["data"]
    return "{}({})".format(package["platformVersion"], package["apiVersion"])


def needed(readelf: Path, library: Path) -> list[str]:
    output = subprocess.run([str(readelf), "-d", str(library)], capture_output=True,
                            text=True).stdout
    return re.findall(r"\(NEEDED\).*\[(.*)\]", output)


def stage_libraries(project: Path, build: Path, search: list[Path], readelf: Path) -> None:
    """Every plugin, and every library the staged ones need, until none is missing.

    harmonydeployqt stages what the executable links, which leaves out every
    plugin, and the libraries the deployment settings knew about when the
    build was configured, which can be out of date. Nothing reports a library
    that is missing: what needs it stays unloaded, or the application dies
    before it can say anything.
    """
    plugins = build / "lib/qtcreator"
    for arch in sorted(project.glob("entry/libs/arm64*")):
        for library in list(plugins.glob("*.so")) + list(plugins.glob("plugins/*.so")):
            shutil.copy2(library, arch)
        added = True
        while added:
            added = False
            for library in sorted(arch.glob("*.so")):
                for name in needed(readelf, library):
                    if (arch / name).exists():
                        continue
                    for directory in search:
                        if (directory / name).exists():
                            shutil.copy2(directory / name, arch, follow_symlinks=True)
                            print("   {} needs {}, taken from {}".format(
                                library.name, name, directory))
                            added = True
                            break
        print("   {} libraries staged in {}".format(len(list(arch.glob("*.so"))), arch))


def adjust_project(project: Path, bundle: str, version: str, arguments: list[str]) -> None:
    stage = project / ABILITY_STAGE
    block = ("  private static appArgs?: Array<string> = [\n"
             + "".join("    {},\n".format(", ".join("'{}'".format(part) for part in group))
                       for group in arguments) + "  ];\n")
    # Replaces the declaration the template has, or a block an earlier run wrote.
    text, count = re.subn(r"  private static appArgs\?: Array<string>(?:;|\s*=\s*\[.*?\];)\n",
                          lambda match: block, stage.read_text(), count=1, flags=re.S)
    if count != 1:
        raise SystemExit("No appArgs declaration to replace in {}".format(stage))
    stage.write_text(text)

    # The device installs a package only under the bundle name its profile is for.
    app = project / "AppScope/app.json5"
    app.write_text(re.sub(r'("bundleName"\s*:\s*")[^"]*(")',
                          lambda match: match.group(1) + bundle + match.group(2),
                          app.read_text(), count=1))

    profile = project / "build-profile.json5"
    profile.write_text(re.sub(r'("compatibleSdkVersion"\s*:\s*")[^"]*(")',
                              lambda match: match.group(1) + version + match.group(2),
                              profile.read_text(), count=1))

    # hvigor rejects a user_grant permission that does not say why it is needed.
    strings = project / "entry/src/main/resources/base/element/string.json"
    data = json.loads(strings.read_text())
    if not any(entry["name"] == "pasteboard_reason" for entry in data["string"]):
        data["string"].append({"name": "pasteboard_reason",
                               "value": "Copy and paste inside the editor"})
        strings.write_text(json.dumps(data, indent=4))

    module = project / "entry/src/main/module.json5"
    text = module.read_text()
    # The manifest has to name the native package, or signing refuses it and the device
    # does not find it.
    if '"hnpPackages"' not in text:
        text = re.sub(r'("module"\s*:\s*\{)',
                      lambda match: match.group(1) + '\n    "hnpPackages": [\n'
                      '      { "package": "qtctools.hnp", "type": "private" }\n    ],',
                      text, count=1)
    if "pasteboard_reason" not in text:
        text = text.replace(
            '{\n            "name": "ohos.permission.READ_PASTEBOARD"\n        }',
            '{\n            "name": "ohos.permission.READ_PASTEBOARD",\n'
            '            "reason": "$string:pasteboard_reason",\n'
            '            "usedScene": {\n                "abilities": [\n'
            '                    "QAbility"\n                ],\n'
            '                "when": "inuse"\n            }\n        }', 1)
    module.write_text(text)


def add_native_package(hap: Path, build: Path, toolchain: Path | None, hnpcli: Path) -> None:
    extras = json.loads((build / "src/app/qtcreator-harmonyos-extras.json").read_text())
    files = [Path(name) for name in extras.get("native-package-files", [])]
    if not files and not toolchain:
        print("   nothing to put into a native package")
        return
    staging = build / "harmonyos-hnp"
    shutil.rmtree(staging, ignore_errors=True)
    tree = staging / "src"
    packed = staging / "package/hnp/arm64-v8a"
    (tree / "bin").mkdir(parents=True)
    packed.mkdir(parents=True)
    # A compiler is an installation rather than a file: clang finds its headers and its
    # runtime relative to the binary, and CMake its modules. So the tree goes in whole.
    if toolchain:
        shutil.copytree(toolchain, tree, symlinks=True, dirs_exist_ok=True)
    # Only "bin" is linked into /data/service/hnp, and the binaries' RUNPATH names it,
    # so the libraries go beside the binaries.
    for name in files:
        shutil.copy2(name, tree / "bin")
    # In the application's context the files belong to someone else, so 0770 means no
    # execute.
    for entry in (tree / "bin").iterdir():
        if not entry.is_symlink():
            entry.chmod(0o755)
    run([hnpcli, "pack", "-i", tree, "-o", packed, "-n", "qtctools", "-v", "1.0"])
    # hvigor packages no native package, so it goes in afterwards, before signing, which
    # covers the whole archive. Stored, as the device maps it.
    with zipfile.ZipFile(hap, "a", compression=zipfile.ZIP_STORED) as archive:
        present = set(archive.namelist())
        for directory in ["hnp/", "hnp/arm64-v8a/"]:
            if directory not in present:
                archive.writestr(zipfile.ZipInfo(directory), b"")
        for package in sorted(packed.iterdir()):
            archive.write(package, "hnp/arm64-v8a/" + package.name)
            print("   {} added to {}".format(package.name, hap.name))


def without_daemon(hvigor: Path, directory: Path) -> Path:
    """An hvigorw for harmonydeployqt, which has no option to run hvigor without its daemon.
    A daemon outlives the project it was started in, which a rerun replaces, and is not
    always reachable when the build connects to it."""
    directory.mkdir(parents=True, exist_ok=True)
    wrapper = directory / "hvigorw"
    wrapper.write_text("#!/bin/sh\nexec {} \"$@\" --no-daemon\n".format(shlex.quote(str(hvigor))))
    wrapper.chmod(0o755)
    return wrapper


def sign(hap: Path, output: Path, toolchains: Path, args: argparse.Namespace,
         environment: dict) -> None:
    password = args.keystore_password_file.read_text().strip()
    command = ["java", "-jar", toolchains / "lib/hap-sign-tool.jar", "sign-app",
               "-mode", "localSign", "-signAlg", "SHA256withECDSA", "-signCode", "1",
               "-keyAlias", args.key_alias, "-keyPwd", password, "-keystorePwd", password,
               "-keystoreFile", args.keystore, "-appCertFile", args.certificate,
               "-profileFile", args.profile, "-inFile", hap, "-outFile", output]
    shown = [part if part != password else "<from {}>".format(args.keystore_password_file.name)
             for part in command]
    run(command, env=environment, shown=shown)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", type=absolute, required=True,
                        help="the build of Qt Creator for HarmonyOS, as build_creator.py makes")
    parser.add_argument("--qt", type=absolute, required=True,
                        help="the Qt for HarmonyOS it was built against")
    parser.add_argument("--packages", type=absolute,
                        help="prefix of the libraries a Qt for HarmonyOS needs at run time")
    parser.add_argument("--sdk", type=absolute, required=True,
                        help="HarmonyOS command-line-tools directory")
    parser.add_argument("--toolchain", type=absolute,
                        help="toolchain tree from build_device_toolchain.py, for building on "
                             "the device")
    parser.add_argument("--source", type=absolute,
                        help="Qt Creator sources, this checkout by default")
    parser.add_argument("--bundle", default="org.qtproject.qtcreator",
                        help="bundle name, the one the signing profile is for, %(default)s "
                             "by default")
    parser.add_argument("--output", type=absolute,
                        help="the package to write, qtcreator.hap in the build by default")
    parser.add_argument("--profile", type=absolute, help="signing profile, .p7b")
    parser.add_argument("--certificate", type=absolute, help="signing certificate, .cer")
    parser.add_argument("--keystore", type=absolute, help="keystore with the key, .p12")
    parser.add_argument("--key-alias", help="alias of the key in the keystore")
    parser.add_argument("--keystore-password-file", type=absolute,
                        help="file holding the passphrase of the keystore and the key")
    parser.add_argument("--mcp-port", type=int,
                        help="start the MCP server on this port, with its ui_* tools on. "
                             "For development packages")
    parser.add_argument("--mcp-token-file", type=absolute,
                        help="file with the token the MCP server requires. Everyone on the "
                             "device can read the launch arguments it goes into")
    parser.add_argument("--load-all", action="store_true",
                        help="load every plugin, including those that are off by default")
    args = parser.parse_args()

    signing = [args.profile, args.certificate, args.keystore, args.key_alias,
               args.keystore_password_file]
    if any(signing) and not all(signing):
        raise SystemExit("Signing needs --profile, --certificate, --keystore, --key-alias "
                         "and --keystore-password-file")
    openharmony = openharmony_path(args.sdk)
    toolchains = openharmony / "toolchains"
    readelf = openharmony / "native/llvm/bin/llvm-readelf"
    hvigor = args.sdk / "bin/hvigorw"
    project = args.build / "src/app/harmonyos-build"
    environment = {"PATH": "{}:{}".format(args.sdk / "bin", os.environ["PATH"])}
    # hvigor runs the first java in PATH.
    if os.environ.get("JAVA_HOME"):
        environment["PATH"] = "{}/bin:{}".format(os.environ["JAVA_HOME"], environment["PATH"])

    # harmonydeployqt runs hvigor on the project as Qt's template has it, and hvigor
    # finds no SDK of another version.
    version = sdk_version(args.sdk)
    template = (args.qt / "src/harmonyos/templates/build-profile.json5").read_text()
    wanted = re.search(r'"compatibleSdkVersion"\s*:\s*"([^"]*)"', template)
    if wanted and wanted.group(1) != version:
        raise SystemExit("{} is for SDK {}, {} is {}".format(args.qt, wanted.group(1),
                                                             args.sdk, version))

    # What an earlier run adjusted would be taken over as it is.
    shutil.rmtree(project, ignore_errors=True)
    # harmonydeployqt also runs hvigor. That package is thrown away, the project still
    # has to be adjusted.
    run(["cmake", "--build", args.build, "--target", "qtcreator_make_hap"],
        env=dict(environment,
                 QT_HARMONYOS_HVIGOR=str(without_daemon(hvigor, args.build / "hvigor"))),
        log=args.build / "make-hap.log")

    resources = project / "entry/src/main/resources/resfile/qtcreator"
    shutil.rmtree(resources, ignore_errors=True)
    shutil.copytree(args.build / "share/qtcreator", resources, symlinks=True)

    arguments = [["-pluginpath", DEVICE_PLUGINS], ["-resourcepath", DEVICE_RESOURCES],
                 # Off by default, and on the device it is what finds the build device
                 # beside Qt Creator.
                 ["-load", "HarmonyOS"]]
    if args.load_all:
        arguments.append(["-load", "all"])
    if args.mcp_port:
        arguments.append(["-load", "McpServer", "-mcp-port", str(args.mcp_port)])
        # The install settings are the defaults the user settings fall back to, so the
        # ui_* tools are on from the first run.
        source = args.source or Path(__file__).resolve().parents[2]
        tools = set()
        for file in (source / "src/plugins/mcpserver").glob("*.cpp"):
            tools |= set(re.findall(r'\.name\("(ui_[a-z_]*)"\)', file.read_text()))
        (resources / "QtProject").mkdir(exist_ok=True)
        (resources / "QtProject/QtCreator.ini").write_text(
            "[McpServer]\n" + "".join("EnabledTools\\{}=true\n".format(tool)
                                      for tool in sorted(tools)))
    if args.mcp_token_file:
        arguments.append(["-mcp-token", args.mcp_token_file.read_text().strip()])

    search = [args.build / "lib/qtcreator", args.build / "lib/qtcreator/plugins",
              args.qt / "lib"]
    if args.packages:
        search.append(args.packages / "lib")
    stage_libraries(project, args.build, search, readelf)
    adjust_project(project, args.bundle, version, arguments)

    log = args.build / "hvigor.log"
    run([hvigor, "assembleHap", "--no-daemon"], cwd=project, env=environment, log=log)
    hap = sorted(project.glob("entry/build/default/outputs/default/*.hap"))[0]
    add_native_package(hap, args.build, args.toolchain, toolchains / "hnpcli")

    if not args.keystore:
        output = args.output or args.build / "src/app/qtcreator-unsigned.hap"
        shutil.copy2(hap, output)
        print("\nPackaged Qt Creator unsigned as {}".format(output))
        return 0

    output = args.output or args.build / "src/app/qtcreator.hap"
    sign(hap, output, toolchains, args, environment)
    print("\nPackaged and signed Qt Creator as {}".format(output))
    return 0


if __name__ == "__main__":
    sys.exit(main())
