// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace breakpoints {

    struct SomeClassWithInlineConstructor
    {
        SomeClassWithInlineConstructor()
        {
            a = 21;
        }
        int a;
    };

    struct SomeBaseClassWithInlineConstructor
    {
        SomeBaseClassWithInlineConstructor()
        {
            a = 21;
        }
        virtual ~SomeBaseClassWithInlineConstructor();
        int a;
    };

    struct SomeDerivedClassWithInlineConstructor
        : SomeBaseClassWithInlineConstructor
    {
        SomeDerivedClassWithInlineConstructor()
        {
            a = 21;
        }
        virtual ~SomeDerivedClassWithInlineConstructor();
        int a;
    };

    template <class T>
    struct SomeTemplatedClassWithInlineConstructor
    {
        SomeTemplatedClassWithInlineConstructor()
        {
            a = 21;
        }
        T a;
    };

    template <class T>
    struct SomeTemplatedBaseClassWithInlineConstructor
    {
        SomeTemplatedBaseClassWithInlineConstructor()
        {
            a = 21;
        }
        virtual ~SomeTemplatedBaseClassWithInlineConstructor();

        T a;
    };

    template <class T>
    struct SomeTemplatedDerivedClassWithInlineConstructor
        : SomeTemplatedBaseClassWithInlineConstructor<T>
    {
        SomeTemplatedDerivedClassWithInlineConstructor()
        {
            a = 21;
        }
        virtual ~SomeTemplatedDerivedClassWithInlineConstructor();
        T a;
    };


    SomeBaseClassWithInlineConstructor::~SomeBaseClassWithInlineConstructor() {}

    SomeDerivedClassWithInlineConstructor::~SomeDerivedClassWithInlineConstructor() {}

    template <typename T>
    SomeTemplatedBaseClassWithInlineConstructor<T>::
        ~SomeTemplatedBaseClassWithInlineConstructor() {}

    template <typename T>
    SomeTemplatedDerivedClassWithInlineConstructor<T>::
        ~SomeTemplatedDerivedClassWithInlineConstructor() {}

    struct X : virtual SomeClassWithInlineConstructor
    {
        X() { a = 1; }
    };

    struct Y : virtual SomeClassWithInlineConstructor
    {
        Y() { a = 2; }
    };

    struct Z : X, Y
    {
        Z() : SomeClassWithInlineConstructor(), X(), Y() {}
    };

    template <class T> T twice(T t)
    {
        return 2 * t;
    }

    template <class T> struct Twice
    {
        Twice(T t) { t_ = 2 * t; }
        T t_;
    };

    void testBreakpoints()
    {
        SomeClassWithInlineConstructor a;
        SomeBaseClassWithInlineConstructor b;
        SomeDerivedClassWithInlineConstructor c;
        SomeTemplatedClassWithInlineConstructor<int> d;
        SomeTemplatedBaseClassWithInlineConstructor<int> e;
        SomeTemplatedDerivedClassWithInlineConstructor<int> f;
        Z z;
        int aa = twice(1);
        int bb = twice(1.0);
        // <=== Break here.
        Twice<int> cc = Twice<int>(1);
        Twice<float> dd = Twice<float>(1.0);
        dummyStatement(&a, &b, &c, &d, &e, &f, &z, &bb, &aa, &cc, &dd);
    }

} // namespace breakpoints
