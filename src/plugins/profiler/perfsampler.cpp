// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfsampler.h"

#include "perfprofilerconstants.h"
#include "perfrecordreader.h"
#include "processpickerdialog.h"
#include "profilertr.h"
#include "sampletrace.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/processinfo.h>
#include <utils/qtcprocess.h>
#include <utils/qtdesignwidgets.h>

#include <QtTaskTree/QThreadFunction>

#include <QDir>
#include <QFile>
#include <QHash>
#include <QPromise>
#include <QTimer>

#include <limits>
#include <mutex>
#include <optional>

using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

namespace {

// perf_event_paranoid up to 2 still permits sampling a process the user owns;
// 3 -- a Debian/Ubuntu addition -- and above deny unprivileged sampling outright.
constexpr int lowestParanoidBlockingUserSampling = 3;
constexpr int paranoidAllowingUserSampling = 2;
constexpr auto paranoidSettingName = "perf_event_paranoid"_L1;
constexpr auto paranoidSysctlKey = "kernel.perf_event_paranoid"_L1;
constexpr auto sysctlConfigFile = "/etc/sysctl.conf"_L1;

// The program driven here, named in messages but never translated.
constexpr auto perfRecordName = "perf record"_L1;

// "perf record" follows a failure with a screenful of advice; keep the message
// box to the part that names the failure.
constexpr int maxReportedErrorLines = 6;

std::optional<int> perfEventParanoid()
{
    const Result<QByteArray> contents =
        FilePath::fromString("/proc/sys/kernel/perf_event_paranoid"_L1).fileContents();
    if (!contents)
        return std::nullopt;
    bool ok = false;
    const int value = QString::fromLatin1(*contents).trimmed().toInt(&ok);
    return ok ? std::optional(value) : std::nullopt;
}

// sysctl lives in /usr/sbin, which a desktop session's PATH need not contain.
FilePath sysctlExecutable()
{
    const FilePath inPath = Environment::systemEnvironment().searchInPath("sysctl");
    if (!inPath.isEmpty())
        return inPath;
    for (const auto &candidate : {"/usr/sbin/sysctl"_L1, "/sbin/sysctl"_L1}) {
        const FilePath path = FilePath::fromString(candidate);
        if (path.isExecutableFile())
            return path;
    }
    return {};
}

// What "perf record" left behind, shared between its own done handler and the
// parsing worker, which is where a sample-less recording is diagnosed.
struct RecordOutcome
{
    QString stdErr;
    bool failed = false; // perf exited non-zero other than by our own stop()
};

// The diagnosis out of "perf record"'s stderr. A successful run still writes
// there ("[ perf record: Captured and wrote ... ]", build-id warnings), so this
// starts at the first "Error:" line, and the caller only asks once perf has
// actually failed.
QString reportedFailure(const QString &recordStdErr)
{
    const QStringList lines = recordStdErr.trimmed().split(u'\n', Qt::SkipEmptyParts);
    qsizetype start = 0;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        if (lines.at(i).trimmed().startsWith("Error:"_L1)) {
            start = i;
            break;
        }
    }
    return lines.mid(start, maxReportedErrorLines).join(u'\n');
}

