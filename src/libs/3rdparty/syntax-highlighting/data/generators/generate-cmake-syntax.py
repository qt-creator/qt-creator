#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2017-2025 Alex Turbov <i.zaufi@gmail.com>
#
# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "click",
#   "jinja2",
#   "lxml",
#   "pyyaml"
# ]
# ///
#
# Generate Kate syntax file for CMake.
#
# Run with [uv] or [pipx]:
#
#   $ uv run generate-cmake-syntax.py cmake.yaml > ../syntax/cmake.xml
#
# or install dependencies manually and run "normally".
#
# [uv]: https://docs.astral.sh/uv
# [pipx]: https://pipx.pypa.io/stable
#

from __future__ import annotations

import functools
import re
from dataclasses import dataclass, field

import click
import jinja2
import yaml
from lxml import etree


_TEMPLATED_NAME = re.compile(r'(?:<[^>]+>)')
_PROPERTY_KEYS = [
    'global-properties'
  , 'directory-properties'
  , 'target-properties'
  , 'source-properties'
  , 'test-properties'
  , 'cache-properties'
  , 'install-properties'
  ]
_KW_RE_LIST = ['kw', 're']
_VAR_KIND_LIST = ['variables', 'deprecated-or-internal-variables', 'environment-variables']
_CONTROL_FLOW_LIST = {
    'break'
  , 'continue'
  , 'elseif'
  , 'else'
  , 'endforeach'
  , 'endif'
  , 'endwhile'
  , 'foreach'
  , 'if'
  , 'return'
  , 'while'
  }
_VAR_REF_ENTITY = '&var_ref_re;'

