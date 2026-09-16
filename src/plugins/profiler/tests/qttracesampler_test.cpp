// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttracesampler_test.h"

#include <profiler/profilerrecorder.h>
#include <profiler/qttracesampler.h>
#include <profiler/traceformat.h>

#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/id.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

namespace Profiler::Internal {

// Names the executable of a Qt application built against a Qt configured with
// "-trace ctf", for the one test that records a real trace. Nothing in the
// build tree qualifies -- the Qt that Qt Creator itself is built against is a
// stock one -- so that test is opt-in.
static const char tracedApplicationVariable[] = "QTC_TEST_QTTRACE_APPLICATION";

static QJsonObject sessionFile(const FilePath &dir)
{
    const Result<QByteArray> contents = (dir / "session.json").fileContents();
    if (!contents)
        return {};
    return QJsonDocument::fromJson(*contents).object();
}

void QtTraceSamplerTest::testSessionFileNamesTheSession()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path()) / "trace";

    QVERIFY(writeTraceSession(location, "myapp", {}));

    // The session's name reaches the trace as its own, and the timeline shows it
    // for the recorded process, so it is what the target is called.
    const QJsonObject session = sessionFile(location);
    QCOMPARE(session.keys(), QStringList{"myapp"});
    QCOMPARE(session.value("myapp").toVariant().toStringList(), QStringList{"all"});
}

void QtTraceSamplerTest::testSessionFileSelectsProviders()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path());

    QVERIFY(writeTraceSession(location, "myapp", {"qtcore", "myapp"}));

    QCOMPARE(sessionFile(location).value("myapp").toVariant().toStringList(),
             (QStringList{"qtcore", "myapp"}));
}

// Makes `recorder` record with this backend, and hands it over. Selected by id
// rather than by the display name selectBackend() matches, which is translated:
// these run inside a Creator instance, whose language is not this test's to
// choose.
static Sampler *selectQtTraceBackend(ProfilerRecorder &recorder)
{
    const int index = recorder.backendIds().indexOf(Id(SamplerIds::QtTrace));
    if (index < 0)
        return nullptr;
    recorder.setCurrentBackend(index);
    return recorder.backendById(SamplerIds::QtTrace);
}

// The directory the backend pointed the target at, as the target itself reads
// it: from the environment the launch is to be made with.
static FilePath preparedLocation(const std::shared_ptr<RecordingSession> &session)
{
    for (const EnvironmentItem &item : session->launchEnvironmentChanges) {
        if (item.name == QLatin1StringView("QTRACE_LOCATION"))
            return FilePath::fromUserInput(item.value);
    }
    return {};
}

void QtTraceSamplerTest::testEarlierTraceIsLeftAlone()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path());

    // Whatever the user's chosen directory already holds is none of the
    // recording's business -- a trace of an earlier one, or anything else that
    // happens to be called "metadata".
    const FilePath earlier = location / "ust";
    QVERIFY(earlier.ensureWritableDir());
    QVERIFY((earlier / "metadata").writeFileContents("/* CTF 1.8 */"));
    QVERIFY((earlier / "channel_0").writeFileContents("\xc1\xfc\x1f\xc1"));

    QtTraceSampler sampler;
    auto *settings = static_cast<QtTraceSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->traceDirectory.setValue(location);

    const auto session = std::make_shared<RecordingSession>();
    session->launchExecutable = FilePath::fromUserInput("myapp");
    sampler.prepareLaunch(session);

    // The target writes into a directory of this start's own, below the one it
    // was given, so the earlier trace is neither deleted nor collected.
    const FilePath prepared = preparedLocation(session);
    QVERIFY2(prepared.isChildOf(location), qPrintable(prepared.toUserOutput()));
    QVERIFY((earlier / "metadata").exists());
    QVERIFY((earlier / "channel_0").exists());

    sampler.completeRecording(session);
    QVERIFY(session->result);
    QVERIFY(!*session->result);
    QVERIFY2(session->result->error().contains("-trace ctf"),
             qPrintable(session->result->error()));
}

void QtTraceSamplerTest::testUnusableTraceDirectoryFailsBeforeTheTargetRuns()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path());

    // A directory that cannot be created -- here because a file of that name is
    // in the way.
    const FilePath blocked = location / "occupied";
    QVERIFY(blocked.writeFileContents("not a directory"));

    const FilePath application = location / "myapp";
    QVERIFY(application.writeFileContents(QByteArray()));

    QtTraceSampler sampler;
    auto *settings = static_cast<QtTraceSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->executable.setValue(application);
    settings->traceDirectory.setValue(blocked);

    // Creating the session is the last point at which a recording can be failed
    // with a reason, and the target has not been started then. Left to
    // prepareLaunch(), this would be reported only after the application had
    // run to completion.
    const Result<std::shared_ptr<RecordingSession>> session = settings->createSession();
    QVERIFY(!session);
    QVERIFY2(session.error().contains(blocked.toUserOutput()), qPrintable(session.error()));
}

