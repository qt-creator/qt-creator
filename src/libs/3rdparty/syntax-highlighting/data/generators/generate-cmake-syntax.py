#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Generate Kate syntax file for CMake
#
# SPDX-FileCopyrightText: 2017-2020 Alex Turbov <i.zaufi@gmail.com>
#
# To install prerequisites:
#
#   $ pip install --user click jinja2 pyyaml
#
# To use:
#
#   $ ./generate-cmake-syntax.py cmake.yaml > ../syntax/cmake.xml
#
import click
import jinja2
import re
import yaml

from lxml import etree


_TEMPLATED_NAME = re.compile('<[^>]+>')
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
_CONTROL_FLOW_LIST = set((
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
))


def try_transform_placeholder_string_to_regex(name):
    '''
        NOTE Some placeholders are not IDs, but numbers...
            `CMAKE_MATCH_<N>` 4 example
    '''
    m = _TEMPLATED_NAME.split(name)
    if 'CMAKE_MATCH_' in m:
        return 'CMAKE_MATCH_[0-9]+'

    if 'CMAKE_ARGV' in m:
        return 'CMAKE_ARGV[0-9]+'

    if 'CMAKE_POLICY_DEFAULT_CMP' in m:
        return 'CMAKE_POLICY_DEFAULT_CMP[0-9]{4}'

    if 'CMAKE_POLICY_WARNING_CMP' in m:
        return 'CMAKE_POLICY_WARNING_CMP[0-9]{4}'

    if 'ARGV' in m:
        return 'ARGV[0-9]+'

    return '&var_ref_re;'.join(m) if 1 < len(m) else name


def try_placeholders_to_regex(names):
    if not names:
        return None
    l = map(try_transform_placeholder_string_to_regex, names)
    l = sorted(l, reverse=True)
    return '\\b(?:' + '|'.join(l) + ')\\b'


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
        cmd['has_target_names_after_kw'] = cmd['has-target-names-after-kw']
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


def remove_duplicate_list_nodes(contexts, highlighting):
    remap = {}

    items_by_kws = {}
    # extract duplicate keyword list
    for items in highlighting:
        if items.tag != 'list':
            break
        k = '<'.join(item.text for item in items)
        name = items.attrib['name']
        rename = items_by_kws.get(k)
        if rename:
            remap[name] = rename
            highlighting.remove(items)
        else:
            items_by_kws[k] = name

    # update keyword list name referenced by each rule
    for context in contexts:
        for rule in context:
            if rule.tag == 'keyword':
                name = rule.attrib['String']
                rule.attrib['String'] = remap.get(name, name)


def remove_duplicate_context_nodes(contexts):
    # 3 levels: ctx, ctx_op and ctx_op_nested
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
            for rule in context:
                ref = remap.get(rule.attrib.get('context'))
                if ref:
                    rule.attrib['context'] = ref


def remove_duplicate_nodes(xml_string):
    parser = etree.XMLParser(resolve_entities=False, collect_ids=False)
    root = etree.fromstring(xml_string.encode(), parser=parser)
    highlighting = root[0]

    contexts = highlighting.find('contexts')

    remove_duplicate_list_nodes(contexts, highlighting)
    remove_duplicate_context_nodes(contexts)

    # reformat comments
    xml = etree.tostring(root)
    xml = re.sub(b'(?=[^\n ])<!--', b'\n<!--', xml)
    xml = re.sub(b'-->(?=[^ \n])', b'-->\n', xml)

    # extract DOCTYPE removed by etree.fromstring and reformat <language>
    doctype = xml_string[:xml_string.find('<highlighting')]

    # remove unformatted <language>
    xml = xml[xml.find(b'<highlighting'):]

    # last comment removed by etree.fromstring
    last_comment = '\n<!-- kate: replace-tabs on; indent-width 2; tab-width 2; -->'

    return f'{doctype}{xml.decode()}{last_comment}'


#BEGIN Jinja filters

def cmd_is_nulary(cmd):
    return cmd.setdefault('nulary?', False)

#END Jinja filters


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
    data['generator_expressions'] = data['generator-expressions']
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