QString noSamplesError(const QString &recordStdErr, bool recordFailed, int receivedSamples,
                       quint64 lostSamples)
{
    const QString reported = recordFailed ? reportedFailure(recordStdErr) : QString();

    // perf names perf_event_paranoid when that is what refused it. Anything else
    // it failed on -- no perf for the running kernel, an unknown event -- would
    // still be in the way with the setting changed, so that is what to report.
    const std::optional<int> paranoid = perfEventParanoid();
    const bool paranoidNamed = reported.isEmpty() || recordStdErr.contains(paranoidSettingName);
    const bool paranoidBlocks = paranoid && *paranoid >= lowestParanoidBlockingUserSampling
                                && paranoidNamed;
    if (paranoidBlocks) {
        QString message = Tr::tr("No samples were captured: \"%1\" is %2, which denies "
                                 "performance monitoring to unprivileged processes. Sampling "
                                 "processes you own needs it set to %3 or less.")
                              .arg(paranoidSettingName)
                              .arg(*paranoid)
                              .arg(paranoidAllowingUserSampling);
        // Where availableFix() has something to offer, the UI puts a button on
        // this message that makes the change; naming the command here as well
        // would only ask the reader which of the two they are meant to use.
        if (sysctlExecutable().isEmpty()) {
            const QString command = QString("sudo sysctl -w %1=%2"_L1)
                                        .arg(paranoidSysctlKey).arg(paranoidAllowingUserSampling);
            message += u'\n' + Tr::tr("Set it with:") + u"\n    "_s + command;
        }
        return message;
    }

    // Any other failure to open events -- an unsupported event, a target already
    // gone -- is named by "perf record" itself, so quote it rather than guess.
    if (!reported.isEmpty()) {
        return Tr::tr("No samples were captured. \"%1\" reported:\n%2")
            .arg(perfRecordName).arg(reported);
    }

    // perf sampled the target, but could not write out a single sample: it
    // took more, or larger ones, than it had time or buffer for.
    if (receivedSamples == 0 && lostSamples > 0) {
        return Tr::tr("\"%1\" lost all %n sample(s) it took, because it could not write them "
                      "out fast enough. Record with a lower sampling frequency, or with a smaller "
                      "stack snapshot size for dwarf call graphs.",
                      nullptr, int(qMin<quint64>(lostSamples, std::numeric_limits<int>::max())))
            .arg(perfRecordName);
    }

    // perf sampled the target fine; every sample was dropped for want of a call
    // stack, which is about how the target was built, not about the recording.
    if (receivedSamples > 0) {
        return Tr::tr("\"%1\" captured %n sample(s), but none of them could be resolved to a "
                      "call stack. Build the profiled binary with frame pointers, and record "
                      "again.",
                      nullptr, receivedSamples)
            .arg(perfRecordName);
    }

    if (paranoid) {
        return Tr::tr("No samples were captured, although \"%1\" is %2, which permits sampling "
                      "your own processes. The target may have exited before \"%3\" attached, or "
                      "never run on the CPU while it was recorded.")
            .arg(paranoidSettingName).arg(*paranoid).arg(perfRecordName);
    }
    return Tr::tr("No samples were captured. The target may have exited immediately, or never "
                  "run on the CPU while it was recorded.");
}

// Snapshots the target's thread names from /proc/<pid>/task/<tid>/comm while
// it is still running, so they can be merged into the trace once decoding
// finishes. This -- not the decoder -- is where thread-name capture belongs.
// "perf record"'s own PERF_RECORD_COMM stream routinely misses a thread
// renamed (QThread::setObjectName() -> pthread_setname_np/prctl(PR_SET_NAME))
// shortly after it is created: a real, reproducible perf/kernel race, not a
// decode bug, so a name that never reached the stream cannot be recovered
// from it. Reading /proc has to happen here, against the known-live target on
// the GUI thread where its Process lives, rather than in the decoder: by
// post-processing time the target may already have exited (it stopped
// recording by quitting, or crashed), and for a *replayed* saved recording
// the sampled pid could belong to an entirely unrelated process, which would
// mislabel every thread. Snapshotting repeatedly during recording (see
// captureRecipe()) captures names as threads appear and guarantees a last-known
// value even when the target dies before it can be stopped cleanly. Mirrors
// PerfByteQueue's mutex pattern: written on the GUI thread, read once on the
// parsing worker.
class CapturedThreadNames
{
public:
    void snapshot(qint64 pid)
    {
        if (pid <= 0)
            return;
        const QDir taskDir(u"/proc/%1/task"_s.arg(pid));
        const QStringList tids = taskDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        std::lock_guard lock(m_mutex);
        for (const QString &tid : tids) {
            bool ok = false;
            const quint64 tidNum = tid.toULongLong(&ok);
            if (!ok)
                continue;
            QFile comm(taskDir.filePath(tid + u"/comm"_s));
            if (!comm.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            const QString name = QString::fromUtf8(comm.readLine()).trimmed();
            if (!name.isEmpty())
                m_names.insert(tidNum, name);
        }
    }

    QHash<quint64, QString> names() const
    {
        std::lock_guard lock(m_mutex);
        return m_names;
    }

private:
    mutable std::mutex m_mutex;
    QHash<quint64, QString> m_names;
};

// Whether this backend can decode a recording in the settings' call-graph
// mode. Checked when a session is created, for early feedback, and again when
// capture starts, which is the only check a recording of a run control -- or
// of a composite backend -- goes through.
Result<> checkCallgraphMode(const Profiler::PerfSettings &settings)
{
    // This backend decodes "perf record"'s output itself (see
    // perfrecordreader.cpp). "fp" and
    // "lbr" both need no unwinding on the consumer side -- the kernel hands
    // back an already-unwound frame chain (fp) or a hardware branch-history
    // buffer usable as one directly (lbr). "dwarf" needs a full DWARF CFI
    // unwinder, which is libdw's (elfutils, see perfdwarfunwinder.h), linked
    // only into Linux x86-64 builds that found its development headers (see
    // CMakeLists.txt/profiler.qbs) -- WITH_LIBDW mirrors that.
    const QString callgraphMode = settings.callgraphMode.itemValue().toString();
#ifdef WITH_LIBDW
    if (callgraphMode != u"fp"_s && callgraphMode != u"lbr"_s
        && callgraphMode != Constants::PerfCallgraphDwarf) {
        return ResultError(Tr::tr(
            "This backend only supports frame-pointer, last-branch-record, or dwarf call "
            "graphs; set \"Call graph mode\" accordingly."));
    }
#else
    if (callgraphMode != u"fp"_s && callgraphMode != u"lbr"_s) {
        return ResultError(Tr::tr(
            "This backend only supports frame-pointer or last-branch-record call graphs; "
            "set \"Call graph mode\" to \"frame pointer\" or \"last branch record\". Dwarf "
            "call graphs need this build to have been compiled with libdw (elfutils) "
            "available."));
    }
#endif
    return ResultOk;
}

} // namespace

