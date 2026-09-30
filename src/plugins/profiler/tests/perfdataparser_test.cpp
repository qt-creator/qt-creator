// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfdataparser_test.h"

#include <profiler/perfdataparser.h>
#include <profiler/perfrecordreader.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>

#include <QTemporaryDir>
#include <QTest>

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

using namespace PerfData;

namespace {

void appendLe(QByteArray &out, quint64 value, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        out.append(char((value >> (8 * i)) & 0xff));
}

QByteArray record(quint32 type, const QByteArray &body, quint16 misc = 0)
{
    QByteArray r;
    appendLe(r, type, 4);
    appendLe(r, misc, 2);
    appendLe(r, 8 + quint64(body.size()), 2);
    return r + body;
}

constexpr quint64 SampleIp = 1ull << 0;
constexpr quint64 SampleTid = 1ull << 1;
constexpr quint64 SampleTime = 1ull << 2;
constexpr quint64 AttrFlagSampleIdAll = 1ull << 18;

// PERF_RECORD_HEADER_ATTR with a 64-byte attr and one id.
QByteArray attrRecord(quint64 sampleType, quint64 flags, quint64 id)
{
    QByteArray body;
    appendLe(body, 0, 4);          // type: hardware
    appendLe(body, 64, 4);         // size
    appendLe(body, 0, 8);          // config
    appendLe(body, 4000, 8);       // sample_freq
    appendLe(body, sampleType, 8);
    appendLe(body, 0, 8);          // read_format
    appendLe(body, flags, 8);
    body.append(QByteArray(64 - 48, '\0'));
    appendLe(body, id, 8);
    return record(64, body);
}

QByteArray pipeStream(const QList<QByteArray> &records)
{
    QByteArray stream("PERFILE2", 8);
    appendLe(stream, 16, 8);
    for (const QByteArray &r : records)
        stream += r;
    return stream;
}

// Every record, as the parser hands them over.
class RecordingHandler final : public PerfDataHandler
{
public:
    void mmap(const Mmap &m) override { mmaps.append(m); }
    void comm(const Comm &c) override { comms.append(c); }
    void task(const Task &t) override { tasks.append(t); }
    void contextSwitch(const ContextSwitch &s) override { switches.append(s); }
    Result<> sample(const Sample &s) override
    {
        samples.append(s);
        return ResultOk;
    }
    void tracingData(const QByteArray &data) override { tracing.append(data); }
    void buildId(const BuildId &b) override { buildIds.append(b); }

    QList<Mmap> mmaps;
    QList<Comm> comms;
    QList<Task> tasks;
    QList<ContextSwitch> switches;
    QList<Sample> samples;
    QList<QByteArray> tracing;
    QList<BuildId> buildIds;
};

Result<> parse(PerfDataParser &parser, const QByteArray &stream, RecordingHandler &handler)
{
    PerfByteQueue queue;
    queue.push(stream);
    queue.close();
    return parser.parseStream(queue, handler);
}

} // namespace

class PerfDataParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReadsTaskAndSwitchTimes();
    void testReadsTracingDataFollowingItsRecord();
    void testNamesEventsFromUpdates();
    void testParsesRecordedFile();
    void testParsesRecordedStreamFile();
    void testStreamsStreamFile();
    void testRejectsSectionsBeyondTheFile();
};

// Non-sample records say when and for which task they happened in a trailer
// laid out like the sample fields the attr asks for -- where the attr has
// sample_id_all. A fork or exit carries its time in the record itself.
void PerfDataParserTest::testReadsTaskAndSwitchTimes()
{
    QByteArray fork;
    appendLe(fork, 20, 4); // pid
    appendLe(fork, 10, 4); // ppid
    appendLe(fork, 21, 4); // tid
    appendLe(fork, 11, 4); // ptid
    appendLe(fork, 5000, 8);

    QByteArray switchTrailer;
    appendLe(switchTrailer, 20, 4);   // pid
    appendLe(switchTrailer, 21, 4);   // tid
    appendLe(switchTrailer, 7000, 8); // time
    constexpr quint16 miscSwitchOut = 1 << 13;

    QByteArray exitBody = fork;
    PerfDataParser parser;
    RecordingHandler handler;
    QVERIFY_RESULT(parse(parser,
                         pipeStream({attrRecord(SampleIp | SampleTid | SampleTime,
                                                AttrFlagSampleIdAll, 1),
                                     record(7, fork),
                                     record(14, switchTrailer, miscSwitchOut),
                                     record(4, exitBody)}),
                         handler));

    QCOMPARE(handler.tasks.size(), 2);
    QVERIFY(!handler.tasks.at(0).exit);
    QCOMPARE(handler.tasks.at(0).pid, 20u);
    QCOMPARE(handler.tasks.at(0).ppid, 10u);
    QCOMPARE(handler.tasks.at(0).tid, 21u);
    QCOMPARE(handler.tasks.at(0).time, quint64(5000));
    QVERIFY(handler.tasks.at(1).exit);

    QCOMPARE(handler.switches.size(), 1);
    const ContextSwitch &contextSwitch = handler.switches.first();
    QVERIFY(contextSwitch.out);
    QCOMPARE(contextSwitch.sampleId.pid, 20u);
    QCOMPARE(contextSwitch.sampleId.tid, 21u);
    QVERIFY(contextSwitch.sampleId.hasTime);
    QCOMPARE(contextSwitch.sampleId.time, quint64(7000));
}

