#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Generate SPDX-Comments syntax file
#
# SPDX-FileCopyrightText: 2020 Alex Turbov <i.zaufi@gmail.com>
# SPDX-License-Identifier: MIT
#
# To install prerequisites:
#
#   $ pip install --user click jinja2
#
# To use:
#
#   $ ./generate-spdx-syntax.py > ../syntax/spdx-comments.xml
#

import json
import pathlib
import urllib.request

import click
import jinja2


def get_json(url):
    with urllib.request.urlopen(url=url) as body:
        return json.load(body)


@click.command()
@click.argument('template', type=click.File('r'), default='./spdx-comments.xml.tpl')
def cli(template):

    data = {
        'licenses': [
            *filter(
                lambda l: not l['licenseId'].endswith('+')
              , get_json(url='https://spdx.org/licenses/licenses.json')['licenses']
              )
          ]
      , 'exceptions': [
            *filter(
                lambda l: not l['licenseExceptionId'].endswith('+')
              , get_json(url='https://spdx.org/licenses/exceptions.json')['exceptions']
              )
          ]
      }

    env = jinja2.Environment(
        keep_trailing_newline=True
      )
    env.block_start_string = '<!--['
    env.block_end_string = ']-->'
    env.variable_start_string = '<!--{'
    env.variable_end_string = '}-->'
    env.comment_start_string = '<!--#'
    env.comment_end_string = '#-->'

    tpl = env.from_string(template.read())
    result = tpl.render(data)
    print(result.strip(), end=None)


if __name__ == '__main__':
    cli()
    # TODO Handle execptions and show errors
