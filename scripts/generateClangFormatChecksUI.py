#!/usr/bin/env python3.10
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import argparse
import os
# for installing use pip3 install robotpy-cppheaderparse
import CppHeaderParser

def parse_arguments():
    parser = argparse.ArgumentParser(description='Clazy checks header file \
    generator')
    parser.add_argument('--clang-format-header-file', help='path to \
    Format.h usually /usr/lib/llvm-x/include/clang/Format/Format.h',
                        default=None, dest='options_header', required=True)
    return parser.parse_args()

def full_ui_content(checks):
    return '''<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>ClangFormat::ClangFormatChecksWidget</class>
 <widget class="QWidget" name="ClangFormat::ClangFormatChecksWidget">
  <property name="maximumSize">
   <size>
    <width>580</width>
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

def lable_ui(name, index, offset = ""):
    return '''   <item row="''' + str(index) + '''" column="0">
    <widget class="QLabel" name="label''' + name + '''">
     <property name="text">
      <string notr="true">''' + offset + name + '''</string>
     </property>
    </widget>
   </item>
'''

def combobox_ui(name, values, index):
    combobox = '''   <item row="''' + str(index) + '''" column="1">
    <widget class="QComboBox" name="''' + name + '''">
     <property name="focusPolicy">
      <enum>Qt::StrongFocus</enum>
     </property>
'''
    for value in values:
        combobox += '''     <item>
      <property name="text">
       <string notr="true">''' + value + '''</string>
      </property>
     </item>
'''
    # for
    combobox += '''    </widget>
   </item>
'''
    return combobox

def string_ui(name, index):
    return '''   <item row="''' + str(index) + '''" column="1">
    <layout class="QHBoxLayout">
     <item>
      <widget class="QLineEdit" name="''' + name + '''">
      </widget>
     </item>
     <item>
      <widget class="QPushButton" name="set''' + name + '''">
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

def vector_ui(name, index):
    return '''   <item row="''' + str(index) + '''" column="1">
    <layout class="QHBoxLayout">
     <item>
      <widget class="QPlainTextEdit" name="''' + name + '''">
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
      <widget class="QPushButton" name="set''' + name + '''">
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

def combobox_ui_bool(name, index):
    return combobox_ui(name, ["Default", "true", "false"], index)

def in_list(list, type):
    for element in list:
        if element["name"] == type:
            return element;
    return

create_checks_index = 0
def create_checks(variables, enums, structs, offset = ""):
    checks = ""
    global create_checks_index
    # create BasedOnStyle combobox ussually not presented in FormatStyle struct
    if 0 == create_checks_index:
        create_checks_index += 1
        checks = lable_ui("BasedOnStyle", create_checks_index)
        checks += combobox_ui("BasedOnStyle", ["LLVM", "Google", "Chromium", "Mozilla", "WebKit", "Microsoft", "GNU"], create_checks_index)

    for variable in variables:
        create_checks_index += 1
        type = variable["type"]
        name = variable["name"]
        enum = in_list(enums, type)
        struct = in_list(structs, type)
        if enum:
            checks += lable_ui(name, create_checks_index, offset)
            checks += combobox_ui(name, [value["name"].split("_")[1] for value in enum["values"]], create_checks_index)
        elif struct:
            checks += lable_ui(name, create_checks_index, offset)
            check = create_checks(struct["properties"]["public"], enums, structs, "  ")
            checks += check
        elif "std::string" == type or "unsigned" == type or "int" == type:
            checks += lable_ui(name, create_checks_index, offset)
            checks += string_ui(name, create_checks_index)
        elif "std::vector<std::string >" == type:
            checks += lable_ui(name, create_checks_index, offset)
            checks += vector_ui(name, create_checks_index)
        elif "bool" == type:
            checks += lable_ui(name, create_checks_index, offset)
            checks += combobox_ui_bool(name, create_checks_index)
    return checks


def main():
    arguments = parse_arguments()
    header = CppHeaderParser.CppHeader(arguments.options_header)

    enums = header.classes["FormatStyle"]["enums"]["public"]
    structs = header.classes["FormatStyle"]["nested_classes"]
    variables = header.classes["FormatStyle"]["properties"]["public"]

    checks = create_checks(variables, enums, structs)

    current_path = os.path.dirname(os.path.abspath(__file__))
    ui_path = os.path.abspath(os.path.join(current_path, '..', 'src',
        'plugins', 'clangformat', 'clangformatchecks.ui'))
    with open(ui_path, 'w') as f:
        f.write(full_ui_content(checks))


if __name__ == "__main__":
    main()
