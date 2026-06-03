// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <commontraceformat/stream/datastreamreader.h>
#include <commontraceformat/stream/tracedirectory.h>

#include <utils/result.h>

#include <QtTest>

#include <initializer_list>

using namespace CommonTraceFormat;
using namespace Qt::StringLiterals;

#ifndef TESTDATA_DIR
#define TESTDATA_DIR ""
#endif

class tst_TraceData : public QObject
{
    Q_OBJECT
private slots:
    // Split per CTF version (2.0 / 1.8) and per corpus (fail / succeed). All
    // share the same body — runTraceData() — driven by the fetched columns.
    void fail_2_0_data();
    void fail_2_0() { runTraceData(); }
    void succeed_2_0_data();
    void succeed_2_0() { runTraceData(); }
    void fail_1_8_data();
    void fail_1_8() { runTraceData(); }
    void succeed_1_8_data();
    void succeed_1_8() { runTraceData(); }
    void deterministicOrdering_1_8_data();
    void deterministicOrdering_1_8() { runTraceData(); }
    void intersection_1_8_data();
    void intersection_1_8() { runTraceData(); }
    void live_1_8_data();
    void live_1_8() { runTraceData(); }
    void packetSeqNum_1_8_data();
    void packetSeqNum_1_8();

private:
    // Shared body: fetch a row and drive TraceDirectory::open + event reading.
    void runTraceData();
};

// expectMeta:      true = metadata should parse OK
// expectNoStream:  true = schema should have 0 DSCs (fail because no stream class defined)
// expectStreamFail:true = metadata OK but a read error is surfaced to the caller
//                  (atEnd() stays false) — distinct from a tolerated truncation
// expectReadError: true = some event decode failed (DataStreamReader::readError()
//                  set), whether surfaced (streamFail) or tolerated as a graceful
//                  end of stream (e.g. an lttng-crash dump)
static void addTraceColumns()
{
    QTest::addColumn<QString>("dir");
    QTest::addColumn<bool>("expectMetaOk");
    QTest::addColumn<bool>("expectNoDsc");
    QTest::addColumn<bool>("expectStreamFail");
    QTest::addColumn<bool>("expectReadError");
}

static void addRow(
    const char *rowName, const QString &path, bool metaOk, bool noDsc, bool streamFail,
    bool readError = false)
{
    QTest::newRow(rowName) << path << metaOk << noDsc << streamFail << readError;
}

// Add one expect-everything-OK row per directory name under `<base>/succeed/`.
static void addSucceedRows(const QString &base, std::initializer_list<const char *> names)
{
    for (const char *name : names) {
        const QString tag = u"succeed/"_s + QString::fromUtf8(name);
        addRow(tag.toUtf8().constData(), base + u"/succeed/"_s + QString::fromUtf8(name),
               true, false, false);
    }
}

// Returns the test-data root for a CTF version, or skips if TESTDATA_DIR unset.
static QString versionBase(const char *version)
{
    if (QStringLiteral(TESTDATA_DIR).isEmpty()) {
        QTest::qSkip("TESTDATA_DIR not set", __FILE__, __LINE__);
        return {};
    }
    return QStringLiteral(TESTDATA_DIR) + u"/"_s + QString::fromUtf8(version);
}

// ── CTF 2.0 ──────────────────────────────────────────────────────────────────
void tst_TraceData::fail_2_0_data()
{
    addTraceColumns();
    const QString base = versionBase("v2");
    if (base.isEmpty())
        return;

    // Degenerate-but-parseable traces: metadata is valid, but the schema has no
    // data stream class (noDsc), or event reading must fail (streamFail).
    addRow("meta-no-trace-cls-no-stream-cls",
           base + u"/fail/meta-no-trace-cls-no-stream-cls"_s, true, true, false);
    addRow("meta-no-stream-cls", base + u"/fail/meta-no-stream-cls"_s, true, true, false);
    addRow("empty-event-record", base + u"/fail/empty-event-record"_s,
           true, false, /*streamFail*/ true, /*readError*/ true);
}

