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

#include <QFutureInterface>
#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QThread>
#include <QProcess>
#include <QMutex>

namespace Android {
class AndroidRunConfiguration;

namespace Internal {

class AndroidRunnerWorker;
class AndroidRunner : public QObject
{
    Q_OBJECT

public:
    AndroidRunner(QObject *parent, AndroidRunConfiguration *runConfig,
                  Core::Id runMode);
    ~AndroidRunner();

    QString displayName() const;
    void setRunnable(const AndroidRunnable &runnable);
    const AndroidRunnable &runnable() const { return m_androidRunnable; }

    void start();
    void stop();

signals:
    void remoteServerRunning(const QByteArray &serverChannel, int pid);
    void remoteProcessStarted(Utils::Port gdbServerPort, Utils::Port qmlPort);
    void remoteProcessFinished(const QString &errString = QString());
    void remoteDebuggerRunning();

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

    void asyncStart(const QString &intentName, const QVector<QStringList> &adbCommands);
    void asyncStop(const QVector<QStringList> &adbCommands);

    void adbParametersChanged(const QString &packageName, const QStringList &selector);
    void avdDetected();

private:
    void checkAVD();
    void launchAVD();

    AndroidRunnable m_androidRunnable;
    AndroidRunConfiguration *m_runConfig;
    QString m_launchedAVDName;
    QThread m_thread;
    QTimer m_checkAVDTimer;
    QScopedPointer<AndroidRunnerWorker> m_worker;
};

} // namespace Internal
} // namespace Android
