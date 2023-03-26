// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
