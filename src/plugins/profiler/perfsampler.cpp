// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfsampler.h"

#include "perfdatareader.h"
#include "perfevent.h"
#include "perfeventtype.h"
#include "perfprofilerconstants.h"
#include "perfprofilertracemanager.h"
#include "processpickerdialog.h"
#include "profilertr.h"
#include "sampletrace.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/processinfo.h>
#include <utils/qtcprocess.h>
#include <utils/qtdesignwidgets.h>

#include <QDataStream>
#include <QPointer>
#include <QTimer>
#include <QtEndian>

#include <optional>

using namespace Profiler;
using namespace Profiler::Internal;
using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

namespace {

// How long elfutils may wait for a debuginfod server to start answering, and how
// often it may try again, when downloading debug information is enabled. Both are
// per build id; see onParserSetup().
constexpr auto debugInfoUrlsVariable = "DEBUGINFOD_URLS"_L1;
constexpr auto debugInfoTimeoutVariable = "DEBUGINFOD_TIMEOUT"_L1;
constexpr auto debugInfoRetryVariable = "DEBUGINFOD_RETRY_LIMIT"_L1;
constexpr int debugInfoTimeoutSeconds = 10;
constexpr int debugInfoRetryLimit = 0;

// Decodes perfparser's live wire protocol (the same one PerfProfilerTraceFile
// reads, see perfprofilertracefile.cpp) directly into a SampleTraceData,
// bypassing PerfProfilerTraceManager: that class exists to back a completely
// different, disk-stash-backed timeline UI this tool does not use. The framing
// (magic + version header, then length-prefixed messages) and the PerfEvent /
// PerfEventType::Location / PerfProfilerTraceManager::Symbol / ::Thread wire
// structures are reused as-is; only the message dispatch is new.
class PerfMessageDecoder
{
public:
    PerfMessageDecoder(SampleTraceData &data, const std::shared_ptr<RecordingSession> &session)
        : m_data(data), m_session(session)
    {}

    void addData(const QByteArray &chunk)
    {
        m_buffer.append(chunk);
        while (parseNextMessage()) { }
    }

    // Samples perfparser delivered, including those dropped for having no
    // resolvable call stack. A recording that captured plenty but resolved none
    // is a symbolication problem, not an empty one, and only this count tells
    // the two apart.
    int receivedSamples() const { return m_receivedSamples; }

private:
    bool parseNextMessage()
    {
        if (m_dataStreamVersion < 0) {
            const int magicSize = int(sizeof(Constants::PerfStreamMagic));
            if (m_buffer.size() < magicSize + int(sizeof(qint32)))
                return false;
            if (strncmp(m_buffer.constData(), Constants::PerfStreamMagic, magicSize) != 0) {
                qWarning("PerfSampler: unrecognized perfparser stream header");
                m_buffer.clear();
                return false;
            }
            m_dataStreamVersion = qFromLittleEndian<qint32>(
                reinterpret_cast<const uchar *>(m_buffer.constData() + magicSize));
            m_buffer.remove(0, magicSize + int(sizeof(qint32)));
        }

        if (m_buffer.size() < int(sizeof(quint32)))
            return false;
        const quint32 messageSize = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(m_buffer.constData()));
        if (m_buffer.size() < int(sizeof(quint32)) + int(messageSize))
            return false;

        const QByteArray message = m_buffer.mid(sizeof(quint32), messageSize);
        m_buffer.remove(0, int(sizeof(quint32)) + int(messageSize));
        handleMessage(message);
        return true;
    }

