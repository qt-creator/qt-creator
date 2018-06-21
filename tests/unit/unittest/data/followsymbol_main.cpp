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
#include "followsymbol_header.h"
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
