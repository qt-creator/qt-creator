// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfdatareader.h"
#include "perfprofilerconstants.h"
#include "perfprofilertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>
#ifndef __EMSCRIPTEN__ // QtSupport is excluded from the WebAssembly build
#include <qtsupport/qtkitaspect.h>
#endif

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QtEndian>

using namespace ProjectExplorer;
using namespace Utils;

namespace Profiler::Internal {

static const qint64 million = static_cast<qint64>(1000000);

PerfDataReader::PerfDataReader(QObject *parent) :
    PerfProfilerTraceFile(parent), m_recording(true), m_dataFinished(false),
    m_localProcessStart(QDateTime::currentMSecsSinceEpoch() * million),
    m_localRecordingEnd(0),
    m_localRecordingStart(0),
    m_remoteProcessStart(std::numeric_limits<qint64>::max()),
    m_lastRemoteTimestamp(0)
{
    connect(&m_input, &Process::done, this, [this] {
        if (m_input.result() == ProcessResult::StartFailed) {
            // The process never ran: there is no output to drain, and emitting finished()
            // here would finalize a trace that was never initialized. Report the failure
            // only.
            emit processFailed(Tr::tr("perfparser failed to start."));
            Core::MessageManager::writeDisrupting(
                Tr::tr("Could not start the perfparser utility program. "
                       "Make sure a working Perf parser is available at the "
                       "location given by the PERFPROFILER_PARSER_FILEPATH "
                       "environment variable."));
            return;
        }

        // process any remaining input before signaling finished()
        readFromProcess();
        if (m_recording || future().isRunning()) {
            m_localRecordingEnd = 0;
            emit finished();
        }
        if (m_input.result() == ProcessResult::TerminatedAbnormally) {
            Core::MessageManager::writeDisrupting(Tr::tr("Perf Data Parser Crashed"));
        } else if (const int exitCode = m_input.exitCode();
                   exitCode != 0 && m_input.result() != ProcessResult::Canceled) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("The Perf data parser failed to process all the samples. "
                       "Your trace is incomplete. The exit code was %1.")
                    .arg(exitCode));
        }
        emit processFinished();
    });

    connect(&m_input, &Process::started, this, [this] {
        emit processStarted();
        if (m_input.processMode() == ProcessMode::Writer) {
            // Flush whatever was buffered before the process started running.
            writeChunk();

            // The delay/timestamp bookkeeping only applies while we feed live data. In
            // Reader mode we're loading from a file, where these calculations make no sense,
            // so the timer stays off.
            startTimer(100);
        }
        if (m_recording) {
            emit starting();
            emit started();
        }
    });

    connect(&m_input, &Process::readyReadStandardOutput,
            this, &PerfDataReader::readFromProcess);
    connect(&m_input, &Process::readyReadStandardError, this, [this] {
        Core::MessageManager::writeSilently(m_input.readAllStandardError());
    });

    m_output.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    setDevice(&m_output);
}

PerfDataReader::~PerfDataReader()
{
    QObject::disconnect(this, &PerfDataReader::processFinished, nullptr, nullptr);
    QObject::disconnect(this, &PerfDataReader::processFailed, nullptr, nullptr);
    m_input.kill();
    m_input.waitForFinished();
    qDeleteAll(m_buffer);
}

void PerfDataReader::loadFromFile(const FilePath &filePath, const QString &executableDirPath,
                                  Kit *kit)
{
    CommandLine cmd{findPerfParser()};
    collectArguments(&cmd, executableDirPath, kit);
    cmd.addArg("--input");
    cmd.addArg(filePath.nativePath());
    createParser(cmd);

    m_remoteProcessStart = 0; // Don't try to guess the timestamps
    // Reader mode: perfparser reads the input file itself; we only read its stdout.
    m_input.setProcessMode(ProcessMode::Reader);
    m_input.start();
}

