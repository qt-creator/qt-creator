/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QtGlobal>

namespace CppTools {
namespace Constants {

const char M_TOOLS_CPP[]              = "CppTools.Tools.Menu";
const char SWITCH_HEADER_SOURCE[]     = "CppTools.SwitchHeaderSource";
const char OPEN_HEADER_SOURCE_IN_NEXT_SPLIT[] = "CppTools.OpenHeaderSourceInNextSplit";
const char TASK_INDEX[]               = "CppTools.Task.Index";
const char TASK_SEARCH[]              = "CppTools.Task.Search";
const char C_SOURCE_MIMETYPE[] = "text/x-csrc";
const char C_HEADER_MIMETYPE[] = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[] = "text/x-c++src";
const char OBJECTIVE_C_SOURCE_MIMETYPE[] = "text/x-objcsrc";
const char OBJECTIVE_CPP_SOURCE_MIMETYPE[] = "text/x-objc++src";
const char CPP_HEADER_MIMETYPE[] = "text/x-c++hdr";
const char QDOC_MIMETYPE[] = "text/x-qdoc";
const char MOC_MIMETYPE[] = "text/x-moc";

// QSettings keys for use by the "New Class" wizards.
const char CPPTOOLS_SETTINGSGROUP[] = "CppTools";
const char LOWERCASE_CPPFILES_KEY[] = "LowerCaseFiles";
enum { lowerCaseFilesDefault = 1 };
const char CPPTOOLS_SORT_EDITOR_DOCUMENT_OUTLINE[] = "SortedMethodOverview";
const char CPPTOOLS_SHOW_INFO_BAR_FOR_HEADER_ERRORS[] = "ShowInfoBarForHeaderErrors";
const char CPPTOOLS_SHOW_INFO_BAR_FOR_FOR_NO_PROJECT[] = "ShowInfoBarForNoProject";
const char CPPTOOLS_MODEL_MANAGER_PCH_USAGE[] = "PCHUsage";
const char CPPTOOLS_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS[]
    = "InterpretAmbiguousHeadersAsCHeaders";
const char CPPTOOLS_SKIP_INDEXING_BIG_FILES[] = "SkipIndexingBigFiles";
const char CPPTOOLS_INDEXER_FILE_SIZE_LIMIT[] = "IndexerFileSizeLimit";

const char CPP_CLANG_BUILTIN_CONFIG_ID_EVERYTHING_WITH_EXCEPTIONS[]
    = "Builtin.EverythingWithExceptions";

const char CPP_CODE_STYLE_SETTINGS_ID[] = "A.Cpp.Code Style";
const char CPP_CODE_STYLE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "Code Style");
const char CPP_FILE_SETTINGS_ID[] = "B.Cpp.File Naming";
const char CPP_FILE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "File Naming");
const char CPP_CODE_MODEL_SETTINGS_ID[] = "C.Cpp.Code Model";
const char CPP_CODE_MODEL_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "Code Model");
const char CPP_DIAGNOSTIC_CONFIG_SETTINGS_ID[] = "C.Cpp.Diagnostic Config";
const char CPP_DIAGNOSTIC_CONFIG_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "Diagnostic Configurations");
const char CPP_SETTINGS_CATEGORY[] = "I.C++";

const char CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID[] = "ClangFixItAvailableMarker";
const char CPP_FUNCTION_DECL_DEF_LINK_MARKER_ID[] = "FunctionDeclDefLinkMarker";

const char CPP_SETTINGS_ID[] = "Cpp";
const char CPP_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++");

const char CURRENT_DOCUMENT_FILTER_ID[] = "Methods in current Document";
const char CURRENT_DOCUMENT_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Symbols in Current Document");

const char CLASSES_FILTER_ID[] = "Classes";
const char CLASSES_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Classes");

const char FUNCTIONS_FILTER_ID[] = "Methods";
const char FUNCTIONS_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Functions");

const char INCLUDES_FILTER_ID[] = "All Included C/C++ Files";
const char INCLUDES_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "All Included C/C++ Files");

const char LOCATOR_FILTER_ID[] = "Classes and Methods";
const char LOCATOR_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Classes, Enums and Functions");

const char SYMBOLS_FIND_FILTER_ID[] = "Symbols";
const char SYMBOLS_FIND_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Symbols");

// CLANG-UPGRADE-CHECK: Checks/update URLs.
//
// Upgrade the version in the URL. Note that we cannot use the macro
// CLANG_VERSION here because it might denote a version that was not yet
// released (e.g. 6.0.1, but only 6.0.0 was released).
constexpr const char TIDY_DOCUMENTATION_URL_TEMPLATE[]
    = "https://releases.llvm.org/7.0.0/tools/clang/tools/extra/docs/clang-tidy/checks/%1.html";

