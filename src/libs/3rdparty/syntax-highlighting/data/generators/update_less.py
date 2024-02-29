#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023 Jonathan Poelen <jonathan.poelen@gmail.com>
# SPDX-License-Identifier: MIT

from pathlib import Path
from urllib import request
import re
import sys


if len(sys.argv) < 1:
  print(f'{sys.argv[0]} syntax/less.xml', file=sys.stderr)
  exit(1)

#
# Extract functions
#

data = request.urlopen('https://lesscss.org/functions/').read().decode()

functions = re.findall(r'</a>([-_\w\d]+)</h3>', data, flags=re.DOTALL)
functions.append('%')

#
# Update syntax
#

sep = '\n            '
new_list = f"""<list name="functions">
            <include>functions##CSS</include>

            <!-- Less functions, @see http://lesscss.org/functions/ -->
            <item>{f'</item>{sep}<item>'.join(sorted(functions))}</item>
        </list>"""

less_filename = Path(sys.argv[1])
less_content = less_filename.read_text()
original_less_content = less_content
less_content = re.sub(r'<list name="functions">.*?</list>',
                      new_list, less_content, count=1, flags=re.DOTALL)

if original_less_content != less_content:
  less_content = re.sub(' version="(\d+)" ', lambda m: f' version="{int(m[1])+1}" ',
                        less_content, count=1)
  less_filename.write_text(less_content)