void PerfDataReader::createParser(const CommandLine &cmd)
{
    clear();
    m_input.setCommand(cmd);
    m_input.setWorkingDirectory(cmd.executable().parentDir());
}

void PerfDataReader::startParser()
{
    traceManager()->clearAll();
    // Writer mode: we stream perf data into perfparser's stdin and read its stdout.
    m_input.setProcessMode(ProcessMode::Writer);
    m_input.start();
}

void PerfDataReader::stopParser()
{
    m_dataFinished = true;
    if (m_input.state() != ProcessState::NotRunning) {
        if (m_recording || future().isRunning()) {
            m_localRecordingEnd = QDateTime::currentMSecsSinceEpoch() * million;
            emit finishing();
            if (m_buffer.isEmpty() && m_input.isRunning())
                m_input.closeWriteChannel();
        } else if (m_buffer.isEmpty()) {
            m_input.closeWriteChannel();
        }
    }
}

qint64 PerfDataReader::delay(qint64 currentTime)
{
    return (currentTime - m_localProcessStart) -
            (m_lastRemoteTimestamp > m_remoteProcessStart ?
                 m_lastRemoteTimestamp - m_remoteProcessStart : 0);
}

void PerfDataReader::triggerRecordingStateChange(bool recording)
{
    if (recording != m_recording) {
        if (m_input.state() != ProcessState::NotRunning) {
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch() * million;
            if (recording) {
                m_localRecordingStart = currentTime;
                emit starting();
            } else {
                m_localRecordingEnd = currentTime;
                emit finishing();
            }
            const int seconds = static_cast<int>(
                        qMin(delay(currentTime) / (1000ll * million),
                             static_cast<qint64>(std::numeric_limits<int>::max())));

            Core::FutureProgress *fp
                = Core::ProgressManager::addTimedTask(future(),
                                                      Tr::tr("Skipping Processing Delay"),
                                                      Constants::PerfProfilerTaskSkipDelay,
                                                      std::chrono::seconds(seconds));
            fp->setToolTip(recording ?
                               Tr::tr("Cancel this to ignore the processing delay and immediately "
                                      "start recording.") :
                               Tr::tr("Cancel this to ignore the processing delay and immediately "
                                      "stop recording."));
            connect(fp, &Core::FutureProgress::canceled, this, [this, recording]() {
                setRecording(recording);
            });
            future().reportStarted();
        } else {
            m_recording = recording;
        }
    }
}

void PerfDataReader::setRecording(bool recording)
{
    if (recording == m_recording)
        return;

    m_recording = recording;
    if (m_recording) {
        m_localRecordingStart = 0;
        emit started();
    } else {
        m_localRecordingEnd = 0;
        emit finished();
    }
    future().reportFinished();
}

void PerfDataReader::timerEvent(QTimerEvent *event)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch() * million;
    if (m_input.state() != ProcessState::NotRunning) {
        // Heartbeat for the disk-spill drain, for the case where the backlog was spilled
        // while the parser was idle: readFromProcess() only pumps us when output arrives.
        if (!m_buffer.isEmpty())
            writeChunk();

        bool waitingForEndDelay = (m_localRecordingEnd != 0 && !m_dataFinished &&
                m_input.processMode() == ProcessMode::Writer);
        bool waitingForStartDelay = m_localRecordingStart != 0;
        qint64 endTime = (m_localRecordingEnd == 0 || waitingForEndDelay) ?
                    currentTime : m_localRecordingEnd;
        qint64 currentDelay = qMax(delay(endTime), 1ll);

        emit updateTimestamps(m_recording ? traceManager()->traceDuration() : -1, currentDelay);
        if (waitingForStartDelay && currentTime - m_localRecordingStart > currentDelay)
            setRecording(true);
        else if (waitingForEndDelay && currentTime - m_localRecordingEnd > currentDelay)
            setRecording(false);
    } else {
        emit updateTimestamps(-1, 0);
        killTimer(event->timerId());
        future().reportCanceled();
    }
}

