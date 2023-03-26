// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "tst_simple.h"
#include <QCoreApplication>

namespace Foo {
namespace Bar {

class bla : public QObject
{
    Q_OBJECT

public:
    bla() {}
    ~bla() {}

private slots:
    void tst_dummy() {}

};

class anotherBla : public QObject
{
    Q_OBJECT

private slots:
    void init() {}
    void cleanup() {}

    void tst_bla() {}
    void test_foo() {}
    void test_bar() {}
    void tst_batz() {}
    void test_hello() {}
    void test_io() { qDebug("ä");}
};

} // namespace Foo
} // namespace Bar

int main(int argc, char* argv[])
{
    Foo::Bar::anotherBla *x2 = new Foo::Bar::anotherBla;
    int result = QTest::qExec(x2, argc, argv);
//  multiple QTest::qExec() calls actually do not work
//  result += QTest::qExec(new Foo::Bar::bla, argc, argv);
    return result;
}

#include "tst_simple.moc"