_HEURISTICS = [
    (
        {'MAX(_(COUNT|MAJOR|MINOR|PATCH|TWEAK))?', 'MIN(_(COUNT|MAJOR|MINOR|PATCH|TWEAK))?'}
      , 'M(AX|IN)(_(COUNT|MAJOR|MINOR|PATCH|TWEAK))?'
    )
  , ({'OUTPUTS', 'OUTPUT_(HEADER|SOURCE)'}, 'OUTPUT(S|_(HEADER|SOURCE))')
  , ({'PREFIX', 'SUFFIX'}, '(PRE|SUF)FIX')
  , ({'CPPCHECK', 'CPPLINT'}, 'CPP(CHECK|LINT)')
  , ({'DEPENDS', 'PREDEPENDS'}, '(PRE)?DEPENDS')
  , ({'ICON', 'ICONURL'}, 'ICON(URL)?')
  , (
        {
            '&var%ref%re;(_INIT)?'
          , 'DEBUG(_INIT)?'
          , 'MINSIZEREL(_INIT)?'
          , 'RELEASE(_INIT)?'
          , 'RELWITHDEBINFO(_INIT)?'
          }
      , '(DEBUG|MINSIZEREL|REL(EASE|WITHDEBINFO)|&var%ref%re;)(_INIT)?'
    )
  , ({'RELEASE', 'RELWITHDEBINFO'}, 'REL(EASE|WITHDEBINFO)')
  , ({'POST', 'POSTUN', 'PRE', 'PREUN'}, 'P(RE|OST)(UN)?')
  , ({'AUTOPROV', 'AUTOREQ', 'AUTOREQPROV'}, 'AUTO(PROV|REQ(PROV)?)')
  , ({'DEFINITIONS', 'OPTIONS'}, '(DEFINI|OP)TIONS')
  , ({'LIB_NAMES', 'LIBRARY'}, 'LIB(_NAMES|RARY)')
  , ({'EXTENSIONS', 'EXTRA_FLAGS'}, 'EXT(ENSIONS|RA_FLAGS)')
  , ({'DISABLED', 'DISPLAY_NAME'}, 'DIS(ABLED|PLAY_NAME)')
  , ({'LIBRARIES', 'LINK_LIBRARIES', 'STATIC_LINK_LIBRARIES'}, '((STATIC_)?LINK_)?LIBRARIES')
  , ({'INCLUDE_DIRS', 'LIBRARY_DIRS'}, '(INCLUDE|LIBRARY)_DIRS')
  , ({'BINARY_DIR', 'SOURCE_DIR'}, '(BINARY|SOURCE)_DIR')
  , ({'CFLAGS(_OTHER)?', 'LDFLAGS(_OTHER)?'}, '(C|LD)FLAGS(_OTHER)?')
  , ({'INCLUDE_DIRECTORIES', 'LIBRARIES'}, '(INCLUDE_DIRECTO|LIBRA)RIES')
  , ({'POSTFLIGHT_&var%ref%re;_SCRIPT', 'PREFLIGHT_&var%ref%re;_SCRIPT'}, 'P(RE|OST)FLIGHT_&var%ref%re;_SCRIPT')
  , ({'DIRECTORIES', 'FRAMEWORK_DIRECTORIES'}, '(FRAMEWORK_)?DIRECTORIES')
  , ({'FILE_FLAG', 'FILE'}, 'FILE(_FLAG)?')
  , ({'DIR_PERMISSIONS', 'FILE_PERMISSIONS'}, '(DIR|FILE)_PERMISSIONS')
  , ({'COMPILER_LAUNCHER', 'LINKER_LAUNCHER'}, '(COMPIL|LINK)ER_LAUNCHER')
  , ({'COMPILER', 'COMPILE_(DEFINI|OP)TIONS'}, 'COMPILE(R|_(DEFINI|OP)TIONS)')
  , ({'LICENSEURL', 'LICENSE_(EXPRESSION|FILE_NAME)'}, 'LICENSE(URL|_(EXPRESSION|FILE_NAME))')
  , ({'NO_SONAME', 'SONAME'}, '(NO_)?SONAME')
  , ({'CODE_SIGN_ON_COPY', 'REMOVE_HEADERS_ON_COPY'}, '(CODE_SIGN|REMOVE_HEADERS)_ON_COPY')
  , ({'(REFERENCE|REFERENCEPROP_&var%ref%re;_TAG)_&var%ref%re;'}, 'REFERENCE(PROP_&var%ref%re;_TAG)?_&var%ref%re;')
  , ({'DISABLE_FIND_PACKAGE', 'REQUIRE_FIND_PACKAGE'}, '(DISABLE|REQUIRE)_FIND_PACKAGE')
  , (
        {'GROUP_USING_&var%ref%re;(_SUPPORTED)?', 'LIBRARY_USING_&var%ref%re;(_SUPPORTED)?'}
      , '(GROUP|LIBRARY)_USING_&var%ref%re;(_SUPPORTED)?'
    )
  , (
        {
            'EXE_LINKER_FLAGS_&var%ref%re;(_INIT)?'
          , 'MODULE_LINKER_FLAGS_&var%ref%re;(_INIT)?'
          , 'SHARED_LINKER_FLAGS_&var%ref%re;(_INIT)?'
          , 'STATIC_LINKER_FLAGS_&var%ref%re;(_INIT)?'
        }
      , '(EXE|MODULE|SHARED|STATIC)_LINKER_FLAGS_&var%ref%re;(_INIT)?'
    )
  , (
        {
            'ARCHIVE_OUTPUT_DIRECTORY'
          , 'COMPILE_PDB_OUTPUT_DIRECTORY'
          , 'LIBRARY_OUTPUT_DIRECTORY'
          , 'PDB_OUTPUT_DIRECTORY'
          , 'RUNTIME_OUTPUT_DIRECTORY'
        }
      , '(ARCHIVE|(COMPILE_)?PDB|LIBRARY|RUNTIME)_OUTPUT_DIRECTORY'
    )
  , (
        {
            'ARCHIVE_OUTPUT_(DIRECTORY|NAME)'
          , 'LIBRARY_OUTPUT_(DIRECTORY|NAME)'
          , 'RUNTIME_OUTPUT_(DIRECTORY|NAME)'
        }
      , '(ARCHIVE|LIBRARY|RUNTIME)_OUTPUT_(DIRECTORY|NAME)'
    )
  , ({'ASM&var_ref_re;', 'ASM&var_ref_re;FLAGS'}, 'ASM&var_ref_re;(FLAGS)?')
  , (
        {
            'CMAKE_POLICY_DEFAULT_CMP[0-9]{4}'
          , 'CMAKE_POLICY_WARNING_CMP[0-9]{4}'
          }
      , 'CMAKE_POLICY_(DEFAULT|WARNING)_CMP[0-9]{4}'
      )
  , ({'CMAKE_ARGV[0-9]+', 'CMAKE_MATCH_[0-9]+'}, 'CMAKE_(ARGV|MATCH_)[0-9]+')
 ]

