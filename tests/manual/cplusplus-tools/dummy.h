// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace test {

class Dummy
{
public:
    Dummy();
    Dummy(int a);

    typedef int INT;

    enum Values {
        v1,
        v2,
        v3
    };

    static const int ONE = 1;
    static const double PI;

    static void sfunc();

    struct Internal
    {
        QString one;
        typedef double DOUBLE;
    };

    void bla(int);
    void bla(const QString &);
    void bla(const QString &) const;
    void bla(int, const QString &) const;

    void foo(int) const {}
    void foo(const QString &) const {}

    QString one;
};

class ChildDummy : public Dummy {};

class GrandChildDummy : public Dummy {};

} // namespace test
