// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfsampler_test.h"

#include <profiler/perfsampler.h>
#include <profiler/sampler.h>
#include <profiler/sampletrace.h>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QtTaskTree/QTaskTree>

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <algorithm>
#include <thread>
#include <optional>

#include <signal.h>

using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace Profiler::Internal {

class PerfSamplerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testSurfacesPerfStderrOnFailure();
    void testReportsParanoidRestriction();
    void testReportsPerfMissingForKernel();
    void testOffersBackendWithoutPerf();
    void testEndsRecordingOnDecodeError();
    void testCompletesPromptlyAndReportsProgress();
    void testRecordsCallGraph_data();
    void testRecordsCallGraph();
    void testRecordsDifferentlyLinkedBinaries_data();
    void testRecordsDifferentlyLinkedBinaries();
    void testRecordsInlinedFunctions();
    void testCapturesThreadName();
    void testCapturesLaunchedThreadNames();
    void testCapturesThreadNamesWhenTargetExitsFirst();
};

// Points the PATH PerfSampler searches for "perf" at `dir` alone, for as long
// as it lives.
class PathOverride
{
public:
    explicit PathOverride(const QString &dir)
        : m_saved(Environment::systemEnvironment().value("PATH"))
    {
        Environment::modifySystemEnvironment({{"PATH", dir}});
    }
    ~PathOverride() { Environment::modifySystemEnvironment({{"PATH", m_saved}}); }

private:
    const QString m_saved;
};

static std::optional<int> perfEventParanoid()
{
    const Result<QByteArray> contents =
        FilePath::fromString("/proc/sys/kernel/perf_event_paranoid"_L1).fileContents();
    if (!contents)
        return std::nullopt;
    bool ok = false;
    const int value = QString::fromLatin1(*contents).trimmed().toInt(&ok);
    return ok ? std::optional(value) : std::nullopt;
}

// perf_event_paranoid denying sampling altogether is not what a test
// recording a real target is about, so it skips then -- and only then.
#define SKIP_IF_PERF_CANNOT_SAMPLE(result) \
    do { \
        if (!(result) && (result).error().contains("perf_event_paranoid"_L1)) { \
            QSKIP(qPrintable(u"\"perf record\" cannot sample in this environment: %1"_s \
                                 .arg((result).error()))); \
        } \
    } while (false)

// The names of `pid`'s threads, read from /proc.
static QStringList threadNamesOf(qint64 pid)
{
    QStringList names;
    const QDir taskDir(u"/proc/%1/task"_s.arg(pid));
    for (const QString &tid : taskDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile comm(taskDir.filePath(tid + u"/comm"_s));
        if (comm.open(QIODevice::ReadOnly | QIODevice::Text))
            names.append(QString::fromUtf8(comm.readLine()).trimmed());
    }
    return names;
}

// Records a sleeping target, which perf is expected to fail on, and returns
// the error the recording ended with -- or an empty string when it did not
// fail, which the QVERIFYs of the caller then catch. With `perfDir` set, that
// is the only place "perf" is looked for; the target is started before.
static QString failedRecordingError(const PerfSampler &sampler, const QString &perfDir = {})
{
    Process target;
    target.setCommand({"sleep", {"60"}});
    target.start();
    if (!target.waitForStarted())
        return {};

    auto session = std::make_shared<RecordingSession>();
    session->pid.store(target.processId());
    {
        std::optional<PathOverride> path;
        if (!perfDir.isEmpty())
            path.emplace(perfDir);
        QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});
    }

    target.stop();
    target.waitForFinished();

    if (!session->result || *session->result)
        return {};
    return session->result->error();
}

void PerfSamplerTest::initTestCase()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("The Perf sampler is Linux-only");
    if (Environment::systemEnvironment().searchInPath("perf").isEmpty())
        QSKIP("no \"perf\" in PATH");
}

