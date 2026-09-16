// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfloader_test.h"

#include <profiler/ctfloader.h>
#include <profiler/ctfvisualizerconstants.h>

#include <utils/filepath.h>

#include <QDataStream>
#include <QFuture>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <limits>

using namespace Profiler::Constants;
using namespace Utils;
using namespace Qt::StringLiterals;

using json = nlohmann::json;

namespace Profiler::Internal {

static const char traceUuid[] = "6ab1eafd-7a3c-4a2f-9d5b-2f9a3b6c8d10";

// Metadata of a trace as Qt's CTF backend writes it: the thread that produced a
// packet is named in the packet context, and no event carries a pid or tid.
// `clockOffset` is the clock's origin in cycles, which Qt states as the
// wall-clock time the trace began.
static QByteArray metadata(quint64 clockOffset = 0)
{
    return QByteArray(R"(/* CTF 1.8 */

typealias integer { size = 8; align = 8; signed = false; } := uint8_t;
typealias integer { size = 32; align = 8; signed = false; } := uint32_t;
typealias integer { size = 64; align = 8; signed = false; } := uint64_t;

trace {
    major = 1;
    minor = 8;
    uuid = ")") + traceUuid + R"(";
    byte_order = le;
    packet.header := struct {
            uint32_t magic;
            uint8_t  uuid[16];
            uint32_t stream_id;
    } align(8);
};

env {
    domain = "ust";
    tracer_name = "qtctf";
    trace_name = "unittest";
};

clock {
    name = "monotonic";
    freq = 1000000000;
    offset = )" + QByteArray::number(clockOffset) + R"(;
};

typealias integer {
    size = 64; align = 8; signed = false;
    map = clock.monotonic.value;
} := uint64_clock_monotonic_t;

struct packet_context {
    uint64_clock_monotonic_t timestamp_begin;
    uint64_clock_monotonic_t timestamp_end;
    uint64_t content_size;
    uint64_t packet_size;
    uint64_t packet_seq_num;
    uint64_t events_discarded;
    uint32_t thread_id;
    string thread_name;
} align(8);

struct event_header {
    uint32_t id;
    uint64_clock_monotonic_t timestamp;
} align(8);

stream {
    id = 0;
    event.header := struct event_header;
    packet.context := struct packet_context;
};

event {
    name = "test:work_entry";
    id = 0;
    stream_id = 0;
    fields := struct {
        uint32_t value;
    };
};

event {
    name = "test:work_exit";
    id = 1;
    stream_id = 0;
    fields := struct {
        uint32_t value;
    };
};
)";
}

// Metadata whose stream maps its timestamps to a clock that is never declared.
// TSDL does not require the clock a timestamp field names to be one of the
// declared ones, so a consumer is left holding a clock name and nothing else --
// no frequency, and above all no origin. The mapping is stated on the stream's
// own header field, which is where a stream's clock is read from.
static QByteArray undeclaredClockMetadata()
{
    QByteArray text = metadata();
    text.replace("    event.header := struct event_header;",
                 R"(    event.header := struct {
        uint32_t id;
        integer { size = 64; align = 8; signed = false; map = clock.absent.value; } timestamp;
    } align(8);)");
    return text;
}

static const char clocklessUuid[] = "2c7f1b40-58d1-4e3e-9bb7-0d9a6f5c4e21";

// Metadata of a trace that declares no clock at all. TSDL falls back to the
// first clock a metadata declares for every stream of it, so a stream without
// one only occurs in a trace without one -- and every event of such a trace
// carries the timestamp 0.
static QByteArray clocklessMetadata()
{
    return QByteArray(R"(/* CTF 1.8 */

typealias integer { size = 8; align = 8; signed = false; } := uint8_t;
typealias integer { size = 32; align = 8; signed = false; } := uint32_t;

trace {
    major = 1;
    minor = 8;
    uuid = ")") + clocklessUuid + R"(";
    byte_order = le;
    packet.header := struct {
            uint32_t magic;
            uint8_t  uuid[16];
            uint32_t stream_id;
    } align(8);
};

stream {
    id = 0;
    event.header := struct {
        uint32_t id;
    } align(8);
};

event {
    name = "test:tick";
    id = 0;
    stream_id = 0;
    fields := struct {
        uint32_t value;
    };
};
)";
}

