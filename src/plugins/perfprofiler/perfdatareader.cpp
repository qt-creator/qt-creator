/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "perfdatareader.h"
#include "perfprofilerconstants.h"
#include "perfprofilerplugin.h"
#include "perfrunconfigurationaspect.h"
#include "perfsettings.h"
#include "perftimelinemodel.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QtEndian>

using namespace ProjectExplorer;

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
    connect(&m_input, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode) {
        emit processFinished();
        // process any remaining input before signaling finished()
        readFromDevice();
        if (m_recording || future().isRunning()) {
            m_localRecordingEnd = 0;
            emit finished();
        }
        if (exitCode != 0) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 tr("Perf Data Parser Failed"),
                                 tr("The Perf data parser failed to process all the samples. "
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
            emit processFailed(tr("perfparser failed to start."));
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 tr("Perf Data Parser Failed"),
                                 tr("Could not start the perfparser utility program. "
                                    "Make sure a working Perf parser is available at the location "
                                    "given by the PERFPROFILER_PARSER_FILEPATH environment "
                                    "variable."));
            break;
        case QProcess::Crashed:
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 tr("Perf Data Parser Crashed"),
                                 tr("This is a bug. Please report it."));
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
    connect(&m_input, &QProcess::readyReadStandardError, this, [this](){
        Core::MessageManager::write(QString::fromLocal8Bit(m_input.readAllStandardError()),
                                    Core::MessageManager::Silent);
    });

    setDevice(&m_input);
}

PerfDataReader::~PerfDataReader()
{
    m_input.kill();
    qDeleteAll(m_buffer);
}

void PerfDataReader::loadFromFile(const QString &filePath, const QString &executableDirPath,
                                  ProjectExplorer::Kit *kit)
{
    createParser(collectArguments(executableDirPath, kit) << QLatin1String("--input") << filePath);
    m_remoteProcessStart = 0; // Don't try to guess the timestamps
    m_input.start(QIODevice::ReadOnly);
}

void PerfDataReader::createParser(const QStringList &arguments)
{
    clear();
    QString program = findPerfParser();
    m_input.setProgram(program);
    m_input.setArguments(arguments);
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
                        future(), tr("Skipping Processing Delay"),
                        Constants::PerfProfilerTaskSkipDelay, seconds);
            fp->setToolTip(recording ?
                               tr("Cancel this to ignore the processing delay and immediately "
                                  "start recording.") :
                               tr("Cancel this to ignore the processing delay and immediately "
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

QStringList PerfDataReader::collectArguments(const QString &executableDirPath, const Kit *kit) const
{
    QStringList arguments;
    if (!executableDirPath.isEmpty())
        arguments << "--app" << executableDirPath;

    if (QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit)) {
        arguments << "--extra" << QString("%1%5%2%5%3%5%4")
                     .arg(QDir::toNativeSeparators(qt->libraryPath().toString()))
                     .arg(QDir::toNativeSeparators(qt->pluginPath().toString()))
                     .arg(QDir::toNativeSeparators(qt->hostBinPath().toString()))
                     .arg(QDir::toNativeSeparators(qt->qmlPath().toString()))
                     .arg(QDir::listSeparator());
    }

    if (auto toolChain = ToolChainKitAspect::cxxToolChain(kit)) {
        Abi::Architecture architecture = toolChain->targetAbi().architecture();
        if (architecture == Abi::ArmArchitecture && toolChain->targetAbi().wordWidth() == 64) {
            arguments << "--arch" << "aarch64";
        } else if (architecture != Abi::UnknownArchitecture) {
            arguments << "--arch" << Abi::toString(architecture);
        }
    }

    QString sysroot = SysRootKitAspect::sysRoot(kit).toString();
    if (!sysroot.isEmpty())
        arguments << "--sysroot" << sysroot;

    return arguments;
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
                                     tr("Cannot Send Data to Perf Data Parser"),
                                     tr("The Perf data parser does not accept further input. "
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

QStringList PerfDataReader::findTargetArguments(const ProjectExplorer::RunControl *runControl) const
{
    ProjectExplorer::Kit *kit = runControl->kit();
    QTC_ASSERT(kit, return QStringList());
    ProjectExplorer::BuildConfiguration *buildConfig = runControl->target()->activeBuildConfiguration();
    QString buildDir = buildConfig ? buildConfig->buildDirectory().toString() : QString();
    return collectArguments(buildDir, kit);
}

QString PerfDataReader::findPerfParser()
{
    QString filePath = QString::fromLocal8Bit(qgetenv("PERFPROFILER_PARSER_FILEPATH"));
    if (filePath.isEmpty()) {
        filePath = QString::fromLatin1("%1/perfparser%2").arg(Core::ICore::libexecPath(),
                                                              QTC_HOST_EXE_SUFFIX);
    }
    return QDir::toNativeSeparators(QDir::cleanPath(filePath));
}

} // namespace Internal
} // namespace PerfProfiler