    void handleMessage(const QByteArray &message)
    {
        QDataStream stream(message);
        stream.setVersion(m_dataStreamVersion);

        PerfEvent event;
        stream >> event;

        // perfparser is single-threaded, so any other message proves it got past
        // the download it last reported. Clear the download state on it instead
        // of trying to recognize the final progress report, which never arrives
        // when a lookup gives up or fails.
        if (event.feature() != PerfEventType::DebugInfoDownloadProgress)
            m_session->clearDebugInfoDownload();

        switch (event.feature()) {
        case PerfEventType::StringDefinition: {
            qint32 id;
            QByteArray value;
            stream >> id >> value;
            m_strings.insert(id, value);
            break;
        }
        case PerfEventType::LocationDefinition: {
            qint32 id;
            PerfEventType::Location location;
            stream >> id >> location;
            m_locations.insert(id, location);
            break;
        }
        case PerfEventType::SymbolDefinition: {
            qint32 id;
            PerfProfilerTraceManager::Symbol symbol;
            stream >> id >> symbol;
            m_symbols.insert(id, symbol);
            break;
        }
        case PerfEventType::Command: {
            PerfProfilerTraceManager::Thread thread;
            stream >> thread;
            if (thread.name >= 0)
                m_data.threadNames.insert(thread.tid, string(thread.name));
            break;
        }
        case PerfEventType::Sample:
        case PerfEventType::TracePointSample:
            appendSample(event);
            break;
        case PerfEventType::Progress: {
            float percent;
            stream >> percent;
            m_session->progress.store(int(percent * 100), std::memory_order_relaxed);
            break;
        }
        case PerfEventType::DebugInfoDownloadProgress: {
            qint32 url;
            qint64 numerator;
            qint64 denominator;
            stream >> url >> numerator >> denominator;
            // A server that has not answered yet announces no size, so there is
            // no percentage to report; 0 stands for that, and the UI then only
            // says a download is running. The URL is the request being made once
            // one is in flight, and the configured server list before that.
            const int percent = denominator > 0
                ? int(qBound(qint64(0), numerator * 100 / denominator, qint64(100)))
                : 0;
            m_session->setDebugInfoDownload(percent, string(url));
            break;
        }
        case PerfEventType::Error: {
            qint32 errorCode;
            QString message2;
            stream >> errorCode >> message2;
            qWarning().noquote() << "perfparser:" << message2;
            break;
        }
        default:
            break; // AttributesDefinition/FeaturesDefinition/TracePointFormat/thread
                   // lifecycle events are not needed for the flat sample list.
        }
    }

    QString string(qint32 id) const { return QString::fromUtf8(m_strings.value(id)); }

    // Resolves one raw frame id to a SampleTraceData label id, memoized. Walks
    // one hop up the inline-parent chain exactly like
    // PerfProfilerTraceManager::symbolLocation() does, then repeats from the
    // caller in appendSample() -- so an address that resolved to several
    // inlined functions still produces one label per inline level, the same
    // way the aggregated view of the IDE's own CPU Usage analyzer does.
    int labelIdFor(qint32 locationId)
    {
        if (auto it = m_labelIds.constFind(locationId); it != m_labelIds.constEnd())
            return it.value();

        const PerfProfilerTraceManager::Symbol &symbol = m_symbols.value(locationId);
        const PerfEventType::Location &location = m_locations.value(locationId);

        SampleTraceData::Label label;
        if (symbol.name != -1) {
            label.name = string(symbol.name);
            label.module = string(symbol.binary);
            label.offset = symbol.relAddr;
        } else {
            label.offset = location.relAddr ? location.relAddr : location.address;
            label.name = u"0x%1"_s.arg(label.offset, 0, 16);
        }
        if (location.file != -1) {
            label.file = string(location.file);
            label.line = location.line;
        }

        const int id = int(m_data.labels.size());
        m_data.labels.append(label);
        m_labelIds.insert(locationId, id);
        return id;
    }

