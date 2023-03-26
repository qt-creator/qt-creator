// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofilertracefile.h"
#include "perftimelinemodelmanager.h"

#include <utils/temporaryfile.h>

#include <QProcess>
#include <QQueue>

namespace ProjectExplorer {
class Kit;
class RunControl;
} // ProjectExplorer

namespace PerfProfiler {
namespace Internal {

Utils::FilePath findPerfParser();

class PerfDataReader : public PerfProfilerTraceFile
{
    Q_OBJECT
public:
    explicit PerfDataReader(QObject *parent = nullptr);
    ~PerfDataReader() override;

    void loadFromFile(const Utils::FilePath &filePath, const QString &executableDirPath,
                      ProjectExplorer::Kit *kit);

    void createParser(const Utils::CommandLine &arguments);
    void startParser();
    void stopParser();

    void addTargetArguments(Utils::CommandLine *cmd, const ProjectExplorer::RunControl *runControl) const;
    void clear();

    bool feedParser(const QByteArray &input);

    // Trigger after delay has passed
    void triggerRecordingStateChange(bool recording);

    // Immediate forced change
    void setRecording(bool recording);

signals:
    void starting();
    void started();
    void finishing();
    void finished();
    void updateTimestamps(qint64 duration, qint64 delay);

    void processStarted();
    void processFinished();
    void processFailed(const QString &msg);

protected:
    void timerEvent(QTimerEvent *) override;
    qint64 adjustTimestamp(qint64 timestamp) override;
    bool acceptsSamples() const override;

private:
    static const int s_maxBufferSize = 1 << 29;

    void collectArguments(Utils::CommandLine *cmd,
                          const QString &executableDirPath,
                          const ProjectExplorer::Kit *kit) const;
    void writeChunk();

    bool m_recording;
    bool m_dataFinished;
    QProcess m_input;
    QQueue<Utils::TemporaryFile *> m_buffer;
    qint64 m_localProcessStart;
    qint64 m_localRecordingEnd;
    qint64 m_localRecordingStart;
    qint64 m_remoteProcessStart;
    qint64 m_lastRemoteTimestamp;

    qint64 delay(qint64 currentTime);
};

} // namespace Internal
} // namespace PerfProfiler
