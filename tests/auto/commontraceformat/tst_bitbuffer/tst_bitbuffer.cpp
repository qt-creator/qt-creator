// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <commontraceformat/binary/bitbuffer.h>

#include <QBuffer>
#include <QtTest>

using namespace CommonTraceFormat;

class tst_BitBuffer : public QObject
{
    Q_OBJECT
private slots:
    void writeRead_8bit_lsb()
    {
        // 8-bit values, LSB-first, little-endian byte order
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(0xAB, 8, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        wb.flush();

        QByteArray raw = dev.data();
        QCOMPARE(raw.size(), 1);
        QCOMPARE(static_cast<uint8_t>(raw[0]), 0xABu);

        BitBuffer rb(raw);
        auto v = rb.readBits(8, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        QVERIFY_RESULT(v);
        QCOMPARE(*v, 0xABu);
    }

    void writeRead_partial_bits()
    {
        // Write 3 bits + 5 bits = 8 bits total
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(0x5, 3, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst); // 101
        wb.writeBits(0x1A, 5, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst); // 11010
        wb.flush();

        QByteArray raw = dev.data();
        QCOMPARE(raw.size(), 1);

        BitBuffer rb(raw);
        auto a = rb.readBits(3, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        auto b = rb.readBits(5, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        QVERIFY_RESULT(a); QVERIFY_RESULT(b);
        QCOMPARE(*a, 0x5u);
        QCOMPARE(*b, 0x1Au);
    }

    void writeRead_32bit_le()
    {
        const quint64 val = 0xDEADBEEFu;
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(val, 32, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        wb.flush();

        QByteArray raw = dev.data();
        QCOMPARE(raw.size(), 4);
        // LE: EF BE AD DE
        QCOMPARE(static_cast<uint8_t>(raw[0]), 0xEFu);
        QCOMPARE(static_cast<uint8_t>(raw[1]), 0xBEu);
        QCOMPARE(static_cast<uint8_t>(raw[2]), 0xADu);
        QCOMPARE(static_cast<uint8_t>(raw[3]), 0xDEu);

        BitBuffer rb(raw);
        auto got = rb.readBits(32, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        QVERIFY_RESULT(got);
        QCOMPARE(*got, val);
    }

    void writeRead_32bit_be()
    {
        const quint64 val = 0xDEADBEEFu;
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(val, 32, ByteOrder::BigEndian, BitOrder::MostSignificantFirst);
        wb.flush();

        QByteArray raw = dev.data();
        QCOMPARE(raw.size(), 4);
        // BE MSB-first: DE AD BE EF
        QCOMPARE(static_cast<uint8_t>(raw[0]), 0xDEu);
        QCOMPARE(static_cast<uint8_t>(raw[1]), 0xADu);
        QCOMPARE(static_cast<uint8_t>(raw[2]), 0xBEu);
        QCOMPARE(static_cast<uint8_t>(raw[3]), 0xEFu);

        BitBuffer rb(raw);
        auto got = rb.readBits(32, ByteOrder::BigEndian, BitOrder::MostSignificantFirst);
        QVERIFY_RESULT(got);
        QCOMPARE(*got, val);
    }

    void alignment()
    {
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(1, 1, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        QCOMPARE(wb.writeBitOffset(), 1);
        wb.alignWriteTo(8);
        QCOMPARE(wb.writeBitOffset(), 8);
        wb.writeBits(0xFF, 8, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        wb.flush();

        QByteArray raw = dev.data();
        QCOMPARE(raw.size(), 2);
        QCOMPARE(static_cast<uint8_t>(raw[0]), 0x01u); // 1 in bit 0, rest 0
        QCOMPARE(static_cast<uint8_t>(raw[1]), 0xFFu);
    }

    void patchBits()
    {
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        // Write placeholder
        wb.writeBits(0, 32, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        qint64 patchOffset = 0;
        wb.writeBits(0xCAFE, 16, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);

        // Patch the first 32 bits
        wb.patchBits(patchOffset, 0x12345678u, 32, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        wb.flush();

        QByteArray raw = dev.data();
        BitBuffer rb(raw);
        auto patched  = rb.readBits(32, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        auto trailing = rb.readBits(16, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        QVERIFY_RESULT(patched); QVERIFY_RESULT(trailing);
        QCOMPARE(*patched,  0x12345678u);
        QCOMPARE(*trailing, 0xCAFEu);
    }

    void roundtrip_64bit()
    {
        const quint64 val = Q_UINT64_C(0xFEDCBA9876543210);
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        wb.writeBits(val, 64, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        wb.flush();

        BitBuffer rb(dev.data());
        auto got = rb.readBits(64, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        QVERIFY_RESULT(got);
        QCOMPARE(*got, val);
    }
};

QTEST_APPLESS_MAIN(tst_BitBuffer)
#include "tst_bitbuffer.moc"
