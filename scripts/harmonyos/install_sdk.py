#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Install a public OpenHarmony SDK for Qt Creator to use as a HarmonyOS SDK.

The two components installed by default are the ones Qt Creator reads: "native"
holds the compiler, the sysroot and the CMake toolchain file, and "toolchains"
holds hdc, which is how a device is reached. That covers compiling and talking
to a device.

It does not cover packaging an application into a .hap, because hvigor, ohpm
and the Node.js they run on are not part of the SDK at all. They come with
Huawei's command-line-tools package, next to the copy of this SDK it carries.

Point Preferences > SDKs > HarmonyOS at the directory given as --path, or at a
command-line-tools directory where one is installed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import stat
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

BASE_URL = "https://repo.huaweicloud.com/openharmony/os"

# Host key to the archive published for it and the directory it holds inside.
ARCHIVES = {
    "linux": ("ohos-sdk-windows_linux-public.tar.gz", "linux"),
    "windows": ("ohos-sdk-windows_linux-public.tar.gz", "windows"),
    "darwin-arm64": ("L2-SDK-MAC-M1-PUBLIC.tar.gz", "darwin"),
    "darwin-x86_64": ("ohos-sdk-mac-public.tar.gz", "darwin"),
}

# The components Qt Creator reads. "native" holds the toolchain and the sysroot,
# "toolchains" holds hdc and the signing tools.
DEFAULT_COMPONENTS = ["native", "toolchains"]


def host_key() -> str:
    system = platform.system()
    if system == "Linux":
        if platform.machine() != "x86_64":
            raise SystemExit("The OpenHarmony SDK is published for Linux on x86-64 only")
        return "linux"
    if system == "Windows":
        return "windows"
    if system == "Darwin":
        return "darwin-arm64" if platform.machine() == "arm64" else "darwin-x86_64"
    raise SystemExit("The OpenHarmony SDK is published for Linux, Windows and macOS only")


def download(url: str, target: Path) -> Path:
    if target.exists():
        print("Have {}".format(target))
        return target
    print("Downloading {}".format(url))
    part = target.with_suffix(target.suffix + "-part")
    urllib.request.urlretrieve(url, str(part))
    part.rename(target)
    return target


def check_sha256(archive: Path, expected_file: Path) -> None:
    expected = expected_file.read_text().split()[0].strip()
    digest = hashlib.sha256()
    with archive.open("rb") as data:
        for block in iter(lambda: data.read(1 << 20), b""):
            digest.update(block)
    if digest.hexdigest() != expected:
        raise SystemExit("Checksum mismatch for {}".format(archive))
    print("Checksum of {} is as published".format(archive.name))


def unzip(component: Path, target: Path) -> None:
    """Unpack a component, keeping the executable bits and the symbolic links.

    zipfile does neither on its own, and without them there is no clang++ and no
    library with a version suffix.
    """
    with zipfile.ZipFile(component) as archive:
        for info in archive.infolist():
            mode = info.external_attr >> 16
            if stat.S_ISLNK(mode):
                link = target / info.filename
                link.parent.mkdir(parents=True, exist_ok=True)
                if link.is_symlink() or link.exists():
                    link.unlink()
                link.symlink_to(archive.read(info).decode())
                continue
            path = Path(archive.extract(info, target))
            if mode & 0o7777:
                os.chmod(path, mode & 0o7777)


def extract_components(archive: Path, host_dir: str, components: list[str],
                       target: Path) -> list[str]:
    """Unpack the requested components of the tarball into target.

    The tarball holds one directory per host, each holding one zip per
    component, and where that directory sits differs between SDK versions. So
    the members are matched by name rather than by depth, in a single streaming
    pass, and only what was asked for is written out.
    """
    pattern = re.compile(r"(?:^|/)" + re.escape(host_dir) + r"/(\w+)-[^/]*\.zip$")
    done = []
    target.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive, "r|gz") as tar:
        for member in tar:
            match = pattern.search(member.name)
            if not match or match.group(1) not in components:
                continue
            name = match.group(1)
            print("Unpacking component {}".format(name))
            data = tar.extractfile(member)
            assert data is not None
            with tempfile.TemporaryDirectory() as temp:
                component = Path(temp) / "{}.zip".format(name)
                with component.open("wb") as out:
                    for block in iter(lambda: data.read(1 << 20), b""):
                        out.write(block)
                unzip(component, target)
            done.append(name)
    return done


def describe(target: Path, components: list[str]) -> None:
    for name in components:
        package = target / name / "oh-uni-package.json"
        if not package.exists():
            continue
        info = json.loads(package.read_text())
        print("{}: version {}, API {}".format(name, info.get("version"),
                                              info.get("apiVersion")))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--path", type=Path, required=True,
                        help="directory to install the SDK into")
    parser.add_argument("--version", default="6.1",
                        help="OpenHarmony release to install, 6.1 by default")
    parser.add_argument("--components", nargs="+", default=DEFAULT_COMPONENTS,
                        help="components to unpack, \"native toolchains\" by default")
    parser.add_argument("--download-path", type=Path,
                        help="where to keep the downloaded archive, a temporary "
                             "directory by default")
    parser.add_argument("--archive", type=Path,
                        help="an already downloaded archive to unpack instead, which "
                             "skips the download and the checksum")
    args = parser.parse_args()

    archive_name, host_dir = ARCHIVES[host_key()]
    url = "{}/{}-Release/{}".format(BASE_URL, args.version, archive_name)

    with tempfile.TemporaryDirectory() as temp:
        if args.archive:
            archive = args.archive
        else:
            download_path = args.download_path or Path(temp)
            download_path.mkdir(parents=True, exist_ok=True)
            # The published name is the same for every release.
            local_name = "{}-{}".format(args.version, archive_name)
            archive = download(url, download_path / local_name)
            checksum = download(url + ".sha256", download_path / (local_name + ".sha256"))
            check_sha256(archive, checksum)
        done = extract_components(archive, host_dir, args.components, args.path)

    missing = [name for name in args.components if name not in done]
    if missing:
        raise SystemExit("The archive holds no {} for {}".format(", ".join(missing), host_dir))

    describe(args.path, done)
    print("\nInstalled {} into {}".format(", ".join(done), args.path))
    print("Set it in Preferences > SDKs > HarmonyOS. Packaging an application "
          "additionally\nneeds hvigor and Node.js from Huawei's command-line-tools.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
