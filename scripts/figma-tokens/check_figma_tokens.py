#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Compare exported Figma variables with the Qt Creator theme files.

Task: verify theme files against Figma token exports

The exported Figma variables are JSON files. They sit next to this
script, are ignored by git and must not be committed. Each maps to a
theme file in share/qtcreator/themes/:

  Export                Theme file
  Colours.tokens.json   primitive-colors.inc
  Dark.tokens.json      dark.figmatokens
  Light.tokens.json     light.figmatokens

Report differences; edit the theme files only once the user confirms.

Tool

  The script lists "differs", "theme only" and "figma only" entries per
  file, and exits with 1 on "differs" or "theme only". It exits with 3
  if a requirement listed at the end of --help is not met; then stop
  and ask the user to provide what is missing. If it fails otherwise or
  the export format changed, fix the script rather than comparing by
  hand, following the rules below.

Mapping rules

  - A token is a leaf object with "$value"; groups nest. A colour
    "$value" is { "hex": "#RRGGBB", "alpha": <0..1>, ... }.
  - Only keys in the [Colors] section of a theme file count.
  - Primitive <family>/<shade> maps to
    Primitive-<Family>-<shade>=AARRGGBB.
  - A semantic token path maps to Token_<Segment>_<Segment>..., each
    segment split on "-" and space with each word capitalized, after
    the renames in SEGMENT_RENAMES. Its value is the primitive name of
    its alias, $extensions["com.figma.aliasData"].targetVariableName
    (e.g. neon/600), not the resolved hex.

Expected "figma only" entries

  These need new Theme::Token_* roles in code (out of scope); mention
  them briefly, not as errors:
  - Token_Basic_Inverted
  - Token_Additional_*, Token_Effects_*, Token_Editor_*
  - primitive families not yet in primitive-colors.inc (as of 2026-10:
    Lemon, Moss, Violet). Grep src/ and share/ before calling one
    unused.

Procedure

  1. Run the script.
  2. For each semantic "differs", check "git log -p" of the theme file:
     an earlier commit may have changed it on purpose, and Figma may
     have moved again since.
  3. Report primitive differences as a table (key, theme value, Figma
     value), semantic differences per mode, "figma only" entries
     briefly, and a proposed edit list.
  4. Once the user confirms, apply the edits and re-run the script
     until it exits with 0. Commit rules are in the top-level
     AGENTS.md.
"""

import argparse
import json
import re
import sys
from pathlib import Path

SCRIPT = Path(__file__).resolve()
SOURCE_DIR = SCRIPT.parents[2]
THEMES_DIR = SOURCE_DIR / 'share' / 'qtcreator' / 'themes'
REQUIREMENTS_NOT_MET = 3  # argparse exits with 2 on usage errors.

SEGMENT_RENAMES = {
    'Base': 'Basic',
    'Primary-accent': 'Accent',
    'Gradient 1': 'Gradient01',
    'Gradient 2': 'Gradient02',
    'info': 'Neutral',
    'stop': 'End',
}


def leaves(node, path=()):
    if isinstance(node, dict) and '$value' in node:
        yield path, node
        return
    for key, child in node.items():
        if not key.startswith('$'):
            yield from leaves(child, path + (key,))


def read_colors(path):
    result = {}
    section = None
    for line in path.read_text(encoding='utf-8').splitlines():
        if m := re.match(r'\s*\[(\w+)\]', line):
            section = m[1]
        elif section == 'Colors' and (m := re.match(r'\s*([\w-]+)=(\S+)', line)):
            result[m[1]] = m[2]
    return result


def primitive_name(alias):
    family, shade = alias.split('/')
    return f'Primitive-{family.capitalize()}-{shade}'


def argb(value):
    return f'{round(value.get("alpha", 1) * 255):02X}{value["hex"][1:].upper()}'


def primitive(path, token):
    return primitive_name('/'.join(path)), argb(token['$value'])


def semantic(path, token):
    words = []
    for segment in path:
        segment = SEGMENT_RENAMES.get(segment, segment)
        words += [w[:1].upper() + w[1:] for w in re.split(r'[- ]', segment)]
    alias = token.get('$extensions', {}).get('com.figma.aliasData', {})
    value = token['$value']
    if 'targetVariableName' in alias:
        value = primitive_name(alias['targetVariableName'])
    elif isinstance(value, dict):
        value = argb(value)
    return 'Token_' + '_'.join(words), str(value)


FILES = (('Colours.tokens.json', 'primitive-colors.inc', primitive),
         ('Dark.tokens.json', 'dark.figmatokens', semantic),
         ('Light.tokens.json', 'light.figmatokens', semantic))


def compare(title, figma, theme):
    """Prints differences, returns True if no theme token differs or is missing."""
    failures = [f'differs:     {k}: theme={theme[k]} figma={v}'
                for k, v in figma.items() if k in theme and v != theme[k]]
    failures += [f'theme only:  {k}={v}' for k, v in theme.items() if k not in figma]
    figma_only = [f'figma only:  {k}={v}' for k, v in figma.items() if k not in theme]
    print(f'### {title}')
    for line in failures + figma_only or ['in sync']:
        print('  ' + line)
    return not failures


def check_requirements(exports, themes):
    """Returns the problems that prevent the comparison."""
    problems = []
    for export, theme_file, _ in FILES:
        path = exports / export
        if not path.is_file():
            problems.append(f'missing export: {path}')
        else:
            try:
                json.loads(path.read_text(encoding='utf-8'))
            except (OSError, ValueError) as e:
                problems.append(f'unreadable export: {path}: {e}')
        if not (themes / theme_file).is_file():
            problems.append(f'missing theme file: {themes / theme_file}')
    return problems


def print_requirements(problems):
    if not problems:
        print('Requirements: all met.')
        return
    print('Requirements: not met. Stop and ask the user to provide:')
    for problem in problems:
        print('  ' + problem)


def main():
    summary, _, instructions = __doc__.partition('\n\n')
    parser = argparse.ArgumentParser(
        prog=f'python -I {SCRIPT.relative_to(SOURCE_DIR).as_posix()}',
        description=summary, epilog=instructions, add_help=False,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-h', '--help', action='store_true',
                        help='show the task instructions and check the requirements')
    parser.add_argument('--exports', type=Path, default=SCRIPT.parent,
                        help='directory of the *.tokens.json files (default: %(default)s)')
    parser.add_argument('--themes', type=Path, default=THEMES_DIR,
                        help='directory of the theme files (default: %(default)s)')
    args = parser.parse_args()

    problems = check_requirements(args.exports, args.themes)
    if args.help:
        parser.print_help()
        print()
        print_requirements(problems)
        return REQUIREMENTS_NOT_MET if problems else 0
    if problems:
        print_requirements(problems)
        return REQUIREMENTS_NOT_MET

    ok = True
    for export, theme_file, entry in FILES:
        tokens = json.loads((args.exports / export).read_text(encoding='utf-8'))
        figma = dict(entry(path, token) for path, token in leaves(tokens))
        ok &= compare(theme_file, figma, read_colors(args.themes / theme_file))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