// "perf record" that cannot sample says why on stderr and writes nothing to
// stdout, so the recording is empty. The reason has to reach the error, or
// all a user learns is that nothing was captured. An event that does not
// exist makes perf fail wherever this runs -- and fail on something no
// perf_event_paranoid level is to blame for, so perf's own words are what has
// to be reported even where that setting also denies sampling.
void PerfSamplerTest::testSurfacesPerfStderrOnFailure()
{
    PerfSampler sampler;
    auto *settings = qobject_cast<PerfSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->perfSettings.events.setValue({"no_such_event"});

    const QString error = failedRecordingError(sampler);
    QVERIFY2(error.contains("no_such_event"_L1), qPrintable(error));
    QVERIFY2(!error.contains("perf_event_paranoid"_L1), qPrintable(error));
}

// Where perf_event_paranoid denies unprivileged sampling, the error has to
// name the setting and the level it needs, which is what the fix offered
// alongside it changes.
void PerfSamplerTest::testReportsParanoidRestriction()
{
    const std::optional<int> paranoid = perfEventParanoid();
    if (!paranoid || *paranoid < 3)
        QSKIP("perf_event_paranoid permits unprivileged sampling here");

    PerfSampler sampler;
    const QString error = failedRecordingError(sampler);
    QVERIFY2(error.contains("perf_event_paranoid"_L1), qPrintable(error));
    QVERIFY2(error.contains(u"is %1"_s.arg(*paranoid)), qPrintable(error));
}

// On Ubuntu, "perf" in PATH is a wrapper that needs a package matching the
// running kernel, and says so on stderr when that is missing -- which can be
// the case whatever perf_event_paranoid says. Stands in a script that fails
// the way that wrapper does.
void PerfSamplerTest::testReportsPerfMissingForKernel()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile wrapper(dir.filePath("perf"));
    QVERIFY(wrapper.open(QIODevice::WriteOnly));
    wrapper.write("#!/bin/sh\n"
                  "echo 'WARNING: perf not found for kernel 1.2.3-4' >&2\n"
                  "echo >&2\n"
                  "echo '  You may need to install the following packages for this specific "
                  "kernel:' >&2\n"
                  "echo '    linux-tools-1.2.3-4-generic' >&2\n"
                  "exit 2\n");
    wrapper.close();
    QVERIFY(wrapper.setPermissions(wrapper.permissions() | QFileDevice::ExeOwner));

    PerfSampler sampler;
    {
        const PathOverride path(dir.path());
        QVERIFY(sampler.isAvailable());
    }
    const QString error = failedRecordingError(sampler, dir.path());
    QVERIFY2(error.contains("perf not found for kernel"_L1), qPrintable(error));
    QVERIFY2(error.contains("linux-tools-1.2.3-4-generic"_L1), qPrintable(error));
    QVERIFY2(!error.contains("perf_event_paranoid"_L1), qPrintable(error));
}

// Without "perf", the backend stays on offer, and starting it says what to
// install -- rather than the backend silently missing from the selection.
void PerfSamplerTest::testOffersBackendWithoutPerf()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    PerfSampler sampler;
    const PathOverride path(dir.path());
    QVERIFY(sampler.isOffered());
    QString error;
    QVERIFY(!sampler.isAvailable(&error));
    QVERIFY2(error.contains("\"perf\""_L1), qPrintable(error));
    QVERIFY2(error.contains("linux-tools"_L1), qPrintable(error));
}

