#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Run the scripts that build for HarmonyOS in the image this directory describes.

The image holds host tools only and is built locally. The scripts, the SDK,
the installer's Qt and the Qt Creator sources are mounted read-only, and
everything the scripts produce goes into --path, owned by the calling user:

    <path>/packages/arm64-ohos   the libraries a Qt for HarmonyOS needs at run time
    <path>/tools                 libexec and bin with the Qt tools for the device
    <path>/creator               the build of Qt Creator for the device

A later run reuses what the earlier one left in --path.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

IMAGE = "qtc-harmonyos"
HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[2]


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
                        help="HarmonyOS command-line-tools directory")
    parser.add_argument("--path", type=absolute, required=True,
                        help="directory for the packages, the tools and their work trees")
    parser.add_argument("--source", type=absolute, default=SOURCE,
                        help="Qt Creator sources, this checkout by default")
    parser.add_argument("--docker", default="docker", help="docker executable")
    args = parser.parse_args()

    if not (args.qt.parent / "sha1s.txt").is_file():
        raise SystemExit("No sha1s.txt beside {}".format(args.qt))
    args.path.mkdir(parents=True, exist_ok=True)

    run([args.docker, "build", "-t", IMAGE, HERE])

    qt = "/qt/" + args.qt.name
    script = ("/scripts/install_third_party.py --sdk /sdk --path /out/packages"
              " && /scripts/build_qt_tools.py --qt {} --sdk /sdk"
              " --packages /out/packages/arm64-ohos --path /out/tools"
              " && /scripts/build_creator.py --qt {} --sdk /sdk --source /source"
              " --packages /out/packages/arm64-ohos --path /out/creator").format(qt, qt)
    run([args.docker, "run", "--rm",
         "--user", "{}:{}".format(os.getuid(), os.getgid()),
         "-e", "HOME=/out",
         "-v", "{}:/scripts:ro".format(HERE.parent),
         "-v", "{}:/sdk:ro".format(args.sdk),
         "-v", "{}:/qt:ro".format(args.qt.parent),
         "-v", "{}:/source:ro".format(args.source),
         "-v", "{}:/out".format(args.path),
         IMAGE, "sh", "-c", script])
    return 0


if __name__ == "__main__":
    sys.exit(main())
