// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <commontraceformat/binary/packetreader.h>
#include <commontraceformat/binary/packetwriter.h>
#include <commontraceformat/metadata/metadatareader.h>
#include <commontraceformat/schema/compoundfieldclasses.h>
#include <commontraceformat/schema/scalarfieldclasses.h>

#include <utils/result.h>

#include <QBuffer>
#include <QtTest>

using namespace CommonTraceFormat;
using namespace Qt::StringLiterals;

static DataStreamClass makeTestDsc()
{
    DataStreamClass dsc;
    dsc.id   = 0;
    dsc.name = u"test"_s;

    auto hdr = std::make_shared<StructureFC>();
    auto idField = std::make_shared<FixedLengthUIntFC>();
    idField->length = 64;
    idField->roles  = {UIntRole::EventRecordClassId};
    hdr->members.append({u"id"_s, std::move(idField), {}});
    dsc.eventRecordHeaderFieldClass = std::move(hdr);

    {
        EventRecordClass erc;
        erc.id   = 0;
        erc.name = u"value_event"_s;
        auto payload = std::make_shared<StructureFC>();
        auto vf = std::make_shared<FixedLengthUIntFC>();
        vf->length = 32;
        payload->members.append({u"value"_s, std::move(vf), {}});
        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
    }

    {
        EventRecordClass erc;
        erc.id   = 1;
        erc.name = u"flag_event"_s;
        auto payload = std::make_shared<StructureFC>();
        auto ff = std::make_shared<FixedLengthUIntFC>();
        ff->length = 8;
        payload->members.append({u"flag"_s, std::move(ff), {}});
        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
    }

    return dsc;
}

// DSC with a packet context (total/content length + 64-bit begin clock) and an
// event header carrying an `id` and a partial (32-bit) default-clock-timestamp.
static DataStreamClass makeClockDsc(int headerTsBits)
{
    DataStreamClass dsc;
    dsc.id   = 0;
    dsc.name = u"clock"_s;
    dsc.defaultClockClassName = u"clk"_s;

    auto ctx = std::make_shared<StructureFC>();
    auto total = std::make_shared<FixedLengthUIntFC>();
    total->length = 32; total->roles = {UIntRole::PacketTotalLength};
    ctx->members.append({u"packet_size"_s, std::move(total), {}});
    auto content = std::make_shared<FixedLengthUIntFC>();
    content->length = 32; content->roles = {UIntRole::PacketContentLength};
    ctx->members.append({u"content_size"_s, std::move(content), {}});
    auto begin = std::make_shared<FixedLengthUIntFC>();
    begin->length = 64; begin->roles = {UIntRole::DefaultClockTimestamp};
    ctx->members.append({u"timestamp_begin"_s, std::move(begin), {}});
    dsc.packetContextFieldClass = std::move(ctx);

    auto hdr = std::make_shared<StructureFC>();
    auto idField = std::make_shared<FixedLengthUIntFC>();
    idField->length = 8; idField->roles = {UIntRole::EventRecordClassId};
    hdr->members.append({u"id"_s, std::move(idField), {}});
    auto tsField = std::make_shared<FixedLengthUIntFC>();
    tsField->length = headerTsBits; tsField->roles = {UIntRole::DefaultClockTimestamp};
    hdr->members.append({u"timestamp"_s, std::move(tsField), {}});
    dsc.eventRecordHeaderFieldClass = std::move(hdr);

    EventRecordClass erc;
    erc.id = 0; erc.name = u"e"_s;
    auto payload = std::make_shared<StructureFC>();
    auto vf = std::make_shared<FixedLengthUIntFC>();
    vf->length = 8;
    payload->members.append({u"v"_s, std::move(vf), {}});
    erc.payloadFieldClass = std::move(payload);
    dsc.eventRecordClasses.append(std::move(erc));
    return dsc;
}