qint64 PerfDataReader::adjustTimestamp(qint64 timestamp)
{
    if (timestamp > m_lastRemoteTimestamp)
        m_lastRemoteTimestamp = timestamp;

    if (timestamp > 0) {
        if (m_remoteProcessStart == std::numeric_limits<qint64>::max()) {
            // Subtract the time since we locally triggered the process. Any mixup in remote
            // timestamps is certainly smaller than that.
            m_remoteProcessStart = timestamp - QDateTime::currentMSecsSinceEpoch() * million
                    + m_localProcessStart;
        }
        return timestamp - m_remoteProcessStart;
    }

    if (m_remoteProcessStart != std::numeric_limits<qint64>::max())
        return m_remoteProcessStart;

    return -1;
}

bool PerfDataReader::acceptsSamples() const
{
    return m_recording;
}

void PerfDataReader::collectArguments(CommandLine *cmd, const QString &exe, const Kit *kit) const
{
    if (!exe.isEmpty()) {
        cmd->addArg("--app");
        cmd->addArg(exe);
    }

#ifndef __EMSCRIPTEN__
    if (QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit)) {
        cmd->addArg("--extra");
        cmd->addArg(QString("%1%5%2%5%3%5%4")
                     .arg(qt->libraryPath().nativePath())
                     .arg(qt->pluginPath().nativePath())
                     .arg(qt->hostBinPath().nativePath())
                     .arg(qt->qmlPath().nativePath())
                     .arg(cmd->executable().pathListSeparator()));
    }
#endif

    if (auto toolChain = ToolchainKitAspect::cxxToolchain(kit)) {
        Abi::Architecture architecture = toolChain->targetAbi().architecture();
        if (architecture == Abi::ArmArchitecture && toolChain->targetAbi().wordWidth() == 64) {
            cmd->addArg("--arch");
            cmd->addArg("aarch64");
        } else if (architecture != Abi::UnknownArchitecture) {
            cmd->addArg("--arch");
            cmd->addArg(Abi::toString(architecture));
        }
    }

    const FilePath sysroot = SysRootKitAspect::sysRoot(kit);
    if (!sysroot.isEmpty()) {
        cmd->addArg("--sysroot");
        cmd->addArg(sysroot.nativePath());
    }
}

static bool checkedWrite(QIODevice *device, const QByteArray &input)
{
    qint64 written = 0;
    const qint64 size = input.size();
    while (written < size) {
        const qint64 bytes = device->write(input.constData() + written, size - written);
        if (bytes < 0)
            return false;

        written += bytes;
    }
    return true;
}

void PerfDataReader::readFromProcess()
{
    // Utils::Process is not a QIODevice, so funnel its stdout into m_output, which the
    // streaming reader in PerfProfilerTraceFile consumes via the QIODevice interface.
    // ProcessOutputBuffer reclaims its memory as the reader drains it.
    m_output.append(m_input.readAllRawStandardOutput());

    // Output means perfparser digested what we fed it, so its write queue has drained:
    // clear the stall counter and push whatever is still spilled on disk. This also takes
    // over the role QIODevice::bytesWritten() had in pumping the drain.
    m_bytesSinceParserOutput = 0;
    readFromDevice();
    if (!m_buffer.isEmpty())
        writeChunk();
}

bool PerfDataReader::parserKeepsUp() const
{
    // perfparser emits trace data as it consumes its stdin, so a count that keeps growing
    // without any output coming back means it stalled and everything we wrote since is
    // still sitting in the process's write queue. Feed it only while it stays under the
    // threshold; the rest waits on disk, keeping Creator's memory bounded.
    return m_bytesSinceParserOutput < s_maxBufferSize;
}

bool PerfDataReader::writeToParser(const QByteArray &data)
{
    if (m_input.writeRaw(data) != data.size())
        return false;

    m_bytesSinceParserOutput += data.size();
    return true;
}

