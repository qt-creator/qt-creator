// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <commontraceformat/binary/fieldvalue.h>
#include <commontraceformat/metadata/metadatareader.h>
#include <commontraceformat/schema/clockclass.h>
#include <commontraceformat/schema/compoundfieldclasses.h>
#include <commontraceformat/schema/scalarfieldclasses.h>
#include <commontraceformat/schema/stringfieldclasses.h>
#include <commontraceformat/stream/datastreamwriter.h>
#include <commontraceformat/stream/tracedirectory.h>
#include <commontraceformat/stream/tracereader.h>
#include <commontraceformat/stream/tracewriter.h>

#include <utils/result.h>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <optional>

using namespace CommonTraceFormat;
using namespace Qt::StringLiterals;

#ifndef TESTDATA_DIR
#define TESTDATA_DIR ""
#endif

// --- Structural FieldValue comparison -------------------------------------
// A trace re-written through TraceWriter does not reproduce the original bytes
// (the writer picks its own packet layout and regenerates packet/event
// headers), so the roundtrip is checked at the value level: the events read
// back must carry the same class id, timestamp and payload/context values.

static bool fieldEqual(const FieldValue &a, const FieldValue &b);

static bool structEqual(const StructureValue &a, const StructureValue &b)
{
    if (a.members.size() != b.members.size())
        return false;
    for (auto it = a.members.constBegin(); it != a.members.constEnd(); ++it) {
        const FieldValue *bv = b.get(it.key());
        if (!bv || !fieldEqual(it.value(), *bv))
            return false;
    }
    return true;
}

static bool fieldEqual(const FieldValue &a, const FieldValue &b)
{
    if (a.index() != b.index())
        return false;

    if (std::holds_alternative<std::shared_ptr<StructureValue>>(a))
        return structEqual(asStructure(a), asStructure(b));

    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(a)) {
        const ArrayValue &aa = asArray(a);
        const ArrayValue &ba = asArray(b);
        if (aa.elements.size() != ba.elements.size())
            return false;
        for (qsizetype i = 0; i < aa.elements.size(); ++i)
            if (!fieldEqual(aa.elements[i], ba.elements[i]))
                return false;
        return true;
    }

    if (std::holds_alternative<std::shared_ptr<VariantValue>>(a)) {
        const VariantValue &av = *std::get<std::shared_ptr<VariantValue>>(a);
        const VariantValue &bv = *std::get<std::shared_ptr<VariantValue>>(b);
        if (av.selectedIndex != bv.selectedIndex)
            return false;
        if (bool(av.value) != bool(bv.value))
            return false;
        return !av.value || fieldEqual(*av.value, *bv.value);
    }

    return a == b; // monostate, bool, quint64, qint64, double, QString, QByteArray
}

static Schema makeTestSchema()
{
    Schema s;

    ClockClass cc;
    cc.name = u"mono"_s;
    cc.frequency = 1000000000ULL;
    cc.origin.isUnixEpoch = true;
    s.clockClasses.append(cc);

    DataStreamClass dsc;
    dsc.id   = 0;
    dsc.name = u"main"_s;
    dsc.defaultClockClassName = u"mono"_s;

    // Event record header: { uint64 id }
    {
        auto hdr = std::make_shared<StructureFC>();
        auto idField = std::make_shared<FixedLengthUIntFC>();
        idField->length = 64;
        idField->roles  = {UIntRole::EventRecordClassId};
        hdr->members.append({u"id"_s, std::move(idField), {}});
        dsc.eventRecordHeaderFieldClass = std::move(hdr);
    }

    // Event class 0: payload { uint32 value, null-terminated string name }
    {
        EventRecordClass erc;
        erc.id   = 0;
        erc.name = u"data"_s;
        auto payload = std::make_shared<StructureFC>();

        auto vf = std::make_shared<FixedLengthUIntFC>();
        vf->length = 32;
        payload->members.append({u"value"_s, std::move(vf), {}});

        auto nf = std::make_shared<NullTerminatedStringFC>();
        payload->members.append({u"name"_s, std::move(nf), {}});

        erc.payloadFieldClass = std::move(payload);
        dsc.eventRecordClasses.append(std::move(erc));
    }

    s.dataStreamClasses.append(std::move(dsc));
    return s;
}