struct Event
{
    quint32 id = 0;
    quint64 timestamp = 0;
};

// One packet holding `events`, produced by the thread `threadId`/`threadName`.
static QByteArray channel(quint32 threadId, const QByteArray &threadName, const QList<Event> &events)
{
    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);

    out << quint32(0xC1FC1FC1);
    const QByteArray uuid = QUuid::fromString(QLatin1StringView(traceUuid)).toRfc4122();
    out.writeRawData(uuid.constData(), uuid.size());
    out << quint32(0); // stream_id

    out << events.first().timestamp << events.last().timestamp;
    out << quint64(0) << quint64(0); // content_size and packet_size, patched below
    out << quint64(0) << quint64(0); // packet_seq_num, events_discarded
    out << threadId;
    out.writeRawData(threadName.constData(), threadName.size() + 1); // NUL-terminated

    for (const Event &event : events)
        out << event.id << event.timestamp << quint32(event.id);

    // Both lengths are in bits, and the packet holds nothing beyond its events.
    const quint64 bits = quint64(packet.size()) * 8;
    QDataStream patch(&packet, QIODevice::ReadWrite);
    patch.setByteOrder(QDataStream::LittleEndian);
    patch.device()->seek(40);
    patch << bits << bits;

    return packet;
}

// The one packet of a clocklessMetadata() trace. It declares no packet context,
// hence no lengths -- the whole file is the packet.
static QByteArray clocklessChannel()
{
    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);

    out << quint32(0xC1FC1FC1);
    const QByteArray uuid = QUuid::fromString(QLatin1StringView(clocklessUuid)).toRfc4122();
    out.writeRawData(uuid.constData(), uuid.size());
    out << quint32(0); // stream_id

    out << quint32(0) << quint32(7); // one "test:tick" event, carrying a value

    return packet;
}

static void writeTrace(const FilePath &dir)
{
    QVERIFY(dir.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(dir.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(dir.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{0, 2000}, {1, 5000}})));
}

static QList<json> load(const QString &dirPath)
{
    QPromise<json> promise;
    promise.start();
    loadCtf2Data(promise, dirPath);
    promise.finish();

    const QFuture<json> future = promise.future();
    QList<json> events;
    for (int i = 0; i < future.resultCount(); ++i)
        events.append(future.resultAt(i));
    return events;
}

void CtfLoaderTest::testThreadsFromPacketContext()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeTrace(FilePath::fromString(dir.path()));

    const QList<json> events = load(dir.path());
    QVERIFY(!events.isEmpty());

    QMap<QString, QString> threadNames; // thread -> name
    QMap<QString, double> durations;    // thread -> duration of its "work" event
    QSet<QString> processes;
    QSet<QString> processNames;
    for (const json &event : events) {
        const QString tid = QString::fromStdString(event.value(CtfThreadIdKey, std::string()));
        const std::string phase = event.value(CtfEventPhaseKey, std::string());
        if (phase == CtfEventTypeMetadata) {
            const std::string name = event.value(CtfEventNameKey, std::string());
            if (name == "thread_name")
                threadNames.insert(tid, QString::fromStdString(event["args"]["name"]));
            else if (name == "process_name")
                processNames.insert(QString::fromStdString(event["args"]["name"]));
        } else if (phase == CtfEventTypeComplete) {
            QCOMPARE(event.value(CtfEventNameKey, std::string()), std::string("test:work"));
            durations.insert(tid, event.value(CtfDurationKey, 0.0));
            processes.insert(QString::fromStdString(event.value(CtfProcessIdKey, std::string())));
        }
    }

    // The lanes are the traced threads, named as the packet context names them,
    // and not the streams the events happen to be stored in.
    QCOMPARE(threadNames, (QMap<QString, QString>{{"17", "alpha"}, {"42", "beta"}}));

    // Entry and exit pair up per thread: 1000..3000 ns and 2000..5000 ns.
    QCOMPARE(durations, (QMap<QString, double>{{"17", 2.0}, {"42", 3.0}}));

    // All threads of one trace belong to one process, named by its session --
    // and the name is stated, so a lane does not have to fall back to the id it
    // is filed under (see CtfTimelineModel::updateName).
    QCOMPARE(processes, QSet<QString>{"unittest"});
    QCOMPARE(processNames, QSet<QString>{"unittest"});
}

