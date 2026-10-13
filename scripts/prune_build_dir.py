#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from __future__ import annotations
import argparse
import os
import sys
from pathlib import Path

OBJECT_SUFFIXES = ('.o', '.obj')


def object_files(build_path: Path):
    for root, _, files in os.walk(build_path):
        if 'CMakeFiles' not in Path(root).relative_to(build_path).parts:
            continue
        for name in files:
            if name.endswith(OBJECT_SUFFIXES):
                yield Path(root) / name


def prune(build_path: Path, dry_run: bool) -> tuple[int, int]:
    count = 0
    size = 0
    for path in object_files(build_path):
        size += path.lstat().st_size
        count += 1
        if not dry_run:
            path.unlink()
    return count, size


def get_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Remove the object files from a build directory, so that they are not '
        'part of the artifacts that are uploaded and unpacked for the test stage')
    parser.add_argument('--build', help='path to the build directory', required=True)
    parser.add_argument('--dry-run', help='only report what would be removed',
                        action='store_true', default=False)
    return parser.parse_args()


def main() -> None:
    args = get_arguments()
    build_path = Path(args.build)
    if not build_path.is_dir():
        sys.exit(f'Build directory {build_path} does not exist')
    count, size = prune(build_path, args.dry_run)
    action = 'Would remove' if args.dry_run else 'Removed'
    print(f'{action} {count} object files, {size / 1e9:.2f} GB')


if __name__ == '__main__':
    main()