class tst_TraceRoundtrip : public QObject
{
    Q_OBJECT
private slots:
    void writeRead_singleStream()
    {
        Schema schema = makeTestSchema();

        QBuffer metaBuf;
        metaBuf.open(QIODevice::WriteOnly);

        QBuffer dataBuf;
        dataBuf.open(QIODevice::WriteOnly);

        // Write trace
        {
            auto twResult = TraceWriter::create(schema, &metaBuf);
            QVERIFY_RESULT(twResult);
            TraceWriter &tw = *twResult;
            DataStreamWriter *ds = tw.openStream(u"main"_s, &dataBuf);
            QVERIFY(ds);

            for (int i = 0; i < 5; ++i) {
                StructureValue payload;
                payload.set(u"value"_s, quint64(i * 10));
                payload.set(u"name"_s,  QString(u"event_%1"_s).arg(i));
                QVERIFY_RESULT(ds->writeEvent(0, payload));
            }
            QVERIFY_RESULT(tw.close());
        }

        // Read metadata
        metaBuf.open(QIODevice::ReadOnly);
        auto readerResult = TraceReader::open(&metaBuf);
        QVERIFY_RESULT(readerResult);
        TraceReader &reader = *readerResult;

        QCOMPARE(reader.schema().dataStreamClasses.size(), 1);
        const DataStreamClass &dsc = reader.schema().dataStreamClasses[0];

        // Read data stream
        dataBuf.open(QIODevice::ReadOnly);
        DataStreamReader *ds = reader.openStream(dsc, &dataBuf);
        QVERIFY(ds);

        for (int i = 0; i < 5; ++i) {
            auto rec = ds->nextEvent();
            QVERIFY_RESULT(rec);
            QCOMPARE(rec->eventClassId, quint64(0));

            const FieldValue *vf = rec->payload.get(u"value"_s);
            QVERIFY(vf);
            QCOMPARE(std::get<quint64>(*vf), quint64(i * 10));

            const FieldValue *nf = rec->payload.get(u"name"_s);
            QVERIFY(nf);
            QCOMPARE(std::get<QString>(*nf), QString(u"event_%1"_s).arg(i));
        }

        // Next call should hit end of stream
        QVERIFY(ds->atEnd() || !ds->nextEvent().has_value());

        // A clean, complete stream reports no decode error.
        QVERIFY(!ds->readError().has_value());
    }

    // A DataStreamReader caches raw pointers into its TraceReader's schema, so
    // moving the TraceReader (and destroying the moved-from source) must not
    // invalidate an already-opened stream. Guards against the schema being held
    // by value, which would relocate it on move and dangle the child readers.
    void readSurvivesReaderMove()
    {
        Schema schema = makeTestSchema();

        QBuffer metaBuf;
        metaBuf.open(QIODevice::WriteOnly);
        QBuffer dataBuf;
        dataBuf.open(QIODevice::WriteOnly);
        {
            auto twResult = TraceWriter::create(schema, &metaBuf);
            QVERIFY_RESULT(twResult);
            DataStreamWriter *dw = twResult->openStream(u"main"_s, &dataBuf);
            QVERIFY(dw);
            for (int i = 0; i < 5; ++i) {
                StructureValue payload;
                payload.set(u"value"_s, quint64(i * 10));
                payload.set(u"name"_s, QString(u"event_%1"_s).arg(i));
                QVERIFY_RESULT(dw->writeEvent(0, payload));
            }
            QVERIFY_RESULT(twResult->close());
        }

        metaBuf.open(QIODevice::ReadOnly);
        dataBuf.open(QIODevice::ReadOnly);

        auto srcResult = TraceReader::open(&metaBuf);
        QVERIFY_RESULT(srcResult);

        DataStreamReader *ds = nullptr;
        std::optional<TraceReader> moved;
        {
            TraceReader src = std::move(*srcResult);
            const DataStreamClass &dsc = src.schema().dataStreamClasses[0];
            ds = src.openStream(dsc, &dataBuf);
            QVERIFY(ds);

            // Read the first two events, then move the reader and let `src` die.
            for (int i = 0; i < 2; ++i) {
                auto rec = ds->nextEvent();
                QVERIFY_RESULT(rec);
                QCOMPARE(std::get<quint64>(*rec->payload.get(u"value"_s)), quint64(i * 10));
            }
            moved.emplace(std::move(src));
        }

        // `src` is destroyed; the remaining events must still decode through the
        // pointer obtained before the move (it now lives inside `moved`).
        for (int i = 2; i < 5; ++i) {
            auto rec = ds->nextEvent();
            QVERIFY_RESULT(rec);
            QCOMPARE(std::get<quint64>(*rec->payload.get(u"value"_s)), quint64(i * 10));
        }
        QVERIFY(ds->atEnd() || !ds->nextEvent().has_value());
    }

