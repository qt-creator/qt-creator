/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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