// Records a real, busy target and checks the two things this backend used to
// get wrong after "Stop Recording": post-processing (debuginfod fetches,
// then per-sample resolution, then writing the trace -- see
// PerfRecordReader::read()/symbolize() and writeSampleTrace()) reports
// visible, non-decreasing progress instead of sitting at 0% throughout, and
// it doesn't quietly stall for tens of seconds when DEBUGINFOD_URLS is
// configured (common: Ubuntu sets it system-wide) but the server is slow or
// unreachable -- every fetch used to wait out a 30s timeout with no feedback
// at all; see the shortened default and the elapsed-time progress ticker in
// PerfRecordDecoder::symbolize().
void PerfSamplerTest::testCompletesPromptlyAndReportsProgress()
{
    // Busy for a bounded amount of work, then gone: "perf record --pid" ends
    // with its target, which is what ends the recording here.
    Process target;
    target.setCommand({"bash", {"-c", "for ((i = 0; i < 1000000; ++i)); do :; done"}});
    target.start();
    QVERIFY(target.waitForStarted());

    PerfSampler sampler;
    auto *settings = qobject_cast<PerfSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->perfSettings.callgraphMode.setValue(1); // "fp": the only mode this reader supports
    settings->downloadDebugInfo.setValue(true); // exercise the fetch stage where a server is set

    auto session = std::make_shared<RecordingSession>();
    session->pid.store(target.processId());

    QList<int> observedProgress;
    QObject::connect(session->reporter(), &RecordingReporter::changed, [&] {
        observedProgress.append(session->progressPercent());
    });

    QElapsedTimer overall;
    overall.start();
    QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});
    const qint64 totalMs = overall.elapsed();

    target.waitForFinished();

    QVERIFY(session->result.has_value());
    SKIP_IF_PERF_CANNOT_SAMPLE(*session->result);
    QVERIFY_RESULT(*session->result);
    // A generous bound: even a fully unreachable debuginfod server (capped at
    // a 5s timeout per fetch, run in parallel) plus the recording and write
    // stages should land nowhere near this; the pre-fix behavior for this
    // exact scenario took 30+ seconds on just one fetch.
    QVERIFY2(totalMs < 20000, qPrintable(u"took %1 ms"_s.arg(totalMs)));

    // Progress must show *some* movement beyond just an initial 0 and a
    // final 100 -- the whole point of the fixes this test exercises.
    const bool sawIntermediateProgress = std::any_of(
        observedProgress.cbegin(), observedProgress.cend(),
        [](int p) { return p > 0 && p < 100; });
    QVERIFY(sawIntermediateProgress);
}

// User-reported: thread names don't seem to show up in a recorded trace.
// Records a real, busy target whose comm name is set to something
// recognizable, then reads the written trace back and checks its name made
// it through -- exercising the real "perf record" -> handleComm() ->
// writeSampleTrace() -> readSampleTrace() round trip end to end, not just
// handleComm()'s own byte parsing in isolation.
void PerfSamplerTest::testCapturesThreadName()
{
    // Linux's comm (/proc/pid/comm) comes from the *executed file's own*
    // name, not argv[0] -- "bash -c 'exec -a NAME ...'" does not affect it at
    // all (verified: comm still reads "bash"/the real binary name). A symlink
    // named the way we want run directly, though, does get picked up: execve()
    // takes the name as given, without resolving the symlink target's own
    // name first.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath bashPath = Environment::systemEnvironment().searchInPath("bash");
    QVERIFY(!bashPath.isEmpty());
    const FilePath renamedBash = FilePath::fromString(dir.filePath("qtcSamplerTest"));
    QVERIFY(QFile::link(bashPath.toFSPathString(), renamedBash.toFSPathString()));

    Process target;
    // Busy for a bounded amount of work, then gone, which ends the recording.
    target.setCommand({renamedBash, {"-c", "for ((i = 0; i < 1000000; ++i)); do :; done"}});
    target.start();
    QVERIFY(target.waitForStarted());
    // Process::processId() reports 0 once the process is no longer running
    // (see target.stop()/waitForFinished() below) -- captured once, up front,
    // rather than re-queried after the target has already been stopped.
    const qint64 targetPid = target.processId();
    QVERIFY(targetPid != 0);

    PerfSampler sampler;
    auto *settings = qobject_cast<PerfSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->perfSettings.callgraphMode.setValue(1); // "fp"

    auto session = std::make_shared<RecordingSession>();
    session->pid.store(targetPid);

    QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});

    target.waitForFinished();

    QVERIFY(session->result.has_value());
    SKIP_IF_PERF_CANNOT_SAMPLE(*session->result);
    QVERIFY_RESULT(*session->result);
    const Result<SampleTraceData> data = readSampleTrace(session->result->value());
    QVERIFY_RESULT(data);

    // The renamed process is single-threaded, so its tid equals its pid.
    const QString name = data->threadNames.value(quint64(targetPid));
    QVERIFY2(!name.isEmpty(), "expected the target's comm name to be captured");
    QCOMPARE(name, u"qtcSamplerTest"_s);
}