@dataclass
class RePartNode:
    children: dict[str, RePartNode] = field(default_factory=dict, hash=False)
    is_leaf: bool = False


@dataclass
class RegexCollection:
    special_cases: list[str] = field(default_factory=list, hash=False)
    re_tree: dict[str, RePartNode] = field(default_factory=dict, hash=False)

    def add_case(self, regex: str) -> RegexCollection:
        self.special_cases.append(regex)
        return self

    def update_tree(self, name_parts: list[str]) -> RegexCollection:
        safe_var_ref = _VAR_REF_ENTITY.replace('_', '%')
        functools.reduce(
            lambda current, part: (
                self.re_tree if current is None else current.children
              ).setdefault(part, RePartNode())
          , (
                safe_var_ref
                  .join(name_parts)
                  .replace(f'{safe_var_ref}_{safe_var_ref}', safe_var_ref)
                  .split('_')
              )
          , None
          ).is_leaf = True
        return self


def try_transform_placeholder_string_to_regex(state: RegexCollection, name: str):
    '''
        NOTE Some placeholders are not IDs, but numbers...
            `CMAKE_MATCH_<N>` 4 example
    '''
    name_parts = _TEMPLATED_NAME.split(name)
    match name_parts:
        case ['CMAKE_MATCH_' as head, ''] | ['CMAKE_ARGV' as head, ''] | ['ARGV' as head, '']:
            return state.add_case(head + '[0-9]+')

        case ['CMAKE_POLICY_DEFAULT_CMP' as head, ''] | ['CMAKE_POLICY_WARNING_CMP' as head, '']:
            return state.add_case(head + '[0-9]{4}')

        case ['', '__TRYRUN_OUTPUT']:
            return state.add_case(f'{_VAR_REF_ENTITY}__TRYRUN_OUTPUT')

        case (['ASM', ''] | ['ASM', 'FLAGS']) as asm_env:
            return state.add_case(f'{asm_env[0]}{_VAR_REF_ENTITY}{asm_env[1]}')

    return state.update_tree(name_parts)


def is_first_subset_of_second(first, second):
    subset = set(first)
    fullset = set(second)
    return subset.issubset(fullset)


def try_optimize_known_alt_groups(groups: list[str]) -> list[str]:
    for case in _HEURISTICS:
        if is_first_subset_of_second(case[0], groups):
            groups = sorted([*filter(lambda item: item not in case[0], groups), case[1]])
    return groups


def try_optimize_trailing_var_ref_regex(groups: list[str]) -> list[str]:
    tail_var_ref_re = '_' + _VAR_REF_ENTITY.replace('_', '%')
    candidates = [*filter(lambda s: s.endswith(tail_var_ref_re), groups)]
    return sorted([
        *filter(lambda item: item not in candidates, groups)
      , f'({"|".join(try_optimize_known_alt_groups([s[:-len(tail_var_ref_re)] for s in candidates]))}){tail_var_ref_re}'
      ]) if len(candidates) > 1 else groups


def build_regex(state: list[str], kv: tuple[str, RePartNode]) -> list[str]:
    name, value = kv
    match (value, len(value.children)):
        case (RePartNode(children={}, is_leaf=True), 0):
            return [*state, name]

        case (node, sz) if sz > 0:
            alt_group = try_optimize_known_alt_groups(
                try_optimize_trailing_var_ref_regex(
                    functools.reduce(build_regex, node.children.items(), [])
                  )
              )

            match (len(alt_group), node.is_leaf):
                case (1, False):
                    return [*state, f'{name}_{alt_group[0]}']

                case (1, True):
                    return [*state, f'{name}(_{alt_group[0]})?']

                case (sz, False) if sz > 0:
                    return [*state, f'{name}_({"|".join(alt_group)})']

                case (sz, True) if sz > 0:
                    return [*state, f'{name}(_({"|".join(alt_group)}))?']

                case _:
                    raise AssertionError('Zero children?')

        case _:
            raise AssertionError(f'NOT MATCHED: {name=}→{value=}')

    return state


