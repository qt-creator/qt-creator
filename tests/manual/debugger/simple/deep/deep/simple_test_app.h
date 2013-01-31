/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SIMPLE_DEBUGGER_TEST_H
#define SIMPLE_DEBUGGER_TEST_H

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

#endif // SIMPLE_DEBUGGER_TEST_H