void tst_TraceData::succeed_2_0_data()
{
    addTraceColumns();
    const QString base = versionBase("v2");
    if (base.isEmpty())
        return;

    addSucceedRows(base, {
        "meta-clk-cls-before-trace-cls", // metadata-only
        "no-packet-context",
        "fl-bm",
        "array-align-elem",
        "struct-array-align-elem",
        "ev-disc-no-ts-begin-end",
        "meta-ctx-sequence",
        "meta-variant-no-underscore",
        "meta-variant-one-underscore",
        "meta-variant-two-underscores",
        "meta-variant-same-with-underscore",
        "meta-variant-reserved-keywords",
        "smalltrace",
        "succeed1",
        "succeed2",
        "env-warning",
        "lttng-event-after-packet",
        "lttng-tracefile-rotation",
        "2packets",
        "barectf-event-before-packet",
        "debug-info",
        "sequence",
        "trace-with-index",
        "wk-heartbeat-u",
        "multi-domains",
        "session-rotation",
    });

    // A crash dump's final packet is truncated/has a bad timestamp: the reader
    // delivers the complete events and ends gracefully, but flags readError().
    addRow("succeed/lttng-crash", base + u"/succeed/lttng-crash"_s,
           true, false, /*streamFail*/ false, /*readError*/ true);

    addRow("intersection/3eventsintersect",
           base + u"/intersection/3eventsintersect"_s, true, false, false);
}

// ── CTF 1.8 ──────────────────────────────────────────────────────────────────
void tst_TraceData::fail_1_8_data()
{
    addTraceColumns();
    const QString base = versionBase("v1.8");
    if (base.isEmpty())
        return;

    // Metadata that must fail to parse: TraceDirectory::open() returns an error.
    for (const char *name : {
             "empty-metadata",                       // no preamble/content
             "fail1",                                // TSDL syntax error
             "metadata-syntax-error",                // TSDL syntax error
             "packet-based-metadata",                // references an undefined type
             "lttng-modules-2.0-pre1",               // malformed metadata packet header
             "invalid-variant-selector-field-class", // variant tag is not an enum
             "invalid-sequence-length-field-class",  // array length is not a uint
             "integer-range",                        // integer literal out of range
             "fail2",                                // NUL byte inside a string literal
         }) {
        const QString tag = u"fail/"_s + QString::fromUtf8(name);
        addRow(tag.toUtf8().constData(), base + u"/fail/"_s + QString::fromUtf8(name),
               /*metaOk*/ false, false, false);
    }

    // Corrupt packet/event data that the reader tolerates (graceful end of
    // stream, atEnd() true) while flagging readError() — same handling as the
    // lttng-crash dump in the succeed corpus. We assert the corruption is
    // reported. valid-events-then-invalid-events has two valid events followed
    // by one whose event-class id (255) is undefined; the truncated cases lose
    // their (single) packet to a bad size/header.
    for (const char *name : {
             "incomplete-packet-header",
             "invalid-packet-size",
             "smalltrace",
             "valid-events-then-invalid-events",
         }) {
        const QString n = QString::fromUtf8(name);
        const QString tag = u"fail/"_s + n;
        // valid-events-then-invalid-events keeps its trace in a `trace/` subdir.
        const QString rel = (n == u"valid-events-then-invalid-events"_s)
                                ? u"/fail/%1/trace"_s.arg(n)
                                : u"/fail/%1"_s.arg(n);
        addRow(tag.toUtf8().constData(), base + rel,
               /*metaOk*/ true, /*noDsc*/ false, /*streamFail*/ false, /*readError*/ true);
    }
}

