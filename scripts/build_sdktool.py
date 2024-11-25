#!/usr/bin/env python3
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from __future__ import annotations
import argparse
import os
from pathlib import Path
from typing import NamedTuple

from common import (is_linux_platform, is_mac_platform, is_windows_platform,
                    download_and_extract, check_print_call, sevenzip_command,
                    get_single_subdir)


class BuildParams(NamedTuple):
    src_path: Path
    build_path: Path
    target_path: Path
    make_command: str
    universal: bool = False
    platform: str | None = None


def qt_static_configure_options() -> list[str]:
    return ['-release', '-opensource', '-confirm-license', '-accessibility',
            '-no-gui',
            '-no-openssl',
            '-no-feature-sql',
            '-qt-zlib',
            '-nomake', 'examples',
            '-nomake', 'tests',
            '-static'] + qt_static_platform_configure_options()


def qt_static_platform_configure_options() -> list[str]:
    if is_windows_platform():
        return ['-static-runtime', '-no-icu']
    if is_linux_platform():
        return ['-no-icu', '-no-glib', '-qt-zlib', '-qt-pcre', '-qt-doubleconversion']
    return []


def get_qt_src_path(qt_build_base: Path) -> Path:
    return qt_build_base / 'src'


def get_qt_build_path(qt_build_base: Path) -> Path:
    return qt_build_base / 'build'


def get_qt_install_path(qt_build_base: Path) -> Path:
    return qt_build_base / 'install'


def configure_qt(params: BuildParams, src: Path, build: Path, install: Path) -> None:
    build.mkdir(parents=True, exist_ok=True)
    configure = src / "configure"
    cmd = [str(configure), "-prefix", str(install)] + qt_static_configure_options()
    if params.platform:
        cmd.extend(['-platform', params.platform])
    if params.universal:
        cmd.extend(['--', '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'])
    check_print_call(cmd, cwd=build)


def build_qt(params: BuildParams, build: Path) -> None:
    check_print_call([params.make_command], cwd=build)


def install_qt(params: BuildParams, build: Path) -> None:
    check_print_call([params.make_command, 'install'], cwd=build)


def build_sdktool_impl(params: BuildParams, qt_install_path: Path) -> None:
    params.build_path.mkdir(parents=True, exist_ok=True)
    cmake_args = [
        'cmake', '-DCMAKE_PREFIX_PATH=' + str(qt_install_path), '-DCMAKE_BUILD_TYPE=Release'
    ]
    # force MSVC on Windows, because it looks for GCC in the PATH first,
    # even if MSVC is first mentioned in the PATH...
    # TODO would be nicer if we only did this if cl.exe is indeed first in the PATH
    if is_windows_platform():
        cmake_args += ['-DCMAKE_C_COMPILER=cl', '-DCMAKE_CXX_COMPILER=cl']
        cmake_args += ['-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded']

    if params.universal:
        cmake_args += ['-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64']

    cmd = cmake_args + ['-G', 'Ninja', str(params.src_path)]
    check_print_call(cmd, cwd=params.build_path)
    check_print_call(['cmake', '--build', '.'], cwd=params.build_path)
    cmd = ['cmake', '--install', '.', '--prefix', str(params.target_path)]
    check_print_call(cmd, cwd=params.build_path)


def sign_sdktool(params: BuildParams,
                 environment: dict[str, str]) -> None:
    signing_identity = environment.get('SIGNING_IDENTITY')
    if not is_mac_platform() or not signing_identity:
        return
    check_print_call(['codesign', '-o', 'runtime', '--force', '-s', signing_identity,
             '-v', 'sdktool'],
            cwd=params.target_path,
            env=environment)


def build_sdktool(
    qt_src_url: str,
    qt_build_base: Path,
    sdktool_src_path: Path,
    sdktool_build_path: Path,
    sdktool_target_path: Path,
    make_command: str,
    universal: bool = False,
    platform: str | None = None,
    environment: dict[str, str] | None = None
) -> None:
    if not environment:
        environment = os.environ.copy()
    params = BuildParams(
        src_path=sdktool_src_path,
        build_path=sdktool_build_path,
        target_path=sdktool_target_path,
        make_command=make_command,
        platform=platform,
        universal=universal
    )
    qt_src = get_qt_src_path(qt_build_base)
    qt_build = get_qt_build_path(qt_build_base)
    qt_install = get_qt_install_path(qt_build_base)
    download_and_extract([qt_src_url], qt_src, qt_build_base)
    qt_src = get_single_subdir(qt_src)
    configure_qt(params, qt_src, qt_build, qt_install)
    build_qt(params, qt_build)
    install_qt(params, qt_build)
    build_sdktool_impl(params, qt_install)
    sign_sdktool(params, environment)


def zip_sdktool(
    sdktool_target_path: Path, out_7zip: Path
) -> None:
    glob = "*.exe" if is_windows_platform() else "*"
    check_print_call(
        cmd=sevenzip_command() + [str(out_7zip), glob],
        cwd=sdktool_target_path
    )


def get_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Build sdktool')
    parser.add_argument('--qt-url', help='URL to Qt sources', required=True)
    parser.add_argument(
        '--qt-build', help='Path that is used for building Qt', required=True, type=Path
    )
    parser.add_argument('--src', help='Path to sdktool sources', required=True, type=Path)
    parser.add_argument(
        '--build', help='Path that is used for building sdktool', required=True, type=Path
    )
    parser.add_argument(
        '--install', help='Path that is used for installing sdktool', required=True, type=Path
    )
    parser.add_argument('--make-command', help='Make command to use for Qt', required=True)
    parser.add_argument('--platform', help='Platform argument for configuring Qt',
                        required=False)
    parser.add_argument('--universal', help='Build universal binaries on macOS',
                        action='store_true', default=False, required=False)
    return parser.parse_args()


def main() -> None:
    args = get_arguments()
    build_sdktool(
        qt_src_url=args.qt_url,
        qt_build_base=args.qt_build,
        sdktool_src_path=args.src,
        sdktool_build_path=args.build,
        sdktool_target_path=args.install,
        make_command=args.make_command,
        platform=args.platform
    )


if __name__ == '__main__':
    main()
