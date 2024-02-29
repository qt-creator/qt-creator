#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023 Jonathan Poelen <jonathan.poelen@gmail.com>
# SPDX-License-Identifier: MIT

from pathlib import Path
from collections import defaultdict
from typing import TextIO
import re
import sys


exclude_line = {
  '  - non-standard\n',
  '  - experimental\n',
  '  - deprecated\n',
  'page-type: css-combinator\n',
  'page-type: css-selector\n',
  'page-type: css-module\n',
  'page-type: landing-page\n',
  'page-type: guide\n',
}

page_type_accepted = {
  'page-type: css-type\n',
  'page-type: css-function\n',
  'page-type: css-property\n',
  'page-type: css-keyword\n',
  'page-type: css-shorthand-property\n',
  'page-type: css-pseudo-element\n',
  'page-type: css-pseudo-class\n',
  'page-type: css-at-rule-descriptor\n',
  'page-type: css-at-rule\n',
  'page-type: css-media-feature\n',
  'page-type: svg-attribute\n',
}

exclude_title = {
  '<alpha-value>',
  '<angle>',
  '<angle-percentage>',
  '<basic-shape>',
  '<calc-constant>',
  '<calc-sum>',
  '<color-interpolation-method>',
  '<color>',
  '<custom-ident>',
  '<dashed-ident>',
  '<display-listitem>',
  '<display-inside>',
  '<dimension>',
  '<easing-function>'
  '<filter-function>',
  '<flex>',
  '<frequency-percentage>',
  '<frequency>',
  '<gradient>',
  '<hex-color>',
  '<hue>',
  '<hue-interpolation-method>',
  '<ident>',
  '<image>',
  '<integer>',
  '<length>',
  '<length-percentage>',
  '<number>',
  '<percentage>',
  '<position>',
  '<ratio>',
  '<resolution>',
  '<string>',
  '<time-percentage>',
  '<time>',
  '<transform-function>',
  '"!important"',
}

properties_ignore_value = (
  'counter-increment',
  'counter-reset',
  'counter-set',
  'text-rendering',
  'page',
)


units: list[str] = []
colors: set[str] = set()
system_colors: set[str] = set()
deprecated_system_colors: set[str] = set()
values: set[str] = set()
properties: set[str] = set()
svg_values: set[str] = set()
svg_properties: set[str] = set()
functions: set[str] = set()
pseudo_classes: set[str] = set()
pseudo_elements: set[str] = set()
experimental_pseudo_classes: set[str] = set()
experimental_pseudo_elements: set[str] = set()
at_rules: set[str] = set()
media_features: set[str] = set()
media_feature_values: set[str] = set()


_update_version_extractor = re.compile(r' version="(\d+)" ')

def update_version(s: str) -> str:
  return _update_version_extractor.sub(lambda m: f' version="{int(m[1])+1}" ', s, count=1)


_md_value_extractor = re.compile(r'(?<=[^\w][ /])`([-\w][-\w\d]+(?:<[^>]+>[?+*])?)`')
_html_value_extractor = re.compile(r'<code>([-\w][-\w\d]+)</code>')
_is_md_value = re.compile(r'^\s*- `')
_is_html_table_desc = re.compile(r'^\s+<td><code>')

def css_parse_values(f: TextIO, prop: str, values: set[str]) -> None:
  line:str = ''
  # Format:
  # ## Syntax or ### Syntax
  #
  # ```css
  # (optional)
  # ```
  # ## Values or ### Values or not...
  #
  # - `ident` or html table <td><code>....</code></td>
  #
  # ## SVG only ... (optional)
  # ## other title
  for line in f:
    if line.endswith('## Syntax\n') or line.endswith('## Values\n') or '## SVG only' in line:
      for line in f:
        if _is_md_value.match(line):
          if 'deprecated' not in line:
            values.update(_md_value_extractor.findall(line))
        elif line.startswith('#'):
          if not (line.endswith('## Values\n') or '## SVG only' in line
                  or (prop == 'display'
                      and (line.endswith('## Grouped values\n')
                           or line.endswith('## Outside\n')
                           or line.endswith('## Inside\n')
                           or line.endswith('## List Item\n')
                           or line.endswith('## Internal\n')
                           or line.endswith('## Box\n')
                           or line.endswith('## Precomposed\n')
                           ))
          ):
            return
        elif line == '```css\n':
          for line in f:
            if line.startswith('```\n'):
              break
        elif _is_html_table_desc.match(line):
          values.update(_html_value_extractor.findall(line))


