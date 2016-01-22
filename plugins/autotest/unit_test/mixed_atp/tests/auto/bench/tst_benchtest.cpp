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
