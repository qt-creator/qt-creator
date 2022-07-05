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

#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <QObject>
#include <QStringList>
#include <QWindowList>

#include <memory>

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

    static SquishTools *instance();

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
    void queryServerSettings();

signals:
    void logOutputReceived(const QString &output);
    void squishTestRunStarted();
    void squishTestRunFinished();
    void resultOutputCreated(const QByteArray &output);
    void queryFinished(const QByteArray &output);

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
    void executeRunnerQuery();
    static Utils::Environment squishEnvironment();
    void onServerFinished();
    void onRunnerFinished();
    void onServerOutput();
    void onServerErrorOutput();
    void onRunnerOutput();
    void onRunnerErrorOutput();
    void onResultsDirChanged(const QString &filePath);
    static void logrotateTestResults();
    void minimizeQtCreatorWindows();
    void restoreQtCreatorWindows();
    bool isValidToStartRunner();
    bool setupRunnerPath();
    void setupAndStartSquishRunnerProcess(const QStringList &arg,
                                          const QString &caseReportFilePath = {});

    std::unique_ptr<SquishXmlOutputHandler> m_xmlOutputHandler;
    Utils::QtcProcess m_serverProcess;
    Utils::QtcProcess m_runnerProcess;
    int m_serverPort = -1;
    QString m_serverHost;
    Request m_request = None;
    State m_state = Idle;
    QString m_suitePath;
    QStringList m_testCases;
    QStringList m_reportFiles;
    QString m_currentResultsDirectory;
    QFile *m_currentResultsXML = nullptr;
    QFileSystemWatcher *m_resultsFileWatcher = nullptr;
    QStringList m_additionalServerArguments;
    QStringList m_additionalRunnerArguments;
    QWindowList m_lastTopLevelWindows;
    enum RunnerMode { NoMode, TestingMode, QueryMode} m_squishRunnerMode = NoMode;
    qint64 m_readResultsCount;
};

} // namespace Internal
} // namespace Squish
