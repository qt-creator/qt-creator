// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perftraceconverter_test.h"

#include <profiler/perfconversion.h>
#include <profiler/perfdataparser.h>
#include <profiler/perfdatareader.h>
#include <profiler/perfevent.h>
#include <profiler/perfeventtype.h>
#include <profiler/perfnativemixed.h>
#include <profiler/perfprofilertracefile.h>
#include <profiler/perfprofilertracemanager.h>
#include <profiler/perfrecordreader.h>
#include <profiler/perfregisters.h>
#include <profiler/perftraceconverter.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>

#include <QBuffer>
#include <QDir>
#include <QFutureInterface>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QtEndian>

#include <atomic>
#include <memory>

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

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
constexpr quint64 SampleCallchain = 1ull << 5;
constexpr quint64 SampleCpu = 1ull << 7;
constexpr quint64 SamplePeriod = 1ull << 8;
constexpr quint64 SampleRaw = 1ull << 10;
constexpr quint64 AttrFlagSampleIdAll = 1ull << 18;
constexpr quint64 PerfContextUser = quint64(-512);

QByteArray attrRecord(quint32 type, quint64 config, quint64 sampleType, quint64 id)
{
    QByteArray body;
    appendLe(body, type, 4);
    appendLe(body, 64, 4);       // size
    appendLe(body, config, 8);
    appendLe(body, 1000, 8);     // sample_period
    appendLe(body, sampleType, 8);
    appendLe(body, 0, 8);        // read_format
    appendLe(body, AttrFlagSampleIdAll, 8);
    body.append(QByteArray(64 - 48, '\0'));
    appendLe(body, id, 8);
    return record(64, body);
}

// The trailer of a non-sample record, for SampleTid | SampleTime | SampleCpu.
QByteArray trailer(quint32 pid, quint32 tid, quint64 time)
{
    QByteArray t;
    appendLe(t, pid, 4);
    appendLe(t, tid, 4);
    appendLe(t, time, 8);
    appendLe(t, 0, 8); // cpu, res
    return t;
}

QByteArray pipeStream(const QList<QByteArray> &records)
{
    QByteArray stream("PERFILE2", 8);
    appendLe(stream, 16, 8);
    for (const QByteArray &r : records)
        stream += r;
    return stream;
}

QByteArray convert(const QByteArray &stream,
                   const std::function<void(PerfTraceConverter &)> &configure = {})
{
    PerfData::PerfDataParser parser;
    QByteArray converted;
    PerfTraceConverter converter(parser, [&converted](const QByteArray &chunk) {
        converted += chunk;
    });
    if (configure)
        configure(converter);
    PerfByteQueue queue;
    queue.push(stream);
    queue.close();
    const Result<> result = parser.parseStream(queue, converter);
    converter.finish();
    if (!result)
        qWarning() << result.error();
    return converted;
}

// The times of the samples in `converted`, in the order they are written.
QList<quint64> sampleTimes(const QByteArray &converted)
{
    QList<quint64> times;
    qsizetype at = sizeof("QPERFSTREAM") + sizeof(qint32);
    while (at + 4 <= converted.size()) {
        const quint32 size = qFromLittleEndian<quint32>(converted.constData() + at);
        QDataStream stream(converted.mid(at + 4, size));
        quint8 feature = 0;
        quint32 pid = 0;
        quint32 tid = 0;
        quint64 time = 0;
        stream >> feature >> pid >> tid >> time;
        if (feature == PerfEventType::Sample)
            times.append(time);
        at += 4 + size;
    }
    return times;
}

// A stream of `count` samples of thread 5, in rounds of 1000.
QByteArray manySamples(int count)
{
    constexpr quint64 sampleType = SampleIp | SampleTid | SampleTime;
    QList<QByteArray> records{attrRecord(1, 1, sampleType, 1)};
    records.reserve(count + count / 1000 + 2);
    for (int i = 0; i < count; ++i) {
        QByteArray sample;
        appendLe(sample, 0x1000, 8);
        appendLe(sample, 5, 4);
        appendLe(sample, 5, 4);
        appendLe(sample, 1000 + quint64(i), 8);
        records.append(record(9, sample));
        if (i % 1000 == 999)
            records.append(record(68, {}));
    }
    return pipeStream(records);
}

// What the CPU Usage analyzer makes of `converted`, as it reads a recording.
bool load(QByteArray converted, PerfProfilerTraceManager &manager)
{
    QBuffer buffer(&converted);
    if (!buffer.open(QIODevice::ReadOnly))
        return false;
    PerfProfilerTraceFile traceFile;
    traceFile.setTraceManager(&manager);
    manager.initialize();
    traceFile.load(&buffer);
    manager.finalize();
    return !traceFile.isCanceled();
}

QList<PerfEvent> events(const PerfProfilerTraceManager &manager)
{
    QList<PerfEvent> all;
    QFutureInterface<void> future;
    future.reportStarted();
    manager.replayPerfEvents([&all](const PerfEvent &event, const PerfEventType &) {
        all.append(event);
    }, nullptr, nullptr, nullptr, future);
    return all;
}

QByteArray symbolName(const PerfProfilerTraceManager &manager, qint32 locationId)
{
    return manager.string(manager.symbol(manager.symbolLocation(locationId)).name);
}

constexpr int RecursionDepth = 10;