void QtTraceSamplerTest::testTraceIsCollectedWhenTheTargetIsGone()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QtTraceSampler sampler;
    auto *settings = static_cast<QtTraceSamplerSettings *>(sampler.settings());
    QVERIFY(settings);
    settings->traceDirectory.setValue(FilePath::fromString(dir.path()));

    const auto session = std::make_shared<RecordingSession>();
    session->launchExecutable = FilePath::fromUserInput("myapp");
    sampler.prepareLaunch(session);
    const FilePath prepared = preparedLocation(session);
    QVERIFY(!prepared.isEmpty());

    // What the target writes as it exits -- Qt writes a thread's events out
    // when a packet fills, or when the application exits -- still belongs to
    // the recording, so the trace is collected after the target is gone rather
    // than when the capture ends.
    QVERIFY((prepared / "ust").ensureWritableDir());
    QVERIFY((prepared / "ust" / "metadata").writeFileContents("/* CTF 1.8 */"));
    QVERIFY((prepared / "ust" / "channel_0").writeFileContents("\xc1\xfc\x1f\xc1"));

    sampler.completeRecording(session);
    QVERIFY(session->result);
    if (!*session->result)
        QFAIL(qPrintable(session->result->error()));
    QCOMPARE(**session->result, prepared / "ust");
}

void QtTraceSamplerTest::testCollectTraceWithoutTrace()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // A target whose Qt has no tracing backend writes nothing at all, which is
    // the mistake most worth explaining.
    const Result<FilePath> trace = collectTrace(FilePath::fromString(dir.path()));
    QVERIFY(!trace);
    QVERIFY2(trace.error().contains("-trace ctf"), qPrintable(trace.error()));
}

void QtTraceSamplerTest::testCollectTraceWithoutEvents()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path());
    QVERIFY((location / "metadata").writeFileContents("/* CTF 1.8 */"));

    // Tracing was there, but no tracepoint was ever reached: no thread wrote a
    // channel. That is a different mistake, and gets a different answer.
    const Result<FilePath> trace = collectTrace(location);
    QVERIFY(!trace);
    QVERIFY2(!trace.error().contains("-trace ctf"), qPrintable(trace.error()));
}

void QtTraceSamplerTest::testCollectTraceFromSessionSubdirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path());

    // With a session file, Qt writes the trace one directory further down.
    const FilePath written = location / "ust";
    QVERIFY(written.ensureWritableDir());
    QVERIFY((written / "metadata").writeFileContents("/* CTF 1.8 */"));
    QVERIFY((written / "channel_0").writeFileContents("\xc1\xfc\x1f\xc1"));

    const Result<FilePath> trace = collectTrace(location);
    if (!trace)
        QFAIL(qPrintable(trace.error()));
    QCOMPARE(*trace, written);
}

void QtTraceSamplerTest::testTraceWrittenOnStopIsCollected()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The stand-in target below is a shell script.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path());

    // Stands in for an application whose Qt writes a thread's events out only
    // as it exits, which is what Qt's CTF backend does for a thread whose
    // packet never filled. It waits to be stopped and writes the channel then,
    // so a recording that collected the trace before stopping the target would
    // find nothing but the metadata.
    //
    // The file it is told to create says it is ready to be stopped: a target
    // stopped before it has installed its handler dies of the signal instead.
    const FilePath ready = location / "ready";
    const FilePath application = location / "traced-app.sh";
    QVERIFY(application.writeFileContents(
        "#!/bin/sh\n"
        "mkdir -p \"$QTRACE_LOCATION/ust\"\n"
        "printf '/* CTF 1.8 */' > \"$QTRACE_LOCATION/ust/metadata\"\n"
        "sleep 30 &\n"
        "pid=$!\n"
        "trap 'printf x > \"$QTRACE_LOCATION/ust/channel_0\"; kill $pid; exit 0' TERM\n"
        ": > \"$1\"\n"
        "wait $pid\n"));
    QVERIFY(application.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

    ProfilerRecorder recorder;
    Sampler *backend = selectQtTraceBackend(recorder);
    QVERIFY(backend);
    auto *settings = static_cast<QtTraceSamplerSettings *>(backend->settings());
    QVERIFY(settings);
    settings->traceDirectory.setValue(location);
    settings->executable.setValue(application);
    settings->arguments.setValue(ProcessArgs::quoteArg(ready.toUserOutput()));

    QSignalSpy finished(&recorder, &ProfilerRecorder::finished);
    QSignalSpy failed(&recorder, &ProfilerRecorder::error);
    recorder.start();

    // Stopping is what makes the target write, so wait until it says it is
    // ready for that.
    QTRY_VERIFY_WITH_TIMEOUT(ready.exists() || !failed.isEmpty(), 30000);
    QVERIFY(failed.isEmpty());
    recorder.stop();

    QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty() || !failed.isEmpty(), 30000);
    if (!failed.isEmpty())
        QFAIL(qPrintable(failed.first().first().toString()));

    // What the target wrote on its way out is the recording's trace.
    const FilePath trace = finished.first().first().value<FilePath>();
    QCOMPARE(trace.fileName(), QString("ust"));
    QVERIFY((trace / "channel_0").exists());
}