// Builds the launch-mode recording settings for tests/manual/sampler-testapp,
// or QSKIPs if it hasn't been built. Its worker threads ("fibonacci",
// "hasher", "bursty", "sleeper") are named via QThread::setObjectName().
static std::shared_ptr<RecordingSession> makeLaunchedTestAppSession(PerfSampler &sampler)
{
    const FilePath buildDir = FilePath::fromString(QString::fromLatin1(QT_TESTCASE_BUILDDIR));
    const FilePath testAppPath = buildDir.parentDir().parentDir().parentDir().pathAppended(
        "tests/manual/sampler-testapp/sampler-testapp");
    if (!testAppPath.exists())
        return {};

    auto *settings = qobject_cast<PerfSamplerSettings *>(sampler.settings());
    if (!settings)
        return {};
    settings->perfSettings.callgraphMode.setValue(1); // "fp"
    settings->attach.setValue(false);
    settings->executable.setValue(testAppPath);

    const Result<std::shared_ptr<RecordingSession>> created = settings->createSession();
    return created ? *created : std::shared_ptr<RecordingSession>{};
}

// User-reported: launching (not attaching to) tests/manual/sampler-testapp
// showed none of its named worker threads with their real names. Root cause,
// confirmed directly against raw "perf record" output (independent of
// anything this reader does): "perf record --pid PID" starts as soon as the
// just-launched target exists (see launchThenCapture()) and races the
// target's own early thread creation -- a thread renamed
// (QThread::setObjectName() -> pthread_setname_np/prctl(PR_SET_NAME)) very
// shortly after being created can have that specific PERF_RECORD_COMM dropped
// by perf/the kernel's own per-thread event setup. Every *other* thread-name
// test here attaches to an already-running, already-settled target, where
// this race never has a chance to occur -- this is the one that launches,
// exercising it for real. CapturedThreadNames (perfsampler.cpp) works around
// the race with an independent source of truth: /proc/<pid>/task/<tid>/comm,
// snapshotted while the target runs.
void PerfSamplerTest::testCapturesLaunchedThreadNames()
{
    PerfSampler sampler;
    const std::shared_ptr<RecordingSession> session = makeLaunchedTestAppSession(sampler);
    if (!session)
        QSKIP("sampler-testapp not found");

    // Stopped once the target has named its workers, which is what there is to
    // capture. Naming a thread reports nothing, so /proc is polled for it the
    // way QTRY_VERIFY polls, with a deadline past which the recording is
    // stopped anyway and the check below fails.
    const QStringList workers{u"fibonacci"_s, u"hasher"_s, u"bursty"_s};
    const QDeadlineTimer deadline(10s);
    QTimer namesCheck;
    namesCheck.setInterval(50);
    QObject::connect(&namesCheck, &QTimer::timeout, [session, workers, deadline] {
        const qint64 pid = session->pid.load();
        if (pid <= 0)
            return;
        const QStringList names = threadNamesOf(pid);
        const bool named = std::all_of(workers.cbegin(), workers.cend(),
                                       [&names](const QString &w) { return names.contains(w); });
        if (named || deadline.hasExpired())
            session->requestStop();
    });
    namesCheck.start();
    QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});
    namesCheck.stop();

    QVERIFY(session->result.has_value());
    SKIP_IF_PERF_CANNOT_SAMPLE(*session->result);
    QVERIFY_RESULT(*session->result);
    const Result<SampleTraceData> data = readSampleTrace(session->result->value());
    QVERIFY_RESULT(data);

    // "sleeper" is deliberately excluded: it is designed (see main.cpp's own
    // comment) to be almost always blocked, so within this short recording
    // window it may get zero "cycles"-based samples at all -- but its name is
    // still captured regardless (see the target-exits test below, which does
    // assert on it).
    const QStringList names = data->threadNames.values();
    for (const QString &expected : workers) {
        QVERIFY2(names.contains(expected),
                 qPrintable(u"missing thread name %1; got: %2"_s.arg(
                     expected, names.join(u", "_s))));
    }
}