void tst_TraceData::succeed_1_8_data()
{
    addTraceColumns();
    const QString base18 = versionBase("v1.8");
    if (base18.isEmpty())
        return;

    // Notable regressions guarded by this corpus:
    //  - lf/crlf-metadata, no-packet-context, …: a plaintext (non-packetized)
    //    `/* CTF 1.8 */` metadata file must be routed to the TSDL parser, not the
    //    CTF 2 reader (which failed with "Expected RS byte (0x1E)…").
    //  - smalltrace: event name as a bare identifier spelled like a keyword
    //    (`name = string;`) must parse.
    //  - 2packets: the stream-level `event.context` (per-event common context,
    //    e.g. `_vpid`) must be captured and consumed while reading.
    //  - ev-disc-no-ts-begin-end / meta-variant-*: big-endian fields must use
    //    most-significant-bit-first bit order (else values come out bit-reversed).
    addSucceedRows(base18, {
        "2packets",
        "array-align-elem",
        "barectf-event-before-packet",
        "crlf-metadata",
        "debug-info",
        "def-clk-freq",
        "env-warning",
        "ev-disc-no-ts-begin-end",
        "event-header-ts-packet-context-no-ts",
        "lf-metadata",
        "lttng-event-after-packet",
        "lttng-tracefile-rotation",
        "meta-ctx-sequence",
        "meta-trailing-byte",
        "meta-variant-no-underscore",
        "meta-variant-one-underscore",
        "meta-variant-two-underscores",
        "meta-variant-same-with-underscore",
        "meta-variant-reserved-keywords",
        "multi-domains",
        "no-packet-context",
        "sequence",
        "session-rotation",
        "smalltrace",
        "struct-array-align-elem",
        "succeed1",
        "succeed2",
        "succeed3",
        "succeed4",
        "trace-with-index",
        "warnings",
        "wk-heartbeat-u",
    });

    // Crash dump: complete events delivered, then a graceful end flagged via
    // readError() (the final packet's begin timestamp exceeds its end).
    addRow("succeed/lttng-crash", base18 + u"/succeed/lttng-crash"_s,
           true, false, /*streamFail*/ false, /*readError*/ true);
}

// Two near-identical traces plus a corrupted variant: b reads cleanly, while a
// and c carry a final event with an undefined class id (255) that the reader
// tolerates but flags via readError().
void tst_TraceData::deterministicOrdering_1_8_data()
{
    addTraceColumns();
    const QString base = versionBase("v1.8");
    if (base.isEmpty())
        return;

    const QString d = base + u"/deterministic-ordering/"_s;
    addRow("deterministic-ordering/b-not-corrupted", d + u"b-not-corrupted"_s, true, false, false);
    addRow("deterministic-ordering/a-corrupted", d + u"a-corrupted"_s,
           true, false, /*streamFail*/ false, /*readError*/ true);
    addRow("deterministic-ordering/c-corrupted", d + u"c-corrupted"_s,
           true, false, /*streamFail*/ false, /*readError*/ true);
}

// Trace-intersection corpus: each opens and reads cleanly. `nostream` carries
// only metadata (a data stream class but no data files), so it has no stream to
// read — the body returns after confirming the trace opened.
void tst_TraceData::intersection_1_8_data()
{
    addTraceColumns();
    const QString base = versionBase("v1.8");
    if (base.isEmpty())
        return;

    const QString i = base + u"/intersection/"_s;
    for (const char *name : {
             "3eventsintersect",
             "3eventsintersectreverse",
             "nointersect",
             "onestream",
             "nostream",
         }) {
        const QString n = QString::fromUtf8(name);
        addRow((u"intersection/"_s + n).toUtf8().constData(), i + n, true, false, false);
    }
}

// CTF-live captures stored on disk. split-metadata exercises metadata spread
// across several metadata packets and reads cleanly; invalid-metadata has a data
// packet whose content length exceeds its total length, tolerated via readError.
void tst_TraceData::live_1_8_data()
{
    addTraceColumns();
    const QString base = versionBase("v1.8");
    if (base.isEmpty())
        return;

    addRow("live/split-metadata", base + u"/live/split-metadata"_s, true, false, false);
    addRow("live/invalid-metadata", base + u"/live/invalid-metadata"_s,
           true, false, /*streamFail*/ false, /*readError*/ true);

    // Not covered: new-streams / stored-values.mctf are CTF-live `.mctf` capture
    // files, not on-disk directory traces (no `metadata` file), so TraceDirectory
    // cannot open them.
}

