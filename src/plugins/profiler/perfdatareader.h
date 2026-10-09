// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfconversion.h"
#include "perfprofilertracefile.h"

#include <utils/temporaryfile.h>

#include <QIODevice>
#include <QQueue>

#include <cstring>


namespace ProjectExplorer {
class Kit;
class RunControl;
} // ProjectExplorer

namespace Profiler::Internal {

// Minimal append-only, sequential QIODevice bridging the converted stream (see
// PerfConversion) to the QIODevice-based streaming reader in PerfProfilerTraceFile. Being
// sequential keeps the reader on its incremental progress path; a random-access device would
// make it derive progress from pos()/size(), which is meaningless for a stream we grow and
// drain in place.
class ProcessOutputBuffer : public QIODevice
{
public:
    explicit ProcessOutputBuffer(QObject *parent = nullptr) : QIODevice(parent) {}

    void append(const QByteArray &data)
    {
        // Drop what the reader already consumed before growing again. The reader stops at
        // the first partial message, so m_readPos rarely lands on m_data.size() exactly;
        // without compacting here the consumed prefix would pile up for the whole run. The
        // memmove only covers the small unread tail.
        if (m_readPos > 0) {
            m_data.remove(0, m_readPos);
            m_readPos = 0;
        }
        m_data.append(data);
    }
    void clearData() { m_data.clear(); m_readPos = 0; }

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override
    {
        return (m_data.size() - m_readPos) + QIODevice::bytesAvailable();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 count = qMin(maxSize, m_data.size() - m_readPos);
        if (count > 0) {
            std::memcpy(data, m_data.constData() + m_readPos, count);
            m_readPos += count;
            if (m_readPos == m_data.size()) // fully drained: reclaim right away
                clearData();
        }
        return count;
    }
    qint64 writeData(const char *, qint64) override { return -1; }

private:
    QByteArray m_data;
    qint64 m_readPos = 0;
};

class PerfDataReader : public PerfProfilerTraceFile
{
    Q_OBJECT
public:
    explicit PerfDataReader(QObject *parent = nullptr);
    ~PerfDataReader() override;

    void loadFromFile(const Utils::FilePath &filePath, const QString &executableDirPath,
                      ProjectExplorer::Kit *kit);

    // `inputUrl`, if set, is where the recording comes from instead of feedParser().
    void createParser(const PerfConversionOptions &options, const QUrl &inputUrl = {});
    void startParser();
    void stopParser();
    // Lets go of the trace manager and kills the parser: for when the document
    // owning the manager goes away while the run is still feeding it.
    void detachTraceManager();

    PerfConversionOptions targetOptions(const ProjectExplorer::RunControl *runControl) const;
    void clear();

    bool feedParser(const QByteArray &input);

    void setRecording(bool recording);

signals:
    void starting();
    void started();
    void finishing();
    void finished();
    void updateTimestamps(qint64 duration, qint64 delay);

    void processStarted();
    void processFinished();

protected:
    void timerEvent(QTimerEvent *) override;
    qint64 adjustTimestamp(qint64 timestamp) override;
    bool acceptsSamples() const override;

private:
    static const int s_maxBufferSize = 1 << 29;

    static PerfConversionOptions collectOptions(const QString &executableDirPath,
                                                const ProjectExplorer::Kit *kit);
    void writeChunk();
    bool parserKeepsUp() const;
    bool writeToParser(const QByteArray &data);
    void readFromConversion();

    bool m_recording;
    bool m_dataFinished;
    PerfConversion m_input;
    // Bridges the converted stream to the QIODevice-based streaming reader in
    // PerfProfilerTraceFile.
    ProcessOutputBuffer m_output;
    // Bytes handed to the conversion since it last produced output, used to keep its
    // input queue bounded. See writeToParser().
    qint64 m_bytesSinceParserOutput = 0;
    QQueue<Utils::TemporaryFile *> m_buffer;
    qint64 m_localProcessStart;
    qint64 m_localRecordingEnd;
    qint64 m_localRecordingStart;
    qint64 m_remoteProcessStart;
    qint64 m_lastRemoteTimestamp;

    qint64 delay(qint64 currentTime);
};

} // namespace Profiler::Internal