void CtfLoaderTest::testThreadsFromEachPacketContext()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());

    // A stream is read packet by packet, and the identity of the thread that
    // wrote one is good for that packet only. Two packets of one stream, from
    // two threads, so a context read once and kept would put the second
    // thread's events on the first thread's lane.
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})
                                   + channel(42, "beta", {{0, 4000}, {1, 6000}})));

    QMap<QString, QString> threadNames; // lane -> name
    QMap<QString, double> durations;    // lane -> duration of its "work" event
    for (const json &event : load(dir.path())) {
        const QString tid = QString::fromStdString(event.value(CtfThreadIdKey, std::string()));
        const std::string phase = event.value(CtfEventPhaseKey, std::string());
        if (phase == CtfEventTypeMetadata) {
            if (event.value(CtfEventNameKey, std::string()) == "thread_name")
                threadNames.insert(tid, QString::fromStdString(event["args"]["name"]));
        } else if (phase == CtfEventTypeComplete) {
            durations.insert(tid, event.value(CtfDurationKey, 0.0));
        }
    }

    QCOMPARE(threadNames, (QMap<QString, QString>{{"17", "alpha"}, {"42", "beta"}}));
    QCOMPARE(durations, (QMap<QString, double>{{"17", 2.0}, {"42", 2.0}}));
}

void CtfLoaderTest::testEpochClockOffsetKeepsResolution()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());

    // Qt states its clock origin as the moment the trace began: milliseconds
    // since the epoch, in nanoseconds. A double holding that many microseconds
    // resolves to a quarter of a microsecond, so an offset folded into the
    // timestamp as a double collapses events that are hundreds of nanoseconds
    // apart -- which is the scale a tracepoint pair lives on.
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata(1789000000000000000ULL)));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 1100}})));

    bool paired = false;
    for (const json &event : load(dir.path())) {
        if (event.value(CtfEventPhaseKey, std::string()) != CtfEventTypeComplete)
            continue;
        // The entry and its exit are 100 ns apart, and stay apart.
        QCOMPARE(event.value(CtfDurationKey, 0.0), 0.1);
        paired = true;
    }
    QVERIFY(paired);
}

void CtfLoaderTest::testATimestampPastTheClockIsNotWrapped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());

    // A trace states its clock's origin and its events' cycle counts
    // independently, and nothing binds their sum to the 64 bits it is computed
    // in. The exit below crosses that, and a sum left to wrap comes out as a
    // handful of cycles -- which no clamp can tell from a timestamp the trace
    // really holds, and which would put the exit a wall-clock era before the
    // entry it closes.
    const quint64 origin = quint64(std::numeric_limits<qint64>::max());
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata(origin)));
    // One cycle past this and the origin no longer fit together.
    const quint64 lastCycle = std::numeric_limits<quint64>::max() - origin;
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(
                    channel(17, "alpha", {{0, lastCycle - 800}, {1, lastCycle + 200}})));

    bool paired = false;
    for (const json &event : load(dir.path())) {
        if (event.value(CtfEventPhaseKey, std::string()) != CtfEventTypeComplete)
            continue;
        // The exit is held at the end of the clock's range instead, which keeps
        // it after its entry and within a trace's length of it.
        const double duration = event.value(CtfDurationKey, -1.0);
        QVERIFY(duration >= 0.0);
        QVERIFY(duration < 1.0e6);
        paired = true;
    }
    // And the pair is there to be judged at all: events are matched in
    // timestamp order, so an exit thrown back before its entry closes nothing
    // and the two decay into unrelated instants.
    QVERIFY(paired);
}

