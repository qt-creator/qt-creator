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
