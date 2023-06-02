// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/mathutils.h>

#include <QtTest>

using namespace Utils;

class tst_MathUtils : public QObject
{
    Q_OBJECT

private slots:
    void interpolateLinear_data();
    void interpolateLinear();
    void interpolateTangential_data();
    void interpolateTangential();
    void interpolateExponential_data();
    void interpolateExponential();
};

void tst_MathUtils::interpolateLinear_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("x1");
    QTest::addColumn<int>("x2");
    QTest::addColumn<int>("y1");
    QTest::addColumn<int>("y2");
    QTest::addColumn<int>("result");

    QTest::newRow("x1") << 2 << 2 << 8 << 10 << 20 << 10;
    QTest::newRow("middleValue") << 5 << 2 << 8 << 10 << 20 << 15;
    QTest::newRow("x2") << 8 << 2 << 8 << 10 << 20 << 20;
    QTest::newRow("belowX1") << -1 << 2 << 8 << 10 << 20 << 5;
    QTest::newRow("aboveX2") << 11 << 2 << 8 << 10 << 20 << 25;
}

void tst_MathUtils::interpolateLinear()
{
    QFETCH(int, x);
    QFETCH(int, x1);
    QFETCH(int, x2);
    QFETCH(int, y1);
    QFETCH(int, y2);
    QFETCH(int, result);

    const int y = MathUtils::interpolateLinear(x, x1, x2, y1, y2);
    QCOMPARE(y, result);
}

void tst_MathUtils::interpolateTangential_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("xHalfLife");
    QTest::addColumn<int>("y1");
    QTest::addColumn<int>("y2");
    QTest::addColumn<int>("result");

    QTest::newRow("zero") << 0 << 8 << 10 << 20 << 10;
    QTest::newRow("halfLife") << 8 << 8 << 10 << 20 << 15;
    QTest::newRow("approxInfinity") << 1000 << 8 << 10 << 20 << 20;
}

void tst_MathUtils::interpolateTangential()
{
    QFETCH(int, x);
    QFETCH(int, xHalfLife);
    QFETCH(int, y1);
    QFETCH(int, y2);
    QFETCH(int, result);

    const int y = MathUtils::interpolateTangential(x, xHalfLife, y1, y2);
    QCOMPARE(y, result);
}

void tst_MathUtils::interpolateExponential_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("xHalfLife");
    QTest::addColumn<int>("y1");
    QTest::addColumn<int>("y2");
    QTest::addColumn<int>("result");

    QTest::newRow("zero") << 0 << 8 << 10 << 20 << 10;
    QTest::newRow("halfLife") << 8 << 8 << 10 << 20 << 15;
    QTest::newRow("approxInfinity") << 1000 << 8 << 10 << 20 << 20;
}

void tst_MathUtils::interpolateExponential()
{
    QFETCH(int, x);
    QFETCH(int, xHalfLife);
    QFETCH(int, y1);
    QFETCH(int, y2);
    QFETCH(int, result);

    const int y = MathUtils::interpolateExponential(x, xHalfLife, y1, y2);
    QCOMPARE(y, result);
}

QTEST_GUILESS_MAIN(tst_MathUtils)

#include "tst_mathutils.moc"
