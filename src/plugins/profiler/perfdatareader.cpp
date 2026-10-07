// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfdatareader.h"
#include "perfprofilertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

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
#ifndef QTPROFILER_WASM // QtSupport is excluded from the standalone viewer
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
using namespace Qt::StringLiterals;

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
    connect(&m_input, &PerfConversion::done, this, [this] {
        // process any remaining input before signaling finished()
        readFromConversion();
        if (m_recording || future().isRunning()) {
            m_localRecordingEnd = 0;
            emit finished();
        }
        for (const QString &warning : m_input.warnings())
            Core::MessageManager::writeDisrupting(warning);
        if (m_input.result() == PerfConversion::Result::Failed) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("The Perf data could not be processed completely. The trace is "
                       "incomplete. %1").arg(m_input.errorString()));
        }
        emit processFinished();
    });

    connect(&m_input, &PerfConversion::started, this, [this] {
        emit processStarted();
        // Flush whatever was buffered before the conversion started.
        if (m_input.isFed())
            writeChunk();

        // The delay/timestamp bookkeeping only applies to live data. When loading
        // from a file these calculations make no sense, so the timer stays off.
        if (m_input.isLive())
            startTimer(100);
        if (m_recording) {
            emit starting();
            emit started();
        }
    });

    connect(&m_input, &PerfConversion::readyRead, this, &PerfDataReader::readFromConversion);

    m_output.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    setDevice(&m_output);
}

PerfDataReader::~PerfDataReader()
{
    QObject::disconnect(this, &PerfDataReader::processFinished, nullptr, nullptr);
    m_input.disconnect();
    m_input.kill();
    m_input.waitForFinished();
    qDeleteAll(m_buffer);
}

void PerfDataReader::loadFromFile(const FilePath &filePath, const QString &executableDirPath,
                                  Kit *kit)
{
    createParser(collectOptions(executableDirPath, kit));
    m_input.setInputFile(filePath);

    m_remoteProcessStart = 0; // Don't try to guess the timestamps
    m_input.start();
}

void PerfDataReader::createParser(const PerfConversionOptions &options, const QUrl &inputUrl)
{
    clear();
    m_input.setOptions(options);
    m_input.setInputFile({});
    m_input.setInputUrl(inputUrl);
}

void PerfDataReader::startParser()
{
    traceManager()->clearAll();
    m_input.start();
}

void PerfDataReader::detachTraceManager()
{
    setTraceManager(nullptr);
    m_recording = false; // The timer must not ask the manager for a duration.
    m_input.kill();      // Output still in flight is discarded by readMessages().
}