// Compiles, into `dir`, a program spending its time RecursionDepth + 1 levels
// deep in converterRecurse(), with frame pointers and debug information.
std::optional<FilePath> compileRecursion(const QTemporaryDir &dir, QString *error)
{
    const FilePath cc = Environment::systemEnvironment().searchInPath("cc");
    if (cc.isEmpty()) {
        *error = u"no \"cc\" in PATH"_s;
        return std::nullopt;
    }
    QFile source(dir.filePath("recurse.c"));
    if (!source.open(QIODevice::WriteOnly))
        return std::nullopt;
    source.write("volatile unsigned long sink;\n"
                 "__attribute__((noinline)) unsigned long converterRecurse(int depth, "
                 "unsigned long acc)\n"
                 "{\n"
                 "    if (depth == 0) {\n"
                 "        for (int i = 0; i < 2000; ++i)\n"
                 "            acc = acc * 6364136223846793005ul + 1442695040888963407ul;\n"
                 "        sink = acc;\n"
                 "        return acc;\n"
                 "    }\n"
                 "    return converterRecurse(depth - 1, acc) + (unsigned long)depth;\n"
                 "}\n"
                 "int main(void)\n"
                 "{\n"
                 "    for (long i = 0; i < 300000; ++i)\n"
                 "        converterRecurse(" + QByteArray::number(RecursionDepth)
                 + ", (unsigned long)i);\n"
                 "    return 0;\n"
                 "}\n");
    source.close();
    const FilePath exe = FilePath::fromString(dir.filePath("recurse"));
    Process compile;
    compile.setCommand({cc, {"-O1", "-g", "-fno-omit-frame-pointer", "-o", exe.path(),
                             source.fileName()}});
    compile.runBlocking();
    if (compile.result() != ProcessResult::FinishedWithSuccess) {
        *error = compile.allOutput();
        return std::nullopt;
    }
    return exe;
}

// Records `exe` with "perf record" and `outputArgs` ("-o", "-" for the stream
// on stdout), with frame-pointer call graphs; the stream, if on stdout.
std::optional<QByteArray> recordWithPerf(const FilePath &exe, const QStringList &outputArgs,
                                         QString *error)
{
    const FilePath perf = Environment::systemEnvironment().searchInPath("perf");
    if (perf.isEmpty()) {
        *error = u"no \"perf\" in PATH"_s;
        return std::nullopt;
    }
    Process record;
    record.setCommand({perf, QStringList{"record"} + outputArgs
                                 + QStringList{"--call-graph", "fp", "--", exe.path()}});
    record.runBlocking(std::chrono::seconds(60));
    if (record.result() != ProcessResult::FinishedWithSuccess) {
        *error = u"\"perf record\" failed here: %1"_s.arg(record.cleanedStdErr());
        return std::nullopt;
    }
    return record.rawStdOut();
}

// How deep the deepest stack of converterRecurse() the analyzer got is, and
// whether its source lines came along.
QPair<int, bool> recursionDepth(const PerfProfilerTraceManager &manager)
{
    int deepest = 0;
    bool hasLine = false;
    for (const PerfEvent &event : events(manager)) {
        if (event.attributeId(0) > PerfEvent::LastSpecialTypeId)
            continue; // not a sample
        int levels = 0;
        for (qint32 frame : event.origFrames()) {
            if (!symbolName(manager, frame).contains("converterRecurse"))
                continue;
            ++levels;
            const PerfEventType::Location &location = manager.location(frame);
            hasLine = hasLine || (manager.string(location.file).endsWith("recurse.c")
                                  && location.line > 0);
        }
        deepest = qMax(deepest, levels);
    }
    return {deepest, hasLine};
}

} // namespace

class PerfTraceConverterTest final : public QObject
{
    Q_OBJECT

private slots:
    void testConvertsEventsAndThreads();
    void testDecodesTracepointFields();
    void testConvertsRealRecording();
    void testLoadsPerfDataFile();
    void testConvertsFedRecording();
    void testFindsDeviceBinaries_data();
    void testFindsDeviceBinaries();
    void testNamesJitCode();
    void testExpandsInlinedFunctions();
    void testWritesEventsOnceNoEarlierOneCanCome();
    void testConvertsRecordingFromConnection();
    void testLetsGoOfRejectedConnection();
    void testHoldsBackConnectionWhileConverting();
    void testRetriesConnectionUntilDeviceListens();
    void testWarnsWhenItCannotUnwind();
    void testIgnoresCompletionOfEarlierRun();
    void testRestartsFedConversion();
    void testForkedChildKeepsParentMappings();
    void testPrefersBranchStackOverUserCallchain();
};

// Samples, with their stacks and event; a thread's name, start and end; a
// context switch; and a loss -- each where the analyzer expects it.
void PerfTraceConverterTest::testConvertsEventsAndThreads()
{
    constexpr quint32 pid = 5;
    constexpr quint64 sampleType = SampleIp | SampleTid | SampleTime | SampleCpu | SamplePeriod
                                   | SampleCallchain;

    QByteArray name;
    appendLe(name, 2, 8);  // PERF_EVENT_UPDATE__NAME
    appendLe(name, 1, 8);  // id
    name.append("task-clock\0\0\0\0\0\0", 16);

    QByteArray comm;
    appendLe(comm, pid, 4);
    appendLe(comm, pid, 4);
    comm.append("worker\0\0", 8);
    comm += trailer(pid, pid, 1000);

    QByteArray fork;
    appendLe(fork, pid, 4);
    appendLe(fork, pid, 4);
    appendLe(fork, 6, 4); // tid
    appendLe(fork, pid, 4);
    appendLe(fork, 1500, 8);

    QByteArray sample;
    appendLe(sample, 0x1000, 8);
    appendLe(sample, pid, 4);
    appendLe(sample, 6, 4);
    appendLe(sample, 2000, 8);
    appendLe(sample, 0, 8);    // cpu, res
    appendLe(sample, 1000, 8); // period
    appendLe(sample, 3, 8);    // callchain: nr
    appendLe(sample, PerfContextUser, 8);
    appendLe(sample, 0x1000, 8);
    appendLe(sample, 0x2000, 8);

    QByteArray lost;
    appendLe(lost, 1, 8);  // id
    appendLe(lost, 12, 8); // lost
    lost += trailer(pid, 6, 2500);

    QByteArray exit = fork;
    exit.replace(16, 8, QByteArray("\xb8\x0b\0\0\0\0\0\0", 8)); // time 3000

    const QByteArray converted = convert(pipeStream({
        attrRecord(1, 1, sampleType, 1),
        record(78, name),
        record(3, comm),
        record(7, fork),
        record(9, sample),
        record(14, trailer(pid, 6, 2200), 1 << 13), // switched out
        record(2, lost),
        record(4, exit),
        record(68, {}),
    }));

    PerfProfilerTraceManager manager;
    QVERIFY(load(converted, manager));

    QCOMPARE(manager.string(manager.thread(pid).name), QByteArray("worker"));

    const QList<PerfEvent> all = events(manager);
    const auto count = [&all](qint32 typeId) {
        return std::count_if(all.cbegin(), all.cend(),
                             [typeId](const PerfEvent &e) { return e.attributeId(0) == typeId; });
    };
    QCOMPARE(count(PerfEvent::ThreadStartTypeId), 1);
    QCOMPARE(count(PerfEvent::ThreadEndTypeId), 1);
    QCOMPARE(count(PerfEvent::ContextSwitchTypeId), 1);
    QCOMPARE(count(PerfEvent::LostTypeId), 1);

    // Attributes are the types below the special ones.
    const auto sampleIt = std::find_if(all.cbegin(), all.cend(), [](const PerfEvent &e) {
        return e.attributeId(0) <= PerfEvent::LastSpecialTypeId;
    });
    QVERIFY(sampleIt != all.cend());
    QCOMPARE(sampleIt->tid(), 6u);
    QCOMPARE(sampleIt->attributeValue(0), quint64(1000));
    QCOMPARE(manager.string(manager.attribute(sampleIt->attributeId(0)).name),
             QByteArray("task-clock"));
    // Innermost first; unmapped addresses are their own symbols.
    QCOMPARE(sampleIt->origFrames().size(), 2);
    QCOMPARE(symbolName(manager, sampleIt->origFrames().at(0)), QByteArray("0x1000"));
    QCOMPARE(symbolName(manager, sampleIt->origFrames().at(1)), QByteArray("0x2000"));

    const auto lostIt = std::find_if(all.cbegin(), all.cend(), [](const PerfEvent &e) {
        return e.attributeId(0) == PerfEvent::LostTypeId;
    });
    QCOMPARE(lostIt->attributeValue(0), quint64(12));
}

