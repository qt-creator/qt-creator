// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <commontraceformat/metadata/metadatareader.h>
#include <commontraceformat/metadata/metadatawriter.h>
#include <commontraceformat/schema/clockclass.h>
#include <commontraceformat/schema/compoundfieldclasses.h>
#include <commontraceformat/schema/scalarfieldclasses.h>
#include <utils/result.h>
#include <QBuffer>
#include <QtTest>

using namespace CommonTraceFormat;
using namespace Qt::StringLiterals;

class tst_MetadataRoundtrip : public QObject
{
    Q_OBJECT
private slots:
    void preamble_noUuid()
    {
        Schema s;
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        MetadataWriter w(&buf);
        QVERIFY_RESULT(w.write(s));

        buf.open(QIODevice::ReadOnly);
        MetadataReader r(&buf);
        auto result = r.read();
        QVERIFY_RESULT(result);
        QVERIFY(!result->uuid.has_value());
    }

    void preamble_withUuid()
    {
        Schema s;
        s.uuid = QUuid::createUuid();

        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        MetadataWriter w(&buf);
        QVERIFY_RESULT(w.write(s));

        buf.open(QIODevice::ReadOnly);
        MetadataReader r(&buf);
        auto result = r.read();
        QVERIFY_RESULT(result);
        QVERIFY(result->uuid.has_value());
        QCOMPARE(*result->uuid, *s.uuid);
    }

    void clockClass_unixEpoch()
    {
        Schema s;
        ClockClass cc;
        cc.name = u"monotonic"_s;
        cc.frequency = 1000000000ULL;
        cc.origin.isUnixEpoch = true;
        s.clockClasses.append(cc);

        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        MetadataWriter w(&buf);
        QVERIFY_RESULT(w.write(s));

        buf.open(QIODevice::ReadOnly);
        MetadataReader r(&buf);
        auto result = r.read();
        QVERIFY_RESULT(result);
        QCOMPARE(result->clockClasses.size(), 1);
        QCOMPARE(result->clockClasses[0].name, u"monotonic"_s);
        QVERIFY(result->clockClasses[0].origin.isUnixEpoch);
    }

    void dataStreamClass_withEvents()
    {
        Schema s;

        // Payload: struct { uint32 ts; }
        auto payload = std::make_shared<StructureFC>();
        auto tsField = std::make_shared<FixedLengthUIntFC>();
        tsField->length = 32;
        payload->members.append({u"ts"_s, std::move(tsField), {}});

        DataStreamClass dsc;
        dsc.id   = 0;
        dsc.name = u"main"_s;

        EventRecordClass erc;
        erc.id   = 0;
        erc.name = u"my_event"_s;
        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
        s.dataStreamClasses.append(std::move(dsc));

        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        MetadataWriter w(&buf);
        QVERIFY_RESULT(w.write(s));

        buf.open(QIODevice::ReadOnly);
        MetadataReader r(&buf);
        auto result = r.read();
        QVERIFY_RESULT(result);
        QCOMPARE(result->dataStreamClasses.size(), 1);
        QCOMPARE(result->dataStreamClasses[0].eventRecordClasses.size(), 1);
        QCOMPARE(result->dataStreamClasses[0].eventRecordClasses[0].name, u"my_event"_s);
    }

    // Data-stream-class and event-record-class ids above 2^53 must survive a
    // write→read round-trip exactly. Reading them via toDouble() would round to
    // the nearest representable double and corrupt the id.
    void dataStreamClass_largeId()
    {
        const quint64 dscId = (quint64(1) << 53) + 1; // 9007199254740993, not exactly a double
        const quint64 ercId = (quint64(1) << 53) + 3;

        Schema s;
        DataStreamClass dsc;
        dsc.id = dscId;
        EventRecordClass erc;
        erc.id = ercId;
        erc.name = u"big"_s;
        dsc.eventRecordClasses.append(std::move(erc));
        s.dataStreamClasses.append(std::move(dsc));

        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        MetadataWriter w(&buf);
        QVERIFY_RESULT(w.write(s));

        buf.open(QIODevice::ReadOnly);
        MetadataReader r(&buf);
        auto result = r.read();
        QVERIFY_RESULT(result);
        QCOMPARE(result->dataStreamClasses.size(), 1);
        QCOMPARE(result->dataStreamClasses[0].id, dscId);
        QCOMPARE(result->dataStreamClasses[0].eventRecordClasses.size(), 1);
        QCOMPARE(result->dataStreamClasses[0].eventRecordClasses[0].id, ercId);
    }

    void rfcFraming()
    {
        // Verify every fragment starts with RS (0x1E) and ends with LF (0x0A)
        Schema s;
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        MetadataWriter w(&buf);
        QVERIFY_RESULT(w.write(s));

        QByteArray raw = buf.data();
        int pos = 0;
        while (pos < raw.size()) {
            QVERIFY2(static_cast<uint8_t>(raw[pos]) == 0x1E,
                     "Fragment must start with RS (0x1E)");
            int lf = raw.indexOf('\n', pos + 1);
            QVERIFY2(lf >= 0, "Fragment must end with LF");
            pos = lf + 1;
        }
    }
};

QTEST_APPLESS_MAIN(tst_MetadataRoundtrip)
#include "tst_metadataroundtrip.moc"