    void appendSample(const PerfEvent &event)
    {
        m_data.pid = event.pid();
        ++m_receivedSamples;

        QList<int> reversedFrames; // built innermost-first, like origFrames()
        for (qint32 frame : event.origFrames()) {
            while (frame >= 0) {
                const qint32 symbolLocationId = m_symbols.contains(frame)
                                                    ? frame
                                                    : m_locations.value(frame).parentLocationId;
                const qint32 resolvedId = symbolLocationId >= 0 ? symbolLocationId : frame;
                reversedFrames.append(labelIdFor(resolvedId));
                frame = symbolLocationId >= 0
                            ? m_locations.value(symbolLocationId).parentLocationId
                            : -1;
            }
        }
        if (reversedFrames.isEmpty())
            return; // matches macsampler.cpp: samples with no resolved stack are dropped

        SampleTraceData::ThreadSample sample;
        sample.tid = event.tid();
        sample.running = true; // a perf sample always fires while its thread is on-CPU
        const qint64 timestampNs = event.timestamp();
        if (m_firstTimestampNs < 0)
            m_firstTimestampNs = timestampNs;
        sample.tsUs = quint64(qMax<qint64>(0, timestampNs - m_firstTimestampNs) / 1000);
        sample.frames.reserve(reversedFrames.size());
        for (auto it = reversedFrames.crbegin(); it != reversedFrames.crend(); ++it)
            sample.frames.append(*it);
        m_data.samples.append(std::move(sample));
    }

    SampleTraceData &m_data;
    std::shared_ptr<RecordingSession> m_session;
    QByteArray m_buffer;
    qint32 m_dataStreamVersion = -1;
    int m_receivedSamples = 0;
    qint64 m_firstTimestampNs = -1;
    QHash<qint32, QByteArray> m_strings;
    QHash<qint32, PerfEventType::Location> m_locations;
    QHash<qint32, PerfProfilerTraceManager::Symbol> m_symbols;
    QHash<qint32, int> m_labelIds;
};

// perf_event_paranoid up to 2 still permits sampling a process the user owns;
// 3 -- a Debian/Ubuntu addition -- and above deny unprivileged sampling outright.
constexpr int lowestParanoidBlockingUserSampling = 3;
constexpr int paranoidAllowingUserSampling = 2;
constexpr auto paranoidSettingName = "perf_event_paranoid"_L1;
constexpr auto paranoidSysctlKey = "kernel.perf_event_paranoid"_L1;
constexpr auto sysctlConfigFile = "/etc/sysctl.conf"_L1;

// The programs driven here, named in messages but never translated.
constexpr auto perfRecordName = "perf record"_L1;
constexpr auto perfParserName = "perfparser"_L1;

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

// What "perf record" left behind, shared between its own done handler and
// perfparser's, which is where a sample-less recording is diagnosed.
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

