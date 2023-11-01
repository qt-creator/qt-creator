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
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <qtsupport/qtkitaspect.h>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QtEndian>

using namespace ProjectExplorer;
using namespace Utils;

namespace PerfProfiler {
namespace Internal {

static const qint64 million = static_cast<qint64>(1000000);

PerfDataReader::PerfDataReader(QObject *parent) :
    PerfProfilerTraceFile(parent), m_recording(true), m_dataFinished(false),
    m_localProcessStart(QDateTime::currentMSecsSinceEpoch() * million),
    m_localRecordingEnd(0),
    m_localRecordingStart(0),
    m_remoteProcessStart(std::numeric_limits<qint64>::max()),
    m_lastRemoteTimestamp(0)
{
    connect(&m_input, &QProcess::finished, this, [this](int exitCode) {
        emit processFinished();
        // process any remaining input before signaling finished()
        readFromDevice();
        if (m_recording || future().isRunning()) {
            m_localRecordingEnd = 0;
            emit finished();
        }
        if (exitCode != 0) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("Perf Data Parser Failed"),
                                 Tr::tr("The Perf data parser failed to process all the samples. "
                                        "Your trace is incomplete. The exit code was %1.")
                                 .arg(exitCode));
        }
    });

    connect(&m_input, &QIODevice::bytesWritten, this, &PerfDataReader::writeChunk);
    connect(&m_input, &QProcess::started, this, [this]() {
        emit processStarted();
        if (m_input.isWritable()) {
            writeChunk();

            // if !writable, we're loading from a file.
            // The timestamp calculations don't make any sense then.
            startTimer(100);
        }
        if (m_recording) {
            emit starting();
            emit started();
        }
    });

    connect(&m_input, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e){
        switch (e) {
        case QProcess::FailedToStart:
            emit processFailed(Tr::tr("perfparser failed to start."));
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("Perf Data Parser Failed"),
                                 Tr::tr("Could not start the perfparser utility program. "
                                        "Make sure a working Perf parser is available at the "
                                        "location given by the PERFPROFILER_PARSER_FILEPATH "
                                        "environment variable."));
            break;
        case QProcess::Crashed:
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("Perf Data Parser Crashed"),
                                 Tr::tr("This is a bug. Please report it."));
            break;
        case QProcess::ReadError:
            qWarning() << "Cannot receive data from perfparser";
            break;
        case QProcess::WriteError:
            qWarning() << "Cannot send data to perfparser";
            break;
        case QProcess::Timedout:
            qWarning() << "QProcess::Timedout";
        default:
            break;
        }
    });

    connect(&m_input, &QProcess::readyReadStandardOutput, this, &PerfDataReader::readFromDevice);
    connect(&m_input, &QProcess::readyReadStandardError, this, [this]() {
        Core::MessageManager::writeSilently(QString::fromLocal8Bit(m_input.readAllStandardError()));
    });

    setDevice(&m_input);
}

PerfDataReader::~PerfDataReader()
{
    m_input.kill();
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
    m_input.start(QIODevice::ReadOnly);
}

void PerfDataReader::createParser(const CommandLine &cmd)
{
    clear();
    const QString program = cmd.executable().path();
    m_input.setProgram(program);
    m_input.setArguments(cmd.splitArguments());
    m_input.setWorkingDirectory(QFileInfo(program).dir().absolutePath());
}

void PerfDataReader::startParser()
{
    traceManager()->clearAll();
    m_input.start(QIODevice::ReadWrite);
}

void PerfDataReader::stopParser()
{
    m_dataFinished = true;
    if (m_input.state() != QProcess::NotRunning) {
        if (m_recording || future().isRunning()) {
            m_localRecordingEnd = QDateTime::currentMSecsSinceEpoch() * million;
            emit finishing();
            if (m_buffer.isEmpty() && m_input.isOpen())
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
        if (m_input.state() != QProcess::NotRunning) {
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

            Core::FutureProgress *fp = Core::ProgressManager::addTimedTask(
                        future(), Tr::tr("Skipping Processing Delay"),
                        Constants::PerfProfilerTaskSkipDelay, seconds);
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
    if (m_input.state() != QProcess::NotRunning) {
        bool waitingForEndDelay = (m_localRecordingEnd != 0 && !m_dataFinished &&
                m_input.isWritable());
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

    if (QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit)) {
        cmd->addArg("--extra");
        cmd->addArg(QString("%1%5%2%5%3%5%4")
                     .arg(qt->libraryPath().nativePath())
                     .arg(qt->pluginPath().nativePath())
                     .arg(qt->hostBinPath().nativePath())
                     .arg(qt->qmlPath().nativePath())
                     .arg(cmd->executable().osType() == OsTypeWindows ? u';' : u':'));
    }

    if (auto toolChain = ToolChainKitAspect::cxxToolChain(kit)) {
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

void PerfDataReader::writeChunk()
{
    if (!m_buffer.isEmpty()) {
        if (m_input.bytesToWrite() < s_maxBufferSize) {
            std::unique_ptr<Utils::TemporaryFile> file(m_buffer.takeFirst());
            file->reset();
            const QByteArray data(file->readAll());
            if (!checkedWrite(&m_input, data)) {
                m_input.disconnect();
                m_input.kill();
                emit finished();
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     Tr::tr("Cannot Send Data to Perf Data Parser"),
                                     Tr::tr("The Perf data parser does not accept further input. "
                                            "Your trace is incomplete."));
            }
        }
    } else if (m_dataFinished && m_input.isWritable()) {
        // Delay closing of the write channel. Closing the channel from within a handler
        // for bytesWritten() is dangerous on windows.
        QTimer::singleShot(0, &m_input, &QProcess::closeWriteChannel);
    }
}

void PerfDataReader::clear()
{
    // not closing the buffer here as input may arrive before createParser()
    m_input.kill();
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
    if (!m_buffer.isEmpty()) {
        auto *file = m_buffer.last();
        if (file->pos() < s_maxBufferSize)
            return checkedWrite(file, input);
    } else if (m_input.isOpen() && m_input.bytesToWrite() < s_maxBufferSize) {
        return checkedWrite(&m_input, input);
    }

    auto file = std::make_unique<Utils::TemporaryFile>("perfdatareader");
    connect(file.get(), &QIODevice::bytesWritten, this, &PerfDataReader::writeChunk);
    if (!file->open() || !checkedWrite(file.get(), input))
        return false;

    m_buffer.append(file.release());
    return true;
}

void PerfDataReader::addTargetArguments(CommandLine *cmd, const RunControl *runControl) const
{
    ProjectExplorer::Kit *kit = runControl->kit();
    QTC_ASSERT(kit, return);
    ProjectExplorer::BuildConfiguration *buildConfig = runControl->target()->activeBuildConfiguration();
    QString buildDir = buildConfig ? buildConfig->buildDirectory().toString() : QString();
    collectArguments(cmd, buildDir, kit);
}

FilePath findPerfParser()
{
    FilePath filePath = FilePath::fromUserInput(qtcEnvironmentVariable("PERFPROFILER_PARSER_FILEPATH"));
    if (filePath.isEmpty())
        filePath = Core::ICore::libexecPath("perfparser" QTC_HOST_EXE_SUFFIX);
    return filePath;
}

} // namespace Internal
} // namespace PerfProfiler
