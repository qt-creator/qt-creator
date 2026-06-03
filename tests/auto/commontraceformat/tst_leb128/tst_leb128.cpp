// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <commontraceformat/util/leb128.h>
#include <QtTest>

#include <limits>

using namespace CommonTraceFormat;

class tst_Leb128 : public QObject
{
    Q_OBJECT
private slots:
    void uleb128_data()
    {
        QTest::addColumn<quint64>("value");
        QTest::addColumn<QByteArray>("encoded");

        QTest::newRow("0")       << quint64(0)        << QByteArray("\x00", 1);
        QTest::newRow("1")       << quint64(1)        << QByteArray("\x01");
        QTest::newRow("127")     << quint64(127)      << QByteArray("\x7f");
        QTest::newRow("128")     << quint64(128)      << QByteArray("\x80\x01", 2);
        QTest::newRow("300")     << quint64(300)      << QByteArray("\xac\x02", 2);
        QTest::newRow("16383")   << quint64(16383)    << QByteArray("\xff\x7f", 2);
        QTest::newRow("16384")   << quint64(16384)    << QByteArray("\x80\x80\x01", 3);
        QTest::newRow("max32")   << quint64(0xFFFFFFFFu) << QByteArray("\xff\xff\xff\xff\x0f", 5);
        QTest::newRow("max64")   << std::numeric_limits<quint64>::max() << QByteArray("\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01", 10);
    }

    void uleb128()
    {
        QFETCH(quint64, value);
        QFETCH(QByteArray, encoded);

        QByteArray out;
        encodeULeb128(value, out);
        QCOMPARE(out, encoded);

        auto [decoded, n] = decodeULeb128(std::span(out.constData(), out.size()));
        QCOMPARE(n, out.size());
        QCOMPARE(decoded, value);
    }

    void sleb128_data()
    {
        QTest::addColumn<qint64>("value");
        QTest::addColumn<QByteArray>("encoded");

        QTest::newRow("0")    << qint64(0)    << QByteArray("\x00", 1);
        QTest::newRow("1")    << qint64(1)    << QByteArray("\x01");
        QTest::newRow("-1")   << qint64(-1)   << QByteArray("\x7f");
        QTest::newRow("63")   << qint64(63)   << QByteArray("\x3f");
        QTest::newRow("64")   << qint64(64)   << QByteArray("\xc0\x00", 2);
        QTest::newRow("-64")  << qint64(-64)  << QByteArray("\x40");
        QTest::newRow("-65")  << qint64(-65)  << QByteArray("\xbf\x7f", 2);
        QTest::newRow("min64")<< std::numeric_limits<qint64>::min() << QByteArray("\x80\x80\x80\x80\x80\x80\x80\x80\x80\x7f", 10);
        QTest::newRow("max64")<< std::numeric_limits<qint64>::max() << QByteArray("\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00", 10);
    }

    void sleb128()
    {
        QFETCH(qint64, value);
        QFETCH(QByteArray, encoded);

        QByteArray out;
        encodeSLeb128(value, out);
        QCOMPARE(out, encoded);

        auto [decoded, n] = decodeSLeb128(std::span(out.constData(), out.size()));
        QCOMPARE(n, out.size());
        QCOMPARE(decoded, value);
    }

    void truncated_uleb128()
    {
        // Incomplete sequence → error
        QByteArray partial("\x80"); // continuation bit set, no next byte
        auto [v, n] = decodeULeb128(std::span(partial.constData(), partial.size()));
        QCOMPARE(n, -1);
    }
};

QTEST_APPLESS_MAIN(tst_Leb128)
#include "tst_leb128.moc"
