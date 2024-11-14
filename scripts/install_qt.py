#!/usr/bin/env python3
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from __future__ import annotations
import argparse
from common import download_and_extract_tuples
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory
from typing import Optional


def get_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Install Qt from individual module archives')
    parser.add_argument('--qt-path', help='path to Qt', type=Path, required=True)
    parser.add_argument('--qt-module', help='Qt module package url (.7z) needed for building',
                        action='append', dest='qt_modules', default=[])

    parser.add_argument('--base-url', help='Base URL for given module_name(s)')
    parser.add_argument(
        'module_name',
        help='Name of Qt module to install, based on --base-url and --base-url-postfix',
        nargs='*'
    )
    parser.add_argument(
        '--base-url-postfix',
        help='Postfix to add to URLs constructed from --base-url and given module_name(s)',
        default=''
    )

    # Linux
    parser.add_argument('--icu7z', help='a file or url where to get ICU libs as 7z')

    args = parser.parse_args(sys.argv[1:])

    return args


def patch_qt(qt_path: Path) -> None:
    print("##### patch Qt #####")
    qmake_binary = qt_path / 'bin' / 'qmake'
    # write qt.conf
    with (qt_path / 'bin' / 'qt.conf').open('w', encoding='utf-8') as qt_conf_file:
        qt_conf_file.write('[Paths]\n')
        qt_conf_file.write('Prefix=..\n')
    subprocess.check_call([str(qmake_binary), '-query'], cwd=qt_path)


def install_qt(
    qt_path: Path,
    qt_modules: list[str],
    icu_url: Optional[str] = None
) -> None:
    """
    Install Qt to directory qt_path with the specified module and library packages.

    Args:
        qt_path: File system path to Qt (target install directory)
        qt_modules: List of Qt module package URLs (.7z)
        icu_url: Local or remote URI to Linux ICU libraries (.7z)
        temp_path: Temporary path used for saving downloaded archives

    Raises:
        SystemExit: When qt_modules list is empty

    """
    if not qt_modules:
        raise SystemExit("No modules specified in qt_modules")
    qt_path = qt_path.resolve()
    need_to_install_qt = not qt_path.exists()

    with TemporaryDirectory() as temporary_dir:
        if need_to_install_qt:
            url_target_tuples = [(url, qt_path) for url in qt_modules]
            if icu_url:
                url_target_tuples.append((icu_url, qt_path / 'lib'))
            download_and_extract_tuples(url_target_tuples, Path(temporary_dir))
            patch_qt(qt_path)


def main() -> None:
    """Main"""
    args: argparse.Namespace = get_arguments()
    # Check that qt_module(s) or base-url/module_name(s) combo is specified
    if not args.qt_modules and not (args.base_url and args.module_name):
        raise SystemExit("'qt-module(s)' and/or 'base-url' with 'module_name(s)' required")
    # Create the list of modules from qt_modules + module_names with base_url and postfix
    qt_modules: list[str] = args.qt_modules
    if args.base_url and args.module_name:
        for module in args.module_name:
            qt_modules += [args.base_url + "/" + module + "/" + module + args.base_url_postfix]

    install_qt(
        qt_path=args.qt_path,
        qt_modules=qt_modules,
        icu_url=args.icu7z
    )


if __name__ == '__main__':
    main()