void PerfDataReader::stopParser()
{
    m_dataFinished = true;
    if (m_input.isRunning()) {
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
    if (m_input.isRunning()) {
        // Heartbeat for the disk-spill drain, for the case where the backlog was spilled
        // while the conversion was idle: readFromConversion() only pumps us when output
        // arrives.
        if (!m_buffer.isEmpty())
            writeChunk();

        bool waitingForEndDelay = (m_localRecordingEnd != 0 && !m_dataFinished &&
                m_input.isLive());
        bool waitingForStartDelay = m_localRecordingStart != 0;
        qint64 endTime = (m_localRecordingEnd == 0 || waitingForEndDelay) ?
                    currentTime : m_localRecordingEnd;
        qint64 currentDelay = qMax(delay(endTime), 1ll);

        emit updateTimestamps(m_recording && traceManager() ? traceManager()->traceDuration() : -1,
                              currentDelay);
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

PerfConversionOptions PerfDataReader::collectOptions(const QString &exe, const Kit *kit)
{
    PerfConversionOptions options;
    if (!exe.isEmpty())
        options.searchPaths.append(FilePath::fromUserInput(exe));

#ifndef QTPROFILER_WASM
    if (QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit)) {
        options.searchPaths << qt->libraryPath() << qt->pluginPath() << qt->hostBinPath()
                            << qt->qmlPath();
    }
#endif

    if (auto toolChain = ToolchainKitAspect::cxxToolchain(kit)) {
        // By the names perfArchitectureFromName() knows.
        const Abi abi = toolChain->targetAbi();
        const bool is64Bit = abi.wordWidth() == 64;
        if (abi.architecture() == Abi::ArmArchitecture)
            options.architecture = is64Bit ? u"aarch64"_s : u"arm"_s;
        else if (abi.architecture() == Abi::X86Architecture)
            options.architecture = is64Bit ? u"x86_64"_s : u"i386"_s;
        else if (abi.architecture() != Abi::UnknownArchitecture)
            options.architecture = Abi::toString(abi.architecture());
    }

    options.sysroot = SysRootKitAspect::sysRoot(kit);
    return options;
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

void PerfDataReader::readFromConversion()
{
    // Funnel the converted stream into m_output, which the streaming reader in
    // PerfProfilerTraceFile consumes via the QIODevice interface. ProcessOutputBuffer
    // reclaims its memory as the reader drains it.
    m_output.append(m_input.readAllOutput());

    // Output means the conversion digested what we fed it, so its input queue has
    // drained: clear the stall counter and push whatever is still spilled on disk.
    m_bytesSinceParserOutput = 0;
    readFromDevice();
    if (!m_buffer.isEmpty())
        writeChunk();
}

bool PerfDataReader::parserKeepsUp() const
{
    // The conversion emits trace data as it consumes its input, so a count that keeps
    // growing without any output coming back means it stalled and everything we wrote
    // since is still sitting in its queue. Feed it only while it stays under the
    // threshold; the rest waits on disk, keeping Creator's memory bounded.
    return m_bytesSinceParserOutput < s_maxBufferSize;
}

bool PerfDataReader::writeToParser(const QByteArray &data)
{
    if (m_input.write(data) != data.size())
        return false;

    m_bytesSinceParserOutput += data.size();
    return true;
}

void PerfDataReader::writeChunk()
{
    // Both write() below and the write channel only exist while we feed live data.
    if (!m_input.isRunning() || !m_input.isFed())
        return;

    // Drain the spilled backlog into the conversion, but only while it keeps up.
    while (!m_buffer.isEmpty() && parserKeepsUp()) {
        std::unique_ptr<Utils::TemporaryFile> file(m_buffer.takeFirst());
        file->reset();
        if (!writeToParser(file->readAll())) {
            m_input.disconnect();
            m_input.kill();
            emit finished();
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("Cannot Process Perf Data"),
                                 Tr::tr("The Perf data processing does not accept further "
                                        "input. The trace is incomplete."));
            return;
        }
    }

    if (!m_buffer.isEmpty()) {
        // The conversion is not keeping up. The backlog stays on disk until it reports
        // progress again, which pumps the drain from readFromConversion().
        return;
    }

    if (m_dataFinished && m_input.isFed())
        QTimer::singleShot(0, &m_input, &PerfConversion::closeWriteChannel);
}

void PerfDataReader::clear()
{
    // not closing the buffer here as input may arrive before createParser()
    m_input.kill();
    // Drop converted output that arrived but was never consumed: PerfProfilerTraceFile::clear()
    // below resets the stream version, so a stale prefix would fail the magic-header check
    // and abort the next run with a spurious "Invalid data format". Both createParser() call
    // sites start from a freshly constructed reader, which is what keeps a dying run from
    // feeding the next one.
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
    // While there is no backlog and the conversion keeps up, hand data straight to it.
    // Otherwise spill to a temporary file and let writeChunk() drain it once it catches
    // up, instead of piling the data up in memory.
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

    // Kick the drain so the freshly spilled data is flushed once the conversion catches up.
    writeChunk();
    return true;
}

PerfConversionOptions PerfDataReader::targetOptions(const RunControl *runControl) const
{
    ProjectExplorer::Kit *kit = runControl->kit();
    QTC_ASSERT(kit, return {});
    ProjectExplorer::BuildConfiguration *buildConfig = runControl->buildConfiguration();
    QString buildDir = buildConfig ? buildConfig->buildDirectory().toUrlishString() : QString();
    return collectOptions(buildDir, kit);
}

} // namespace Profiler::Internal