def try_placeholders_to_regex(names):
    if not names:
        return None

    data = functools.reduce(
        try_transform_placeholder_string_to_regex
      , names
      , RegexCollection()
      )

    return (
        '\\b(?:'
      + '|'.join(
            try_optimize_known_alt_groups(
                try_optimize_trailing_var_ref_regex(
                    functools.reduce(
                        build_regex
                      , data.re_tree.items()
                      , data.special_cases
                      )
                  )
              )
          ).replace('%', '_')
      + ')\\b'
      )


def partition_iterable(fn, iterable):
    true, false = [], []
    for i in iterable:
        (false, true)[int(fn(i))].append(i)
    return true, false


def _transform_command_set(cmd, list_name):
    args, args_re = partition_iterable(lambda x: _TEMPLATED_NAME.search(x) is None, cmd[list_name])
    del cmd[list_name]
    list_name = list_name.replace('-', '_')

    cmd[list_name] = {k: sorted(set(v)) for k, v in zip(_KW_RE_LIST, [args, args_re])}
    cmd[list_name]['re'] = try_placeholders_to_regex(args_re)

    return cmd


def transform_command(cmd):
    can_be_nulary = True

    if 'name' not in cmd:
        raise RuntimeError('Command have no name')

    if 'named-args' in cmd:
        new_cmd = _transform_command_set(cmd, 'named-args')
        assert new_cmd == cmd
        can_be_nulary = False

    if 'special-args' in cmd:
        new_cmd = _transform_command_set(cmd, 'special-args')
        assert new_cmd == cmd
        can_be_nulary = False

    if 'property-args' in cmd:
        new_cmd = _transform_command_set(cmd, 'property-args')
        assert new_cmd == cmd
        can_be_nulary = False

    cmd['nested_parentheses'] = cmd.get('nested-parentheses?', False)

    if 'first-arg-is-target?' in cmd:
        cmd['first_arg_is_target'] = cmd['first-arg-is-target?']
        can_be_nulary = False

    if 'first-args-are-targets?' in cmd:
        cmd['first_args_are_targets'] = cmd['first-args-are-targets?']
        can_be_nulary = False

    if 'has-target-name-after-kw' in cmd:
        cmd['has_target_name_after_kw'] = cmd['has-target-name-after-kw']
        can_be_nulary = False

    if 'has-target-names-after-kw' in cmd:
        match cmd['has-target-names-after-kw']:
            case str():
                cmd['has_target_names_after_kw'] = [cmd['has-target-names-after-kw']]
            case list():
                cmd['has_target_names_after_kw'] = cmd['has-target-names-after-kw']
            case _:
                raise TypeError('Unexpected type for `has-target-names-after-kw`')
        can_be_nulary = False

    if 'second-arg-is-target?' in cmd:
        cmd['second_arg_is_target'] = cmd['second-arg-is-target?']
        can_be_nulary = False

    if 'nulary?' in cmd and cmd['nulary?'] and not can_be_nulary:
        raise RuntimeError('Command `{}` w/ args declared nulary!?'.format(cmd['name']))

    if 'start-region' in cmd:
        cmd['start_region'] = cmd['start-region']

    if 'end-region' in cmd:
        cmd['end_region'] = cmd['end-region']

    cmd['attribute'] = 'Control Flow' if cmd['name'] in _CONTROL_FLOW_LIST else 'Command'

    return cmd


def remove_duplicate_list_nodes(root):
    remap = {}
    items_by_kws = {}

    # extract duplicate keyword list
    for items in root.iterfind('highlighting/list'):
        key = '<'.join(item.text for item in items)
        name = items.attrib['name']
        if rename := items_by_kws.get(key):
            remap[name] = rename
            items.getparent().remove(items)
        else:
            items_by_kws[key] = name

    # update keyword list name referenced by each rule
    for rule in root.iterfind('highlighting/contexts/context/keyword'):
        name = rule.attrib['String']
        rule.attrib['String'] = remap.get(name, name)


def remove_duplicate_context_nodes(root):
    contexts = root[0].find('contexts')
    # 3 levels: ctx, ctx_op and ctx_op_nested
    # TODO Refactor it!
    for _ in range(3):
        remap = {}
        duplicated = {}

        # remove duplicate nodes
        for context in contexts:
            name = context.attrib['name']
            context.attrib['name'] = 'dummy'
            ref = duplicated.setdefault(etree.tostring(context), [])
            if ref:
                contexts.remove(context)
            else:
                context.attrib['name'] = name
                ref.append(name)
            remap[name] = ref[0]

        # update context name referenced by each rule
        for context in contexts:
            ref = remap.get(context.attrib.get('fallthroughContext'))
            if ref:
                context.attrib['fallthroughContext'] = ref
            for rule in context:
                ref = remap.get(rule.attrib.get('context'))
                if ref:
                    rule.attrib['context'] = ref


