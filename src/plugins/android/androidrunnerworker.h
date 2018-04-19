/****************************************************************************
**
** Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QFuture>
#include <QTcpSocket>

#include "androidrunnable.h"

namespace ProjectExplorer {
class RunControl;
}

namespace Android {
namespace Internal {

const int MIN_SOCKET_HANDSHAKE_PORT = 20001;

static inline void deleter(QProcess *p)
{
    p->terminate();
    if (!p->waitForFinished(1000)) {
        p->kill();
        p->waitForFinished();
    }
    // Might get deleted from its own signal handler.
    p->deleteLater();
}

class AndroidRunnerWorkerBase : public QObject
{
    Q_OBJECT
public:
    AndroidRunnerWorkerBase(ProjectExplorer::RunControl *runControl, const AndroidRunnable &runnable);
    ~AndroidRunnerWorkerBase() override;
    bool adbShellAmNeedsQuotes();
    bool runAdb(const QStringList &args, int timeoutS = 10);
    void adbKill(qint64 pid);
    QStringList selector() const;
    void forceStop();
    void logcatReadStandardError();
    void logcatReadStandardOutput();
    void logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError);
    void setAndroidRunnable(const AndroidRunnable &runnable);

    virtual void asyncStart();
    virtual void asyncStop();
    virtual void handleRemoteDebuggerRunning();
    virtual void handleJdbWaiting();
    virtual void handleJdbSettled();

signals:
    void remoteProcessStarted(Utils::Port gdbServerPort, const QUrl &qmlServer, int pid);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

protected:
    enum class JDBState {
        Idle,
        Waiting,
        Settled
    };
    virtual void onProcessIdChanged(qint64 pid);

    // Create the processes and timer in the worker thread, for correct thread affinity
    AndroidRunnable m_androidRunnable;
    QString m_adb;
    qint64 m_processPID = -1;
    std::unique_ptr<QProcess, decltype(&deleter)> m_adbLogcatProcess;
    std::unique_ptr<QProcess, decltype(&deleter)> m_psIsAlive;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    QRegExp m_logCatRegExp;
    QFuture<qint64> m_pidFinder;
    bool m_useCppDebugger = false;
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;
    Utils::Port m_localGdbServerPort; // Local end of forwarded debug socket.
    QUrl m_qmlServer;
    QByteArray m_lastRunAdbRawOutput;
    QString m_lastRunAdbError;
    JDBState m_jdbState = JDBState::Idle;
    Utils::Port m_localJdbServerPort;
    std::unique_ptr<QProcess, decltype(&deleter)> m_gdbServerProcess;
    std::unique_ptr<QProcess, decltype(&deleter)> m_jdbProcess;
};


class AndroidRunnerWorker : public AndroidRunnerWorkerBase
{
    Q_OBJECT
public:
    AndroidRunnerWorker(ProjectExplorer::RunControl *runControl, const AndroidRunnable &runnable);
    void asyncStart() override;
};


class AndroidRunnerWorkerPreNougat : public AndroidRunnerWorkerBase
{
    Q_OBJECT
public:
    AndroidRunnerWorkerPreNougat(ProjectExplorer::RunControl *runControl, const AndroidRunnable &runnable);
    void asyncStart() override;
};

} // namespace Internal
} // namespace Android