// A tracepoint's fields come out of its record by the format the recording's
// tracing data gives, by name.
void PerfTraceConverterTest::testDecodesTracepointFields()
{
    constexpr quint64 tracepointId = 316;
    const QByteArray format = "name: sched_switch\n"
                              "ID: 316\n"
                              "format:\n"
                              "\tfield:unsigned short common_type;\toffset:0;\tsize:2;\tsigned:0;\n"
                              "\tfield:int common_pid;\toffset:4;\tsize:4;\tsigned:1;\n"
                              "\n"
                              "\tfield:pid_t prev_pid;\toffset:8;\tsize:4;\tsigned:1;\n"
                              "\tfield:char next_comm[16];\toffset:12;\tsize:16;\tsigned:0;\n"
                              "\n"
                              "print fmt: \"prev_pid=%d\", REC->prev_pid\n";
    QByteArray tracing("\x17\x08\x44tracing", 10);
    tracing.append("0.6", 4);
    tracing.append(char(0));          // little-endian
    tracing.append(char(8));          // long size
    appendLe(tracing, 4096, 4);       // page size
    tracing.append("header_page", 12);
    appendLe(tracing, 0, 8);
    tracing.append("header_event", 13);
    appendLe(tracing, 0, 8);
    appendLe(tracing, 0, 4);          // ftrace formats
    appendLe(tracing, 1, 4);          // systems
    tracing.append("sched", 6);
    appendLe(tracing, 1, 4);
    appendLe(tracing, quint64(format.size()), 8);
    tracing.append(format);
    appendLe(tracing, 0, 4);          // kallsyms
    appendLe(tracing, 0, 4);          // printk formats
    while (tracing.size() % 8)
        tracing.append(char(0));
    QByteArray tracingRecord;
    appendLe(tracingRecord, quint64(tracing.size()), 4);

    QByteArray raw;
    appendLe(raw, tracepointId, 2);
    appendLe(raw, 0, 2);
    appendLe(raw, 7, 4);              // common_pid
    appendLe(raw, quint64(-42), 4);   // prev_pid
    raw.append("bash\0\0\0\0\0\0\0\0\0\0\0\0", 16);
    const auto sampleAt = [&raw](quint64 time) {
        QByteArray sample;
        appendLe(sample, 0x1000, 8);
        appendLe(sample, 7, 4);
        appendLe(sample, 7, 4);
        appendLe(sample, time, 8);
        appendLe(sample, 0, 8);       // cpu
        appendLe(sample, quint64(raw.size()), 4);
        sample.append(raw);
        while ((sample.size() + 8) % 8)
            sample.append(char(0));
        return record(9, sample);
    };

    const QByteArray converted = convert(pipeStream({
        attrRecord(2, tracepointId, SampleIp | SampleTid | SampleTime | SampleCpu | SampleRaw, 1),
        record(66, tracingRecord) + tracing,
        // Two: the analyzer's trace spans from its first event to its last.
        sampleAt(1000),
        sampleAt(2000),
    }));

    PerfProfilerTraceManager manager;
    QVERIFY(load(converted, manager));
    QCOMPARE(manager.tracePoints().size(), 1);
    const PerfProfilerTraceManager::TracePoint &tracePoint = manager.tracePoint(qint32(tracepointId));
    QCOMPARE(manager.string(tracePoint.system), QByteArray("sched"));
    QCOMPARE(manager.string(tracePoint.name), QByteArray("sched_switch"));

    const QList<PerfEvent> all = events(manager);
    QCOMPARE(all.size(), 2);
    QHash<QByteArray, QVariant> fields;
    for (auto it = all.first().traceData().cbegin(); it != all.first().traceData().cend(); ++it)
        fields.insert(manager.string(it.key()), it.value());
    QCOMPARE(fields.value("prev_pid").toLongLong(), -42);
    QCOMPARE(fields.value("next_comm").toByteArray(), QByteArray("bash"));
    QVERIFY(!fields.contains("common_pid"));
}