    // Loads each on-disk v2 trace, reads all of its events, writes the trace
    // back out to a fresh directory through TraceWriter/DataStreamWriter, then
    // re-reads it and verifies the events survived the roundtrip unchanged.
    void roundtripV2_data()
    {
        QTest::addColumn<QString>("dir");

        if (QStringLiteral(TESTDATA_DIR).isEmpty())
            return;

        const QString base = QStringLiteral(TESTDATA_DIR) + u"/v2"_s;
        // fail/ traces are meant to be rejected, so they are not roundtripped.
        for (const QString &group : {u"succeed"_s, u"intersection"_s}) {
            QDir d(base + u"/"_s + group);
            const QStringList subdirs =
                d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const QString &sub : subdirs) {
                const QString row = group + u"/"_s + sub;
                QTest::newRow(qPrintable(row)) << d.filePath(sub);
            }
        }
    }

    void roundtripV2()
    {
        QFETCH(QString, dir);
        if (QStringLiteral(TESTDATA_DIR).isEmpty())
            QSKIP("TESTDATA_DIR not set");

        constexpr int kEventLimit = 100000;

        auto tdResult = TraceDirectory::open(dir);
        QVERIFY_RESULT(tdResult);
        const TraceDirectory &td = *tdResult;
        QVERIFY(!td.traces().isEmpty());

        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        int traceIndex = 0;
        for (const TraceDirectory::Trace &trace : td.traces()) {
            const Schema &schema = *trace.schema;

            // 1. Read every event of every stream of the original trace.
            struct StreamEvents {
                quint64             dscId = 0;
                QList<EventRecord>  events;
            };
            QList<StreamEvents> originals;
            for (const TraceDirectory::Stream &st : trace.streams) {
                if (!st.dsc)
                    continue;
                StreamEvents se;
                se.dscId = st.dsc->id;
                DataStreamReader *r = st.reader;
                while (!r->atEnd()) {
                    auto evt = r->nextEvent();
                    if (!evt) {
                        // A failed read at end-of-stream is clean EOF; anything
                        // else is a genuine read error.
                        QVERIFY2(r->atEnd(),
                                 qPrintable(u"read error in %1"_s.arg(dir)));
                        break;
                    }
                    se.events.append(std::move(*evt));
                    QVERIFY2(se.events.size() <= kEventLimit,
                             "event count exceeded limit");
                }
                originals.append(std::move(se));
            }

            // 2. Write the trace back out to disk.
            const QString outDir = tmp.filePath(u"trace%1"_s.arg(traceIndex++));
            QVERIFY(QDir().mkpath(outDir));

            QFile metaFile(outDir + u"/metadata"_s);
            QVERIFY(metaFile.open(QIODevice::WriteOnly));

            QList<QFile *> dataFiles;
            {
                auto twResult = TraceWriter::create(schema, &metaFile);
                QVERIFY_RESULT(twResult);
                TraceWriter &tw = *twResult;

                for (int i = 0; i < originals.size(); ++i) {
                    auto *df = new QFile(outDir + u"/stream%1"_s.arg(i));
                    dataFiles.append(df);
                    QVERIFY(df->open(QIODevice::WriteOnly));

                    DataStreamWriter *w = tw.openStreamById(originals[i].dscId, df);
                    QVERIFY2(w, qPrintable(
                        u"no data stream class with id %1"_s.arg(originals[i].dscId)));

                    for (const EventRecord &e : std::as_const(originals[i].events)) {
                        QVERIFY_RESULT(w->writeEvent(e.eventClassId, e.payload,
                                                     e.specificContext, e.commonContext,
                                                     e.timestamp));
                    }
                }
                QVERIFY_RESULT(tw.close()); // flushes streams
            }
            metaFile.close();
            for (QFile *df : dataFiles)
                df->close();

            // 3. Re-read the written-back trace and compare event by event.
            QFile metaIn(metaFile.fileName());
            QVERIFY(metaIn.open(QIODevice::ReadOnly));
            auto readerResult = TraceReader::open(&metaIn);
            QVERIFY_RESULT(readerResult);
            TraceReader &reader = *readerResult;

            QCOMPARE(reader.schema().dataStreamClasses.size(),
                     schema.dataStreamClasses.size());

            for (int i = 0; i < originals.size(); ++i) {
                const DataStreamClass *dsc2 =
                    reader.schema().findDataStreamClass(originals[i].dscId);
                QVERIFY(dsc2);

                QVERIFY(dataFiles[i]->open(QIODevice::ReadOnly));
                DataStreamReader *r2 = reader.openStream(*dsc2, dataFiles[i]);
                QVERIFY(r2);

                const QList<EventRecord> &expected = originals[i].events;
                for (int j = 0; j < expected.size(); ++j) {
                    QVERIFY2(!r2->atEnd(),
                             qPrintable(u"stream %1: fewer events than written"_s.arg(i)));
                    auto got = r2->nextEvent();
                    QVERIFY_RESULT(got);

                    const QString ctx =
                        u"%1 stream %2 event %3"_s.arg(dir).arg(i).arg(j);
                    QCOMPARE(got->eventClassId, expected[j].eventClassId);
                    QCOMPARE(got->timestamp, expected[j].timestamp);
                    QVERIFY2(structEqual(got->payload, expected[j].payload),
                             qPrintable(u"payload mismatch: "_s + ctx));
                    QVERIFY2(structEqual(got->commonContext, expected[j].commonContext),
                             qPrintable(u"common-context mismatch: "_s + ctx));
                    QVERIFY2(structEqual(got->specificContext, expected[j].specificContext),
                             qPrintable(u"specific-context mismatch: "_s + ctx));
                }

                // No extra events beyond what was written.
                if (!r2->atEnd()) {
                    auto extra = r2->nextEvent();
                    QVERIFY2(!extra.has_value(),
                             qPrintable(u"stream %1: more events than written"_s.arg(i)));
                }
            }
            metaIn.close();
            qDeleteAll(dataFiles);
        }
    }
};

QTEST_APPLESS_MAIN(tst_TraceRoundtrip)
#include "tst_traceroundtrip.moc"
