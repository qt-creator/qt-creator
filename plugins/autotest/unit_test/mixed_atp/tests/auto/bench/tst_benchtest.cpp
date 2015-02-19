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
#include <QString>
#include <QtTest>

class BenchTest : public QObject
{
    Q_OBJECT

public:
    BenchTest();

private Q_SLOTS:
    void testCase1();
    void testCase1_data();
};

BenchTest::BenchTest()
{
}

void BenchTest::testCase1()
{
    QFETCH(bool, localAware);

    QString str1 = QLatin1String("Hello World");
    QString str2 = QLatin1String("Hallo Welt");
    if (!localAware) {
        QBENCHMARK {
            str1 == str2;
        }
    } else {
        QBENCHMARK {
            str1.localeAwareCompare(str2) == 0;
        }
    }
}

void BenchTest::testCase1_data()
{
    QTest::addColumn<bool>("localAware");
    QTest::newRow("localAware") << true;
    QTest::newRow("simple") << false;
}

QTEST_MAIN(BenchTest)

#include "tst_benchtest.moc"