// A real recording of a program compiled here: its functions have to come out
// named, with their source lines, aggregated by function, at depth.
void PerfTraceConverterTest::testConvertsRealRecording()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("perf is Linux-only");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString error;
    const std::optional<FilePath> exe = compileRecursion(dir, &error);
    if (!exe)
        QSKIP(qPrintable(error));
    const std::optional<QByteArray> stream = recordWithPerf(*exe, {"-o", "-"}, &error);
    if (!stream)
        QSKIP(qPrintable(error));

    PerfProfilerTraceManager manager;
    QVERIFY(load(convert(*stream), manager));

    for (const PerfEvent &event : events(manager)) {
        for (qint32 frame : event.origFrames()) {
            if (symbolName(manager, frame).contains("converterRecurse")) {
                const PerfProfilerTraceManager::Symbol &symbol
                    = manager.symbol(manager.symbolLocation(frame));
                QCOMPARE(manager.string(symbol.binary), QByteArray("recurse"));
            }
        }
    }
    const auto [deepest, hasLine] = recursionDepth(manager);
    QVERIFY2(deepest >= RecursionDepth,
             qPrintable(u"deepest stack held %1 of %2 levels"_s.arg(deepest)
                            .arg(RecursionDepth + 1)));
    QVERIFY(hasLine);
}

// A perf.data file, loaded as the analyzer's "Load perf.data File" does.
void PerfTraceConverterTest::testLoadsPerfDataFile()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("perf is Linux-only");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString error;
    const std::optional<FilePath> exe = compileRecursion(dir, &error);
    if (!exe)
        QSKIP(qPrintable(error));
    const QString file = dir.filePath("perf.data");
    if (!recordWithPerf(*exe, {"-o", file}, &error))
        QSKIP(qPrintable(error));

    PerfProfilerTraceManager manager;
    QSignalSpy loaded(&manager, &Timeline::TimelineTraceManager::loadFinished);
    manager.loadFromPerfData(FilePath::fromString(file), dir.path(), nullptr);
    QVERIFY(loaded.wait(60000));

    const auto [deepest, hasLine] = recursionDepth(manager);
    QVERIFY2(deepest >= RecursionDepth,
             qPrintable(u"deepest stack held %1 of %2 levels"_s.arg(deepest)
                            .arg(RecursionDepth + 1)));
    QVERIFY(hasLine);
}

// A recording fed to PerfDataReader as it arrives, as a run of the analyzer
// does with what "perf record" writes to its output.
void PerfTraceConverterTest::testConvertsFedRecording()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("perf is Linux-only");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString error;
    const std::optional<FilePath> exe = compileRecursion(dir, &error);
    if (!exe)
        QSKIP(qPrintable(error));
    const std::optional<QByteArray> stream = recordWithPerf(*exe, {"-o", "-"}, &error);
    if (!stream)
        QSKIP(qPrintable(error));

    PerfProfilerTraceManager manager;
    PerfDataReader reader;
    reader.setTraceManager(&manager);
    connect(&reader, &PerfDataReader::started, &manager, &PerfProfilerTraceManager::initialize);
    connect(&reader, &PerfDataReader::finished, &manager, &PerfProfilerTraceManager::finalize);
    QSignalSpy finished(&reader, &PerfDataReader::processFinished);

    reader.createParser({});
    reader.startParser();
    // In pieces, as a run delivers it.
    for (qsizetype at = 0; at < stream->size(); at += 4096)
        QVERIFY(reader.feedParser(stream->mid(at, 4096)));
    reader.stopParser();
    QVERIFY(finished.wait(60000));

    const auto [deepest, hasLine] = recursionDepth(manager);
    QVERIFY2(deepest >= RecursionDepth,
             qPrintable(u"deepest stack held %1 of %2 levels"_s.arg(deepest)
                            .arg(RecursionDepth + 1)));
    QVERIFY(hasLine);
}

void PerfTraceConverterTest::testFindsDeviceBinaries_data()
{
    QTest::addColumn<bool>("bySysroot");
    QTest::newRow("in the sysroot") << true;
    QTest::newRow("by name, in a search path") << false;
}

