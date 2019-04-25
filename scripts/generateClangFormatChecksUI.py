#!/usr/bin/env python
############################################################################
#
# Copyright (C) 2019 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

import argparse
import json
import os
import docutils.nodes
import docutils.parsers.rst
import docutils.utils

def full_ui_content(checks):
    return '''<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>ClangFormat::ClangFormatChecksWidget</class>
 <widget class="QWidget" name="ClangFormat::ClangFormatChecksWidget">
  <property name="maximumSize">
   <size>
    <width>480</width>
    <height>16777215</height>
   </size>
  </property>
  <layout class="QGridLayout" name="checksLayout">
''' + checks + '''  </layout>
 </widget>
 <resources/>
 <connections/>
</ui>
'''

def parse_arguments():
    parser = argparse.ArgumentParser(description='Clazy checks header file generator')
    parser.add_argument('--clang-format-options-rst', help='path to ClangFormatStyleOptions.rst',
        default=None, dest='options_rst')
    return parser.parse_args()

def parse_rst(text):
    parser = docutils.parsers.rst.Parser()
    components = (docutils.parsers.rst.Parser,)
    settings = docutils.frontend.OptionParser(components=components).get_default_values()
    document = docutils.utils.new_document('<rst-doc>', settings=settings)
    parser.parse(text, document)
    return document

def createItem(key, value, index):
    label = '''   <item row="''' + str(index) + '''" column="0">
    <widget class="QLabel" name="label''' + key + '''">
     <property name="text">
      <string notr="true">''' + key + '''</string>
     </property>
    </widget>
   </item>
'''
    value_item = ''
    if value[0] == 'bool':
        value_item = '''   <item row="''' + str(index) + '''" column="1">
    <widget class="QComboBox" name="''' + key + '''">
     <property name="focusPolicy">
      <enum>Qt::StrongFocus</enum>
     </property>
     <item>
      <property name="text">
       <string notr="true">Default</string>
      </property>
     </item>
     <item>
      <property name="text">
       <string notr="true">true</string>
      </property>
     </item>
     <item>
      <property name="text">
       <string notr="true">false</string>
      </property>
     </item>
    </widget>
   </item>
'''
    elif value[0].startswith('std::string') or value[0] == 'unsigned' or value[0] == 'int':
        value_item = '''   <item row="''' + str(index) + '''" column="1">
    <layout class="QHBoxLayout">
     <item>
      <widget class="QLineEdit" name="''' + key + '''">
      </widget>
     </item>
     <item>
      <widget class="QPushButton" name="set''' + key + '''">
       <property name="maximumSize">
        <size>
         <width>40</width>
         <height>16777215</height>
        </size>
       </property>
       <property name="text">
        <string notr="true">Set</string>
       </property>
      </widget>
     </item>
    </layout>
   </item>
'''
    elif value[0].startswith('std::vector'):
        value_item = '''   <item row="''' + str(index) + '''" column="1">
    <layout class="QHBoxLayout">
     <item>
      <widget class="QPlainTextEdit" name="''' + key + '''">
       <property name="sizePolicy">
        <sizepolicy hsizetype="Expanding" vsizetype="Fixed"/>
       </property>
       <property name="maximumSize">
        <size>
         <width>16777215</width>
         <height>50</height>
        </size>
       </property>
      </widget>
     </item>
     <item>
      <widget class="QPushButton" name="set''' + key + '''">
       <property name="maximumSize">
        <size>
         <width>40</width>
         <height>16777215</height>
        </size>
       </property>
       <property name="text">
        <string notr="true">Set</string>
       </property>
      </widget>
     </item>
    </layout>
   </item>
'''
    else:
        if ' ' in value[1]:
            value_item = ''
            for i, val in enumerate(value):
                if i == 0:
                    continue
                index += 1
                space_index = val.find(' ')
                val = val[space_index + 1:]
                value_item += '''   <item row="''' + str(index) + '''" column="0">
    <widget class="QLabel" name="label''' + val + '''">
     <property name="text">
      <string notr="true">  ''' + val + '''</string>
     </property>
    </widget>
   </item>
'''
                value_item += '''   <item row="''' + str(index) + '''" column="1">
    <widget class="QComboBox" name="''' + val + '''">
     <property name="focusPolicy">
      <enum>Qt::StrongFocus</enum>
     </property>
     <item>
      <property name="text">
       <string notr="true">Default</string>
      </property>
     </item>
     <item>
      <property name="text">
       <string notr="true">true</string>
      </property>
     </item>
     <item>
      <property name="text">
       <string notr="true">false</string>
      </property>
     </item>
    </widget>
   </item>
'''
        else:
            value_item = '''   <item row="''' + str(index) + '''" column="1">
    <widget class="QComboBox" name="''' + key + '''">
     <property name="focusPolicy">
      <enum>Qt::StrongFocus</enum>
     </property>
'''
            if key == 'Language':
                value_item += '''     <property name="enabled">
      <bool>false</bool>
     </property>
'''
            if index > 0:
                value_item += '''     <item>
      <property name="text">
       <string notr="true">Default</string>
      </property>
     </item>
'''
            for i, val in enumerate(value):
                if i == 0:
                    continue
                underline_index = val.find('_')
                val = val[underline_index + 1:]
                value_item += '''     <item>
      <property name="text">
       <string notr="true">''' + val + '''</string>
      </property>
     </item>
'''
            value_item += '''    </widget>
   </item>
'''

    return label + value_item, index

class MyVisitor(docutils.nodes.NodeVisitor):
    in_bullet_list = False
    in_bullet_list_paragraph = False
    tree = {}
    last_key = ''
    def visit_term(self, node):
        node_values = node.traverse(condition=docutils.nodes.Text)
        name = node_values[0].astext()
        self.last_key = name
        self.tree[name] = [node_values[2].astext()]
    def visit_bullet_list(self, node):
        self.in_bullet_list = True
    def depart_bullet_list(self, node):
        self.in_bullet_list = False
    def visit_paragraph(self, node):
        if self.in_bullet_list:
            self.in_bullet_list_paragraph = True
    def depart_paragraph(self, node):
        self.in_bullet_list_paragraph = False
    def visit_literal(self, node):
        if self.in_bullet_list_paragraph:
            value = node.traverse(condition=docutils.nodes.Text)[0].astext()
            self.tree[self.last_key].append(value)
            self.in_bullet_list_paragraph = False
    def unknown_visit(self, node):
        """Called for all other node types."""
        #print(node)
        pass
    def unknown_departure(self, node):
        pass

def main():
    arguments = parse_arguments()

    content = file(arguments.options_rst).read()
    document = parse_rst(content)
    visitor = MyVisitor(document)
    document.walkabout(visitor)
    keys = visitor.tree.keys()
    basedOnStyleKey = 'BasedOnStyle'
    keys.remove(basedOnStyleKey)
    keys.sort()

    text = ''
    line, index = createItem(basedOnStyleKey, visitor.tree[basedOnStyleKey], 0)
    text += line
    index = 1
    for key in keys:
        line, index = createItem(key, visitor.tree[key], index)
        text += line
        index += 1

    current_path = os.path.dirname(os.path.abspath(__file__))
    ui_path = os.path.abspath(os.path.join(current_path, '..', 'src',
        'plugins', 'clangformat', 'clangformatchecks.ui'))
    with open(ui_path, 'w') as f:
        f.write(full_ui_content(text))

if __name__ == "__main__":
    main()
