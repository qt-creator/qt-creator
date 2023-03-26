// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0








// This space intentionally left empty













#include "header.h"
#include "cursor.h"
Fooish::Bar::Bar() {

}

class X;

using YYY = Fooish::Bar;

int foo() {
    YYY bar;
    bar.member = 30;
    Fooish::Barish barish;
    bar.member++;
    barish.mem = Fooish::flvalue;

    barish.foo(1.f, 2);
    foo(1, 2.f);
    return 1;

    X* x;
}

int Fooish::Barish::foo(float p, int u)
{
    return ::foo() + p + u;
}

class FooClass
{
public:
    FooClass();
    static int mememember;
};

FooClass::FooClass() {
    NonFinalStruct nfStruct; nfStruct.function();
}

int main() {
    return foo() + FooClass::mememember + TEST_DEFINE;
}

class Bar
{
public:
    int operator&();
    Bar& operator[](int);
};

int Bar::operator&() {
    return 0;
}

Bar& Bar::operator[](int) {
    return *this;
}

struct S {
    union  {
        int i = 12;
        void *something;
    };
    int func(bool b) {
        if (b)
            return i;
        int i = 42;
        return i;
    }

    YYY getYYY() const { return YYY(); }
    Bar getBar() const { return Bar(); }
};

static const auto yyyVar = S().getYYY();
static const auto barVar = S().getBar();