// A recording from a device names the device's paths. Its binaries are found
// here under the kit's sysroot, by the same path, or by their name in the
// directories the application was built in; and a kernel address does not
// resolve against this machine's kernel symbols.
void PerfTraceConverterTest::testFindsDeviceBinaries()
{
    QFETCH(bool, bySysroot);
    if (!HostOsInfo::isLinuxHost())
        QSKIP("The binaries compiled here are ELF only on Linux");
    const FilePath cc = Environment::systemEnvironment().searchInPath("cc");
    const FilePath nm = Environment::systemEnvironment().searchInPath("nm");
    if (cc.isEmpty() || nm.isEmpty())
        QSKIP("no \"cc\" or \"nm\" in PATH");

    // Non-PIE, so that its addresses are known without running it.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString targetDir = bySysroot ? u"sysroot/opt/target"_s : u"build/src/app"_s;
    QVERIFY(QDir(dir.path()).mkpath(targetDir));
    const QString exe = dir.filePath(targetDir + u"/recurse"_s);
    QFile source(dir.filePath("recurse.c"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("__attribute__((noinline)) int deviceFunction(int x) { return x * 3 + 1; }\n"
                 "int main(void) { return deviceFunction(1); }\n");
    source.close();
    Process compile;
    compile.setCommand({cc, {"-O1", "-g", "-no-pie", "-o", exe, source.fileName()}});
    compile.runBlocking();
    QVERIFY2(compile.result() == ProcessResult::FinishedWithSuccess,
             qPrintable(compile.allOutput()));
    Process symbols;
    symbols.setCommand({nm, {"-P", exe}});
    symbols.runBlocking();
    quint64 function = 0;
    for (const QString &line : symbols.cleanedStdOut().split(u'\n')) {
        const QStringList fields = line.split(u' ');
        if (fields.size() >= 3 && fields.at(0) == u"deviceFunction"_s)
            function = fields.at(2).toULongLong(nullptr, 16);
    }
    QVERIFY(function != 0);

    // The text segment of a non-PIE x86-64 executable, as it maps it.
    constexpr quint32 pid = 77;
    QByteArray mmap;
    appendLe(mmap, pid, 4);
    appendLe(mmap, pid, 4);
    appendLe(mmap, 0x400000, 8); // addr
    appendLe(mmap, 0x100000, 8); // len
    appendLe(mmap, 0, 8);        // pgoff
    mmap.append("/opt/target/recurse", 20);
    while (mmap.size() % 8)
        mmap.append(char(0));
    mmap += trailer(pid, pid, 500);

    constexpr quint64 PerfContextKernel = quint64(-128);
    const quint64 kernelAddress = 0xffffffff81000000ull;
    const auto sampleAt = [&](quint64 time) {
        QByteArray sample;
        appendLe(sample, function + 4, 8);
        appendLe(sample, pid, 4);
        appendLe(sample, pid, 4);
        appendLe(sample, time, 8);
        appendLe(sample, 0, 8);    // cpu
        appendLe(sample, 1, 8);    // period
        appendLe(sample, 4, 8);    // callchain: nr
        appendLe(sample, PerfContextKernel, 8);
        appendLe(sample, kernelAddress, 8);
        appendLe(sample, PerfContextUser, 8);
        appendLe(sample, function + 4, 8);
        return record(9, sample);
    };
    const QByteArray stream = pipeStream({
        attrRecord(1, 1, SampleIp | SampleTid | SampleTime | SampleCpu | SamplePeriod
                             | SampleCallchain, 1),
        record(1, mmap),
        sampleAt(1000),
        sampleAt(2000),
    });

    const QByteArray converted = convert(stream, [&](PerfTraceConverter &converter) {
        if (bySysroot) {
            converter.symbolizer().setBinaryLocations(FilePath::fromString(dir.filePath("sysroot")),
                                                      {});
        } else {
            converter.symbolizer().setBinaryLocations({},
                                                      {FilePath::fromString(dir.filePath("build"))});
        }
        converter.setForeignRecording(true);
    });
    PerfProfilerTraceManager manager;
    QVERIFY(load(converted, manager));

    const QList<PerfEvent> all = events(manager);
    QVERIFY(!all.isEmpty());
    const PerfEvent &sample = all.first();
    QCOMPARE(sample.origFrames().size(), 2);
    // Innermost first: the kernel's frame, then the one in the program.
    QCOMPARE(symbolName(manager, sample.origFrames().at(0)),
             u"0x%1"_s.arg(kernelAddress, 0, 16).toUtf8());
    QCOMPARE(symbolName(manager, sample.origFrames().at(1)), QByteArray("deviceFunction"));
    const PerfProfilerTraceManager::Symbol &symbol
        = manager.symbol(manager.symbolLocation(sample.origFrames().at(1)));
    QCOMPARE(manager.string(symbol.path), QByteArray("/opt/target/recurse"));
    QCOMPARE(QString::fromUtf8(manager.string(symbol.actualPath)), exe);
}

// Code a JIT generated is in no module's symbols. QML's JIT writes where its
// functions are into /tmp/perf-<pid>.map for perf, with the code itself in
// an executable memfd region, and the analyzer tells JavaScript frames by that
// region's name (see perfnativemixed.cpp).
void PerfTraceConverterTest::testNamesJitCode()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("JIT maps are a Linux convention");
    // A pid no process here has, so that no real map is in the way.
    constexpr quint32 pid = 4194000;
    QFile map(u"/tmp/perf-%1.map"_s.arg(pid));
    if (map.exists())
        QSKIP("a JIT map of that pid exists already");
    QVERIFY(map.open(QIODevice::WriteOnly));
    const QScopeGuard removeMap([&map] { map.remove(); });
    map.write("7f0000001000 40 compute\n7f0000001040 20 other\n");
    map.close();

    QByteArray mmap;
    appendLe(mmap, pid, 4);
    appendLe(mmap, pid, 4);
    appendLe(mmap, 0x7f0000000000ull, 8); // addr
    appendLe(mmap, 0x10000, 8);           // len
    appendLe(mmap, 0, 8);                 // pgoff
    mmap.append("/memfd:JITCode:QtQml (deleted)", 31);
    while (mmap.size() % 8)
        mmap.append(char(0));
    mmap += trailer(pid, pid, 500);
    const auto sampleAt = [&](quint64 time) {
        QByteArray sample;
        appendLe(sample, 0x7f0000001010ull, 8);
        appendLe(sample, pid, 4);
        appendLe(sample, pid, 4);
        appendLe(sample, time, 8);
        appendLe(sample, 0, 8); // cpu
        appendLe(sample, 1, 8); // period
        appendLe(sample, 1, 8); // callchain: nr
        appendLe(sample, 0x7f0000001010ull, 8);
        return record(9, sample);
    };

    PerfProfilerTraceManager manager;
    QVERIFY(load(convert(pipeStream({
                     attrRecord(1, 1, SampleIp | SampleTid | SampleTime | SampleCpu
                                          | SamplePeriod | SampleCallchain, 1),
                     record(1, mmap),
                     sampleAt(1000),
                     sampleAt(2000),
                 })),
                 manager));

    const QList<PerfEvent> all = events(manager);
    QVERIFY(!all.isEmpty());
    const qint32 frame = all.first().origFrames().value(0, -1);
    QVERIFY(frame >= 0);
    QCOMPARE(symbolName(manager, frame), QByteArray("compute"));
    QCOMPARE(frameKind(manager, frame), FrameKind::Js);
}

// The analyzer shows the functions inlined at an address as frames of their
// own. Expands a recording's frames the way PerfProfilerTraceManager does,
// and expects them innermost first, each by its own name.
void PerfTraceConverterTest::testExpandsInlinedFunctions()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("perf is Linux-only");
    const FilePath cc = Environment::systemEnvironment().searchInPath("cc");
    if (cc.isEmpty())
        QSKIP("no \"cc\" in PATH");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile source(dir.filePath("inlined.c"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("volatile unsigned long sink;\n"
                 "static inline __attribute__((always_inline)) unsigned long\n"
                 "inlinedLeaf(unsigned long acc)\n"
                 "{\n"
                 "    for (int i = 0; i < 2000; ++i)\n"
                 "        acc = acc * 6364136223846793005ul + 1442695040888963407ul;\n"
                 "    return acc;\n"
                 "}\n"
                 "static inline __attribute__((always_inline)) unsigned long\n"
                 "inlinedMiddle(unsigned long acc)\n"
                 "{\n"
                 "    return inlinedLeaf(acc) ^ 0x55;\n"
                 "}\n"
                 "__attribute__((noinline)) unsigned long outerFunction(unsigned long acc)\n"
                 "{\n"
                 "    const unsigned long result = inlinedMiddle(acc);\n"
                 "    sink = result;\n"
                 "    return result;\n"
                 "}\n"
                 "int main(void)\n"
                 "{\n"
                 "    for (long i = 0; i < 300000; ++i)\n"
                 "        outerFunction((unsigned long)i);\n"
                 "    return 0;\n"
                 "}\n");
    source.close();
    const FilePath exe = FilePath::fromString(dir.filePath("inlined"));
    Process compile;
    compile.setCommand({cc, {"-O2", "-g", "-fno-omit-frame-pointer", "-o", exe.path(),
                             source.fileName()}});
    compile.runBlocking();
    QVERIFY2(compile.result() == ProcessResult::FinishedWithSuccess,
             qPrintable(compile.allOutput()));
    QString error;
    const std::optional<QByteArray> stream = recordWithPerf(exe, {"-o", "-"}, &error);
    if (!stream)
        QSKIP(qPrintable(error));

    PerfProfilerTraceManager manager;
    QVERIFY(load(convert(*stream), manager));

    const auto expanded = [&manager](const PerfEvent &event) {
        QList<QByteArray> names;
        for (qint32 frame : event.origFrames()) {
            while (frame >= 0) {
                const qint32 symbolLocation = manager.symbol(frame).name != -1
                                                  ? frame
                                                  : manager.location(frame).parentLocationId;
                if (symbolLocation < 0)
                    break;
                names.append(manager.string(manager.symbol(symbolLocation).name));
                frame = manager.location(symbolLocation).parentLocationId;
            }
        }
        return names;
    };
    const QList<QByteArray> expected{"inlinedLeaf", "inlinedMiddle", "outerFunction"};
    bool found = false;
    for (const PerfEvent &event : events(manager)) {
        const QList<QByteArray> names = expanded(event);
        for (qsizetype i = 0; i + 3 <= names.size(); ++i) {
            if (names.mid(i, 3) == expected)
                found = true;
        }
    }
    QVERIFY(found);
}

// Events arrive from several per-CPU buffers; a round only completes those
// up to where the round before reached, as a buffer read late in it may
// still hold earlier ones, which the next round brings.
void PerfTraceConverterTest::testWritesEventsOnceNoEarlierOneCanCome()
{
    constexpr quint64 sampleType = SampleIp | SampleTid | SampleTime;
    const auto sample = [](quint64 time) {
        QByteArray body;
        appendLe(body, 0x1000, 8);
        appendLe(body, 5, 4);
        appendLe(body, 5, 4);
        appendLe(body, time, 8);
        return record(9, body);
    };

    const QByteArray converted = convert(pipeStream({
        attrRecord(1, 1, sampleType, 1),
        sample(1000),
        record(68, {}),
        sample(3000),
        record(68, {}),
        sample(2000), // CPU 1's, read late
        sample(4000),
        record(68, {}),
    }));

    QCOMPARE(sampleTimes(converted), QList<quint64>({1000, 2000, 3000, 4000}));
}

// A device that sends the recording over a connection (qdb) rather than on
// the run's output: all of it arrives, though the conversion is slower than
// the connection, and the run is live, with its delay kept track of.
void PerfTraceConverterTest::testConvertsRecordingFromConnection()
{
    // More than the conversion takes in at once from a connection.
    constexpr int SampleCount = 250'000;
    const QByteArray stream = manySamples(SampleCount);

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QUrl url;
    url.setScheme("tcp");
    url.setHost(server.serverAddress().toString());
    url.setPort(server.serverPort());

    PerfProfilerTraceManager manager;
    PerfDataReader reader;
    reader.setTraceManager(&manager);
    connect(&reader, &PerfDataReader::started, &manager, &PerfProfilerTraceManager::initialize);
    connect(&reader, &PerfDataReader::finished, &manager, &PerfProfilerTraceManager::finalize);
    QSignalSpy finished(&reader, &PerfDataReader::processFinished);
    QSignalSpy timestamps(&reader, &PerfDataReader::updateTimestamps);

    reader.createParser({}, url);
    reader.startParser();
    QVERIFY(server.waitForNewConnection(10000));
    QTcpSocket *device = server.nextPendingConnection();
    device->write(stream.left(stream.size() / 2));
    // The run ends while the device still sends; that does not end the
    // recording.
    reader.stopParser();
    device->write(stream.mid(stream.size() / 2));
    connect(device, &QTcpSocket::bytesWritten, device, [device] {
        if (device->bytesToWrite() == 0)
            device->disconnectFromHost();
    });
    QVERIFY(finished.wait(120000));

    const QList<PerfEvent> all = events(manager);
    const auto samples = std::count_if(all.cbegin(), all.cend(), [](const PerfEvent &e) {
        return e.attributeId(0) <= PerfEvent::LastSpecialTypeId;
    });
    QCOMPARE(samples, SampleCount);
    QVERIFY(std::any_of(timestamps.cbegin(), timestamps.cend(), [](const QList<QVariant> &args) {
        return args.at(1).toLongLong() > 0;
    }));
}

// What the conversion rejects, the device no longer gets to send.
void PerfTraceConverterTest::testLetsGoOfRejectedConnection()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QUrl url;
    url.setScheme("tcp");
    url.setHost(server.serverAddress().toString());
    url.setPort(server.serverPort());

    PerfConversion conversion;
    QSignalSpy done(&conversion, &PerfConversion::done);
    conversion.setInputUrl(url);
    conversion.start();
    QVERIFY(server.waitForNewConnection(10000));
    QTcpSocket *device = server.nextPendingConnection();
    QSignalSpy disconnected(device, &QTcpSocket::disconnected);
    device->write(QByteArray("NOTPERF!") + QByteArray(1 << 16, '\0'));

    QVERIFY(done.wait(10000));
    QCOMPARE(conversion.result(), PerfConversion::Result::Failed);
    QVERIFY(disconnected.wait(10000));
}

// What a connection delivers faster than the conversion takes it waits with
// the device, and comes once the conversion has caught up.
void PerfTraceConverterTest::testHoldsBackConnectionWhileConverting()
{
    // Beyond what the connection and the socket buffer hold.
    constexpr int SampleCount = 1'000'000;
    const QByteArray stream = manySamples(SampleCount);

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QUrl url;
    url.setScheme("tcp");
    url.setHost(server.serverAddress().toString());
    url.setPort(server.serverPort());

    PerfConversion conversion;
    QByteArray converted;
    connect(&conversion, &PerfConversion::readyRead, &conversion, [&] {
        converted += conversion.readAllOutput();
    });
    QSignalSpy done(&conversion, &PerfConversion::done);
    conversion.setInputUrl(url);
    // Full after every read.
    conversion.setMaxQueuedBytes(1);
    conversion.start();
    QVERIFY(server.waitForNewConnection(10000));
    QTcpSocket *device = server.nextPendingConnection();
    device->write(stream);
    connect(device, &QTcpSocket::bytesWritten, device, [device] {
        if (device->bytesToWrite() == 0)
            device->disconnectFromHost();
    });

    QVERIFY(done.wait(60000));
    converted += conversion.readAllOutput();
    QCOMPARE(conversion.result(), PerfConversion::Result::Success);
    QCOMPARE(sampleTimes(converted).size(), SampleCount);
}

// A device listens only once its application runs, which the conversion's
// first attempt to connect can come before.
void PerfTraceConverterTest::testRetriesConnectionUntilDeviceListens()
{
    QTcpServer probe;
    QVERIFY(probe.listen(QHostAddress::LocalHost));
    const quint16 port = probe.serverPort();
    probe.close();
    QUrl url;
    url.setScheme("tcp");
    url.setHost(QHostAddress(QHostAddress::LocalHost).toString());
    url.setPort(port);

    PerfConversion conversion;
    QSignalSpy done(&conversion, &PerfConversion::done);
    conversion.setInputUrl(url);
    conversion.start();
    auto socket = conversion.findChild<QTcpSocket *>();
    QVERIFY(socket);
    // Listens once the first attempt is refused.
    QTcpServer server;
    connect(socket, &QTcpSocket::errorOccurred, &server, [&server, port] {
        if (!server.isListening())
            server.listen(QHostAddress::LocalHost, port);
    });
    // The retry needs the event loop, which waitForNewConnection() blocks.
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
    QTcpSocket *device = server.nextPendingConnection();
    device->write(manySamples(100));
    connect(device, &QTcpSocket::bytesWritten, device, [device] {
        if (device->bytesToWrite() == 0)
            device->disconnectFromHost();
    });

    QVERIFY(done.wait(10000));
    QCOMPARE(conversion.result(), PerfConversion::Result::Success);
}

// A build without libdw cannot unwind "dwarf" call graphs, which leaves the
// samples without the user part of their stacks; the user is told, once.
void PerfTraceConverterTest::testWarnsWhenItCannotUnwind()
{
    constexpr quint64 SampleRegsUser = 1ull << 12;
    const PerfRegisterLayout layout = perfRegisterLayout(hostPerfArchitecture());
    if (layout.ip < 0)
        QSKIP("perf has no user registers for this architecture.");
    const quint64 regsMask = (1ull << layout.sp) | (1ull << layout.ip); // sp sorts first

    QByteArray attr;
    appendLe(attr, 1, 4);  // type
    appendLe(attr, 96, 4); // size: up to sample_regs_user and past it
    appendLe(attr, 1, 8);  // config
    appendLe(attr, 1000, 8);
    appendLe(attr, SampleIp | SampleTid | SampleTime | SampleCallchain | SampleRegsUser, 8);
    attr.append(QByteArray(80 - 32, '\0'));
    appendLe(attr, regsMask, 8); // sample_regs_user
    attr.append(QByteArray(96 - 88, '\0'));
    appendLe(attr, 1, 8); // id
    QList<QByteArray> records{record(64, attr)};
    for (quint64 time : {1000, 2000}) {
        QByteArray sample;
        appendLe(sample, 0x1000, 8); // ip
        appendLe(sample, 5, 4);
        appendLe(sample, 5, 4);
        appendLe(sample, time, 8);
        appendLe(sample, 0, 8);              // callchain: perf leaves the user part out
        appendLe(sample, 2, 8);              // abi: PERF_SAMPLE_REGS_ABI_64
        appendLe(sample, 0x7fff0000, 8);     // sp
        appendLe(sample, 0x1000, 8);         // ip
        records.append(record(9, sample));
    }
    const QByteArray stream = pipeStream(records);

    const auto warningsFor = [&stream](bool canUnwind) {
        QStringList warnings;
        convert(stream, [&warnings, canUnwind](PerfTraceConverter &converter) {
            converter.setCanUnwind(canUnwind);
            converter.setWarningHandler([&warnings](const QString &warning) {
                warnings.append(warning);
            });
        });
        return warnings;
    };
    const QStringList cannot = warningsFor(false);
    QCOMPARE(cannot.size(), 1);
    QVERIFY2(cannot.first().contains("frame pointer"_L1), qPrintable(cannot.first()));
    QVERIFY(warningsFor(true).isEmpty());
}

// A start() while the previous run's completion is still on its way: that
// completion must not end the new run.
void PerfTraceConverterTest::testIgnoresCompletionOfEarlierRun()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath recording = FilePath::fromString(dir.filePath("recording"));
    QVERIFY_RESULT(recording.writeFileContents(manySamples(1000)));

    PerfConversion conversion;
    conversion.setInputFile(recording);
    int started = 0;
    int doneCount = 0;
    int readyReadAfterDone = 0;
    bool allDelivered = false;
    connect(&conversion, &PerfConversion::started, this, [&started] { ++started; });
    connect(&conversion, &PerfConversion::readyRead, this, [&] {
        if (doneCount > 0)
            ++readyReadAfterDone;
    });
    connect(&conversion, &PerfConversion::done, this, [&] {
        ++doneCount;
        // Whatever the conversion queued before done is delivered before this.
        QMetaObject::invokeMethod(this, [&allDelivered] { allDelivered = true; },
                                  Qt::QueuedConnection);
    });
    conversion.start();
    conversion.start(); // ends the first run, whose completion is queued

    QTRY_VERIFY(allDelivered);
    QCOMPARE(started, 1);
    QCOMPARE(doneCount, 1);
    QCOMPARE(readyReadAfterDone, 0);
    QCOMPARE(conversion.result(), PerfConversion::Result::Success);
}

