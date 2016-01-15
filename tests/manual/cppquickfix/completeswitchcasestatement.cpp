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

namespace Foo {
enum Orientation {
    Vertical,
    Horizontal
};
}

class Bar {
    enum AnEnum {
        AValue,
        AnotherValue
    };
};

enum Types {
    TypeA,
    TypeC,
    TypeB,
    TypeD,
    TypeE = TypeD
};

using namespace Foo;

int main()
{
    int j;
    Types t = TypeA;
    Types t2 = TypeB;
    bool b = true;
    enum { foo, bla } i;

    // all handled, don't activate
    switch (t) {
    case TypeA:
    case TypeB:
    case TypeC:
    case TypeD:
    case TypeE:
        ;
    }

    // TypeD must still be added for the outer switch
    switch (t) {
    case TypeA:
        switch (t2) {
        case TypeD: ;
        default: ;
        }
        break;
    case TypeB:
    case TypeE:
        break;
    default:
        ;
    }

    // Resolve type for expressions as switch condition
    switch (b ? t : t2) {
    case TypeA:;
    }

    // Namespaces
    Foo::Orientation o;
    switch (o) {
    case Vertical:
        break;
    }

    // Class members
    Bar::AnEnum a;
    switch (a) {
    case Bar::AnotherValue:
        break;
    }

    // Not a named type
    switch (i) {
    case bla:
        ;
    }

    // ignore pathological case that doesn't have a compound statement
    switch (i)
    case foo: ;
}
