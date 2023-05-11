// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/textutils.h>

#include <QtTest>

using namespace Utils::Text;

class tst_Text : public QObject
{
    Q_OBJECT

private slots:
    void testRangeLength_data();
    void testRangeLength();
};

void tst_Text::testRangeLength_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<Range>("range");
    QTest::addColumn<int>("length");

    QTest::newRow("empty range") << QString() << Range{{1, 0}, {1, 0}} << 0;

    QTest::newRow("range on same line") << QString() << Range{{1, 0}, {1, 1}} << 1;

    QTest::newRow("range spanning lines") << QString("foo\nbar") << Range{{1, 0}, {2, 0}} << 4;

    QTest::newRow("range spanning lines and column offset")
        << QString("foo\nbar") << Range{{1, 1}, {2, 1}} << 4;

    QTest::newRow("range spanning lines and begin column offset")
        << QString("foo\nbar") << Range{{1, 1}, {2, 0}} << 3;

    QTest::newRow("range spanning lines and end column offset")
        << QString("foo\nbar") << Range{{1, 0}, {2, 1}} << 5;

    QTest::newRow("hyper range") << QString("foo\nbar\nfoobar") << Range{{2, 1}, {3, 1}} << 4;

    QTest::newRow("flipped range") << QString() << Range{{2, 0}, {1, 0}} << -1;

    QTest::newRow("out of range") << QString() << Range{{1, 0}, {2, 0}} << -1;
}

void tst_Text::testRangeLength()
{
    QFETCH(QString, text);
    QFETCH(Range, range);
    QFETCH(int, length);

    QCOMPARE(range.length(text), length);
}

QTEST_GUILESS_MAIN(tst_Text)

#include "tst_text.moc"
