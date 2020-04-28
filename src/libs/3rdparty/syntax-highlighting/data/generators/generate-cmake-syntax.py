#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Generate Kate syntax file for CMake
#
# Copyright (c) 2017-2019 Alex Turbov <i.zaufi@gmail.com>
#
# To install prerequisites:
#
#   $ pip install --user click jinja2 yaml
#
# To use:
#
#   $ ./generate-cmake-syntax.py cmake.yaml > ../syntax/cmake.xml
#
import click
import jinja2
import pathlib
import re
import yaml

import pprint


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
_VAR_KIND_LIST = ['variables', 'environment-variables']


def try_transform_placeholder_string_to_regex(name):
    '''
        NOTE Some placeholders are not IDs, but numbers...
            `CMAKE_MATCH_<N>` 4 example
    '''
    m = _TEMPLATED_NAME.split(name)
    if 'CMAKE_MATCH_' in m:
        return '\\bCMAKE_MATCH_[0-9]+\\b'

    if 'CMAKE_ARGV' in m:
        return '\\bCMAKE_ARGV[0-9]+\\b'

    if 'CMAKE_POLICY_DEFAULT_CMP' in m:
        return '\\bCMAKE_POLICY_DEFAULT_CMP[0-9]{4}\\b'

    if 'CMAKE_POLICY_WARNING_CMP' in m:
        return '\\bCMAKE_POLICY_WARNING_CMP[0-9]{4}\\b'

    return '\\b{}\\b'.format('&id_re;'.join(list(m))) if 1 < len(m) else name


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
    cmd[list_name]['re'] = [*map(lambda x: try_transform_placeholder_string_to_regex(x), args_re)]

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

    cmd['nested_parentheses'] = cmd['nested-parentheses?'] if 'nested-parentheses?' in cmd else False

    if 'nulary?' in cmd and cmd['nulary?'] and not can_be_nulary:
        raise RuntimeError('Command `{}` w/ args declared nulary!?'.format(cmd['name']))

    return cmd


#BEGIN Jinja filters

def cmd_is_nulary(cmd):
    assert not ('named-args' in cmd or 'special-args' in cmd or 'property-args' in cmd)
    return 'nulary?' in cmd and cmd['nulary?']

#END Jinja filters


@click.command()
@click.argument('input_yaml', type=click.File('r'))
@click.argument('template', type=click.File('r'), default='./cmake.xml.tpl')
def cli(input_yaml, template):
    data = yaml.load(input_yaml)

    # Partition `variables` and `environment-variables` lists into "pure" (key)words and regexes to match
    for var_key in _VAR_KIND_LIST:
        data[var_key] = {
            k: sorted(set(v)) for k, v in zip(
                _KW_RE_LIST
              , [*partition_iterable(lambda x: _TEMPLATED_NAME.search(x) is None, data[var_key])]
              )
        }
        data[var_key]['re'] = [
            *map(
                lambda x: try_transform_placeholder_string_to_regex(x)
              , data[var_key]['re']
              )
          ]

    # Transform properties and make all-properties list
    data['properties'] = {}
    for prop in _PROPERTY_KEYS:
        python_prop_list_name = prop.replace('-', '_')
        props, props_re = partition_iterable(lambda x: _TEMPLATED_NAME.search(x) is None, data[prop])
        del data[prop]

        data['properties'][python_prop_list_name] = {
            k: sorted(set(v)) for k, v in zip(_KW_RE_LIST, [props, props_re])
          }
        data['properties'][python_prop_list_name]['re'] = [
            *map(lambda x: try_transform_placeholder_string_to_regex(x), props_re)
          ]

    data['properties']['kinds'] = [*map(lambda name: name.replace('-', '_'), _PROPERTY_KEYS)]

    # Make all commands list
    data['commands'] = [
        *map(
            lambda cmd: transform_command(cmd)
          , data['scripting-commands'] + data['project-commands'] + data['ctest-commands'])
      ]

    # Fix node names to be accessible from Jinja template
    data['generator_expressions'] = data['generator-expressions']
    data['environment_variables'] = data['environment-variables']
    del data['generator-expressions']
    del data['environment-variables']

    env = jinja2.Environment(
        keep_trailing_newline=True
      )

    # Register convenience filters
    env.tests['nulary'] = cmd_is_nulary

    tpl = env.from_string(template.read())
    result = tpl.render(data)
    print(result)


if __name__ == '__main__':
    cli()
    # TODO Handle execptions and show errors
