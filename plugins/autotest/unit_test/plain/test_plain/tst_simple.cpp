/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/
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