PerfSamplerSettings::PerfSamplerSettings()
{
    setSettingsGroup("PerfSampler");

    // PerfSettings defaults to auto-apply off: its other use, the IDE's CPU
    // Usage analyzer, edits it through a modal options dialog with an
    // explicit OK/Apply step. Embedded live in this tool's sidebar (see
    // createPerfConfigWidget() below) there is no such step, so without this
    // every control in it -- call graph mode included -- would only ever
    // update its own volatileValue(), never the value() that createSession()
    // and perfRecordArguments() actually read; the widget would look
    // responsive while silently recording with stale settings.
    perfSettings.setAutoApply(true);

    attach.setSettingsKey("Attach");
    attach.setLabel(Tr::tr("Attach to a running process"),
                    BoolAspect::LabelPlacement::AtCheckBox);

    downloadDebugInfo.setSettingsKey("DownloadDebugInfo");
    downloadDebugInfo.setDefaultValue(false);
    downloadDebugInfo.setLabel(Tr::tr("Download missing debug information"),
                               BoolAspect::LabelPlacement::AtCheckBox);
    downloadDebugInfo.setToolTip(
        Tr::tr("Let the profiler fetch debug information it does not find locally from the "
               "debuginfod servers listed in the DEBUGINFOD_URLS environment variable. This "
               "resolves symbols in system libraries that have no debug package installed, but "
               "it happens while the captured samples are processed, so a slow or unreachable "
               "server delays the trace considerably."));
    updateTargetEnabled();
    connect(&attach, &BoolAspect::changed, this, [this] { updateTargetEnabled(); });

    setLayouter([this] {
        using namespace Layouting;
        auto pick = new QtcButton(Tr::tr("Select Process…"), QtcButton::SmallSecondary);
        auto picked = new QtcLabel(m_pickedName.isEmpty() ? Tr::tr("No process selected")
                                                          : m_pickedName,
                                   QtcLabel::Secondary);
        const auto updatePick = [this, pick, picked] {
            const bool enabled = !targetChosenElsewhere() && attach();
            pick->setEnabled(enabled);
            picked->setEnabled(enabled);
        };
        updatePick();
        connect(&attach, &BoolAspect::changed, pick, updatePick);
        connect(this, &SamplerSettings::targetSelectionChanged, pick, updatePick);
        connect(pick, &QAbstractButton::clicked, this, [this, picked] {
            const std::optional<ProcessInfo> info = ProcessPickerDialog::pickProcess();
            if (!info)
                return;
            m_pickedPid = info->processId;
            m_pickedName = FilePath::fromUserInput(info->executable).fileName();
            picked->setText(m_pickedName);
        });
        QWidget *perfConfig = perfSettings.createPerfConfigWidget(nullptr);
        const auto updatePerfConfig = [this, perfConfig] {
            perfConfig->setEnabled(!optionsChosenElsewhere());
        };
        updatePerfConfig();
        connect(this, &SamplerSettings::optionsSelectionChanged, perfConfig, updatePerfConfig);
        return Column {
            executable,
            arguments,
            workingDirectory,
            Row { attach, pick, picked, st },
            downloadDebugInfo,
            perfConfig,
            noMargin,
        };
    });
}