// The scenario that actually bit the user: the target is gone by the time
// post-processing runs, so nothing can read its /proc entries then. Here the
// target exits on its own mid-recording (SIGTERM, which sampler-testapp
// catches and quits cleanly on -- standing in for the user closing its
// window) instead of being asked to stop cleanly via a stop request, so its
// /proc/<pid>/task tree has vanished before the parse worker resolves
// samples. Names can therefore only come from the periodic snapshots
// CapturedThreadNames took *while the target was alive* -- exactly what an
// approach that read /proc at post-processing time (or relied on perf's own
// COMM events) would lose. "sleeper" is asserted on precisely because it is
// nearly always blocked: /proc lists every thread whether or not it was ever
// sampled, so its name survives even with no samples of its own.
void PerfSamplerTest::testCapturesThreadNamesWhenTargetExitsFirst()
{
    PerfSampler sampler;
    const std::shared_ptr<RecordingSession> session = makeLaunchedTestAppSession(sampler);
    if (!session)
        QSKIP("sampler-testapp not found");

    // Give it time to create and name its worker threads (and to be snapshotted
    // at least once), then make it exit on its own -- no stop request, so the
    // target is gone before the parse worker runs.
    QTimer::singleShot(1500, [session] {
        const qint64 pid = session->pid.load();
        if (pid > 0)
            ::kill(pid_t(pid), SIGTERM);
    });
    QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});

    QVERIFY(session->result.has_value());
    SKIP_IF_PERF_CANNOT_SAMPLE(*session->result);
    QVERIFY_RESULT(*session->result);
    const Result<SampleTraceData> data = readSampleTrace(session->result->value());
    QVERIFY_RESULT(data);

    const QStringList names = data->threadNames.values();
    for (const QString &expected : {u"fibonacci"_s, u"hasher"_s, u"bursty"_s, u"sleeper"_s}) {
        QVERIFY2(names.contains(expected),
                 qPrintable(u"missing thread name %1; got: %2"_s.arg(
                     expected, names.join(u", "_s))));
    }
}

// A call chain of known depth for testRecordsCallGraph() to find in the
// recorded stacks. Not a tail call, so every level keeps a frame of its own.
static volatile quint64 g_callGraphSink = 0;

Q_NEVER_INLINE static quint64 callGraphTestRecurse(int depth, quint64 acc)
{
    if (depth == 0) {
        for (int i = 0; i < 2000; ++i)
            acc = acc * 6364136223846793005ull + 1442695040888963407ull;
        g_callGraphSink = acc;
        return acc;
    }
    return callGraphTestRecurse(depth - 1, acc) + quint64(depth);
}

void PerfSamplerTest::testRecordsCallGraph_data()
{
    QTest::addColumn<int>("callgraphMode"); // index into PerfSettings::callgraphMode
    QTest::newRow("dwarf") << 0;
    QTest::newRow("fp") << 1;
    QTest::newRow("lbr") << 2;
}