void CtfLoaderTest::testClocklessStreamKeepsResolution()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath root = FilePath::fromString(dir.path());

    // A trace that declares no clock timestamps all its events 0. Counting it
    // as one whose clock begins at zero would put the origin of the whole load
    // there, which is the precision loss above all over again for the trace
    // that does have a clock.
    const quint64 epoch = 1789000000000000000ULL;
    const FilePath clocked = root / u"clocked"_s;
    QVERIFY(clocked.ensureWritableDir());
    QVERIFY(clocked.pathAppended("metadata").writeFileContents(metadata(epoch)));
    QVERIFY(clocked.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 1100}})));

    const FilePath untimed = root / u"untimed"_s;
    QVERIFY(untimed.ensureWritableDir());
    QVERIFY(untimed.pathAppended("metadata").writeFileContents(clocklessMetadata()));
    QVERIFY(untimed.pathAppended("channel_0").writeFileContents(clocklessChannel()));

    bool paired = false;
    bool ticked = false;
    for (const json &event : load(dir.path())) {
        if (event.value(CtfEventNameKey, std::string()) == "test:tick") {
            // The clockless event stays where it was, at the start of the
            // timeline, rather than a wall-clock era before it.
            QCOMPARE(event.value(CtfTracingClockTimestampKey, -1.0), 0.0);
            ticked = true;
        }
        if (event.value(CtfEventPhaseKey, std::string()) != CtfEventTypeComplete)
            continue;
        // And the clocked trace keeps its 100 ns.
        QCOMPARE(event.value(CtfDurationKey, 0.0), 0.1);
        paired = true;
    }
    QVERIFY(ticked);
    QVERIFY(paired);
}

void CtfLoaderTest::testStreamOfAnUndeclaredClockKeepsResolution()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath root = FilePath::fromString(dir.path());

    // A stream whose clock the metadata never declares says nothing about its
    // origin, least of all that it begins at zero. Counting it as one that does
    // would put the origin of the whole load there, which is the precision loss
    // above all over again for the trace whose clock is declared.
    const quint64 epoch = 1789000000000000000ULL;
    const FilePath clocked = root / u"clocked"_s;
    QVERIFY(clocked.ensureWritableDir());
    QVERIFY(clocked.pathAppended("metadata").writeFileContents(metadata(epoch)));
    QVERIFY(clocked.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 1100}})));

    const FilePath unclocked = root / u"unclocked"_s;
    QVERIFY(unclocked.ensureWritableDir());
    QVERIFY(unclocked.pathAppended("metadata").writeFileContents(undeclaredClockMetadata()));
    QVERIFY(unclocked.pathAppended("channel_0")
                .writeFileContents(channel(42, "beta", {{0, 2000}, {1, 2100}})));

    QList<double> durations;
    for (const json &event : load(dir.path())) {
        if (event.value(CtfEventPhaseKey, std::string()) == CtfEventTypeComplete)
            durations.append(event.value(CtfDurationKey, 0.0));
    }

    // Both keep their 100 ns: the clocked trace is not dragged back to the
    // epoch, and the other counts from an origin of its own.
    QCOMPARE(durations.size(), 2);
    for (double duration : durations)
        QCOMPARE(duration, 0.1);
}

void CtfLoaderTest::testThreadLanesStayDistinctAcrossTraces()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath root = FilePath::fromString(dir.path());

    // Two recordings of one session, each in a domain subdirectory of its own,
    // as LTTng and Qt's CTF backend both write them -- so neither the trace
    // directories nor the session directories above them are told apart by
    // name, and the metadata names the same session for both. Their thread ids
    // are unique per trace only: both recorded a thread 17.
    for (const QString &recording : {u"first"_s, u"second"_s}) {
        const FilePath traceDir = root / recording / u"session"_s / u"ust"_s;
        QVERIFY(traceDir.ensureWritableDir());
        QVERIFY(traceDir.pathAppended("metadata").writeFileContents(metadata()));
        // The two threads' events interleave, so a shared lane would not just
        // merge them: the pairing would close one trace's entry with the
        // other's exit.
        const bool isFirst = recording == u"first"_s;
        QVERIFY(traceDir.pathAppended("channel_0")
                    .writeFileContents(channel(17, isFirst ? "alpha" : "beta",
                                               isFirst ? QList<Event>{{0, 1000}, {1, 5000}}
                                                       : QList<Event>{{0, 2000}, {1, 9000}})));
    }

    QMap<QString, QString> threadNames; // lane -> name
    QMap<QString, QString> threadIds;   // lane -> thread id the trace stated
    QMap<QString, double> durations;    // lane -> duration of its "work" event
    QSet<QString> processes;
    QSet<QString> processNames;
    for (const json &event : load(dir.path())) {
        const QString tid = QString::fromStdString(event.value(CtfThreadIdKey, std::string()));
        const std::string phase = event.value(CtfEventPhaseKey, std::string());
        if (phase == CtfEventTypeMetadata) {
            const json &args = event["args"];
            const QString stated
                = QString::fromStdString(args.value(CtfMetadataDisplayIdKey, std::string()));
            const std::string name = event.value(CtfEventNameKey, std::string());
            if (name == "thread_name") {
                threadNames.insert(tid, QString::fromStdString(args["name"]));
                threadIds.insert(tid, stated);
            } else if (name == "process_name") {
                processNames.insert(QString::fromStdString(args["name"]));
                // A process named after a session has no id beyond that name,
                // and says so by stating it as one.
                QCOMPARE(stated, QString::fromStdString(args["name"]));
            }
        } else if (phase == CtfEventTypeComplete) {
            durations.insert(tid, event.value(CtfDurationKey, 0.0));
            processes.insert(QString::fromStdString(event.value(CtfProcessIdKey, std::string())));
        }
    }

    // Each trace's thread 17 has a lane of its own, named after the thread it
    // really is, and its own event keeps its own length: 1000..5000 ns and
    // 2000..9000 ns.
    QCOMPARE(threadNames.size(), 2);
    QCOMPARE(QSet<QString>(threadNames.cbegin(), threadNames.cend()),
             (QSet<QString>{"alpha", "beta"}));
    QCOMPARE(QSet<double>(durations.cbegin(), durations.cend()), (QSet<double>{4.0, 7.0}));

    // And the two recordings are two processes, although they name one session:
    // the timeline groups a lane under its process, and colours it by that.
    QCOMPARE(processes.size(), 2);

    // What the reader is shown carries none of that qualification: both lanes
    // state the thread id their trace did, and the two processes are told apart
    // by a number on the shared session name rather than by the key the lanes
    // happen to be filed under.
    QCOMPARE(QSet<QString>(threadIds.cbegin(), threadIds.cend()), QSet<QString>{"17"});
    QCOMPARE(processNames, (QSet<QString>{"unittest #1", "unittest #2"}));
}

void CtfLoaderTest::testUnnamedTraceIsCalledAfterItsRecording()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Without a session name in the metadata, the trace is called after the
    // directory it was found in -- which for a trace in a domain subdirectory
    // is the recording above it, not the domain.
    QByteArray unnamed = metadata();
    QVERIFY(unnamed.contains("trace_name"));
    unnamed.replace("    trace_name = \"unittest\";\n", "");

    const FilePath traceDir = FilePath::fromString(dir.path()) / u"myrecording"_s / u"ust"_s;
    QVERIFY(traceDir.ensureWritableDir());
    QVERIFY(traceDir.pathAppended("metadata").writeFileContents(unnamed));
    QVERIFY(traceDir.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));

    QSet<QString> processes;
    for (const json &event : load(dir.path())) {
        if (event.value(CtfEventPhaseKey, std::string()) == CtfEventTypeComplete)
            processes.insert(QString::fromStdString(event.value(CtfProcessIdKey, std::string())));
    }
    QCOMPARE(processes, QSet<QString>{"myrecording"});
}

} // namespace Profiler::Internal