class tst_PacketRoundtrip : public QObject
{
    Q_OBJECT
private slots:
    // Full 64-bit timestamps are returned verbatim.
    void timestampFull64()
    {
        DataStreamClass dsc = makeClockDsc(64);

        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        PacketWriter pw(&dev, dsc);
        StructureValue ctx;
        ctx.set(u"timestamp_begin"_s, quint64(0));
        pw.beginPacket(ctx);
        StructureValue p; p.set(u"v"_s, quint64(1));
        pw.writeEventRecord(0, p, {}, {}, quint64(0x1122334455667788ULL));
        pw.writeEventRecord(0, p, {}, {}, quint64(0x1122334455667799ULL));
        QVERIFY_RESULT(pw.finalize());

        dev.open(QIODevice::ReadOnly);
        PacketReader pr(&dev, dsc);
        QVERIFY(pr.openPacket());
        auto e0 = pr.nextEvent();
        QVERIFY_RESULT(e0);
        QCOMPARE(e0->timestamp, quint64(0x1122334455667788ULL));
        auto e1 = pr.nextEvent();
        QVERIFY_RESULT(e1);
        QCOMPARE(e1->timestamp, quint64(0x1122334455667799ULL));
    }

    // A 32-bit partial timestamp is extended against the 64-bit packet-begin
    // clock value, and low-bit wraparound carries into the high bits (spec 6.3).
    void timestampPartialWraparound()
    {
        DataStreamClass dsc = makeClockDsc(32);

        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        PacketWriter pw(&dev, dsc);
        StructureValue ctx;
        ctx.set(u"timestamp_begin"_s, quint64(0x100000000ULL)); // high bits = 1<<32
        pw.beginPacket(ctx);
        StructureValue p; p.set(u"v"_s, quint64(0));
        pw.writeEventRecord(0, p, {}, {}, quint64(0x00000005ULL));
        pw.writeEventRecord(0, p, {}, {}, quint64(0x00000010ULL));
        pw.writeEventRecord(0, p, {}, {}, quint64(0x00000001ULL)); // wraps: < previous low
        QVERIFY_RESULT(pw.finalize());

        dev.open(QIODevice::ReadOnly);
        PacketReader pr(&dev, dsc);
        QVERIFY(pr.openPacket());

        auto e0 = pr.nextEvent();
        QVERIFY_RESULT(e0);
        QCOMPARE(e0->timestamp, quint64(0x100000005ULL));
        auto e1 = pr.nextEvent();
        QVERIFY_RESULT(e1);
        QCOMPARE(e1->timestamp, quint64(0x100000010ULL));
        auto e2 = pr.nextEvent();
        QVERIFY_RESULT(e2);
        QCOMPARE(e2->timestamp, quint64(0x200000001ULL)); // high bits incremented
    }

    void writeReadTwoEventClasses()
    {
        DataStreamClass dsc = makeTestDsc();

        QBuffer dev;
        dev.open(QIODevice::WriteOnly);
        PacketWriter pw(&dev, dsc);
        pw.beginPacket();

        StructureValue p0;
        p0.set(u"value"_s, quint64(0xCAFEBABEu));
        pw.writeEventRecord(0, p0);

        StructureValue p1;
        p1.set(u"flag"_s, quint64(0xFF));
        pw.writeEventRecord(1, p1);

        StructureValue p2;
        p2.set(u"value"_s, quint64(42));
        pw.writeEventRecord(0, p2);

        QVERIFY_RESULT(pw.finalize());

        dev.open(QIODevice::ReadOnly);
        PacketReader pr(&dev, dsc);
        QVERIFY(pr.openPacket());

        auto e0 = pr.nextEvent();
        QVERIFY_RESULT(e0);
        QCOMPARE(e0->eventClassId, quint64(0));
        QCOMPARE(std::get<quint64>(*e0->payload.get(u"value"_s)), quint64(0xCAFEBABEu));

        auto e1 = pr.nextEvent();
        QVERIFY_RESULT(e1);
        QCOMPARE(e1->eventClassId, quint64(1));
        QCOMPARE(std::get<quint64>(*e1->payload.get(u"flag"_s)), quint64(0xFF));

        auto e2 = pr.nextEvent();
        QVERIFY_RESULT(e2);
        QCOMPARE(e2->eventClassId, quint64(0));
        QCOMPARE(std::get<quint64>(*e2->payload.get(u"value"_s)), quint64(42));
    }
};

QTEST_APPLESS_MAIN(tst_PacketRoundtrip)
#include "tst_packetroundtrip.moc"