// The launch settings are irrelevant while attaching, and both are while the
// target comes from a run configuration.
void PerfSamplerSettings::updateTargetEnabled()
{
    const bool own = !targetChosenElsewhere();
    attach.setEnabled(own);
    const bool launching = own && !attach();
    executable.setEnabled(launching);
    arguments.setEnabled(launching);
    workingDirectory.setEnabled(launching);
}

void PerfSamplerSettings::updateOptionsEnabled()
{
    downloadDebugInfo.setEnabled(!optionsChosenElsewhere());
}

Result<std::shared_ptr<RecordingSession>> PerfSamplerSettings::createSession() const
{
    if (Result<> mode = checkCallgraphMode(perfSettings); !mode)
        return ResultError(mode.error());

    auto session = std::make_shared<RecordingSession>();
    if (attach()) {
        if (m_pickedPid == 0)
            return ResultError(Tr::tr("Select a process to attach to."));
        session->pid = m_pickedPid;
        session->processName = m_pickedName;
        return session;
    }
    if (Result<> launch = fillLaunch(*session); !launch)
        return ResultError(launch.error());
    return session;
}

void PerfSamplerSettings::readSettings()
{
    SamplerSettings::readSettings();
    perfSettings.readSettings();
}

void PerfSamplerSettings::writeSettings() const
{
    SamplerSettings::writeSettings();
    perfSettings.writeSettings();
}

PerfSampler::PerfSampler()
    : m_settings(std::make_unique<PerfSamplerSettings>())
{}

PerfSampler::~PerfSampler() = default;

QString PerfSampler::displayName() const
{
    return Tr::tr("Perf Sampler");
}

bool PerfSampler::isAvailable(QString *error) const
{
    if (!HostOsInfo::isLinuxHost()) {
        if (error)
            *error = Tr::tr("The Perf sampler is only implemented on Linux.");
        return false;
    }
    if (Environment::systemEnvironment().searchInPath("perf").isEmpty()) {
        if (error) {
            *error = Tr::tr("The \"perf\" command was not found in PATH. Install the perf "
                            "package of your distribution, which is named \"perf\" or "
                            "\"linux-tools\" on most of them, and record again.");
        }
        return false;
    }
    return true;
}

// Offered wherever it can work at all, so that a missing "perf" is reported
// when recording starts rather than hiding the backend.
bool PerfSampler::isOffered() const
{
    return HostOsInfo::isLinuxHost();
}

SamplerSettings *PerfSampler::settings() const
{
    return m_settings.get();
}

std::optional<SamplerFix> PerfSampler::availableFix() const
{
    const std::optional<int> paranoid = perfEventParanoid();
    if (!paranoid || *paranoid < lowestParanoidBlockingUserSampling)
        return std::nullopt;

    const FilePath sysctl = sysctlExecutable();
    if (sysctl.isEmpty())
        return std::nullopt;

    const QString assignment = QString("%1=%2"_L1)
                                   .arg(paranoidSysctlKey).arg(paranoidAllowingUserSampling);
    const QString persistentSetting = QString("%1 = %2"_L1)
                                          .arg(paranoidSysctlKey).arg(paranoidAllowingUserSampling);
    const QString buttonText = Tr::tr("Allow Sampling");
    return SamplerFix{
        buttonText,
        Tr::tr("\"%1\" sets \"%2\" to %3 for you, asking for your password, and then records "
               "again. The setting reverts on reboot; to keep it, add \"%4\" to %5.")
            .arg(buttonText)
            .arg(paranoidSysctlKey)
            .arg(paranoidAllowingUserSampling)
            .arg(persistentSetting)
            .arg(sysctlConfigFile),
        CommandLine{sysctl, {"-w", assignment}},
    };
}