def css_parse_named_colors(f: TextIO) -> set[str]:
  return set(re.findall('\n      <td>(?:\n        )?<code>([a-z]+)</code>', f.read()))


def css_parse_units(f: TextIO) -> list[str]:
  return re.findall(r'`([^`]+)`', ''.join(re.findall(r'\n\| (`[^|]+)', f.read())))


_svg_values_extractor = re.compile(r'<th scope="row">Value</th>\n\s*<td>(.*?)</td>', re.DOTALL)
_svg_value_extractor = re.compile(r'<code>([-\w\d]+)</code>')

def css_parse_svg_attribute(f: TextIO, prop: str, properties: set[str], values: set[str]) -> None:
  contents = f.read()
  if 'can be used as a CSS property' in contents:
    properties.add(prop)
    m = _svg_values_extractor.search(contents)
    if m:
      values.update(_svg_value_extractor.findall(m[1]))


_experimental_selector_extractor = re.compile(r'\n- {{CSSxRef([^}]+)}} {{Experimental_Inline}}')
_selector_extractor = re.compile(r'":+([-\w\d]+)[()]*"')

def css_parse_pseudo_classes_or_elements(f: TextIO) -> tuple[
  set[str],  # experimental
  list[str]
]:
  s = f.read()
  experimental_str = ''.join(_experimental_selector_extractor.findall(s))
  return (set(_selector_extractor.findall(experimental_str)), _selector_extractor.findall(s))


if len(sys.argv) < 5:
  print(f'''{Path(sys.argv[0]).name} content-main-directory syntax/css.xml sass-site-directory syntax/scss.xml

content-main-directory is https://github.com/mdn/content/ (https://github.com/mdn/content/archive/refs/heads/main.zip)
sass-site-directory is https://github.com/sass/sass-site/tree/main (https://github.com/sass/sass-site/archive/refs/heads/main.zip)
''', file=sys.stderr)
  exit(1)

css_dir = Path(sys.argv[1])
css_filename = Path(sys.argv[2])
scss_dir = Path(sys.argv[3])
scss_filename = Path(sys.argv[4])


tmp_pseudo_classes = (set(), ())
tmp_pseudo_elements = (set(), ())

for pattern in (
  'files/en-us/web/svg/attribute/**/',
  'files/en-us/web/css/**/',
):
  for md in css_dir.glob(pattern):
    with open(md/'index.md', encoding='utf8') as f:
      if f.readline() != '---\n':
        continue

      title = f.readline()[7:-1]
      if title in exclude_title:
        continue

      if title.startswith('"'):
        title = title[1:-1]

      page_type = ''
      for line in f:
        if line in exclude_line:
          page_type = ''
          break

        if line.startswith('page-type: '):
          if line not in page_type_accepted:
            raise Exception(f'Unknown {line[:-1]}')
          page_type = line[11:-1]

        if line == '---\n':
          break

      if page_type == 'css-property' or page_type == 'css-at-rule-descriptor':
        properties.add(title)
        if not title.endswith('-name') and title not in properties_ignore_value:
          css_parse_values(f, title, values)
      elif page_type == 'css-shorthand-property':
        properties.add(title)
      elif page_type == 'css-pseudo-class':
        pseudo_classes.add(title[1:].removesuffix('()'))
      elif page_type == 'css-pseudo-element':
        pseudo_elements.add(title[2:].removesuffix('()'))
      elif page_type == 'css-type':
        if title == '<named-color>':
          colors = css_parse_named_colors(f)
        if title == '<system-color>':
          css_parse_values(f, '', system_colors)
          deprecated_system_colors = set(re.findall('\n- `([^`]+)` {{deprecated_inline}}', f.read()))
        else:
          css_parse_values(f, '', values)
      elif page_type == 'css-function':
        functions.add(title[:-2])
      elif page_type == 'css-at-rule':
        at_rules.add(title)
      elif page_type == 'css-media-feature':
        media_features.add(title)
        css_parse_values(f, title, media_feature_values)
      elif page_type == 'css-keyword':
        values.add(title)
      elif title == 'CSS values and units':
        units = css_parse_units(f)
      elif title == 'Pseudo-classes':
        tmp_pseudo_classes = css_parse_pseudo_classes_or_elements(f)
      elif title == 'Pseudo-elements':
        tmp_pseudo_elements = css_parse_pseudo_classes_or_elements(f)
      elif page_type == 'svg-attribute':
        css_parse_svg_attribute(f, title, svg_properties, svg_values)
      elif title == 'CSS value functions':
        functions.update(re.findall(r'\n- {{CSSxRef\("[^"]+", "([-\w\d]+)\(\)"\)}}\n', f.read()))