// A run fed through write() waits for input that only the thread calling
// start() can give; starting again must not wait for it.
void PerfTraceConverterTest::testRestartsFedConversion()
{
    const QByteArray stream = manySamples(100);
    std::atomic<PerfConversion *> running = nullptr;
    std::unique_ptr<QThread> restarting(QThread::create([&] {
        PerfConversion conversion;
        running = &conversion;
        conversion.start();
        conversion.write(stream.left(stream.size() / 2));
        conversion.start();
        running = nullptr;
        conversion.kill();
        conversion.waitForFinished();
    }));
    const auto join = qScopeGuard([&] {
        // Gives the first run the end of its input, should start() wait for it.
        if (!restarting->isFinished()) {
            if (PerfConversion *conversion = running.load())
                conversion->closeWriteChannel();
        }
        restarting->wait();
    });
    restarting->start();
    QTRY_VERIFY_WITH_TIMEOUT(restarting->isFinished(), 10000);
}

// A child forked without exec() runs in the mappings it inherited, which the
// kernel does not report for it again.
void PerfTraceConverterTest::testForkedChildKeepsParentMappings()
{
    constexpr quint32 parent = 5;
    constexpr quint32 child = 6;
    QByteArray mmap;
    appendLe(mmap, parent, 4);
    appendLe(mmap, parent, 4);
    appendLe(mmap, 0x400000, 8); // addr
    appendLe(mmap, 0x100000, 8); // len
    appendLe(mmap, 0, 8);        // pgoff
    mmap.append("/opt/target/app\0", 16);
    mmap += trailer(parent, parent, 500);

    QByteArray fork;
    appendLe(fork, child, 4);
    appendLe(fork, parent, 4);
    appendLe(fork, child, 4);  // tid
    appendLe(fork, parent, 4); // ptid
    appendLe(fork, 1000, 8);

    QByteArray sample;
    appendLe(sample, 0x401000, 8);
    appendLe(sample, child, 4);
    appendLe(sample, child, 4);
    appendLe(sample, 2000, 8);
    appendLe(sample, 0, 8); // cpu, res
    appendLe(sample, 1, 8); // period

    const QByteArray converted = convert(pipeStream({
        attrRecord(1, 1, SampleIp | SampleTid | SampleTime | SampleCpu | SamplePeriod, 1),
        record(1, mmap),
        record(7, fork),
        record(9, sample),
    }));
    PerfProfilerTraceManager manager;
    QVERIFY(load(converted, manager));

    const QList<PerfEvent> all = events(manager);
    const auto sampleIt = std::find_if(all.cbegin(), all.cend(), [](const PerfEvent &e) {
        return e.attributeId(0) <= PerfEvent::LastSpecialTypeId;
    });
    QVERIFY(sampleIt != all.cend());
    QCOMPARE(sampleIt->origFrames().size(), 1);
    const PerfProfilerTraceManager::Symbol &symbol
        = manager.symbol(manager.symbolLocation(sampleIt->origFrames().at(0)));
    QCOMPARE(manager.string(symbol.path), QByteArray("/opt/target/app"));
}

