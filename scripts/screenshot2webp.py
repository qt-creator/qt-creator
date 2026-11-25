#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

r"""Convert a screenshot into an optimized WebP image for the documentation.

Task: add or update a screenshot in the documentation

Screenshots are in doc/qtcreator/images, doc/qtcreatordev/images and
their subfolders. Add or update them only as WebP images that this
script creates. This does not apply to the icons in
doc/qtcreator/images/icons.

Tool

  The script encodes SOURCE once with each preset in PRESETS and
  measures how much each result differs from SOURCE, with the DSSIM
  metric of ImageMagick: 0 means identical. It keeps the result with
  the lowest cost: the file size, doubled for each DSSIM_DOUBLING_STEP
  of difference. TARGET is a .webp file or a directory. It defaults to
  SOURCE with the suffix .webp. The script prints the size, the
  difference and the cost for each preset, and the preset it kept. It
  exits with 3 if a requirement listed at the end of --help is not met;
  then stop and ask the user to provide what is missing. If it fails
  otherwise, fix the script rather than converting by hand.

Procedure

  1. Run the script on the original capture, not on a WebP image that
     the script created earlier: lossy presets lose quality with each
     pass. Name TARGET like the other images in its folder.
  2. Reference the image in the .qdoc file with
     \image <name>.webp {<alt text>}.
  3. If the WebP image replaces an image in another format, remove that
     image with "git rm" and grep doc/ for its file name to update all
     references to it.
  4. Do not commit the original capture. Commit rules are in the
     top-level AGENTS.md.
"""

import argparse
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import NamedTuple

SCRIPT = Path(__file__).resolve()
SOURCE_DIR = SCRIPT.parents[1]
REQUIREMENTS_NOT_MET = 3  # argparse exits with 2 on usage errors.

COMMON_OPTIONS = ['-strip']
# Method 6 is the most thorough of the libwebp encoding methods 0 to 6
# (default: 4). It makes lossy files smaller, but not lossless ones.
LOSSY_OPTIONS = ['-define', 'webp:lossless=false', '-define', 'webp:method=6']
PRESETS = (
    ('lossless', ['-define', 'webp:lossless=true']),
    ('reduced colors', ['-colors', '255', '-define', 'webp:lossless=true']),
    ('reduced colors, no dither',
     ['+dither', '-colors', '255', '-define', 'webp:lossless=true']),
    # Below quality 50, the cost often drops further, but text and thin
    # lines visibly blur. Above quality 75, the cost only grows.
    ('lossy, quality 50', ['-quality', '50'] + LOSSY_OPTIONS),
    ('lossy, quality 60', ['-quality', '60'] + LOSSY_OPTIONS),
    ('lossy, quality 75', ['-quality', '75'] + LOSSY_OPTIONS),
)
DSSIM_DOUBLING_STEP = 0.02


class Candidate(NamedTuple):
    preset: str
    path: Path
    size: int
    difference: float


def check_requirements(magick):
    """Returns the problems that prevent the conversion."""
    if not magick:
        return ['ImageMagick 7, with "magick" in PATH']
    try:
        formats, metrics = (subprocess.run([magick, '-list', kind], check=True,
                                           capture_output=True, text=True).stdout
                            for kind in ('format', 'metric'))
    except (OSError, subprocess.CalledProcessError) as e:
        return [f'working ImageMagick: {magick} failed: {e}']
    problems = []
    # Builds with module support list a Module column before the Mode column.
    m = re.search(r'^\s*WEBP\*?\s+(?:\S+\s+)?([r-][w-][+-])\s', formats, re.MULTILINE)
    if not m or 'w' not in m[1]:
        problems.append(f'WebP support in ImageMagick ({magick})')
    if 'DSSIM' not in metrics.split():
        problems.append(f'the DSSIM metric in ImageMagick ({magick})')
    return problems


def print_requirements(problems):
    if not problems:
        print('Requirements: all met.')
        return
    print('Requirements: not met. Stop and ask the user to provide:')
    for problem in problems:
        print('  ' + problem)


def run(args):
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f'{" ".join(args)} failed:\n{result.stderr}')
    return result.stdout


def encode(magick, source, options, target):
    run([magick, str(source)] + COMMON_OPTIONS + options + [str(target)])
    return target.stat().st_size


def measure_difference(magick, source, encoded):
    """Returns the DSSIM of encoded to source, both shown on a white background.

    Lossless WebP changes the color of fully transparent pixels. That is
    invisible and must not count as a difference."""
    args = [magick, str(source), str(encoded), '-alpha', 'remove', '-metric', 'DSSIM',
            '-compare', '-format', '%[distortion]', 'info:']
    output = run(args)
    try:
        return float(output)
    except ValueError:
        sys.exit(f'{" ".join(args)} printed no number:\n{output}')


def cost(candidate):
    """Returns the file size, doubled for each DSSIM_DOUBLING_STEP of difference.

    A much worse image needs a much smaller file to win."""
    return candidate.size * 2 ** (candidate.difference / DSSIM_DOUBLING_STEP)


def choose_winner(candidates):
    """Returns the candidate with the lowest cost, the first one on a tie."""
    return min(candidates, key=cost)


def main():
    summary, _, instructions = __doc__.partition('\n\n')
    parser = argparse.ArgumentParser(
        prog=f'python -I {SCRIPT.relative_to(SOURCE_DIR).as_posix()}',
        description=summary, epilog=instructions, add_help=False,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-h', '--help', action='store_true',
                        help='show the task instructions and check the requirements')
    parser.add_argument('source', metavar='SOURCE', type=Path, nargs='?',
                        help='the original capture')
    parser.add_argument('target', metavar='TARGET', type=Path, nargs='?',
                        help='the .webp file to write, or its directory')
    args = parser.parse_args()

    magick = shutil.which('magick')
    problems = check_requirements(magick)
    if args.help:
        parser.print_help()
        print()
        print_requirements(problems)
        return REQUIREMENTS_NOT_MET if problems else 0
    if problems:
        print_requirements(problems)
        return REQUIREMENTS_NOT_MET

    source = args.source
    if not source:
        parser.error('SOURCE is required')
    if not source.is_file():
        parser.error(f'SOURCE is not a file: {source}')
    target = args.target or source.with_suffix('.webp')
    if target.is_dir():
        target = target / (source.stem + '.webp')
    if target.suffix.lower() != '.webp':
        parser.error(f'TARGET is neither a .webp file nor a directory: {target}')
    if target.resolve() == source.resolve():
        parser.error('TARGET is SOURCE. Convert the original capture instead.')

    with tempfile.TemporaryDirectory() as temp:
        candidates = []
        print(f'{"bytes":>10}  {"DSSIM":>10}  {"cost":>10}  preset')
        for index, (preset, options) in enumerate(PRESETS):
            path = Path(temp) / f'{index}.webp'
            size = encode(magick, source, options, path)
            candidate = Candidate(preset, path, size, measure_difference(magick, source, path))
            print(f'{candidate.size:>10}  {candidate.difference:>10.6f}  '
                  f'{cost(candidate):>10.0f}  {candidate.preset}')
            candidates.append(candidate)
        winner = choose_winner(candidates)
        shutil.copyfile(winner.path, target)
    print(f'Wrote {target} with preset "{winner.preset}": {winner.size} bytes, '
          f'source {source.stat().st_size} bytes.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
