#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Build Qt Creator for a HarmonyOS device, with all it needs there, as a package.

Everything runs in the image that docker/Dockerfile describes, which holds
host tools only and is built locally. The inputs are mounted read-only:

- the installer's Qt for HarmonyOS, with the desktop Qt of the same version
  beside it,
- the HarmonyOS command-line tools, of the SDK version that Qt's application
  template names, unpacked or as the zip they are downloaded as, which is
  unpacked into --path once,
- optionally the device-hosted native SDK ("native-ohos-x64"), for the
  toolchain that lets the device build. Without it, the toolchain stage
  downloads a pinned one,
- optionally the signing profile, certificate and keystore, and a file with
  the passphrase of the keystore, and the profile and certificate for the
  runner, whose bundle name is a different one,
- this checkout, as the Qt Creator sources and the scripts.

The stages, each a script beside this one, and what they leave in --path:

    packages    install_third_party.py      packages/arm64-ohos
    tools       build_qt_tools.py           tools
    creator     build_creator.py            creator
    toolchain   build_device_toolchain.py   toolchain
    hap         build_hap.py                qtcreator.hap, or qtcreator-unsigned.hap
    runner      build_runner_hap.py         qtcrunner.hap, or qtcrunner-unsigned.hap

All other sources are pinned downloads. A later run reuses what the earlier
one left in --path, so after the first run a stage costs little unless its
inputs changed.

