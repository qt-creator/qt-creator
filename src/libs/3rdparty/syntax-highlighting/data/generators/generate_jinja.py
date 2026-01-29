#!/usr/bin/env python3
# Copyright (C) 2025 The Qt Company Ltd.,
# SPDX-License-Identifier: MIT
"""Infuse Jinja into KDE Syntax Highlighting grammars.

Creates highlighting grammars which highlight the original language just like
the source grammars did but also adds Jinja highlighting on top. If other
language files are referenced in a source, they are recursively generated and
referenced as well.
"""

import argparse
import os
import sys
import time
import xml.etree.ElementTree as ET


def check_grammar_dirs(args):
    """Check and canonize all grammar search folders before use."""
    if args.grammar_dir is None:
        args.grammar_dir = []
    for i, gr_dir in enumerate(args.grammar_dir):
        if not os.path.isdir(gr_dir):
            print(f'Search folder {gr_dir} does not exist.')
            sys.exit(1)
        args.grammar_dir[i] = os.path.realpath(gr_dir)
    for grammar_xml in args.grammar_xmls:
        folder = os.path.dirname(os.path.realpath(grammar_xml))
        if folder not in args.grammar_dir:
            args.grammar_dir.append(folder)


def frontend():
    """Frontend CLI handling"""
    parser = argparse.ArgumentParser(
            description=__doc__,
            epilog='Copyright (C) 2025 The Qt Company Ltd., '
                   'author: zoltan.gera@qt.io')
    parser.add_argument('-d', '--dry-run', action='store_true',
                        help='do not generate grammars, just run through the '
                             'given sources and print the output filenames')
    parser.add_argument('-j', '--jinja-xml',
                        help='set the location of the Jinja grammar to be '
                             'used for infusion (default: folder of this '
                             'script is searched for jinja.xml)')
    parser.add_argument('-x', '--xtra-ext', action='append',
                        help='add one extra Jinja template extension to the '
                             'generated grammars besides .jinja, .j2, .jinja2 '
                             '(can be used multiple times, e.g. -x tpl)')
    parser.add_argument('-g', '--grammar-dir', action='append',
                        help='add an extra search folder from where to read '
                             'grammars referenced by the source grammars '
                             '(default: all folders of source grammars, can '
                             'be used multiple times)')
    parser.add_argument('-p', '--prefix',
                        help='set how the name of the generated XMLs should '
                             'start (default: jinja-)')
    parser.add_argument('-o', '--output-dir',
                        help='give the output folder for the generated '
                             'grammars (default: current folder)')
    parser.add_argument('grammar_xmls', nargs='+',
                        help='source grammar XMLs to be infused with Jinja')
    args = parser.parse_args()

    for grammar_xml in args.grammar_xmls:
        if not os.path.isfile(grammar_xml):
            print(f'Source grammar file {grammar_xml} does not exist.')
            sys.exit(1)
    if args.output_dir is None:
        args.output_dir = '.'
    args.output_dir = os.path.realpath(args.output_dir)
    if not os.path.isdir(args.output_dir):
        print('Destination folder does not exist.')
        sys.exit(1)
    if args.prefix is None:
        args.prefix = 'jinja-'
    check_grammar_dirs(args)
    if args.jinja_xml is None:
        args.jinja_xml = os.path.dirname(sys.argv[0]) if sys.argv[0] else '.'
        if args.jinja_xml == '':
            args.jinja_xml = '.'
        args.jinja_xml += '/jinja.xml'
    if not os.path.isfile(args.jinja_xml):
        print('Could not find jinja.xml grammar.')
        sys.exit(1)

    return args


def get_tree_language(tree):
    """Extract language identifier string from a grammar tree after roughly
    checking format, return None on failure."""
    root = tree.getroot()
    if root.tag != 'language':
        return None
    language = root.get('name')
    highlights = root.find('highlighting')
    if highlights is None:
        return None
    contexts = highlights.find('contexts')
    item_datas = highlights.find('itemDatas')
    if contexts is None or item_datas is None:
        return None
    return language


def get_grammars(grammar_dirs, src_xmls):
    """Parse all grammars and return language to tree and filename mappings.
    Also return an initial list of to-be-processed files."""
    grammars = {}
    xmls = {}
    to_visit = set()

    for grammar_dir in grammar_dirs:
        for file_name in os.listdir(grammar_dir):
            if not file_name.lower().endswith('.xml'):
                continue
            file_path = os.path.join(grammar_dir, file_name)
            try:
                tree = ET.parse(file_path)
            except ET.ParseError:
                continue
            language = get_tree_language(tree)
            if language is None:
                continue
            grammars[language] = tree
            xmls[language] = file_path
            for src_xml in src_xmls:
                if os.path.samefile(file_path, src_xml):
                    to_visit.add(language)
                    break

    return grammars, xmls, to_visit


def brand_header(lang_tag):
    """Insert branding and make sure it can be detected as a generated file.
    """
    branding = ET.Comment(
            'Generated by generate_jinja.py, the Jinja infusion script for '
            'KDE Syntax Highlighting')
    if lang_tag.text is not None and lang_tag.text.strip() == '':
        branding.tail = lang_tag.text
    lang_tag.insert(0, branding)
    lang_tag.set('generated', 'true')


