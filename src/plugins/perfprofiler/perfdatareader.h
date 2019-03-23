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

#pragma once

#include "perfprofilertracefile.h"
#include "perftimelinemodelmanager.h"

#include <projectexplorer/target.h>

#include <utils/temporaryfile.h>

#include <QProcess>
#include <QQueue>

namespace PerfProfiler {
namespace Internal {

class PerfDataReader : public PerfProfilerTraceFile
{
    Q_OBJECT
public:
    explicit PerfDataReader(QObject *parent = nullptr);
    ~PerfDataReader() override;

    void loadFromFile(const QString &filePath, const QString &executableDirPath,
                      ProjectExplorer::Kit *kit);

    void createParser(const QStringList &arguments);
    void startParser();
    void stopParser();

    QStringList findTargetArguments(const ProjectExplorer::RunConfiguration *rc) const;
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

    QStringList collectArguments(const QString &executableDirPath,
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

    static QString findPerfParser();
    qint64 delay(qint64 currentTime);
};

} // namespace Internal
} // namespace PerfProfiler