// In a stream, the tracing data -- the formats of the tracepoints recorded --
// follows its PERF_RECORD_HEADER_TRACING_DATA record rather than being inside
// it, and the records after it have to be found past it.
void PerfDataParserTest::testReadsTracingDataFollowingItsRecord()
{
    const QByteArray tracing("\x17\x08\x44tracing0.6\0________", 24);
    QByteArray tracingRecord;
    appendLe(tracingRecord, quint64(tracing.size()), 4);

    QByteArray sample;
    appendLe(sample, 0x1234, 8); // ip
    appendLe(sample, 7, 4);      // pid
    appendLe(sample, 7, 4);      // tid
    appendLe(sample, 1000, 8);   // time

    PerfDataParser parser;
    RecordingHandler handler;
    QVERIFY_RESULT(parse(parser,
                         pipeStream({attrRecord(SampleIp | SampleTid | SampleTime, 0, 1),
                                     record(66, tracingRecord) + tracing,
                                     record(9, sample)}),
                         handler));
    QCOMPARE(handler.tracing.size(), 1);
    QCOMPARE(handler.tracing.first(), tracing);
    QCOMPARE(handler.samples.size(), 1);
    QCOMPARE(handler.samples.first().ip, quint64(0x1234));
}

// A stream names its events in PERF_RECORD_EVENT_UPDATE records.
void PerfDataParserTest::testNamesEventsFromUpdates()
{
    QByteArray update;
    appendLe(update, 2, 8); // PERF_EVENT_UPDATE__NAME
    appendLe(update, 42, 8); // id
    update.append("cycles:u\0\0\0\0\0\0\0\0", 16);

    PerfDataParser parser;
    RecordingHandler handler;
    QVERIFY_RESULT(parse(parser,
                         pipeStream({attrRecord(SampleIp | SampleTid | SampleTime, 0, 42),
                                     record(78, update)}),
                         handler));
    QCOMPARE(parser.attrs().size(), 1);
    QCOMPARE(parser.attrs().first().name, QByteArray("cycles:u"));
}

// A perf.data file as "perf record -o <file>" writes it: its attrs and their
// ids in a section of their own, event names, architecture and build ids in
// feature sections after the data, and records of a program perf ran --
// named by its exec, and ending by its exit.
void PerfDataParserTest::testParsesRecordedFile()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("perf is Linux-only");
    const FilePath perf = Environment::systemEnvironment().searchInPath("perf");
    if (perf.isEmpty())
        QSKIP("no \"perf\" in PATH");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString file = dir.filePath("perf.data");
    Process record;
    record.setCommand({perf, {"record", "-o", file, "--", "bash", "-c",
                              "for ((i = 0; i < 300000; ++i)); do :; done"}});
    record.runBlocking();
    if (record.result() != ProcessResult::FinishedWithSuccess)
        QSKIP(qPrintable(u"\"perf record\" failed here: %1"_s.arg(record.allOutput())));

    PerfDataParser parser;
    RecordingHandler handler;
    int lastProgress = -1;
    QVERIFY_RESULT(parser.parseFile(FilePath::fromString(file), handler,
                                    [&lastProgress](int percent) { lastProgress = percent; }));

    QVERIFY(!parser.attrs().isEmpty());
    QVERIFY(!parser.attrs().first().ids.isEmpty());
    QVERIFY2(!parser.attrs().first().name.isEmpty(), "expected the event's name");
    QVERIFY(!parser.architecture().isEmpty());
    QVERIFY(!handler.samples.isEmpty());
    QVERIFY(std::any_of(handler.comms.cbegin(), handler.comms.cend(),
                        [](const Comm &comm) { return comm.exec && comm.name == u"bash"_s; }));
    QVERIFY(std::any_of(handler.tasks.cbegin(), handler.tasks.cend(),
                        [](const Task &task) { return task.exit; }));
    QVERIFY(std::any_of(handler.mmaps.cbegin(), handler.mmaps.cend(),
                        [](const Mmap &mmap) { return mmap.path.endsWith(u"/bash"_s); }));
    QVERIFY(lastProgress >= 0);
}