QString noSamplesError(const QString &recordStdErr, bool recordFailed, int receivedSamples)
{
    const std::optional<int> paranoid = perfEventParanoid();
    if (paranoid && *paranoid >= lowestParanoidBlockingUserSampling) {
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
    if (recordFailed) {
        const QString reported = reportedFailure(recordStdErr);
        if (!reported.isEmpty())
            return Tr::tr("No samples were captured. \"%1\" reported:\n%2")
                .arg(perfRecordName).arg(reported);
    }

    // perf sampled the target fine; every sample was dropped for want of a call
    // stack, which is about debug information, not about the recording.
    if (receivedSamples > 0) {
        return Tr::tr("\"%1\" captured %n sample(s), but none of them could be resolved to a "
                      "call stack. Install debug information for the profiled binary and its "
                      "libraries, or build it with frame pointers, and record again.",
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

} // namespace

PerfSamplerSettings::PerfSamplerSettings()
{
    setSettingsGroup("PerfSampler");

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
    // The launch settings are irrelevant while attaching.
    const auto updateLaunchEnabled = [this] {
        const bool launching = !attach();
        executable.setEnabled(launching);
        arguments.setEnabled(launching);
        workingDirectory.setEnabled(launching);
    };
    updateLaunchEnabled();
    connect(&attach, &BoolAspect::changed, this, updateLaunchEnabled);

    setLayouter([this] {
        using namespace Layouting;
        auto pick = new QtcButton(Tr::tr("Select Process…"), QtcButton::SmallSecondary);
        auto picked = new QtcLabel(m_pickedName.isEmpty() ? Tr::tr("No process selected")
                                                          : m_pickedName,
                                   QtcLabel::Secondary);
        pick->setEnabled(attach());
        connect(&attach, &BoolAspect::changed, pick, [this, pick] { pick->setEnabled(attach()); });
        connect(pick, &QAbstractButton::clicked, this, [this, picked] {
            const std::optional<ProcessInfo> info = ProcessPickerDialog::pickProcess();
            if (!info)
                return;
            m_pickedPid = info->processId;
            m_pickedName = FilePath::fromUserInput(info->executable).fileName();
            picked->setText(m_pickedName);
        });
        return Column {
            executable,
            arguments,
            workingDirectory,
            Row { attach, pick, picked, st },
            downloadDebugInfo,
            perfSettings.createPerfConfigWidget(nullptr),
        };
    });
}

Result<std::shared_ptr<RecordingSession>> PerfSamplerSettings::createSession() const
{
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
        if (error)
            *error = Tr::tr("The \"perf\" command was not found in PATH.");
        return false;
    }
    const FilePath parser = findPerfParser();
    if (!parser.isExecutableFile()) {
        if (error) {
            *error = Tr::tr("The %1 helper tool was not found at \"%2\".")
                         .arg(perfParserName).arg(parser.toUserOutput());
        }
        return false;
    }
    return true;
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
    const FilePath perfExe = Environment::systemEnvironment().searchInPath("perf");
    const FilePath parserExe = findPerfParser();
    const QString recordArgs = m_settings->perfSettings.perfRecordArguments();
    const bool downloadDebugInfo = m_settings->downloadDebugInfo();

    auto sampleData = std::make_shared<SampleTraceData>();
    auto decoder = std::make_shared<PerfMessageDecoder>(*sampleData, session);
    auto parserProcessPtr = std::make_shared<QPointer<Process>>();
    auto recordOutcome = std::make_shared<RecordOutcome>();
    auto parserStdErr = std::make_shared<QString>();

    const auto onRecordSetup = [session, perfExe, recordArgs, parserProcessPtr,
                                recordOutcome](Process &process) {
        CommandLine cmd(perfExe,
                        {"record", "--pid", QString::number(session->pid.load()), "-o", "-"});
        cmd.addArgs(recordArgs, CommandLine::Raw);
        process.setCommand(cmd);

        // "perf record" states the actual reason for a permission failure (e.g.
        // the current perf_event_paranoid restriction) on stderr; kept around so
        // onParserDone can quote it instead of guessing why no samples arrived.
        QObject::connect(&process, &Process::readyReadStandardError, &process,
                         [p = &process, recordOutcome] {
            recordOutcome->stdErr.append(QString::fromLocal8Bit(p->readAllRawStandardError()));
        });

        // Polls session->stop (set by the "Stop Recording" button) and asks
        // perf record to finish; there is no signal to hook for this since the
        // whole recipe is event-driven on the GUI thread (unlike
        // CallStackSampler's worker-thread loop, which polls the same flag).
        auto *stopPoll = new QTimer(&process);
        stopPoll->setInterval(50);
        QObject::connect(stopPoll, &QTimer::timeout, &process, [session, p = &process] {
            if (session->stop.load(std::memory_order_relaxed))
                p->stop();
        });
        stopPoll->start();

        QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                         [p = &process, parserProcessPtr] {
            if (Process *parser = parserProcessPtr->data())
                parser->writeRaw(p->readAllRawStandardOutput());
        });

        session->markStarted(); // perf is attached; the duration clock can start
    };
    const auto onRecordDone = [session, parserProcessPtr, recordOutcome](const Process &process,
                                                                        DoneWith result) {
        // Stopping recording calls process.stop(), which itself is reported as
        // DoneWith::Error (see Process::stop()'s ProcessResult::Canceled) -- so
        // only a genuine failure to launch perf is treated as an error here;
        // otherwise, whether perf exited by request or the target died on its
        // own, let perfparser finish and let onParserDone's sample count decide.
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
        if (Process *parser = parserProcessPtr->data())
            parser->closeWriteChannel();
    };

    const auto onParserSetup = [parserExe, parserProcessPtr, decoder, parserStdErr,
                                downloadDebugInfo](Process &process) {
        *parserProcessPtr = &process;
        process.setCommand(CommandLine(parserExe));

        // Symbol lookups that come up empty locally are answered from a debuginfod
        // server, which perfparser does synchronously while it processes the
        // samples. elfutils reads the server list from DEBUGINFOD_URLS, so the
        // environment we hand the process is what decides, and setting it here
        // also covers a value inherited from somewhere other than this one.
        Environment environment = Environment::systemEnvironment();
        if (downloadDebugInfo) {
            // A server that accepts the connection and then stays silent costs
            // elfutils DEBUGINFOD_TIMEOUT seconds per attempt, and it makes
            // DEBUGINFOD_RETRY_LIMIT further attempts -- 90 seconds and two
            // retries by default, so 4.5 minutes for one build id, and it pays
            // that for every build id it cannot resolve locally. That is the wait
            // that makes post-processing look hung. Allow a fraction of it and no
            // retry: a healthy server answers in well under a second, and one
            // that ignored the first request will ignore the second too. Values
            // already in the environment are a deliberate choice, so leave those.
            if (!environment.hasKey(debugInfoTimeoutVariable)) {
                environment.set(debugInfoTimeoutVariable,
                                QString::number(debugInfoTimeoutSeconds));
            }
            if (!environment.hasKey(debugInfoRetryVariable))
                environment.set(debugInfoRetryVariable, QString::number(debugInfoRetryLimit));
        } else {
            // An empty list is how elfutils is told not to ask anyone.
            environment.set(debugInfoUrlsVariable, {});
        }
        process.setEnvironment(environment);
        process.setProcessMode(ProcessMode::Writer); // perf record's output is written to its stdin
        QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                         [p = &process, decoder] { decoder->addData(p->readAllRawStandardOutput()); });
        // perfparser rejecting an argument or giving up mid-stream leaves the
        // recording empty; without its stderr that looks like a target that never
        // ran, so keep it for onParserDone to quote.
        QObject::connect(&process, &Process::readyReadStandardError, &process,
                         [p = &process, parserStdErr] {
            parserStdErr->append(QString::fromLocal8Bit(p->readAllRawStandardError()));
        });
    };
    const auto onParserDone = [session, sampleData, decoder, parserProcessPtr, recordOutcome,
                               parserStdErr](const Process &process, DoneWith result) {
        // perfparser may still have unread, already-flushed output pending.
        if (Process *parser = parserProcessPtr->data())
            decoder->addData(parser->readAllRawStandardOutput());

        // Nothing can be downloading any more, whether or not a final progress
        // report made it out before perfparser exited.
        session->clearDebugInfoDownload();

        if (session->result)
            return;
        if (result == DoneWith::Error && process.error() == ProcessError::FailedToStart) {
            session->result = ResultError(
                Tr::tr("Failed to start %1: %2")
                    .arg(perfParserName).arg(process.errorString()));
            return;
        }
        // perfparser started but gave up, so it -- not the recording -- is what
        // went wrong, and it has already said why.
        if (result == DoneWith::Error) {
            const QString reported = parserStdErr->trimmed();
            session->result = ResultError(
                reported.isEmpty()
                    ? Tr::tr("%1 failed with exit code %2.")
                          .arg(perfParserName).arg(process.exitCode())
                    : Tr::tr("%1 failed: %2").arg(perfParserName).arg(reported));
            return;
        }
        if (sampleData->samples.isEmpty()) {
            session->result = ResultError(
                noSamplesError(recordOutcome->stdErr, recordOutcome->failed,
                               decoder->receivedSamples()));
            return;
        }

        const FilePath dir = uniqueTracePath("qtprofiler-sample"_L1);
        if (!dir.createDir()) {
            session->result = ResultError(
                Tr::tr("Cannot create temporary trace directory %1.").arg(dir.toUserOutput()));
            return;
        }

        const auto writeProgress = [session](int percent) {
            session->progress.store(percent, std::memory_order_relaxed);
        };
        if (Result<> r = writeSampleTrace(*sampleData, dir, writeProgress); !r) {
            session->result = ResultError(r.error());
            return;
        }
        session->result = dir;
    };

    return Group {
        parallel,
        ProcessTask(onRecordSetup, onRecordDone),
        ProcessTask(onParserSetup, onParserDone),
    };
}

} // namespace QmlProfiler::Internal
