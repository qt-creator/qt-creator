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
#include "tst_foo.h"
#include <QtTest>
#include <QCoreApplication>
#include <QDebug>

Foo::Foo()
{
}

Foo::~Foo()
{
}

void Foo::initTestCase()
{
}

void Foo::cleanupTestCase()
{
    QWARN("Warning!");
}

void Foo::test_caseZero()
{
    QCOMPARE(1, 2);
}

void Foo::test_case1()
{
    qDebug() << "test_case1";
    QFETCH(int, val);

    QEXPECT_FAIL("test2", "2", Continue);
    QCOMPARE(val, 1);
    QEXPECT_FAIL("test1", "bla", Abort);
    QCOMPARE(val, 2);
    QVERIFY2(true, "Hallo");
}

void Foo::test_case1_data()
{
    QTest::addColumn<int>("val");
    QTest::newRow("test1") << 1;
    QTest::newRow("test2") << 2;
    QTest::newRow("test3") << 3;
    QTest::newRow("test4") << 4;
}

void Foo::test_case2()
{
    QThread::sleep(1);
    qDebug() << "test_case2 - all pass";
    QSKIP("Skip for now", SkipAll);
    QCOMPARE(1 ,1);
    QVERIFY(true);
}

void Foo::test_case4()
{
    qDebug("äøæ");
    QSKIP("Skipping test_case4", SkipSingle);
    QFAIL("bla");
}

QTEST_MAIN(Foo)
