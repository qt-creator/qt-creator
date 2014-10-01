/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYAPPLICATIONRUNNER_H
#define QNX_INTERNAL_BLACKBERRYAPPLICATIONRUNNER_H

#include "blackberrydeviceconfiguration.h"
#include "blackberryprocessparser.h"
#include "qnxversionnumber.h"

#include <projectexplorer/runconfiguration.h>

#include <ssh/sshconnection.h>
#include <utils/environment.h>

#include <QObject>
#include <QProcess>
#include <QDateTime>

namespace QSsh { class SshRemoteProcessRunner; }

namespace Qnx {
namespace Internal {

class BlackBerryRunConfiguration;
class BlackBerryLogProcessRunner;
class BlackBerryDeviceInformation;

class BlackBerryApplicationRunner : public QObject
{
    Q_OBJECT
public:
    enum LaunchFlag
    {
        CppDebugLaunch = 0x1,
        QmlDebugLaunch = 0x2,
        QmlDebugLaunchBlocking = 0x4,
        QmlProfilerLaunch = 0x8
    };
    Q_DECLARE_FLAGS(LaunchFlags, LaunchFlag)

public:
    explicit BlackBerryApplicationRunner(const LaunchFlags &launchFlags, BlackBerryRunConfiguration *runConfiguration, QObject *parent = 0);

    bool isRunning() const;
    qint64 pid() const;

public slots:
    void start();
    ProjectExplorer::RunControl::StopResult stop();

signals:
    void output(const QString &msg, Utils::OutputFormat format);
    void started();
    void finished();

    void startFailed(const QString &msg);

private slots:
    void startFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void stopFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void readStandardOutput();
    void readStandardError();

    void disconnectFromDeviceSignals(Core::Id deviceId);
    void startRunningStateTimer();
    void determineRunningState();
    void readRunningStateStandardOutput();

    void setPid(qint64 pid);
    void setApplicationId(const QString &applicationId);

    void launchApplication();
    void checkDeployMode();
    void startLogProcessRunner();

    void displayConnectionOutput(Core::Id deviceId, const QString &output);
    void checkDeviceRuntimeVersion(int status);

    void checkQmlJsDebugArguments();
    void checkQmlJsDebugArgumentsManifestLoaded();
    void checkQmlJsDebugArgumentsManifestSaved();

private:
    void reset();
    void queryDeviceInformation();

    LaunchFlags m_launchFlags;

    qint64 m_pid;
    QString m_appId;

    bool m_running;
    bool m_stopping;

    Utils::Environment m_environment;
    QString m_deployCmd;
    BlackBerryDeviceConfiguration::ConstPtr m_device;
    QString m_barPackage;
    QSsh::SshConnectionParameters m_sshParams;

    QProcess *m_launchProcess;
    QProcess *m_stopProcess;
    BlackBerryProcessParser m_launchStopProcessParser;
    BlackBerryDeviceInformation *m_deviceInfo;

    BlackBerryLogProcessRunner *m_logProcessRunner;

    QTimer *m_runningStateTimer;
    QProcess *m_runningStateProcess;

    QnxVersionNumber m_bbApiLevelVersion;

    int m_qmlDebugServerPort;
    QProcess *m_checkQmlJsDebugArgumentsProcess;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYAPPLICATIONRUNNER_H
