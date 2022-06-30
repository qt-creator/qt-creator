/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QWindowList>

QT_BEGIN_NAMESPACE
class QFile;
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace Squish {
namespace Internal {

class SquishXmlOutputHandler;

class SquishTools : public QObject
{
    Q_OBJECT
public:
    explicit SquishTools(QObject *parent = nullptr);
    ~SquishTools() override;

    enum State {
        Idle,
        ServerStarting,
        ServerStarted,
        ServerStartFailed,
        ServerStopped,
        ServerStopFailed,
        RunnerStarting,
        RunnerStarted,
        RunnerStartFailed,
        RunnerStopped
    };

    State state() const { return m_state; }
    void runTestCases(const QString &suitePath,
                      const QStringList &testCases = QStringList(),
                      const QStringList &additionalServerArgs = QStringList(),
                      const QStringList &additionalRunnerArgs = QStringList());
signals:
    void logOutputReceived(const QString &output);
    void squishTestRunStarted();
    void squishTestRunFinished();
    void resultOutputCreated(const QByteArray &output);

private:
    enum Request {
        None,
        ServerStopRequested,
        ServerQueryRequested,
        RunnerQueryRequested,
        RunTestRequested,
        RecordTestRequested,
        KillOldBeforeRunRunner,
        KillOldBeforeRecordRunner,
        KillOldBeforeQueryRunner
    };

    void setState(State state);
    void startSquishServer(Request request);
    void stopSquishServer();
    void startSquishRunner();
    static QProcessEnvironment squishEnvironment();
    Q_SLOT void onServerFinished(int exitCode, QProcess::ExitStatus status = QProcess::NormalExit);
    Q_SLOT void onRunnerFinished(int exitCode, QProcess::ExitStatus status = QProcess::NormalExit);
    void onServerOutput();
    void onServerErrorOutput();
    void onRunnerOutput();
    void onRunnerErrorOutput();
    void onResultsDirChanged(const QString &filePath);
    static void logrotateTestResults();
    void minimizeQtCreatorWindows();
    void restoreQtCreatorWindows();

    QProcess *m_serverProcess;
    QProcess *m_runnerProcess;
    int m_serverPort;
    QString m_serverHost;
    Request m_request;
    State m_state;
    QString m_suitePath;
    QStringList m_testCases;
    QStringList m_reportFiles;
    QString m_currentResultsDirectory;
    QFile *m_currentResultsXML;
    QFileSystemWatcher *m_resultsFileWatcher;
    QStringList m_additionalServerArguments;
    QStringList m_additionalRunnerArguments;
    QWindowList m_lastTopLevelWindows;
    bool m_testRunning;
    qint64 m_readResultsCount;
    SquishXmlOutputHandler *m_xmlOutputHandler;
};

} // namespace Internal
} // namespace Squish
