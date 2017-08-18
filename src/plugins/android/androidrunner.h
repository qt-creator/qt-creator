/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidconfigurations.h"
#include "androidrunnable.h"

#include <projectexplorer/runconfiguration.h>
#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qmldebug/qmloutputparser.h>

#include <QFutureInterface>
#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QThread>
#include <QProcess>
#include <QMutex>

namespace Android {
namespace Internal {

class AndroidRunnerWorker;

class AndroidRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit AndroidRunner(ProjectExplorer::RunControl *runControl);
    ~AndroidRunner() override;

    void setRunnable(const AndroidRunnable &runnable);
    const AndroidRunnable &runnable() const { return m_androidRunnable; }

    Utils::Port gdbServerPort() const { return m_gdbServerPort; }
    QUrl qmlServer() const { return m_qmlServer; }
    Utils::ProcessHandle pid() const { return m_pid; }

    void start() override;
    void stop() override;

signals:
    void asyncStart();
    void asyncStop();
    void remoteDebuggerRunning();
    void qmlServerReady(const QUrl &serverUrl);
    void androidRunnableChanged(const AndroidRunnable &runnable);
    void avdDetected();

private:
    void qmlServerPortReady(Utils::Port port);
    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);
    void gotRemoteOutput(const QString &output);
    void handleRemoteProcessStarted(Utils::Port gdbServerPort, const QUrl &qmlServer, int pid);
    void handleRemoteProcessFinished(const QString &errString = QString());
    void checkAVD();
    void launchAVD();

    AndroidRunnable m_androidRunnable;
    QString m_launchedAVDName;
    QThread m_thread;
    QTimer m_checkAVDTimer;
    QScopedPointer<AndroidRunnerWorker> m_worker;
    QPointer<ProjectExplorer::Target> m_target;
    Utils::Port m_gdbServerPort;
    QUrl m_qmlServer;
    Utils::ProcessHandle m_pid;
    QmlDebug::QmlOutputParser m_outputParser;
};

} // namespace Internal
} // namespace Android