ExecutableItem PerfSampler::captureRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    if (Result<> mode = checkCallgraphMode(m_settings->perfSettings); !mode) {
        return Group {
            onGroupSetup([session, error = mode.error()] {
                session->result = ResultError(error);
                return SetupResult::StopWithError;
            }),
        };
    }

    const FilePath perfExe = Environment::systemEnvironment().searchInPath("perf");
    const QString recordArgs = m_settings->perfSettings.perfRecordArguments();
    const bool downloadDebugInfo = m_settings->downloadDebugInfo();

    // The only thing that crosses from "perf record"'s Process (GUI thread)
    // to the parsing worker (below) is raw bytes, via this queue -- plus what
    // "perf record" left behind (see RecordOutcome) and the target's thread
    // names snapshotted while it runs (see CapturedThreadNames).
    auto queue = std::make_shared<PerfByteQueue>();
    auto recordOutcome = std::make_shared<RecordOutcome>();
    auto threadNames = std::make_shared<CapturedThreadNames>();

    const auto onRecordSetup = [session, perfExe, recordArgs, queue, recordOutcome,
                                threadNames](Process &process) {
        CommandLine cmd(perfExe,
                        {"record", "--pid", QString::number(session->pid.load()),
                         "-k", "CLOCK_MONOTONIC", "-o", "-"});
        cmd.addArgs(recordArgs, CommandLine::Raw);
        process.setCommand(cmd);

        // "perf record" states the actual reason for a permission failure (e.g.
        // the current perf_event_paranoid restriction) on stderr; kept around so
        // the parsing worker can quote it instead of guessing why no samples
        // arrived.
        QObject::connect(&process, &Process::readyReadStandardError, &process,
                         [p = &process, recordOutcome] {
            recordOutcome->stdErr.append(QString::fromLocal8Bit(p->readAllRawStandardError()));
        });

        // Asks "perf record" to finish when the recording is stopped. This
        // recipe is event-driven on the GUI thread and has no loop of its own
        // to notice the flag in (unlike CallStackSampler's worker-thread loop),
        // so it hooks the request instead. The process is the context, so the
        // handler goes away with the task. The snapshot here is the last one
        // taken while the target is certainly alive, right before perf stops.
        session->onStopRequested(&process, [session, threadNames, p = &process] {
            threadNames->snapshot(session->pid.load());
            p->stop();
        });

        // Periodically snapshot thread names while the target runs: names
        // appear as its threads are created and renamed, and a target that
        // exits on its own (rather than via a stop request) would
        // otherwise be gone by post-processing time -- see CapturedThreadNames.
        threadNames->snapshot(session->pid.load());
        auto *namePoll = new QTimer(&process);
        namePoll->setInterval(500);
        QObject::connect(namePoll, &QTimer::timeout, &process, [session, threadNames] {
            threadNames->snapshot(session->pid.load());
        });
        namePoll->start();

        QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                         [p = &process, queue] { queue->push(p->readAllRawStandardOutput()); });
        // A task tree destroyed while running calls no done handler.
        QObject::connect(&process, &QObject::destroyed, [queue] { queue->close(); });

        // perf is attached, so the duration clock can start -- unless the
        // recording begins paused, whose trace starts at the first resume.
        if (!session->isPaused()) {
            session->markStarted();
        } else {
            // Weak: the session keeps its handlers.
            session->onPausedChanged(&process, [weak = std::weak_ptr(session)](bool paused) {
                if (const std::shared_ptr<RecordingSession> session = weak.lock(); session && !paused)
                    session->markStarted();
            });
        }
    };
    const auto onRecordDone = [session, queue, recordOutcome](const Process &process,
                                                             DoneWith result) {
        // Stopping recording calls process.stop(), which itself is reported as
        // DoneWith::Error (see Process::stop()'s ProcessResult::Canceled) -- so
        // only a genuine failure to launch perf is treated as an error here;
        // otherwise, whether perf exited by request or the target died on its
        // own, let the parsing worker finish and let its sample count decide.
        if (result == DoneWith::Error && process.error() == ProcessError::FailedToStart
            && !session->result) {
            session->result = ResultError(
                Tr::tr("Failed to start \"%1\": %2")
                    .arg(perfRecordName).arg(process.errorString()));
        }
        // Stopping recording cancels perf; anything else non-zero is perf giving
        // up on its own, which is what makes its stderr worth quoting.
        recordOutcome->failed = result == DoneWith::Error
                                && process.result() != ProcessResult::Canceled;
        // Last, because the parsing worker reads recordOutcome once the queue
        // has run dry, and the queue's lock orders that after this.
        queue->close();
    };

    const auto onParseSetup = [session, queue, recordOutcome, threadNames,
                               downloadDebugInfo](QThreadFunction<Result<FilePath>> &parsing) {
        parsing.setThreadFunctionData([session, queue, recordOutcome, threadNames,
                                       downloadDebugInfo](QPromise<Result<FilePath>> &promise) {
            const auto parse = [&]() -> Result<FilePath> {
                PerfRecordReader reader;
                reader.setDownloadDebugInfo(downloadDebugInfo);
                reader.setCancelCheck([&promise] { return promise.isCanceled(); });
                reader.setSampleFilter([session](qint64 timestampNs) {
                    return !session->wasPausedAt(timestampNs);
                });
                reader.setDebugInfoDownloadHandler([session](int percent, const QString &urls) {
                    if (percent < 0)
                        session->clearDebugInfoDownload();
                    else
                        session->setDebugInfoDownload(percent, urls);
                });
                // Symbolication (debuginfod fetches, then per-sample resolution)
                // only starts once recording stops and can take a while for a
                // large trace; without this it reports nothing and the progress
                // bar sits at 0% for that whole stretch. Given half the budget,
                // the rest going to writeSampleTrace() below.
                const auto readProgress = [session](int percent) {
                    session->setProgress(percent / 2);
                };
                Result<SampleTraceData> data = reader.read(*queue, readProgress);
                if (!data) {
                    // Nothing reads what "perf record" still writes, so end it --
                    // on the GUI thread, which stop requests belong to -- and let
                    // its output run dry rather than pile up in the queue.
                    QMetaObject::invokeMethod(session->reporter(), [session] {
                        session->requestStop();
                    }, Qt::QueuedConnection);
                    while (!queue->atEnd())
                        queue->pop();
                    return ResultError(data.error());
                }
                if (data->samples.isEmpty()) {
                    if (!session->isStarted() && !recordOutcome->failed) {
                        return ResultError(session->isStopRequested()
                                               ? stoppedBeforeResumeMessage()
                                               : exitedBeforeResumeMessage());
                    }
                    quint64 lost = 0;
                    for (const SampleTraceData::LostSamples &gap : data->lostSamples)
                        lost += gap.count;
                    return ResultError(noSamplesError(recordOutcome->stdErr, recordOutcome->failed,
                                                      reader.receivedSamples(), lost));
                }
                data->pausedRangesUs = pausedRangesUs(session->pausedIntervals(),
                                                      reader.firstTimestampNs(),
                                                      data->samples.last().tsUs);

                // Fill in any thread names PERF_RECORD_COMM missed from the live
                // /proc snapshots taken during recording (see CapturedThreadNames);
                // perf's own events win when both have a name for a tid.
                const QHash<quint64, QString> liveNames = threadNames->names();
                for (auto it = liveNames.cbegin(); it != liveNames.cend(); ++it) {
                    if (!data->threadNames.contains(it.key()))
                        data->threadNames.insert(it.key(), it.value());
                }

                if (promise.isCanceled())
                    return ResultError(Tr::tr("The recording was canceled."));
                const FilePath dir = uniqueTracePath("qtprofiler-sample"_L1);
                if (!dir.createDir()) {
                    return ResultError(Tr::tr("Cannot create temporary trace directory %1.")
                                           .arg(dir.toUserOutput()));
                }

                const auto writeProgress = [session](int percent) {
                    session->setProgress(50 + percent / 2);
                };
                if (Result<> r = writeSampleTrace(*data, dir, writeProgress); !r)
                    return ResultError(r.error());
                return dir;
            };
            promise.addResult(parse());
        });
    };
    const auto onParseDone = [session](const QThreadFunction<Result<FilePath>> &parsing,
                                       DoneWith result) {
        // A "perf record" that failed to start has already said so, which is
        // more to the point than the empty recording it left behind.
        if (result != DoneWith::Cancel && !session->result)
            session->result = parsing.result();
    };

    // continueOnError: stopping "perf record" (a stop request, or the target
    // exiting) is reported as an error (Process::stop() -> ProcessResult::
    // Canceled), which is the *expected* end of a normal recording, not a
    // failure. Without this, the default StopOnError policy would cancel the
    // still-running parse worker right as it's about to finish, discarding a
    // perfectly good result -- the parse worker's own onParseDone is what
    // actually decides success/failure via session->result.
    return Group {
        parallel,
        continueOnError,
        ProcessTask(onRecordSetup, onRecordDone),
        QThreadFunctionTask<Result<FilePath>>(onParseSetup, onParseDone),
    };
}

} // namespace Profiler::Internal