// Records this very process in each call-graph mode while a worker thread
// recurses through callGraphTestRecurse(), and checks the recursion comes
// out of the recording at depth -- "perf record", the reader and, for
// dwarf, the unwinder, all for real. The worker starts once capture is live
// and does a fixed amount of work; finishing it is what ends the recording.
void PerfSamplerTest::testRecordsCallGraph()
{
    QFETCH(int, callgraphMode);
    constexpr int depth = 16;

    PerfSampler sampler;
    auto *settings = qobject_cast<PerfSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->perfSettings.callgraphMode.setValue(callgraphMode);
    const QString mode = settings->perfSettings.callgraphMode.itemValue().toString();
    // Only for its verdict on the call-graph mode, which rejects dwarf in a
    // build without libdw; this test records its own process instead.
    settings->attach.setValue(false);
    settings->executable.setValue(FilePath::fromString("/bin/true"));
    const Result<std::shared_ptr<RecordingSession>> created = settings->createSession();
    if (!created && created.error().contains("libdw"_L1))
        QSKIP(qPrintable(created.error()));
    QVERIFY_RESULT(created);

    auto session = std::make_shared<RecordingSession>();
    session->pid.store(QCoreApplication::applicationPid());

    std::thread worker;
    QObject::connect(session->reporter(), &RecordingReporter::changed, [session, &worker] {
        if (worker.joinable() || !session->isStarted())
            return;
        worker = std::thread([session] {
            for (int i = 0; i < 100000; ++i)
                callGraphTestRecurse(depth, quint64(i));
            QMetaObject::invokeMethod(session->reporter(), [session] { session->requestStop(); });
        });
    });
    QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});
    if (worker.joinable())
        worker.join();

    QVERIFY(session->result.has_value());
    // lbr needs a CPU that records branches, which virtual machines rarely
    // pass through.
    if (!*session->result && mode == u"lbr"_s
        && session->result->error().contains("branch stack"_L1)) {
        QSKIP(qPrintable(session->result->error()));
    }
    SKIP_IF_PERF_CANNOT_SAMPLE(*session->result);
    QVERIFY_RESULT(*session->result);
    const Result<SampleTraceData> data = readSampleTrace(session->result->value());
    QVERIFY_RESULT(data);

    int deepest = 0;
    for (const SampleTraceData::ThreadSample &sample : data->samples) {
        const int levels = int(std::count_if(sample.frames.cbegin(), sample.frames.cend(),
                                             [&data](int labelId) {
            return data->labels.at(labelId).name.contains("callGraphTestRecurse"_L1);
        }));
        deepest = qMax(deepest, levels);
    }
    // lbr's hardware buffer holds as few as 16 branches, which cuts its
    // stacks short of the recursion's full depth.
    const int expected = mode == u"lbr"_s ? depth / 2 : depth;
    QVERIFY2(deepest >= expected,
             qPrintable(u"deepest stack held %1 of %2 levels"_s.arg(deepest).arg(depth + 1)));
}