// With "--call-graph lbr", the user part of the call chain is a frame pointer
// walk, garbage in code without frame pointers; the branch stack has the
// callers.
void PerfTraceConverterTest::testPrefersBranchStackOverUserCallchain()
{
    constexpr quint64 SampleBranchStack = 1ull << 11;
    constexpr quint32 pid = 5;
    // Two, as a trace of a single event is empty.
    const auto sampleAt = [](quint64 time) {
        QByteArray sample;
        appendLe(sample, 0x1000, 8);
        appendLe(sample, pid, 4);
        appendLe(sample, pid, 4);
        appendLe(sample, time, 8);
        appendLe(sample, 0, 8); // cpu, res
        appendLe(sample, 1, 8); // period
        appendLe(sample, 4, 8); // callchain: nr
        appendLe(sample, PerfContextUser, 8);
        appendLe(sample, 0x1000, 8);
        appendLe(sample, 0x5000, 8); // what the frame pointer walk made of it
        appendLe(sample, 0x6000, 8);
        appendLe(sample, 2, 8); // branch stack: nr
        for (quint64 from : {0x2000, 0x3000}) {
            appendLe(sample, from, 8);
            appendLe(sample, 0, 8); // to
            appendLe(sample, 0, 8); // flags
        }
        return record(9, sample);
    };

    const QByteArray converted = convert(pipeStream({
        attrRecord(1, 1, SampleIp | SampleTid | SampleTime | SampleCpu | SamplePeriod
                             | SampleCallchain | SampleBranchStack, 1),
        sampleAt(1000),
        sampleAt(2000),
    }));
    PerfProfilerTraceManager manager;
    QVERIFY(load(converted, manager));

    const QList<PerfEvent> all = events(manager);
    const auto sampleIt = std::find_if(all.cbegin(), all.cend(), [](const PerfEvent &e) {
        return e.attributeId(0) <= PerfEvent::LastSpecialTypeId;
    });
    QVERIFY(sampleIt != all.cend());
    QList<QByteArray> frames;
    for (qint32 frame : sampleIt->origFrames())
        frames.append(symbolName(manager, frame));
    const QList<QByteArray> expected{"0x1000", "0x2000", "0x3000"};
    QCOMPARE(frames, expected);
}

QObject *createPerfTraceConverterTest()
{
    return new PerfTraceConverterTest;
}

} // namespace Profiler::Internal

#include "perftraceconverter_test.moc"
