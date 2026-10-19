#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Verify the Android SDK Command-line Tools archives in sdk_definitions.json.

Reads "common"/"sdk_tools_url" from share/qtcreator/android/sdk_definitions.json,
pairs every archive URL "<key>" with its checksum "<key>_sha256", downloads the
archives and compares their SHA-256 with the listed one. Run it after changing
the URLs or checksums, before Qt Creator downloads the tools for users.

The archives are hashed while they are downloaded and are not written to disk.

Exits with 0 if all archives match, with 1 otherwise.
"""

import argparse
import hashlib
import http.client
import json
import sys
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

DEFAULT_DEFINITIONS = (Path(__file__).resolve().parent.parent
                       / 'share' / 'qtcreator' / 'android' / 'sdk_definitions.json')
SHA256_SUFFIX = '_sha256'
CHUNK_SIZE = 1024 * 1024
TIMEOUT_SECONDS = 60


def archive_pairs(sdk_tools):
    """Returns ({key: (url, sha256)}, [errors]) for the "sdk_tools_url" object."""
    pairs = {}
    errors = []
    for key, value in sdk_tools.items():
        if key.endswith(SHA256_SUFFIX):
            if key[:-len(SHA256_SUFFIX)] not in sdk_tools:
                errors.append(f'{key}: no archive URL "{key[:-len(SHA256_SUFFIX)]}"')
            continue
        sha256 = sdk_tools.get(key + SHA256_SUFFIX)
        if sha256 is None:
            errors.append(f'{key}: no checksum "{key + SHA256_SUFFIX}"')
            continue
        if not isinstance(value, str):
            errors.append(f'{key}: archive URL is not a string')
            continue
        if not isinstance(sha256, str):
            errors.append(f'{key + SHA256_SUFFIX}: checksum is not a string')
            continue
        pairs[key] = (value, sha256.lower())
    return pairs, errors


def download_sha256(url):
    sha256 = hashlib.sha256()
    with urllib.request.urlopen(url, timeout=TIMEOUT_SECONDS) as response:
        while chunk := response.read(CHUNK_SIZE):
            sha256.update(chunk)
    return sha256.hexdigest()


def verify(key, url, expected):
    try:
        actual = download_sha256(url)
    except (OSError, ValueError, http.client.HTTPException) as e:
        return False, f'{key}: cannot download {url}: {e}'
    if actual != expected:
        return False, f'{key}: checksum mismatch for {url}\n' \
                      f'    expected {expected}\n' \
                      f'    actual   {actual}'
    return True, f'{key}: OK for {url}\n' \
                 f'    sha256 {actual}'


def main():
    parser = argparse.ArgumentParser(
        description='Download the Android SDK Command-line Tools archives listed in '
                    'sdk_definitions.json and verify their SHA-256 checksums.')
    parser.add_argument('definitions', nargs='?', type=Path, default=DEFAULT_DEFINITIONS,
                        help='the sdk_definitions.json to check (default: %(default)s)')
    args = parser.parse_args()

    try:
        definitions = json.loads(args.definitions.read_text(encoding='utf-8'))
        sdk_tools = definitions['common']['sdk_tools_url']
    except (OSError, ValueError, KeyError, TypeError) as e:
        print(f'Cannot read "common"/"sdk_tools_url" from {args.definitions}: {e}',
              file=sys.stderr)
        return 1

    pairs, errors = archive_pairs(sdk_tools)
    for error in errors:
        print(error)
    if not pairs:
        print('No archives to verify.')
        return 1

    with ThreadPoolExecutor(max_workers=len(pairs)) as executor:
        futures = [executor.submit(verify, key, url, sha256)
                   for key, (url, sha256) in pairs.items()]
        results = [future.result() for future in futures]

    for _, message in results:
        print(message)
    return 0 if not errors and all(ok for ok, _ in results) else 1


if __name__ == '__main__':
    sys.exit(main())
