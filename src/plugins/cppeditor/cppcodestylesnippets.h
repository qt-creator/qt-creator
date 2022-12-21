// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace CppEditor {
namespace Constants {

static const char *DEFAULT_CODE_STYLE_SNIPPETS[] = {
R"==(#include <math.h>

class Complex
    {
public:
    Complex(double re, double im)
        : _re(re), _im(im)
        {}
    double modulus() const
        {
        return sqrt(_re * _re + _im * _im);
        }
private:
    double _re;
    double _im;
    };

void bar(int i)
    {
    static int counter = 0;
    counter += i;
    }

namespace Foo
    {
    namespace Bar
        {
        void foo(int a, int b)
            {
            for (int i = 0; i < a; i++)
                {
                if (i < b)
                    bar(i);
                else
                    {
                    bar(i);
                    bar(b);
                    }
                }
            }
        } // namespace Bar
    } // namespace Foo
)==",
R"==(#include <math.h>

class Complex
    {
public:
    Complex(double re, double im)
        : _re(re), _im(im)
        {}
    double modulus() const
        {
        return sqrt(_re * _re + _im * _im);
        }
private:
    double _re;
    double _im;
    };

void bar(int i)
    {
    static int counter = 0;
    counter += i;
    }

namespace Foo
    {
    namespace Bar
        {
        void foo(int a, int b)
            {
            for (int i = 0; i < a; i++)
                {
                if (i < b)
                    bar(i);
                else
                    {
                    bar(i);
                    bar(b);
                    }
                }
            }
        } // namespace Bar
    } // namespace Foo
)==",
R"==(namespace Foo
{
namespace Bar
{
class FooBar
    {
public:
    FooBar(int a)
        : _a(a)
        {}
    int calculate() const
        {
        if (a > 10)
            {
            int b = 2 * a;
            return a * b;
            }
        return -a;
        }
private:
    int _a;
    };
enum class E
{
    V1,
    V2,
    V3
};
}
}
)==",
R"==(#include "bar.h"

int foo(int a)
    {
    switch (a)
        {
        case 1:
            bar(1);
            break;
        case 2:
            {
            bar(2);
            break;
            }
        case 3:
        default:
            bar(3);
            break;
        }
    return 0;
    }
)==",
R"==(void foo() {
    if (a &&
        b)
        c;

    while (a ||
           b)
        break;
    a = b +
        c;
    myInstance.longMemberName +=
            foo;
    myInstance.longMemberName += bar +
                                 foo;
}
)==",
R"==(int *foo(const Bar &b1, Bar &&b2, int*, int *&rpi)
{
    int*pi = 0;
    int*const*const cpcpi = &pi;
    int*const*pcpi = &pi;
    int**const cppi = &pi;

    void (*foo)(char *s) = 0;
    int (*bar)[] = 0;

    return pi;
}
)=="};

} // namespace Constants
} // namespace CppEditor
