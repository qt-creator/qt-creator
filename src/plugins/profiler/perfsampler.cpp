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
#include <QDateTime>
#include <QDir>
#include <QPointer>
#include <QStandardPaths>
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
    PerfMessageDecoder(SampleTraceData &data, std::atomic<int> *progress)
        : m_data(data), m_progress(progress)
    {}

    void addData(const QByteArray &chunk)
    {
        m_buffer.append(chunk);
        while (parseNextMessage()) { }
    }

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
            if (m_progress)
                m_progress->store(int(percent * 100), std::memory_order_relaxed);
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
    std::atomic<int> *m_progress = nullptr;
    QByteArray m_buffer;
    qint32 m_dataStreamVersion = -1;
    qint64 m_firstTimestampNs = -1;
    QHash<qint32, QByteArray> m_strings;
    QHash<qint32, PerfEventType::Location> m_locations;
    QHash<qint32, PerfProfilerTraceManager::Symbol> m_symbols;
    QHash<qint32, int> m_labelIds;
};

} // namespace

PerfSamplerSettings::PerfSamplerSettings()
{
    setSettingsGroup("PerfSampler");

    attach.setSettingsKey("Attach");
    attach.setLabel(Tr::tr("Attach to a running process"),
                    BoolAspect::LabelPlacement::AtCheckBox);
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
            *error = Tr::tr("The perfparser helper tool was not found at \"%1\".")
                         .arg(parser.toUserOutput());
        }
        return false;
    }
    return true;
}

SamplerSettings *PerfSampler::settings() const
{
    return m_settings.get();
}

ExecutableItem PerfSampler::captureRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    const FilePath perfExe = Environment::systemEnvironment().searchInPath("perf");
    const FilePath parserExe = findPerfParser();
    const QString recordArgs = m_settings->perfSettings.perfRecordArguments();

    auto sampleData = std::make_shared<SampleTraceData>();
    auto decoder = std::make_shared<PerfMessageDecoder>(*sampleData, &session->progress);
    auto parserProcessPtr = std::make_shared<QPointer<Process>>();

    const auto onRecordSetup = [session, perfExe, recordArgs, parserProcessPtr](Process &process) {
        CommandLine cmd(perfExe,
                        {"record", "--pid", QString::number(session->pid.load()), "-o", "-"});
        cmd.addArgs(recordArgs, CommandLine::Raw);
        process.setCommand(cmd);

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
    const auto onRecordDone = [session, parserProcessPtr](const Process &process, DoneWith result) {
        // Stopping recording calls process.stop(), which itself is reported as
        // DoneWith::Error (see Process::stop()'s ProcessResult::Canceled) -- so
        // only a genuine failure to launch perf is treated as an error here;
        // otherwise, whether perf exited by request or the target died on its
        // own, let perfparser finish and let onParserDone's sample count decide.
        if (result == DoneWith::Error && process.error() == QProcess::FailedToStart
            && !session->result) {
            session->result = ResultError(
                Tr::tr("Failed to start \"perf record\": %1").arg(process.errorString()));
        }
        if (Process *parser = parserProcessPtr->data())
            parser->closeWriteChannel();
    };

    const auto onParserSetup = [parserExe, parserProcessPtr, decoder](Process &process) {
        *parserProcessPtr = &process;
        process.setCommand(CommandLine(parserExe));
        process.setProcessMode(ProcessMode::Writer); // perf record's output is written to its stdin
        QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                         [p = &process, decoder] { decoder->addData(p->readAllRawStandardOutput()); });
    };
    const auto onParserDone = [session, sampleData, decoder, parserProcessPtr](
                                  const Process &process, DoneWith result) {
        // perfparser may still have unread, already-flushed output pending.
        if (Process *parser = parserProcessPtr->data())
            decoder->addData(parser->readAllRawStandardOutput());

        if (session->result)
            return;
        if (result == DoneWith::Error && process.error() == QProcess::FailedToStart) {
            session->result = ResultError(
                Tr::tr("Failed to start perfparser: %1").arg(process.errorString()));
            return;
        }
        if (sampleData->samples.isEmpty()) {
            session->result = ResultError(
                Tr::tr("No samples were captured. The target may have exited immediately, or "
                       "\"perf_event_paranoid\" may be blocking unprivileged sampling."));
            return;
        }

        const QString dirName = u"qtprofiler-sample-%1"_s.arg(QDateTime::currentMSecsSinceEpoch());
        const QString dirPath =
            QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(dirName);
        if (!QDir().mkpath(dirPath)) {
            session->result = ResultError(
                Tr::tr("Cannot create temporary trace directory %1.").arg(dirPath));
            return;
        }

        const FilePath dir = FilePath::fromString(dirPath);
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