// The stream "perf record -o -" writes, kept in a file: a file with the
// stream's framing rather than perf.data's own.
void PerfDataParserTest::testParsesRecordedStreamFile()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("perf is Linux-only");
    const FilePath perf = Environment::systemEnvironment().searchInPath("perf");
    if (perf.isEmpty())
        QSKIP("no \"perf\" in PATH");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString file = dir.filePath("stream.data");
    Process record;
    record.setCommand({"bash", {"-c", "\"$0\" record -o - -- bash -c 'for ((i = 0; i < 300000; "
                                      "++i)); do :; done' > \"$1\"",
                                perf.toFSPathString(), file}});
    record.runBlocking();
    if (record.result() != ProcessResult::FinishedWithSuccess)
        QSKIP(qPrintable(u"\"perf record\" failed here: %1"_s.arg(record.allOutput())));

    PerfDataParser parser;
    RecordingHandler handler;
    QVERIFY_RESULT(parser.parseFile(FilePath::fromString(file), handler));
    QVERIFY(!parser.attrs().isEmpty());
    QVERIFY(!handler.samples.isEmpty());
    QVERIFY(std::any_of(handler.comms.cbegin(), handler.comms.cend(),
                        [](const Comm &comm) { return comm.exec && comm.name == u"bash"_s; }));
}

// A stream written to a file is parsed as it is read, not read first:
// progress comes with the records.
void PerfDataParserTest::testStreamsStreamFile()
{
    QByteArray sample;
    appendLe(sample, 0x1000, 8);
    QList<QByteArray> records{attrRecord(SampleIp, 0, 1)};
    for (int i = 0; i < 1000; ++i)
        records.append(record(9, sample));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath file = FilePath::fromString(dir.filePath("stream.data"));
    QVERIFY_RESULT(file.writeFileContents(pipeStream(records)));

    // Where the progress was at each sample.
    class : public PerfDataHandler
    {
    public:
        Result<> sample(const Sample &) override
        {
            atSample.append(progress);
            return ResultOk;
        }
        int progress = -1;
        QList<int> atSample;
    } handler;
    PerfDataParser parser;
    QVERIFY_RESULT(parser.parseFile(file, handler, [&handler](int percent) {
        handler.progress = percent;
    }));
    const QList<int> &progressAtSample = handler.atSample;
    QCOMPARE(progressAtSample.size(), 1000);
    QVERIFY(progressAtSample.first() < 10);
    QVERIFY(progressAtSample.last() > 90);
    QCOMPARE(handler.progress, 100);
}

// A corrupt or truncated perf.data whose header points beyond its end is an
// error, not a read (or an allocation) of what is not there.
void PerfDataParserTest::testRejectsSectionsBeyondTheFile()
{
    QByteArray header("PERFILE2", 8);
    appendLe(header, 104, 8);             // header size
    appendLe(header, 16 + 64, 8);         // attr_size
    appendLe(header, 104, 8);             // attrs: offset,
    appendLe(header, 1ull << 40, 8);      // size -- a terabyte
    appendLe(header, 104, 8);             // data
    appendLe(header, 0, 8);
    appendLe(header, 0, 16);              // event_types
    appendLe(header, 0, 32);              // adds_features

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath file = FilePath::fromString(dir.filePath("perf.data"));
    QVERIFY_RESULT(file.writeFileContents(header));

    PerfDataParser parser;
    RecordingHandler handler;
    QVERIFY(!parser.parseFile(file, handler));
}

QObject *createPerfDataParserTest()
{
    return new PerfDataParserTest;
}

} // namespace Profiler::Internal

#include "perfdataparser_test.moc"