experimental_pseudo_classes = tmp_pseudo_classes[0]
experimental_pseudo_classes -= pseudo_classes
pseudo_classes.update(tmp_pseudo_classes[1])

experimental_pseudo_elements = tmp_pseudo_elements[0]
experimental_pseudo_elements -= pseudo_elements
pseudo_elements.update(tmp_pseudo_elements[1])


global_values = {
  'auto',
  'inherit',
  'initial',
  'revert',
  'revert-layer',
  'unset',
}
values -= global_values
svg_values -= global_values
pseudo_classes -= experimental_pseudo_classes
pseudo_elements -= experimental_pseudo_elements

# add values of functions
values.update((
  # repeat()
  'auto-fill',
  'auto-fit',
))

# move some keyword colors in values
for special_color in ('transparent', 'currentcolor'):
  values.add(special_color)
  colors.discard(special_color)

# fix not specified value in mdn file
if 'user-invalid' in experimental_pseudo_classes:
  pseudo_classes.discard('user-valid')
  experimental_pseudo_classes.add('user-valid')
media_features.update((
    'min-width',
    'max-width',
    'min-height',
    'max-height',
))

# fix errors in mdn file
for e in ('has', 'host-context'):
  pseudo_classes.add(e)
  experimental_pseudo_classes.discard(e)

# @font-format functions
functions.update((
    'format',
    'local',
    'tech',
))


# def show(name, values):
#   print(f'{name} ({len(values)}):')
#   print('\n'.join(sorted(values)), end='\n\n')
#
# show('properties', properties)
# show('svg properties', svg_properties)
# show('values', values)
# show('svg values', svg_values)
# show('global values', global_values)
# show('functions', functions)
# show('pseudo-classes', pseudo_classes)
# show('pseudo-elements', pseudo_elements)
# show('experimental pseudo-classes', experimental_pseudo_classes)
# show('experimental pseudo-elements', experimental_pseudo_elements)
# show('at-rules', at_rules)
# show('media-features', media_features)
# show('media-features values', media_feature_values)
# show('colors', colors)
# show('system colors', system_colors)
# show('deprecated system colors', deprecated_system_colors)
# show('units', units)
# print('units reg:', '|'.join(units))


#
# Update CSS
#

sep = '\n            '
css_replacements = {
  prop: f'</item>{sep}<item>'.join(sorted(seq))
  for prop, seq in (
    ('properties', properties),
    ('values', values),
    ('value keywords', global_values),
    ('functions', functions),
    ('pseudo-classes', pseudo_classes),
    ('pseudo-elements', pseudo_elements),
    ('media features', media_features)
  )
}
for prop, seq in (('properties', svg_properties - properties), ('values', svg_values - values)):
    if seq:
        items = f'</item>{sep}<item>'.join(sorted(seq))
        css_replacements[prop] += f'</item>\n{sep}<!-- SVG only -->\n{sep}<item>{items}'

rep1 = f'</item>{sep}<item>'.join(sorted(colors))
rep2 = f'</item>{sep}<item>'.join(sorted(system_colors))
css_replacements['colors'] = f'{rep1}</item>{sep}{sep}<!-- System colors -->{sep}<item>{rep2}'

item_extractor = re.compile('<item>([^-<][^<]*)')

current_at_rules = set()

def _css_update_and_extract_items(m) -> str:
  seq = css_replacements.get(m[1])
  if seq:
    end = '        ' if m[3] == '</list>' else sep
    return f'<list name="{m[1]}">{sep}<item>{seq}</item>\n{end}{m[3]}'

  current_at_rules.update(item_extractor.findall(m[2]))
  return m[0]


