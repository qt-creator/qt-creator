// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