def remove_duplicate_nodes(raw_xml: str) -> str:
    parser = etree.XMLParser(resolve_entities=False, collect_ids=False)
    root = etree.fromstring(raw_xml.encode(), parser=parser)

    remove_duplicate_list_nodes(root)
    remove_duplicate_context_nodes(root)

    # reformat comments
    xml = etree.tostring(root)
    xml = re.sub(b'(?=[^\n ])<!--', b'\n<!--', xml)
    xml = re.sub(b'-->(?=[^ \n])', b'-->\n', xml)

    # extract DOCTYPE removed by etree.fromstring and reformat <language>
    doctype = raw_xml[:raw_xml.find('<highlighting')]

    # remove unformatted <language>
    xml = xml[xml.find(b'<highlighting'):]

    # last comment removed by etree.fromstring
    last_comment = '\n<!-- kate: replace-tabs on; indent-width 2; tab-width 2; -->'

    return f'{doctype}{xml.decode()}{last_comment}'


# BEGIN Jinja filters

def cmd_is_nulary(cmd):
    return cmd.setdefault('nulary?', False)

# END Jinja filters


@click.command()
@click.argument('input_yaml', type=click.File('r'))
@click.argument('template', type=click.File('r'), default='./cmake.xml.tpl')
def cli(input_yaml, template):
    data = yaml.load(input_yaml, Loader=yaml.BaseLoader)

    # Partition `variables` and `environment-variables` lists into "pure" (key)words and regexes to match
    for var_key in _VAR_KIND_LIST:
        data[var_key] = {
            k: sorted(set(v)) for k, v in zip(
                _KW_RE_LIST
              , [*partition_iterable(lambda x: _TEMPLATED_NAME.search(x) is None, data[var_key])]
              )
        }
        data[var_key]['re'] = try_placeholders_to_regex(data[var_key]['re'])

    # Transform properties and make all-properties list
    data['properties'] = {}
    for prop in _PROPERTY_KEYS:
        python_prop_list_name = prop.replace('-', '_')
        props, props_re = partition_iterable(lambda x: _TEMPLATED_NAME.search(x) is None, data[prop])
        del data[prop]

        data['properties'][python_prop_list_name] = {
            k: sorted(set(v)) for k, v in zip(_KW_RE_LIST, [props, props_re])
          }
        data['properties'][python_prop_list_name]['re'] = try_placeholders_to_regex(props_re)

    data['properties']['kinds'] = list(map(lambda name: name.replace('-', '_'), _PROPERTY_KEYS))

    # Make all commands list
    data['commands'] = list(
        map(
            transform_command
          , data['scripting-commands'] + data['project-commands'] + data['ctest-commands']
          )
      )
    data['standard_module_commands'] = list(
        map(
            transform_command
          , data['standard-module-commands']
          )
      )
    del data['standard-module-commands']

    # Fix node names to be accessible from Jinja template
    data['generator_expressions'] = (ex for ex in data['generator-expressions'] if isinstance(ex, str))
    data['complex_generator_expressions'] = [ex for ex in data['generator-expressions'] if not isinstance(ex, str)]
    data['deprecated_or_internal_variables'] = data['deprecated-or-internal-variables']
    data['environment_variables'] = data['environment-variables']
    del data['generator-expressions']
    del data['deprecated-or-internal-variables']
    del data['environment-variables']

    env = jinja2.Environment(
        keep_trailing_newline=True
      )
    env.block_start_string = '<!--['
    env.block_end_string = ']-->'
    env.variable_start_string = '<!--{'
    env.variable_end_string = '}-->'
    env.comment_start_string = '<!--#'
    env.comment_end_string = '#-->'

    # Register convenience filters
    env.tests['nulary'] = cmd_is_nulary

    tpl = env.from_string(template.read())
    result = tpl.render(data)
    result = remove_duplicate_nodes(result)

    print(result)


if __name__ == '__main__':
    cli()
    # TODO Handle execptions and show errors