void QtTraceSamplerTest::testFailingTargetIsReportedRatherThanTheEmptyTrace()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The stand-in target below is a shell script.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath location = FilePath::fromString(dir.path());

    // Stands in for an application that comes up and then fails: a library it
    // cannot load, an argument it will not take. It writes no trace, and what
    // explains that is its own failure -- not the absence of tracing from the Qt
    // it was built against, which is what an empty location otherwise means.
    const FilePath application = location / "failing-app.sh";
    QVERIFY(application.writeFileContents("#!/bin/sh\nexit 3\n"));
    QVERIFY(application.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

    ProfilerRecorder recorder;
    Sampler *backend = selectQtTraceBackend(recorder);
    QVERIFY(backend);
    auto *settings = static_cast<QtTraceSamplerSettings *>(backend->settings());
    QVERIFY(settings);
    settings->traceDirectory.setValue(location);
    settings->executable.setValue(application);
    settings->arguments.setValue(QString());

    QSignalSpy finished(&recorder, &ProfilerRecorder::finished);
    QSignalSpy failed(&recorder, &ProfilerRecorder::error);
    recorder.start();

    QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty() || !failed.isEmpty(), 30000);
    QVERIFY(finished.isEmpty());

    // The recording says which target failed, rather than advising the user
    // about a Qt configuration that is not the problem.
    const QString error = failed.first().first().toString();
    QVERIFY2(error.contains(application.fileName()), qPrintable(error));
    QVERIFY2(!error.contains("-trace ctf"), qPrintable(error));
}

// Records a real application, which needs one built against a Qt that has the
// tracepoints compiled in; QTC_TEST_QTTRACE_APPLICATION names it.
void QtTraceSamplerTest::testRecordsATracedApplication()
{
    const QString application = qEnvironmentVariable(tracedApplicationVariable);
    if (application.isEmpty()) {
        QSKIP("Set QTC_TEST_QTTRACE_APPLICATION to an application built against a Qt "
              "configured with \"-trace ctf\" to run this test.");
    }

    ProfilerRecorder recorder;
    Sampler *backend = selectQtTraceBackend(recorder);
    QVERIFY(backend);
    auto *settings = static_cast<QtTraceSamplerSettings *>(backend->settings());
    QVERIFY(settings);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    settings->traceDirectory.setValue(FilePath::fromString(dir.path()));
    settings->executable.setValue(FilePath::fromUserInput(application));
    settings->arguments.setValue(QString());

    QSignalSpy finished(&recorder, &ProfilerRecorder::finished);
    QSignalSpy failed(&recorder, &ProfilerRecorder::error);
    recorder.start();

    // The application records until it exits on its own, which ends the
    // recording and hands over the trace.
    QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty() || !failed.isEmpty(), 60000);
    if (!failed.isEmpty())
        QFAIL(qPrintable(failed.first().first().toString()));

    // What the recording produced is the directory the target wrote into, and
    // it holds a trace: the metadata, and a channel per traced thread.
    const FilePath trace = finished.first().first().value<FilePath>();
    QVERIFY((trace / "metadata").exists());
    QVERIFY(!trace.dirEntries(FileFilter({"channel_*"}, DirFilterFlag::Files)).isEmpty());

    // The recording used the directory it was told to, whether directly or
    // through the "ust" subdirectory a session file moves it into.
    const FilePath location = FilePath::fromString(dir.path());
    QVERIFY2(trace == location || trace.isChildOf(location), qPrintable(trace.toUserOutput()));

    // And what it produced is a trace the viewer opens: this is the seam
    // between recording it and showing it.
    QCOMPARE(identifyTrace(trace).format, TraceFormat::Ctf);
}

} // namespace Profiler::Internal
