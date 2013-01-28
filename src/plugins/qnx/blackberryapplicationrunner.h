/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYAPPLICATIONRUNNER_H
#define QNX_INTERNAL_BLACKBERRYAPPLICATIONRUNNER_H

#include <projectexplorer/runconfiguration.h>

#include <ssh/sshconnection.h>
#include <utils/environment.h>

#include <QObject>
#include <QProcess>
#include <QDateTime>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Qnx {
namespace Internal {

class BlackBerryRunConfiguration;

class BlackBerryApplicationRunner : public QObject
{
    Q_OBJECT
public:
    explicit BlackBerryApplicationRunner(bool debugMode, BlackBerryRunConfiguration *runConfiguration, QObject *parent = 0);

    bool isRunning() const;
    qint64 pid() const;

    ProjectExplorer::RunControl::StopResult stop();

public slots:
    void start();
    void checkSlog2Info();

signals:
    void output(const QString &msg, Utils::OutputFormat format);
    void started();
    void finished();

    void startFailed(const QString &msg);

private slots:
    bool showQtMessage(const QString& pattern, const QString& line);
    void tailApplicationLog();
    void startFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void stopFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void readStandardOutput();
    void readStandardError();

    void handleTailOutput();
    void handleTailError();
    void handleTailConnectionError();

    void startRunningStateTimer();
    void determineRunningState();
    void readRunningStateStandardOutput();

    void handleSlog2InfoFound();
    void readLaunchTime();

private:
    void reset();
    void killTailProcess();

    bool m_debugMode;
    bool m_slog2infoFound;

    QDateTime m_launchDateTime;

    qint64 m_pid;
    QString m_appId;

    bool m_running;
    bool m_stopping;

    Utils::Environment m_environment;
    QString m_deployCmd;
    QString m_deviceHost;
    QString m_password;
    QString m_barPackage;
    QSsh::SshConnectionParameters m_sshParams;

    QProcess *m_launchProcess;
    QProcess *m_stopProcess;
    QSsh::SshRemoteProcessRunner *m_tailProcess;
    QSsh::SshRemoteProcessRunner *m_testSlog2Process;
    QSsh::SshRemoteProcessRunner *m_launchDateTimeProcess;
    QTimer *m_runningStateTimer;
    QProcess *m_runningStateProcess;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYAPPLICATIONRUNNER_H
