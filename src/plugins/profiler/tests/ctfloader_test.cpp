// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfloader_test.h"

#include <profiler/ctfloader.h>
#include <profiler/ctfplainviewmanager.h>
#include <profiler/ctftracebackend.h>
#include <profiler/ctftimelinemodel.h>
#include <profiler/ctftracemanager.h>
#include <profiler/ctfvisualizerconstants.h>

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/filepath.h>

#include <QDataStream>
#include <QFuture>
#include <QHBoxLayout>
#include <QMenu>
#include <QSignalSpy>
#include <QToolButton>
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
// wall-clock time the trace began. The one event class that names no provider
// stands for a kernel recording read beside a Qt one, whose events are plain
// "sched_switch".
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

event {
    name = "other:idle_entry";
    id = 2;
    stream_id = 0;
    fields := struct {
        uint32_t value;
    };
};

event {
    name = "other:idle_exit";
    id = 3;
    stream_id = 0;
    fields := struct {
        uint32_t value;
    };
};

event {
    name = "sched_switch";
    id = 4;
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

static QList<json> load(const QString &dirPath, const QStringList &providers = {})
{
    QPromise<json> promise;
    promise.start();
    loadCtf2Data(promise, dirPath, providers);
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

void CtfLoaderTest::testProvidersOfATrace()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeTrace(FilePath::fromString(dir.path()));

    // Taken from the event classes the metadata declares, so a provider that
    // fired no event is offered too -- writeTrace() records none of "other" --
    // and sorted, since the order they were declared in is the producer's
    // business and no order to show a reader. A class that names no provider
    // adds none: it is not a provider called "".
    QCOMPARE(ctfTraceProviders(dir.path()), (QStringList{"other", "test"}));

    QTemporaryDir empty;
    QVERIFY(empty.isValid());
    QCOMPARE(ctfTraceProviders(empty.path()), QStringList());
}

void CtfLoaderTest::testLoadRestrictedToOneProvider()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());

    // One thread per provider, so that a load restricted to one of them leaves
    // the other's thread with no event at all.
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{2, 2000}, {3, 5000}})));

    const auto categories = [](const QList<json> &events) {
        QMap<QString, QString> result; // event name -> the provider it names
        for (const json &event : events) {
            if (event.value(CtfEventPhaseKey, std::string()) == CtfEventTypeComplete) {
                result.insert(
                    QString::fromStdString(event.value(CtfEventNameKey, std::string())),
                    QString::fromStdString(event.value(CtfEventCategoryKey, std::string())));
            }
        }
        return result;
    };
    const auto lanes = [](const QList<json> &events) {
        QSet<QString> result;
        for (const json &event : events) {
            if (event.value(CtfEventNameKey, std::string()) == "thread_name")
                result.insert(QString::fromStdString(event["args"]["name"]));
        }
        return result;
    };

    // Unrestricted, both threads are there and every event says which provider
    // it came from.
    const QList<json> all = load(dir.path());
    QCOMPARE(categories(all),
             (QMap<QString, QString>{{"test:work", "test"}, {"other:idle", "other"}}));
    QCOMPARE(lanes(all), (QSet<QString>{"alpha", "beta"}));

    // Restricted, the other provider's events are not emitted -- and neither is
    // a name for the thread that produced only them, which would leave the
    // timeline with a row that has nothing on it.
    const QList<json> restricted = load(dir.path(), {"other"});
    QCOMPARE(categories(restricted), (QMap<QString, QString>{{"other:idle", "other"}}));
    QCOMPARE(lanes(restricted), QSet<QString>{"beta"});
}

void CtfLoaderTest::testEventsOfNoProviderAreAlwaysLoaded()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());

    // One thread producing a provider's events, and one producing nothing but
    // the events that name no provider.
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{4, 2000}, {4, 5000}})));

    const auto eventNames = [](const QList<json> &events) {
        QSet<QString> result;
        for (const json &event : events) {
            if (event.value(CtfEventPhaseKey, std::string()) != CtfEventTypeMetadata)
                result.insert(QString::fromStdString(event.value(CtfEventNameKey, std::string())));
        }
        return result;
    };

    QCOMPARE(eventNames(load(dir.path())), (QSet<QString>{"test:work", "sched_switch"}));

    // An event that says where it came from is left out by a restriction that
    // does not name it. One that says nothing came from no provider, so no
    // provider being cleared is about it -- and there is no entry that would
    // bring it back, since a provider it does not have cannot be selected.
    QCOMPARE(eventNames(load(dir.path(), {"other"})), QSet<QString>{"sched_switch"});
    QCOMPARE(eventNames(load(dir.path(), {"test"})),
             (QSet<QString>{"test:work", "sched_switch"}));
}

void CtfLoaderTest::testRestrictingAProviderReloadsTheTrace()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{2, 2000}, {3, 5000}})));

    Timeline::RangeDetailsWidget details;
    CtfPlainViewManager manager(&details);
    QSignalSpy loaded(&manager, &CtfPlainViewManager::loadFinished);

    manager.loadCtf2(path);
    QVERIFY(loaded.wait());

    const auto laneNames = [&manager] {
        QStringList names;
        for (const CtfTimelineModel *model : manager.traceManager()->getSortedThreads())
            names.append(model->displayName());
        names.sort();
        return names;
    };

    // A trace is shown whole: what it declares and what is on show are the
    // same list, so neither stands for the other.
    QCOMPARE(manager.traceProviders(), (QStringList{"other", "test"}));
    QCOMPARE(manager.shownProviders(), (QStringList{"other", "test"}));
    QCOMPARE(laneNames(), (QStringList{"alpha (17)", "beta (42)"}));

    // Dropping one reads the trace again: the thread that produced nothing but
    // that provider's events is not a lane of the timeline any more, while the
    // trace still declares the provider it can be taken back from.
    manager.setShownProviders({"other"});
    QVERIFY(loaded.wait());
    QCOMPARE(manager.shownProviders(), QStringList{"other"});
    QCOMPARE(manager.traceProviders(), (QStringList{"other", "test"}));
    QCOMPARE(laneNames(), QStringList{"beta (42)"});

    // Showing no provider at all is no state to be in, and is not entered.
    // What is shown is recorded before the trace is read, so the state right
    // after the call already says whether it was taken.
    manager.setShownProviders({});
    QCOMPARE(manager.shownProviders(), QStringList{"other"});

    manager.setShownProviders({"other", "test"});
    QVERIFY(loaded.wait());
    QCOMPARE(laneNames(), (QStringList{"alpha (17)", "beta (42)"}));
}

void CtfLoaderTest::testAThreadRestrictionOutlivesAProviderChange()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{2, 2000}, {3, 5000}})));

    Timeline::RangeDetailsWidget details;
    CtfPlainViewManager manager(&details);
    CtfTraceManager *traceManager = manager.traceManager();
    QSignalSpy loaded(&manager, &CtfPlainViewManager::loadFinished);

    manager.loadCtf2(path);
    QVERIFY(loaded.wait());

    const auto shownLanes = [traceManager] {
        QStringList names;
        for (const CtfTimelineModel *model : traceManager->shownThreads())
            names.append(model->displayName());
        names.sort();
        return names;
    };

    // "alpha" produces nothing but the events of the "test" provider.
    traceManager->setThreadRestriction("17", true);
    QCOMPARE(shownLanes(), QStringList{"alpha (17)"});

    // Clearing that provider takes the thread the timeline is restricted to
    // with it. What is left is shown whole: a restriction naming a thread that
    // the trace on show does not have would leave the timeline with no lane at
    // all, and no entry to take the restriction back by.
    manager.setShownProviders({"other"});
    QVERIFY(loaded.wait());
    QCOMPARE(shownLanes(), QStringList{"beta (42)"});
    QVERIFY(traceManager->showsAllThreads());

    // The restriction is still the reader's: reading the trace again is not
    // being asked to show every thread, so bringing the provider back brings
    // back a timeline restricted as it was.
    manager.setShownProviders({"other", "test"});
    QVERIFY(loaded.wait());
    QVERIFY(traceManager->isRestrictedTo("17"));
    QCOMPARE(shownLanes(), QStringList{"alpha (17)"});
}

void CtfLoaderTest::testARestrictionReplacesOneFromAClearedProvider()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{2, 2000}, {3, 5000}})));

    Timeline::RangeDetailsWidget details;
    CtfPlainViewManager manager(&details);
    CtfTraceManager *traceManager = manager.traceManager();
    QSignalSpy loaded(&manager, &CtfPlainViewManager::loadFinished);

    manager.loadCtf2(path);
    QVERIFY(loaded.wait());

    const auto shownLanes = [traceManager] {
        QStringList names;
        for (const CtfTimelineModel *model : traceManager->shownThreads())
            names.append(model->displayName());
        names.sort();
        return names;
    };

    // "alpha" produces nothing but the events of the "test" provider, so
    // clearing that provider leaves the timeline with no restricted thread on
    // it -- and the thread menu with nothing ticked.
    traceManager->setThreadRestriction("17", true);
    manager.setShownProviders({"other"});
    QVERIFY(loaded.wait());
    QVERIFY(traceManager->showsAllThreads());

    // What the reader asks for over that menu is the whole answer to which
    // threads are shown. The restriction from before is not part of it: it
    // was made of threads the trace on show does not have.
    traceManager->setThreadRestriction("42", true);
    QCOMPARE(shownLanes(), QStringList{"beta (42)"});

    manager.setShownProviders({"other", "test"});
    QVERIFY(loaded.wait());
    QVERIFY(!traceManager->isRestrictedTo("17"));
    QCOMPARE(shownLanes(), QStringList{"beta (42)"});
}

void CtfLoaderTest::testTheShownRangeOutlivesAProviderChange()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    // The trace begins with the "test" provider and ends with it, so a trace
    // read without that provider is a shorter one: what is left is in the
    // middle of what the whole trace spans.
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(
                    channel(17, "alpha", {{0, 1000}, {1, 1500}, {0, 8000}, {1, 9000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(
                    channel(42, "beta", {{2, 2000}, {3, 2500}, {2, 3000}, {3, 3500}})));

    Timeline::RangeDetailsWidget details;
    CtfPlainViewManager manager(&details);
    Timeline::TimelineZoomControl *zoom = manager.zoomControl();
    QSignalSpy loaded(&manager, &CtfPlainViewManager::loadFinished);

    manager.loadCtf2(path);
    QVERIFY(loaded.wait());

    // A trace that is opened is looked at whole.
    const qint64 wholeStart = zoom->traceStart();
    const qint64 wholeEnd = zoom->traceEnd();
    QCOMPARE(zoom->rangeStart(), wholeStart);
    QCOMPARE(zoom->rangeEnd(), wholeEnd);

    // The reader zooms into the end of the trace, which is past everything the
    // "other" provider recorded.
    zoom->setRange(wholeStart + zoom->traceDuration() * 3 / 4, wholeEnd);

    // None of that stretch is in the trace read without "test", so there is
    // nothing to go back to, and what was read is looked at whole.
    manager.setShownProviders({"other"});
    QVERIFY(loaded.wait());
    QVERIFY(zoom->traceEnd() < wholeEnd);
    QCOMPARE(zoom->rangeStart(), zoom->traceStart());
    QCOMPARE(zoom->rangeEnd(), zoom->traceEnd());

    // A stretch the trace read next does have is gone back to: reading the
    // trace for a provider is not the reader asking to see all of it again.
    const qint64 start = zoom->traceStart() + zoom->traceDuration() / 4;
    const qint64 end = zoom->traceEnd();
    zoom->setRange(start, end);
    QCOMPARE(zoom->rangeStart(), start);
    QCOMPARE(zoom->rangeEnd(), end);

    manager.setShownProviders({"other", "test"});
    QVERIFY(loaded.wait());
    QCOMPARE(zoom->traceEnd(), wholeEnd);
    QCOMPARE(zoom->rangeStart(), start);
    QCOMPARE(zoom->rangeEnd(), end);
}

void CtfLoaderTest::testClearedViewsShowTheNextTraceWhole()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{2, 2000}, {3, 5000}})));

    Timeline::RangeDetailsWidget details;
    CtfPlainViewManager manager(&details);
    QSignalSpy loaded(&manager, &CtfPlainViewManager::loadFinished);

    manager.loadCtf2(path);
    QVERIFY(loaded.wait());
    manager.setShownProviders({"other"});
    QVERIFY(loaded.wait());
    QCOMPARE(manager.shownProviders(), QStringList{"other"});

    // A trace that is rewritten while it is open is reloaded by clearing the
    // views and loading the same path again. That is the trace being opened,
    // not its providers being changed: what the recording now declares is read
    // again, and all of it is shown. A restriction carried over would hide a
    // whole new recording, with a menu naming the providers of the one before.
    manager.clear();
    QCOMPARE(manager.traceProviders(), QStringList());
    QCOMPARE(manager.shownProviders(), QStringList());

    manager.loadCtf2(path);
    QVERIFY(loaded.wait());
    QCOMPARE(manager.traceProviders(), (QStringList{"other", "test"}));
    QCOMPARE(manager.shownProviders(), (QStringList{"other", "test"}));

    QStringList laneNames;
    for (const CtfTimelineModel *model : manager.traceManager()->getSortedThreads())
        laneNames.append(model->displayName());
    laneNames.sort();
    QCOMPARE(laneNames, (QStringList{"alpha (17)", "beta (42)"}));
}

void CtfLoaderTest::testRestrictionToASilentProviderSaysSo()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // Declares "other" and records nothing of it.
    writeTrace(FilePath::fromString(dir.path()));

    Timeline::RangeDetailsWidget details;
    CtfPlainViewManager manager(&details);
    QSignalSpy loaded(&manager, &CtfPlainViewManager::loadFinished);
    QSignalSpy failed(&manager, &CtfPlainViewManager::error);

    manager.loadCtf2(FilePath::fromString(dir.path()));
    QVERIFY(loaded.wait());
    QVERIFY(failed.isEmpty());

    // A provider the trace declares is offered whether or not its tracepoints
    // were reached, so restricting to one can leave the timeline empty. What is
    // reported then is the restriction, not a trace that has nothing in it.
    manager.setShownProviders({"other"});
    QVERIFY(loaded.wait());
    QCOMPARE(failed.size(), 1);
    QVERIFY(failed.first().first().toString().contains("other"));
    QVERIFY(manager.traceManager()->isEmpty());

    // And putting the other one back is the way out of that, so the trace
    // still declares what it no longer shows.
    QCOMPARE(manager.traceProviders(), (QStringList{"other", "test"}));
    manager.setShownProviders({"other", "test"});
    QVERIFY(loaded.wait());
    QVERIFY(!manager.traceManager()->isEmpty());
}

// What the provider menu offers, as "<name>[x]" for a provider that is shown
// and "<name>[ ]" for one that is not, with a "!" on an entry that cannot be
// toggled.
static QStringList menuState(const QMenu *menu)
{
    QStringList entries;
    for (const QAction *action : menu->actions()) {
        entries.append(QString("%1[%2]%3").arg(action->text(),
                                               action->isChecked() ? u"x"_s : u" "_s,
                                               action->isEnabled() ? QString() : u"!"_s));
    }
    return entries;
}

void CtfLoaderTest::testTheProviderMenuSaysWhatIsShown()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());
    QVERIFY(path.pathAppended("metadata").writeFileContents(metadata()));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));
    QVERIFY(path.pathAppended("channel_1")
                .writeFileContents(channel(42, "beta", {{2, 2000}, {3, 5000}})));

    // The toolbar the editor puts the backend's controls on. Without it they
    // have no parent, and showing one would put a window of its own on the
    // desktop. It is declared before the backend that owns them, so that they
    // are gone -- and unparented by their own destructor -- before it is.
    QWidget toolBar;
    QHBoxLayout toolBarLayout(&toolBar);

    Timeline::RangeDetailsWidget details;
    CtfTraceBackend backend(&details);
    for (QWidget *widget : backend.toolBarWidgets())
        toolBarLayout.addWidget(widget);

    QSignalSpy loaded(&backend, &CtfTraceBackend::loadFinished);
    backend.load(path);
    QVERIFY(loaded.wait());

    // The providers sit in the last of the backend's toolbar controls.
    auto button = qobject_cast<QToolButton *>(backend.toolBarWidgets().last());
    QVERIFY(button);
    QMenu *menu = button->menu();

    // A trace as it was opened shows every provider it has, and every entry is
    // ticked: an empty menu and a full one must not mean the same thing.
    QVERIFY(!button->isHidden());
    QCOMPARE(menuState(menu), (QStringList{"other[x]", "test[x]"}));

    // Clearing an entry drops that provider from the timeline, and the entry
    // that is left cannot be cleared in turn: an empty timeline is no state to
    // leave a reader in, and it would tick nothing while showing everything.
    menu->actions().last()->trigger();
    QVERIFY(loaded.wait());
    QCOMPARE(menuState(menu), (QStringList{"other[x]!", "test[ ]"}));

    // A change that is not taken puts the menu back to what is shown rather
    // than leaving it saying something else.
    menu->actions().first()->trigger();
    QCOMPARE(menuState(menu), (QStringList{"other[x]!", "test[ ]"}));

    // And ticking an entry again brings its provider back.
    menu->actions().last()->trigger();
    QVERIFY(loaded.wait());
    QCOMPARE(menuState(menu), (QStringList{"other[x]", "test[x]"}));
}

void CtfLoaderTest::testOneProviderIsNothingToChooseFrom()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath path = FilePath::fromString(dir.path());

    QByteArray oneProvider = metadata();
    QVERIFY(oneProvider.contains("other:"));
    oneProvider.replace("other:", "test:");
    QVERIFY(path.pathAppended("metadata").writeFileContents(oneProvider));
    QVERIFY(path.pathAppended("channel_0")
                .writeFileContents(channel(17, "alpha", {{0, 1000}, {1, 3000}})));

    QWidget toolBar;
    QHBoxLayout toolBarLayout(&toolBar);

    Timeline::RangeDetailsWidget details;
    CtfTraceBackend backend(&details);
    for (QWidget *widget : backend.toolBarWidgets())
        toolBarLayout.addWidget(widget);

    QSignalSpy loaded(&backend, &CtfTraceBackend::loadFinished);
    backend.load(path);
    QVERIFY(loaded.wait());

    auto button = qobject_cast<QToolButton *>(backend.toolBarWidgets().last());
    QVERIFY(button);

    // A trace of one provider offers the entry that cannot be cleared, and
    // nothing else: there is no other state its menu could be put in, so the
    // button is not shown at all.
    QCOMPARE(menuState(button->menu()), QStringList{"test[x]!"});
    QVERIFY(button->isHidden());
}

} // namespace Profiler::Internal