def infuse_header(grammar, jinja_tag, xtra_ext):
    """Modify language descriptors to fit the created hybrid grammar."""
    lang_tag = grammar.getroot()
    lang_tag.set('name', jinja_tag.get('name') + '/' + lang_tag.get('name'))
    lang_tag.set('section', jinja_tag.get('section'))
    lang_tag.set('version', str(round(time.time())))
    lang_tag.set('license', jinja_tag.get('license'))
    lang_tag.set('priority', lang_tag.get('priority', '0'))
    lang_tag.set('kateversion', max(lang_tag.get('kateversion'),
                                    jinja_tag.get('kateversion')))

    if lang_tag.get('extensions'):
        exts = ''
        for ext in lang_tag.get('extensions').split(';'):
            if exts:
                exts += ';'
            exts += f'{ext}.jinja;{ext}.jinja2;{ext}.j2'
            if xtra_ext:
                for xext in xtra_ext:
                    exts += f';{ext}.{xext}'
        lang_tag.set('extensions', exts)

    if lang_tag.get('mimetype'):
        mimes = ''
        for mime in lang_tag.get('mimetype').split(';'):
            if mimes:
                mimes += ';'
            mimes += f'{mime}.jinja'
        lang_tag.set('mimetype', mimes)

    if lang_tag.get('alternativeNames'):
        altnames = ''
        for altname in lang_tag.get('alternativeNames').split(';'):
            if altnames:
                altnames += ';'
            altnames += jinja_tag.get('name') + '/' + altname
        lang_tag.set('alternativeNames', altnames)

    brand_header(lang_tag)


def remove_deprecated(grammar):
    """Remove deprecated things to bring grammar up to date."""
    for context_tag in grammar.findall('./highlighting/contexts/context'):
        if context_tag.get('fallthrough') is not None:
            del context_tag.attrib['fallthrough']


def jinjize_reference(ref, known, jinja_name, to_do):
    """Return the same reference but change to the jinjized version of the
    syntax if the referred language is known. In the latter case, add the
    language to the process queue as well."""
    if '##' not in ref:
        return ref
    ref_root, lang = ref.split('##', 1)
    if lang not in known:
        return ref
    to_do.add(lang)
    return ref_root + '##' + jinja_name + '/' + lang


def jinjize(grammar, jinja, known):
    """Change all known outside references to Jinja versions and add them to
    the todo list. Hook Jinja grammar into all contexts."""
    jinja_name = jinja.getroot().get('name')
    boot_context = jinja.find('./highlighting/contexts/context[1]').get('name')
    boot_context = boot_context + '##' + jinja_name
    to_do = set()

    for include_tag in grammar.findall('./highlighting/list/include'):
        if include_tag.text is not None:
            include_tag.text = jinjize_reference(include_tag.text, known,
                                                 jinja_name, to_do)

    for ctx_tag in grammar.findall('./highlighting/contexts/context'):
        val = ctx_tag.get('lineEndContext')
        if val is not None:
            ctx_tag.set('lineEndContext',
                        jinjize_reference(val, known, jinja_name, to_do))

        val = ctx_tag.get('lineEmptyContext')
        if val is not None:
            ctx_tag.set('lineEmptyContext',
                        jinjize_reference(val, known, jinja_name, to_do))

        val = ctx_tag.get('fallthroughContext')
        if val is not None:
            ctx_tag.set('fallthroughContext',
                        jinjize_reference(val, known, jinja_name, to_do))

        for rule_tag in ctx_tag.findall('./*'):
            if rule_tag.tag is None:
                continue
            val = rule_tag.get('context')
            if val is not None:
                rule_tag.set('context',
                             jinjize_reference(val, known, jinja_name, to_do))

        jincl_tag = ET.Element('IncludeRules', {'context': boot_context})
        if ctx_tag.text is not None and ctx_tag.text.strip() == '':
            jincl_tag.tail = ctx_tag.text
        ctx_tag.insert(0, jincl_tag)

    return to_do


def main():
    """Set up and process all explicit and referred sources."""
    args = frontend()
    grammars, xmls, to_do = get_grammars(args.grammar_dir, args.grammar_xmls)
    done = set()

    try:
        jinja = ET.parse(args.jinja_xml)
    except ET.ParseError:
        print('Cannot parse jinja.xml syntax file.')
        sys.exit(2)

    while to_do:
        lang = to_do.pop()
        out_file = os.path.join(args.output_dir,
                                args.prefix + os.path.basename(xmls[lang]))
        infuse_header(grammars[lang], jinja.getroot(), args.xtra_ext)
        remove_deprecated(grammars[lang])
        refd = jinjize(grammars[lang], jinja, grammars.keys())
        if not args.dry_run:
            grammars[lang].write(out_file, encoding='unicode')
        else:
            print(out_file)
        done.add(lang)
        refd -= done
        to_do |= refd

    sys.exit(0)


if __name__ == '__main__':
    main()