// Lost-packet accounting: a gap in per-stream packet sequence numbers (spec
// 4.2.1) is counted, summed here across the trace's streams. Expected totals are
// taken from the raw packet_seq_num values in the trace data.
void tst_TraceData::packetSeqNum_1_8_data()
{
    QTest::addColumn<QString>("dir");
    QTest::addColumn<quint64>("expectLostPackets");

    const QString base = versionBase("v1.8");
    if (base.isEmpty())
        return;

    auto add = [&](const char *name, quint64 lost) {
        QTest::newRow(name) << (base + u"/packet-seq-num/"_s + QString::fromUtf8(name)) << lost;
    };
    add("no-lost", 0);
    add("no-lost-not-starting-at-0", 0);
    add("2-lost-before-last", 2);
    // "2-streams" is the stream count; "lost-in-N" is how many of them have a gap.
    add("2-streams-lost-in-1", 2);  // gap in 1 stream: s0 [0,1,2,3,6]=2, s1 none
    add("2-streams-lost-in-2", 6);  // gaps in both streams: s0 =2, s1 [0,4,6]=4
    add("7-lost-between-2-with-index", 7);
}

void tst_TraceData::packetSeqNum_1_8()
{
    QFETCH(QString, dir);
    QFETCH(quint64, expectLostPackets);

    auto tdResult = TraceDirectory::open(dir);
    QVERIFY_RESULT(tdResult);
    QVERIFY(!tdResult->traces().isEmpty());

    quint64 totalLost = 0;
    for (const TraceDirectory::Stream &st : tdResult->traces().first().streams) {
        DataStreamReader *r = st.reader;
        if (!r)
            continue;
        int n = 0;
        while (!r->atEnd()) {
            if (!r->nextEvent())
                break;
            if (++n > 100000) {
                QFAIL("Event count exceeded limit — possible infinite loop");
                return;
            }
        }
        totalLost += r->lostPacketCount();
    }
    QCOMPARE(totalLost, expectLostPackets);
}

void tst_TraceData::runTraceData()
{
    QFETCH(QString, dir);
    QFETCH(bool, expectMetaOk);
    QFETCH(bool, expectNoDsc);
    QFETCH(bool, expectStreamFail);
    QFETCH(bool, expectReadError);

    const int eventLimit = 100000;

    // TraceDirectory handles metadata discovery (including domain/per-PID/
    // session-rotation subdirectories), rotated-tracefile concatenation, and
    // per-packet data-stream-class selection.
    auto tdResult = TraceDirectory::open(dir);

    // Metadata that must NOT parse: TraceDirectory::open() should return an error
    // (e.g. a syntax error or an invalid metadata packet).
    if (!expectMetaOk) {
        QVERIFY2(!tdResult, "Expected metadata open to fail but it succeeded");
        return;
    }

    QVERIFY_RESULT(tdResult);
    const TraceDirectory &td = *tdResult;
    QVERIFY(!td.traces().isEmpty());
    const TraceDirectory::Trace &trace = td.traces().first();

    if (expectNoDsc) {
        QVERIFY(trace.schema->dataStreamClasses.isEmpty());
        return;
    }

    // --- 2. Read events from the first stream (if present) ---
    if (trace.streams.isEmpty())
        return;

    DataStreamReader *dsReader = trace.streams.first().reader;

    int eventCount = 0;
    bool hadParseError = false;
    while (!dsReader->atEnd()) {
        auto evt = dsReader->nextEvent();
        if (!evt) {
            // A failed read that did not reach end-of-stream is a genuine error.
            if (!dsReader->atEnd())
                hadParseError = true;
            break;
        }
        ++eventCount;
        if (eventCount > eventLimit) {
            QFAIL("Event count exceeded limit — possible infinite loop");
            return;
        }
    }

    if (expectStreamFail) {
        QVERIFY2(hadParseError,
                 "Expected stream read failure but events were read successfully");
    } else {
        QVERIFY2(!hadParseError,
                 "Unexpected stream read error");
    }

    // The reader records any decode error (tolerated or surfaced) in readError();
    // verify it is set exactly when this row expects corrupted data.
    QCOMPARE(dsReader->readError().has_value(), expectReadError);
}

QTEST_APPLESS_MAIN(tst_TraceData)
#include "tst_tracedata.moc"