// A recording the reader cannot decode -- an event with a sample format it
// does not know, as "--weight" in the additional arguments gives -- ends as
// soon as that is clear, rather than when the user stops it. The stand-in
// "perf" writes such a stream and then keeps running, as perf would.
void PerfSamplerTest::testEndsRecordingOnDecodeError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray stream("PERFILE2", 8);
    const auto appendLe = [&stream](quint64 value, int bytes) {
        for (int i = 0; i < bytes; ++i)
            stream.append(char((value >> (8 * i)) & 0xff));
    };
    appendLe(16, 8);                      // pipe header size
    constexpr int attrSize = 64;
    appendLe(64, 4);                      // PERF_RECORD_HEADER_ATTR
    appendLe(0, 2);                       // misc
    appendLe(8 + attrSize + 8, 2);        // size: header, attr, one id
    appendLe(0, 4);                       // attr.type
    appendLe(attrSize, 4);                // attr.size
    appendLe(0, 8);                       // config
    appendLe(4000, 8);                    // sample_freq
    appendLe((1u << 0) | (1u << 14), 8);  // sample_type: IP | WEIGHT
    stream.append(QByteArray(attrSize - 32, '\0'));
    appendLe(1, 8);                       // id
    QFile data(dir.filePath("stream"));
    QVERIFY(data.open(QIODevice::WriteOnly));
    data.write(stream);
    data.close();

    QFile wrapper(dir.filePath("perf"));
    QVERIFY(wrapper.open(QIODevice::WriteOnly));
    wrapper.write("#!/bin/sh\ncat '" + data.fileName().toLocal8Bit() + "'\nexec sleep 30\n");
    wrapper.close();
    QVERIFY(wrapper.setPermissions(wrapper.permissions() | QFileDevice::ExeOwner));

    PerfSampler sampler;
    QElapsedTimer elapsed;
    elapsed.start();
    const QString error = failedRecordingError(sampler, dir.path());
    QVERIFY2(error.contains("sample format"_L1), qPrintable(error));
    // The stand-in ends by itself after 30 s; ending well before is the stop.
    QVERIFY2(elapsed.elapsed() < 20000, qPrintable(u"took %1 ms"_s.arg(elapsed.elapsed())));
}

void PerfSamplerTest::testRecordsDifferentlyLinkedBinaries_data()
{
    QTest::addColumn<QStringList>("linkArgs");
    QTest::addColumn<int>("callgraphMode"); // index into PerfSettings::callgraphMode
    QTest::newRow("non-PIE, fp") << QStringList{"-no-pie"} << 1;
    QTest::newRow("non-PIE, dwarf") << QStringList{"-no-pie"} << 0;
    QTest::newRow("lld, fp") << QStringList{"-fuse-ld=lld"} << 1;
    QTest::newRow("lld, dwarf") << QStringList{"-fuse-ld=lld"} << 0;
}