The packages are installed with "hdc install -r <path>/qtcreator.hap" and
"hdc install -r <path>/qtcrunner.hap". Qt Creator on the device runs what it
builds through the runner.
"""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
from pathlib import Path

IMAGE = "qtc-harmonyos"
HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[1]
STAGES = ["packages", "tools", "creator", "toolchain", "hap", "runner"]


def absolute(path: str) -> Path:
    return Path(path).expanduser().resolve()


def run(command: list) -> None:
    command = [str(part) for part in command]
    print("+ {}".format(" ".join(command)), flush=True)
    if subprocess.run(command).returncode != 0:
        raise SystemExit("{} failed".format(" ".join(command[:2])))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--qt", type=absolute, required=True,
                        help="the installer's Qt for HarmonyOS, such as "
                             "~/Qt/6.12.0/harmonyos_arm64_v8a, with the desktop Qt beside it")
    parser.add_argument("--sdk", type=absolute, required=True,
                        help="HarmonyOS command-line tools, the directory or the zip they "
                             "are downloaded as")
    parser.add_argument("--device-sdk", type=absolute,
                        help="the device-hosted native SDK, its native-ohos-x64 zip or the "
                             "directory it was unpacked into. Downloaded by default")
    parser.add_argument("--path", type=absolute, required=True,
                        help="directory for everything the stages produce and their work "
                             "trees")
    parser.add_argument("--source", type=absolute, default=SOURCE,
                        help="Qt Creator sources, this checkout by default")
    parser.add_argument("--stages", default=",".join(STAGES),
                        help="comma-separated stages to run, all by default")
    parser.add_argument("--profile", type=absolute, help="signing profile, .p7b")
    parser.add_argument("--certificate", type=absolute, help="signing certificate, .cer")
    parser.add_argument("--keystore", type=absolute, help="keystore with the key, .p12")
    parser.add_argument("--key-alias", help="alias of the key in the keystore")
    parser.add_argument("--keystore-password-file", type=absolute,
                        help="file holding the passphrase of the keystore and the key")
    parser.add_argument("--bundle", help="bundle name, the one the signing profile is for")
    parser.add_argument("--runner-profile", type=absolute,
                        help="signing profile for the runner, .p7b")
    parser.add_argument("--runner-certificate", type=absolute,
                        help="signing certificate for the runner, .cer")
    parser.add_argument("--runner-bundle",
                        help="bundle name of the runner, the one its profile is for")
    parser.add_argument("--mcp-port", type=int,
                        help="start the MCP server on this port. For development packages")
    parser.add_argument("--mcp-token-file", type=absolute,
                        help="file with the token the MCP server requires")
    parser.add_argument("--docker", default="docker", help="docker executable")
    args = parser.parse_args()

    stages = args.stages.split(",")
    unknown = set(stages) - set(STAGES)
    if unknown:
        raise SystemExit("Unknown stages: {}".format(", ".join(sorted(unknown))))
    if not (args.qt.parent / "sha1s.txt").is_file():
        raise SystemExit("No sha1s.txt beside {}".format(args.qt))
    args.path.mkdir(parents=True, exist_ok=True)

    mounts = {"/scripts": HERE, "/qt": args.qt.parent, "/source": args.source}
    qt = "/qt/" + args.qt.name
    unpack = ""
    if args.sdk.is_dir():
        sdk = "/sdk"
        mounts[sdk] = args.sdk
    elif args.sdk.is_file():
        sdk = "/out/command-line-tools"
        archive = "/inputs/sdk/" + args.sdk.name
        mounts[archive] = args.sdk
        # unzip keeps the permission bits, which zipfile drops.
        unpack = ("test -d {sdk} || {{ rm -rf {tmp} && unzip -q {archive} -d {tmp} && "
                  "mv {tmp}/command-line-tools {sdk} && rm -rf {tmp}; }} && ").format(
                      sdk=sdk, tmp="/out/sdk-unpacking", archive=shlex.quote(archive))
    else:
        raise SystemExit("No command-line tools at {}".format(args.sdk))
    commands = {
        "packages": ["/scripts/install_third_party.py", "--sdk", sdk,
                     "--path", "/out/packages"],
        "tools": ["/scripts/build_qt_tools.py", "--qt", qt, "--sdk", sdk,
                  "--packages", "/out/packages/arm64-ohos", "--path", "/out/tools"],
        "creator": ["/scripts/build_creator.py", "--qt", qt, "--sdk", sdk,
                    "--source", "/source", "--packages", "/out/packages/arm64-ohos",
                    "--path", "/out/creator"],
        "toolchain": ["/scripts/build_device_toolchain.py", "--sdk", sdk,
                      "--path", "/out/toolchain"],
        "hap": ["/scripts/build_hap.py", "--build", "/out/creator", "--qt", qt,
                "--sdk", sdk, "--packages", "/out/packages/arm64-ohos",
                "--source", "/source"],
        "runner": ["/scripts/build_runner_hap.py", "--qt", qt, "--sdk", sdk,
                   "--packages", "/out/packages/arm64-ohos", "--source", "/source",
                   "--path", "/out/runner"],
    }

    if args.device_sdk:
        mounts["/device-sdk"] = args.device_sdk if args.device_sdk.is_dir() \
            else args.device_sdk.parent
        commands["toolchain"] += ["--device-sdk", "/device-sdk" if args.device_sdk.is_dir()
                                  else "/device-sdk/" + args.device_sdk.name]
    if "toolchain" in stages or (args.path / "toolchain" / "bin").is_dir():
        commands["hap"] += ["--toolchain", "/out/toolchain"]

    # Each file on its own, so that nothing else beside it becomes visible, and under
    # its own name, because the signing tool goes by the extension.
    for option, value in [("--profile", args.profile), ("--certificate", args.certificate),
                          ("--keystore", args.keystore),
                          ("--keystore-password-file", args.keystore_password_file),
                          ("--mcp-token-file", args.mcp_token_file)]:
        if value:
            inside = "/inputs/{}/{}".format(option.lstrip("-"), value.name)
            mounts[inside] = value
            commands["hap"] += [option, inside]
            # The runner is signed with the same key, under a profile of its own.
            if args.runner_profile and option.startswith("--keystore"):
                commands["runner"] += [option, inside]
    for option, value in [("--profile", args.runner_profile),
                          ("--certificate", args.runner_certificate)]:
        if value:
            inside = "/inputs/runner-{}/{}".format(option.lstrip("-"), value.name)
            mounts[inside] = value
            commands["runner"] += [option, inside]
    if args.key_alias:
        commands["hap"] += ["--key-alias", args.key_alias]
        if args.runner_profile:
            commands["runner"] += ["--key-alias", args.key_alias]
    if args.runner_bundle:
        commands["runner"] += ["--bundle", args.runner_bundle]
    if args.bundle:
        commands["hap"] += ["--bundle", args.bundle]
    if args.mcp_port:
        commands["hap"] += ["--mcp-port", str(args.mcp_port)]
    commands["hap"] += ["--output", "/out/qtcreator.hap" if args.keystore
                        else "/out/qtcreator-unsigned.hap"]
    commands["runner"] += ["--output", "/out/qtcrunner.hap" if args.runner_profile
                           else "/out/qtcrunner-unsigned.hap"]

    run([args.docker, "build", "-t", IMAGE, HERE / "docker"])

    script = " && ".join(shlex.join(commands[stage]) for stage in STAGES if stage in stages)
    script = unpack + script
    command = [args.docker, "run", "--rm",
               "--user", "{}:{}".format(os.getuid(), os.getgid()),
               "-e", "HOME=/out", "-v", "{}:/out".format(args.path)]
    for inside, outside in mounts.items():
        command += ["-v", "{}:{}:ro".format(outside, inside)]
    run(command + [IMAGE, "sh", "-c", script])
    return 0


if __name__ == "__main__":
    sys.exit(main())
