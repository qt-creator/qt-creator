#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

r"""List the images under doc/ that no tracked file in the repository references.

A permissive git grep (fixed strings, case-insensitive) collects every line
that contains the file name of an image. A case-sensitive match then finds the
names in these lines that are not part of a longer file name ("my-run.png" and
"Run.png" do not contain "run.png"), together with the path written in front
of each name. A written path references an image if

- relative to the directory of the file that contains it, it leads to the
  image, as in QML, CSS, HTML, resource and build files, or
- it is in documentation text and the path of the image ends with it,
  ignoring "./" and "../", as qdoc looks images up in its imagedirs.
  Documentation text is a file under doc/, a line that contains \image or
  \inlineimage, or a link to an image on doc.qt.io.

So ":/utils/images/snapshot.png" in a source file does not reference
doc/qtcreator/images/icons/snapshot.png, but "\image icons/snapshot.png" does.
A name without a path, as in "\image snapshot.png", references every image
of that name under doc/. Lines in comments and in \omit blocks count as
references.

The images in the directories that --exclude names, by default icons/ and
numbers/, are not checked.

The unreferenced images are printed to stdout, one path per line, relative to
the top level of the repository.
"""

import argparse
import posixpath
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path, PurePosixPath

IMAGE_SUFFIXES = {'.gif', '.jpeg', '.jpg', '.png', '.svg', '.webp'}
DEFAULT_EXCLUDED_DIRS = ('icons', 'numbers')
QDOC_IMAGE_COMMAND = re.compile(rb'\\(?:inline)?image\b')
DOC_IMAGE_URL = re.compile(r'https?://doc\.qt\.io/[^/]+/images/')
URL_SCHEME = re.compile(r'^[a-z]*:')  # Also matches the ":" of ":/" resource paths.


def git(root, *args, stdin=None):
    result = subprocess.run(['git', '-C', str(root), *args], input=stdin, capture_output=True)
    if result.returncode not in (0, 1):  # 1: git grep found nothing
        sys.exit(f'git {" ".join(args)} failed:\n{result.stderr.decode(errors="replace")}')
    return result.stdout


def is_excluded(path, excluded_dirs):
    parent = f'/{PurePosixPath(path).parent}/'
    return any(f'/{excluded.strip("/")}/' in parent for excluded in excluded_dirs)


def doc_images(root, excluded_dirs):
    """Returns the paths of the images under doc/, keyed by file name, and the
    number of images skipped because they are in excluded directories."""
    paths = git(root, 'ls-files', '-z', '--', 'doc').decode().split('\0')
    images = defaultdict(list)
    skipped = 0
    for path in paths:
        if PurePosixPath(path).suffix.lower() not in IMAGE_SUFFIXES:
            continue
        if is_excluded(path, excluded_dirs):
            skipped += 1
        else:
            images[PurePosixPath(path).name].append(path)
    return images, skipped


def lead_to(file, text, written, paths):
    """Returns the paths that a path written in a line of file leads to."""
    url = DOC_IMAGE_URL.match(written)
    if url:
        written = written[url.end():]
    targets = set()
    if not URL_SCHEME.match(written):
        resolved = posixpath.normpath(posixpath.join(posixpath.dirname(file), written))
        if resolved in paths:
            targets.add(resolved)
    if url or file.startswith('doc/') or QDOC_IMAGE_COMMAND.search(text):
        tail = '/'.join(part for part in URL_SCHEME.sub('', written).split('/')
                        if part not in ('', '.', '..'))
        targets.update(path for path in paths if path == tail or path.endswith('/' + tail))
    return targets


def find_references(root, images, excluded_file, verbose):
    """Returns the referenced images and, if verbose, per name the lines that contain
    the name but reference no image of that name."""
    names = list(images)
    # The path in front of the name is captured. A trailing "." ends a sentence,
    # a "." followed by a word character continues the file name ("foo.png.bak").
    pattern = re.compile(rb'(?<![\w./:-])([\w./:-]*/)?('
                         + b'|'.join(re.escape(name.encode()) for name in names)
                         + rb')(?![\w-]|\.\w)')
    output = git(root, 'grep', '-I', '-n', '-z', '-i', '-F', '-f', '-',
                 '--', '.', f':(exclude){excluded_file}', stdin='\n'.join(names).encode())

    referenced = set()
    rejected = defaultdict(list)
    for line in output.split(b'\n'):
        if not line:
            continue
        file, line_number, text = line.split(b'\0', 2)
        file = file.decode()
        resolved = set()
        for prefix, name in pattern.findall(text):
            name = name.decode()
            if file in images[name]:
                continue
            targets = lead_to(file, text, prefix.decode() + name, images[name])
            if targets:
                referenced |= targets
                resolved.add(name)
        if not verbose:
            continue
        hit = f'{file}:{line_number.decode()}: {text.strip().decode(errors="replace")}'
        lowered = text.lower()
        for name in names:
            if (name not in resolved and file not in images[name]
                    and name.lower().encode() in lowered):
                rejected[name].append(hit)
    return referenced, rejected


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='print to stderr the lines that contain the name of an image '
                             'but reference no image of that name')
    parser.add_argument('--exclude', metavar='DIR', nargs='*', default=DEFAULT_EXCLUDED_DIRS,
                        help='skip the images in directories with these names or paths '
                             f'(default: {" ".join(DEFAULT_EXCLUDED_DIRS)}); '
                             'without DIR, check all images')
    args = parser.parse_args()

    script = Path(__file__).resolve()
    root = Path(git(script.parent, 'rev-parse', '--show-toplevel').decode().strip())
    images, skipped = doc_images(root, args.exclude)
    referenced, rejected = find_references(
        root, images, script.relative_to(root).as_posix(), args.verbose)

    for name in sorted(rejected):
        print(f'{name}: rejected', *rejected[name], sep='\n    ', file=sys.stderr)

    all_paths = [path for paths in images.values() for path in paths]
    unreferenced = sorted(path for path in all_paths if path not in referenced)
    for path in unreferenced:
        print(path)
    print(f'{len(unreferenced)} of {len(all_paths)} images under doc/ are unreferenced.',
          file=sys.stderr)
    if skipped:
        excluded = ', '.join(f'{excluded.strip("/")}/' for excluded in args.exclude)
        print(f'Not checked: {skipped} images in {excluded}.', file=sys.stderr)


if __name__ == '__main__':
    main()