constexpr const char CLAZY_DOCUMENTATION_URL_TEMPLATE[]
    = "https://github.com/KDE/clazy/blob/master/docs/checks/README-%1.md";

static const char *DEFAULT_CODE_STYLE_SNIPPETS[]
    = {"#include <math.h>\n"
       "\n"
       "class Complex\n"
       "    {\n"
       "public:\n"
       "    Complex(double re, double im)\n"
       "        : _re(re), _im(im)\n"
       "        {}\n"
       "    double modulus() const\n"
       "        {\n"
       "        return sqrt(_re * _re + _im * _im);\n"
       "        }\n"
       "private:\n"
       "    double _re;\n"
       "    double _im;\n"
       "    };\n"
       "\n"
       "void bar(int i)\n"
       "    {\n"
       "    static int counter = 0;\n"
       "    counter += i;\n"
       "    }\n"
       "\n"
       "namespace Foo\n"
       "    {\n"
       "    namespace Bar\n"
       "        {\n"
       "        void foo(int a, int b)\n"
       "            {\n"
       "            for (int i = 0; i < a; i++)\n"
       "                {\n"
       "                if (i < b)\n"
       "                    bar(i);\n"
       "                else\n"
       "                    {\n"
       "                    bar(i);\n"
       "                    bar(b);\n"
       "                    }\n"
       "                }\n"
       "            }\n"
       "        } // namespace Bar\n"
       "    } // namespace Foo\n",
       "#include <math.h>\n"
       "\n"
       "class Complex\n"
       "    {\n"
       "public:\n"
       "    Complex(double re, double im)\n"
       "        : _re(re), _im(im)\n"
       "        {}\n"
       "    double modulus() const\n"
       "        {\n"
       "        return sqrt(_re * _re + _im * _im);\n"
       "        }\n"
       "private:\n"
       "    double _re;\n"
       "    double _im;\n"
       "    };\n"
       "\n"
       "void bar(int i)\n"
       "    {\n"
       "    static int counter = 0;\n"
       "    counter += i;\n"
       "    }\n"
       "\n"
       "namespace Foo\n"
       "    {\n"
       "    namespace Bar\n"
       "        {\n"
       "        void foo(int a, int b)\n"
       "            {\n"
       "            for (int i = 0; i < a; i++)\n"
       "                {\n"
       "                if (i < b)\n"
       "                    bar(i);\n"
       "                else\n"
       "                    {\n"
       "                    bar(i);\n"
       "                    bar(b);\n"
       "                    }\n"
       "                }\n"
       "            }\n"
       "        } // namespace Bar\n"
       "    } // namespace Foo\n",
       "namespace Foo\n"
       "{\n"
       "namespace Bar\n"
       "{\n"
       "class FooBar\n"
       "    {\n"
       "public:\n"
       "    FooBar(int a)\n"
       "        : _a(a)\n"
       "        {}\n"
       "    int calculate() const\n"
       "        {\n"
       "        if (a > 10)\n"
       "            {\n"
       "            int b = 2 * a;\n"
       "            return a * b;\n"
       "            }\n"
       "        return -a;\n"
       "        }\n"
       "private:\n"
       "    int _a;\n"
       "    };\n"
       "}\n"
       "}\n",
       "#include \"bar.h\"\n"
       "\n"
       "int foo(int a)\n"
       "    {\n"
       "    switch (a)\n"
       "        {\n"
       "        case 1:\n"
       "            bar(1);\n"
       "            break;\n"
       "        case 2:\n"
       "            {\n"
       "            bar(2);\n"
       "            break;\n"
       "            }\n"
       "        case 3:\n"
       "        default:\n"
       "            bar(3);\n"
       "            break;\n"
       "        }\n"
       "    return 0;\n"
       "    }\n",
       "void foo() {\n"
       "    if (a &&\n"
       "        b)\n"
       "        c;\n"
       "\n"
       "    while (a ||\n"
       "           b)\n"
       "        break;\n"
       "    a = b +\n"
       "        c;\n"
       "    myInstance.longMemberName +=\n"
       "            foo;\n"
       "    myInstance.longMemberName += bar +\n"
       "                                 foo;\n"
       "}\n",
       "int *foo(const Bar &b1, Bar &&b2, int*, int *&rpi)\n"
       "{\n"
       "    int*pi = 0;\n"
       "    int*const*const cpcpi = &pi;\n"
       "    int*const*pcpi = &pi;\n"
       "    int**const cppi = &pi;\n"
       "\n"
       "    void (*foo)(char *s) = 0;\n"
       "    int (*bar)[] = 0;\n"
       "\n"
       "    return pi;\n"
       "}\n"};

} // namespace Constants
} // namespace CppTools
