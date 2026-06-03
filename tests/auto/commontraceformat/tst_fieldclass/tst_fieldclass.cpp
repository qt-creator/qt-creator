// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <commontraceformat/binary/bitbuffer.h>
#include <commontraceformat/binary/fieldreader.h>
#include <commontraceformat/binary/fieldwriter.h>
#include <commontraceformat/metadata/metadatareader.h>
#include <commontraceformat/schema/blobfieldclasses.h>
#include <commontraceformat/schema/compoundfieldclasses.h>
#include <commontraceformat/schema/scalarfieldclasses.h>
#include <commontraceformat/schema/stringfieldclasses.h>

#include <utils/result.h>

#include <QBuffer>
#include <QtTest>

using namespace CommonTraceFormat;
using namespace Qt::StringLiterals;

class tst_FieldClass : public QObject
{
    Q_OBJECT

    static FieldValue roundtrip(const FieldClass &fc, const FieldValue &in)
    {
        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        FieldWriter(wb).write(fc, in);
        wb.flush();

        BitBuffer rb(dev.data());
        auto result = FieldReader(rb).read(fc);
        if (!result) return {};
        return *result;
    }

private slots:
    void fixedUInt32()
    {
        FixedLengthUIntFC fc;
        fc.length = 32;
        auto out = roundtrip(fc, quint64(0xDEADBEEF));
        QCOMPARE(std::get<quint64>(out), quint64(0xDEADBEEF));
    }

    void fixedSInt8_negative()
    {
        FixedLengthSIntFC fc;
        fc.length = 8;
        auto out = roundtrip(fc, qint64(-1));
        QCOMPARE(std::get<qint64>(out), qint64(-1));
    }

    void fixedBool()
    {
        FixedLengthBoolFC fc;
        fc.length = 8;
        QCOMPARE(std::get<bool>(roundtrip(fc, bool(true))),  true);
        QCOMPARE(std::get<bool>(roundtrip(fc, bool(false))), false);
    }

    void fixedFloat64()
    {
        FixedLengthFloatFC fc;
        fc.length = 64;
        double v = 3.14159265358979;
        auto out = roundtrip(fc, v);
        QCOMPARE(std::get<double>(out), v);
    }

    void variableLengthUInt()
    {
        VariableLengthUIntFC fc;
        auto out = roundtrip(fc, quint64(300));
        QCOMPARE(std::get<quint64>(out), quint64(300));
    }

    void variableLengthSInt_negative()
    {
        VariableLengthSIntFC fc;
        auto out = roundtrip(fc, qint64(-65));
        QCOMPARE(std::get<qint64>(out), qint64(-65));
    }

    void nullTerminatedString()
    {
        NullTerminatedStringFC fc;
        auto out = roundtrip(fc, QString(u"hello"_s));
        QCOMPARE(std::get<QString>(out), u"hello"_s);
    }

    void staticLengthString()
    {
        StaticLengthStringFC fc;
        fc.length = 8;
        auto out = roundtrip(fc, QString(u"hi"_s));
        QCOMPARE(std::get<QString>(out), u"hi"_s);
    }

    void staticLengthBlob()
    {
        StaticLengthBlobFC fc;
        fc.length = 4;
        QByteArray data("\x01\x02\x03\x04", 4);
        auto out = roundtrip(fc, data);
        QCOMPARE(std::get<QByteArray>(out), data);
    }

    void structure_twoFields()
    {
        auto fc = std::make_shared<StructureFC>();

        auto u32 = std::make_shared<FixedLengthUIntFC>();
        u32->length = 32;
        fc->members.append({u"x"_s, std::move(u32), {}});

        auto s8 = std::make_shared<FixedLengthSIntFC>();
        s8->length = 8;
        fc->members.append({u"y"_s, std::move(s8), {}});

        StructureValue sv;
        sv.set(u"x"_s, quint64(42));
        sv.set(u"y"_s, qint64(-7));

        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        FieldWriter(wb).write(*fc, makeStructureValue(sv));
        wb.flush();

        BitBuffer rb(dev.data());
        auto result = FieldReader(rb).read(*fc);
        QVERIFY_RESULT(result);
        QVERIFY(isStructure(*result));

        const auto &out = asStructure(*result);
        QCOMPARE(std::get<quint64>(*out.get(u"x"_s)), quint64(42));
        QCOMPARE(std::get<qint64>(*out.get(u"y"_s)),  qint64(-7));
    }

    void staticArray()
    {
        StaticLengthArrayFC fc;
        fc.length = 3;
        auto elem = std::make_shared<FixedLengthUIntFC>();
        elem->length = 8;
        fc.elementFieldClass = std::move(elem);

        ArrayValue av;
        av.elements = {quint64(10), quint64(20), quint64(30)};

        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        BitBuffer wb(&dev);
        FieldWriter(wb).write(fc, makeArrayValue(av));
        wb.flush();

        BitBuffer rb(dev.data());
        auto result = FieldReader(rb).read(fc);
        QVERIFY_RESULT(result);
        QVERIFY(isArray(*result));

        const auto &out = asArray(*result);
        QCOMPARE(std::get<quint64>(out.elements[0]), quint64(10));
        QCOMPARE(std::get<quint64>(out.elements[2]), quint64(30));
    }
};

QTEST_APPLESS_MAIN(tst_FieldClass)
#include "tst_fieldclass.moc"
