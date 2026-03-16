// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtTest/qtestcase.h>

#include <utils/covariantcallback.h>

using namespace Utils;

class Base
{
public:
};

class Derived : public Base
{
public:
    void deleteMe() { delete this; }
    void dontDeleteMe() {}
    void dontDeleteMeButInt(int) {}
    void deleteMeAndInt(int) { delete this; }
};

void freeDeleteMe(Derived *p) { delete p; }
void freeFunc(Derived *) {}
void freeSharedFunc(std::shared_ptr<Derived>, int) {}

using RawPtrCallback = CovariantCallback<void(Base *)>;
using SharedPtrCallback = CovariantCallback<void(std::shared_ptr<Base>)>;

using RawPtrCallbackWithInt = CovariantCallback<void(Base *, int)>;
using SharedPtrCallbackWithInt = CovariantCallback<void(std::shared_ptr<Base>, int)>;

class tst_covariantcallback : public QObject
{
    Q_OBJECT

private slots:
    void raw()
    {
        RawPtrCallback callback = [](Derived *p) { delete p; };
        callback(new Derived());
    }

    void shared()
    {
        SharedPtrCallback callback = [](std::shared_ptr<Derived>) {};
        callback(std::make_shared<Derived>());

        SharedPtrCallback callbackConst = [](const std::shared_ptr<Derived> &) {};
        callbackConst(std::make_shared<Derived>());
    }

    void rawShared()
    {
        SharedPtrCallback callback = [](Derived *) {};
        callback(std::make_shared<Derived>());
    }

    void memberFunction()
    {
        RawPtrCallback callback = &Derived::deleteMe;
        callback(new Derived());
    }

    void freeFunction()
    {
        RawPtrCallback callback = freeDeleteMe;
        callback(new Derived());
    }

    void functionPtr()
    {
        void (*funcPtr)(Derived *) = freeDeleteMe;
        RawPtrCallback callback = funcPtr;
        callback(new Derived());
    }

    void functor() {
        struct Functor {
            void operator()(Derived *p) { delete p; }
        };

        RawPtrCallback callback = Functor{};
        callback(new Derived());
    }

    void memberFunctionShared()
    {
        SharedPtrCallback callback = &Derived::dontDeleteMe;
        callback(std::make_shared<Derived>());
    }

    void freeFunctionShared()
    {
        SharedPtrCallback callback = freeFunc;
        callback(std::make_shared<Derived>());
    }

    void functionPtrShared()
    {
        void (*funcPtr)(Derived *) = freeFunc;
        SharedPtrCallback callback = funcPtr;
        callback(std::make_shared<Derived>());
    }

    void functorShared()
    {
        struct Functor
        {
            void operator()(Derived *) {}
        };

        SharedPtrCallback callback = Functor{};
        callback(std::make_shared<Derived>());
    }

    void withExtraArgs()
    {
        RawPtrCallbackWithInt callback = [](Derived *p, int) { delete p; };
        callback(new Derived(), 42);
    }

    void withExtraArgsShared()
    {
        SharedPtrCallbackWithInt callback = [](std::shared_ptr<Derived>, int) {};
        callback(std::make_shared<Derived>(), 42);
    }

    void withExtraArgsMemberFunction()
    {
        RawPtrCallbackWithInt callback = &Derived::deleteMeAndInt;
        callback(new Derived(), 42);
    }

    void withExtraArgsSharedMemberFunction()
    {
        SharedPtrCallbackWithInt callback = &Derived::dontDeleteMeButInt;
        callback(std::make_shared<Derived>(), 42);
    }

    void withExtraArgsRawShared()
    {
        SharedPtrCallbackWithInt callback = [](Derived *, int) {};
        callback(std::make_shared<Derived>(), 42);
    }

    void withExtraArgsFreeFunction()
    {
        SharedPtrCallbackWithInt callback = freeSharedFunc;
        callback(std::make_shared<Derived>(), 42);
    }
};
QTEST_GUILESS_MAIN(tst_covariantcallback)

#include "tst_covariantcallback.moc"