css_content = css_filename.read_text()
original_css_content = css_content

names = f"{'|'.join(css_replacements)}|at-rules(?: definitions)?"
css_content = re.sub(rf'<list name="({names})">(.*?)(</list>|<!-- manual list -->)',
                     _css_update_and_extract_items, css_content, flags=re.DOTALL)

_regexpr_unit_prefix = r'(<RegExpr attribute="Unit".*?String="\(%\|\()'
regexpr_unit_extractor = re.compile(fr'{_regexpr_unit_prefix}([^)]+)')

css_content = regexpr_unit_extractor.sub('\\1' + "|".join(units), css_content, 1)

if original_css_content != css_content:
  css_content = update_version(css_content)
  css_filename.write_text(css_content)


def show_at_rule_difference(language: str, old_at_rules: set[str], new_at_rules: set[str]) -> None:
  at_rule_added = new_at_rules - old_at_rules
  at_rule_removed = old_at_rules - new_at_rules
  nl = '\n  '
  if at_rule_added or at_rule_removed:
    print(f"""\x1b[31m{language} At-rules requires a manual update
New ({len(at_rule_added)}):\x1b[0m
  {nl.join(at_rule_added)}
\x1b[31mRemoved ({len(at_rule_removed)}):\x1b[0m
  {nl.join(at_rule_removed)}""")

show_at_rule_difference('CSS', current_at_rules, at_rules)

#
# Extract SCSS data
#

scss_functions:list[str] = []
scss_at_rules:set[str] = {'@content', '@return'}

_function_list_extractor = re.compile(r'{% function (.*?) %}')
_function_extractor = re.compile(r"'([-._a-zA-Z0-9]+)\(")
_at_rule_extractor = re.compile(r'@[-a-z0-9]+')

for md in sorted(scss_dir.glob('source/documentation/modules/**/*.md')):
  func_list = _function_list_extractor.findall(md.read_text())
  func_items = set(_function_extractor.findall(''.join(func_list)))
  scss_functions.append(f'\n{sep}<!-- {md.stem} -->')
  scss_functions.extend(f'{sep}<item>{func}</item>' for func in sorted(func_items - functions))

for md in scss_dir.glob('source/documentation/at-rules/**/*.md'):
  with open(md) as f:
    f.readline()
    scss_at_rules.update(_at_rule_extractor.findall(f.readline()))

subproperties = set(
  '-'.join(splitted[i:n])
  for prop in properties
  for splitted in (prop.rsplit('-', prop.count('-') - 1)  # '-aaa-bbb' -> ['-aaa', 'bbb']
                   if prop.startswith('-')
                   else prop.split('-'), )  # 'aaa-bbb' -> ['aaa', 'bbb']
  for i in range(len(splitted))
  for n in range(i+1, len(splitted)+1)
)

#
# Update SCSS
#

scss_current_at_rules = set()

def _scss_update_and_extract_items(m) -> str:
  name = m[1]

  if name == 'functions':
    return f"""<list name="functions">
            <include>functions##CSS</include>

            <!-- https://sass-lang.com/documentation/modules/ -->{f''.join(scss_functions)}
        </list>"""

  if name == 'at-rules':
    scss_current_at_rules.update(_at_rule_extractor.findall(m[2]))
    return m[0]

  # sub-properties
  items = f'</item>{sep}<item>'.join(sorted(subproperties - properties))
  return f'<list name="{name}">{sep}<item>{items}</item>\n        </list>'

scss_content = scss_filename.read_text()
original_scss_content = scss_content

scss_content = re.sub(r'<list name="(sub-properties|functions|at-rules)">(.*?)</list>',
                      _scss_update_and_extract_items, scss_content, count=3, flags=re.DOTALL)

scss_content = re.sub(r'<!ENTITY pseudoclasses "[^"]*">',
                      f'<!ENTITY pseudoclasses "{"|".join(sorted(pseudo_classes))}">',
                      scss_content, count=1)

scss_content = regexpr_unit_extractor.sub('\\1' + "|".join(units), scss_content, 1)

if original_scss_content != scss_content:
  scss_content = update_version(scss_content)
  scss_filename.write_text(scss_content)

show_at_rule_difference('SCSS', scss_current_at_rules, scss_at_rules)