void PerfDataReader::writeChunk()
{
    // Both writeRaw() below and the write channel only exist while we feed live data.
    if (!m_input.isRunning() || m_input.processMode() != ProcessMode::Writer)
        return;

    // Drain the spilled backlog into perfparser's stdin, but only while it keeps up.
    while (!m_buffer.isEmpty() && parserKeepsUp()) {
        std::unique_ptr<Utils::TemporaryFile> file(m_buffer.takeFirst());
        file->reset();
        if (!writeToParser(file->readAll())) {
            m_input.disconnect();
            m_input.kill();
            emit finished();
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("Cannot Send Data to Perf Data Parser"),
                                 Tr::tr("The Perf data parser does not accept further input. "
                                        "Your trace is incomplete."));
            return;
        }
    }

    if (!m_buffer.isEmpty()) {
        // perfparser is not keeping up. The backlog stays on disk until it reports progress
        // again, which pumps the drain from readFromProcess().
        return;
    }

    if (m_dataFinished && m_input.processMode() == ProcessMode::Writer) {
        // Delay closing of the write channel. Closing the channel from within a write
        // handler is dangerous on Windows.
        QTimer::singleShot(0, &m_input, &Process::closeWriteChannel);
    }
}

void PerfDataReader::clear()
{
    // not closing the buffer here as input may arrive before createParser()
    m_input.kill();
    // Drop parser output that arrived but was never consumed: PerfProfilerTraceFile::clear()
    // below resets the stream version, so a stale prefix would fail the magic-header check
    // and abort the next run with a spurious "Invalid data format". The kill() above is
    // asynchronous, so this does not cover a tail still to be delivered via done() - both
    // createParser() call sites start from a freshly constructed reader, which is what keeps
    // a dying run from feeding the next one.
    m_output.clearData();
    m_bytesSinceParserOutput = 0;
    qDeleteAll(m_buffer);
    m_buffer.clear();
    m_dataFinished = false;
    m_localProcessStart = QDateTime::currentMSecsSinceEpoch() * million;
    m_localRecordingEnd = 0;
    m_localRecordingStart = 0;
    m_lastRemoteTimestamp = 0;
    m_remoteProcessStart = std::numeric_limits<qint64>::max();
    PerfProfilerTraceFile::clear();
}

bool PerfDataReader::feedParser(const QByteArray &input)
{
    // While there is no backlog and perfparser keeps up, hand data straight to its stdin.
    // Otherwise spill to a temporary file and let writeChunk() drain it once the parser
    // catches up, instead of piling the data up in the process's in-memory write queue.
    if (m_buffer.isEmpty() && m_input.isRunning() && parserKeepsUp())
        return writeToParser(input);

    if (!m_buffer.isEmpty()) {
        auto *file = m_buffer.last();
        if (file->pos() < s_maxBufferSize)
            return checkedWrite(file, input);
    }

    auto file = std::make_unique<Utils::TemporaryFile>("perfdatareader");
    if (!file->open() || !checkedWrite(file.get(), input))
        return false;

    m_buffer.append(file.release());

    // Kick the drain so the freshly spilled data is flushed once the process catches up.
    writeChunk();
    return true;
}

void PerfDataReader::addTargetArguments(CommandLine *cmd, const RunControl *runControl) const
{
    ProjectExplorer::Kit *kit = runControl->kit();
    QTC_ASSERT(kit, return);
    ProjectExplorer::BuildConfiguration *buildConfig = runControl->buildConfiguration();
    QString buildDir = buildConfig ? buildConfig->buildDirectory().toUrlishString() : QString();
    collectArguments(cmd, buildDir, kit);
}

FilePath findPerfParser()
{
    FilePath filePath = FilePath::fromUserInput(qtcEnvironmentVariable("PERFPROFILER_PARSER_FILEPATH"));
    if (filePath.isEmpty())
        filePath = Core::ICore::libexecPath("perfparser" QTC_HOST_EXE_SUFFIX);
    return filePath;
}

} // namespace Profiler::Internal