// A non-PIE executable, or one lld links, has its code at link addresses
// other than its file offsets. Records such a program, compiled here, and
// checks its recursion comes out of the recording named and at depth: symbols
// and lines are looked up by link address, and the unwinder is told where the
// module is by the same.
void PerfSamplerTest::testRecordsDifferentlyLinkedBinaries()
{
    QFETCH(QStringList, linkArgs);
    QFETCH(int, callgraphMode);
    constexpr int depth = 12;

    const FilePath cc = Environment::systemEnvironment().searchInPath("cc");
    if (cc.isEmpty())
        QSKIP("no \"cc\" in PATH");
    if (linkArgs.contains("-fuse-ld=lld")
        && Environment::systemEnvironment().searchInPath("ld.lld").isEmpty()) {
        QSKIP("no \"ld.lld\" in PATH");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile source(dir.filePath("recurse.c"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("volatile unsigned long sink;\n"
                 "__attribute__((noinline)) unsigned long linkedRecurse(int depth, unsigned long acc)\n"
                 "{\n"
                 "    if (depth == 0) {\n"
                 "        for (int i = 0; i < 2000; ++i)\n"
                 "            acc = acc * 6364136223846793005ul + 1442695040888963407ul;\n"
                 "        sink = acc;\n"
                 "        return acc;\n"
                 "    }\n"
                 "    return linkedRecurse(depth - 1, acc) + (unsigned long)depth;\n"
                 "}\n"
                 "int main(void)\n"
                 "{\n"
                 "    for (long i = 0; i < 400000; ++i)\n"
                 "        linkedRecurse(" + QByteArray::number(depth) + ", (unsigned long)i);\n"
                 "    return 0;\n"
                 "}\n");
    source.close();
    const QString exe = dir.filePath("recurse");
    Process compile;
    compile.setCommand({cc, QStringList{"-O1", "-g", "-fno-omit-frame-pointer"} + linkArgs
                                + QStringList{"-o", exe, source.fileName()}});
    compile.runBlocking();
    QVERIFY2(compile.result() == ProcessResult::FinishedWithSuccess,
             qPrintable(compile.allOutput()));

    Process target;
    target.setCommand({FilePath::fromString(exe), {}});
    target.start();
    QVERIFY(target.waitForStarted());

    PerfSampler sampler;
    auto *settings = qobject_cast<PerfSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->perfSettings.callgraphMode.setValue(callgraphMode);
    const QString mode = settings->perfSettings.callgraphMode.itemValue().toString();
    auto session = std::make_shared<RecordingSession>();
    session->pid.store(target.processId());
    QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});
    target.waitForFinished();

    QVERIFY(session->result.has_value());
    SKIP_IF_PERF_CANNOT_SAMPLE(*session->result);
    if (!*session->result && mode == u"dwarf"_s && session->result->error().contains("libdw"_L1))
        QSKIP(qPrintable(session->result->error()));
    QVERIFY_RESULT(*session->result);
    const Result<SampleTraceData> data = readSampleTrace(session->result->value());
    QVERIFY_RESULT(data);

    int deepest = 0;
    bool hasLine = false;
    for (const SampleTraceData::ThreadSample &sample : data->samples) {
        int levels = 0;
        for (int labelId : sample.frames) {
            const SampleTraceData::Label &label = data->labels.at(labelId);
            if (label.name.contains("linkedRecurse"_L1)) {
                ++levels;
                hasLine = hasLine || (label.file.endsWith("recurse.c"_L1) && label.line > 0);
            }
        }
        deepest = qMax(deepest, levels);
    }
    QVERIFY2(deepest >= depth,
             qPrintable(u"deepest stack held %1 of %2 levels"_s.arg(deepest).arg(depth + 1)));
    QVERIFY(hasLine);
}

// The functions inlined at an address are frames of their own, as they were
// when the sampler resolved its stacks through an external parser: root first,
// the function the code is in, and then what was inlined into it, level by
// level.
void PerfSamplerTest::testRecordsInlinedFunctions()
{
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
                 "    for (long i = 0; i < 400000; ++i)\n"
                 "        outerFunction((unsigned long)i);\n"
                 "    return 0;\n"
                 "}\n");
    source.close();
    const QString exe = dir.filePath("inlined");
    Process compile;
    compile.setCommand({cc, {"-O2", "-g", "-fno-omit-frame-pointer", "-o", exe,
                             source.fileName()}});
    compile.runBlocking();
    QVERIFY2(compile.result() == ProcessResult::FinishedWithSuccess,
             qPrintable(compile.allOutput()));

    Process target;
    target.setCommand({FilePath::fromString(exe), {}});
    target.start();
    QVERIFY(target.waitForStarted());

    PerfSampler sampler;
    auto *settings = qobject_cast<PerfSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->perfSettings.callgraphMode.setValue(1); // "fp"
    auto session = std::make_shared<RecordingSession>();
    session->pid.store(target.processId());
    QTaskTree::runBlocking(Group{sampler.recordRecipe(session)});
    target.waitForFinished();

    QVERIFY(session->result.has_value());
    SKIP_IF_PERF_CANNOT_SAMPLE(*session->result);
    QVERIFY_RESULT(*session->result);
    const Result<SampleTraceData> data = readSampleTrace(session->result->value());
    QVERIFY_RESULT(data);

    const QStringList expected{u"outerFunction"_s, u"inlinedMiddle"_s, u"inlinedLeaf"_s};
    bool found = false;
    for (const SampleTraceData::ThreadSample &sample : data->samples) {
        QStringList names;
        for (int labelId : sample.frames)
            names.append(data->labels.at(labelId).name);
        for (qsizetype i = 0; i + 3 <= names.size(); ++i)
            found = found || names.mid(i, 3) == expected;
    }
    QVERIFY(found);
}

QObject *createPerfSamplerTest()
{
    return new PerfSamplerTest;
}

} // namespace Profiler::Internal

#include "perfsampler_test.moc"
