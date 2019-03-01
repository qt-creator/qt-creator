/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

namespace CppTools {
namespace Constants {

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
