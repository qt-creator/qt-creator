// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
    qWarning("Warning!");
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
