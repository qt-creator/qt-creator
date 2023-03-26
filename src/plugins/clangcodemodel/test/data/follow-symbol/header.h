// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0








// This space intentionally left empty













#pragma once

#define TEST_DEFINE 1
namespace Fooish
{
float flvalue = 100.f;

class Bar;

class Bar {
public:
    Bar();

    volatile int member = 0;
};

struct Barish
{
    int foo(float p, int u);
    int mem = 10;
};
}

class FooClass;

int foo(const float p, int u);

int foo();

int foo(float p, int u)
{
    return foo() + p + u;
}

int foo(int x, float y);
