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

#include "blackberryapplicationrunner.h"

#include "blackberrydeployconfiguration.h"
#include "blackberrydeviceconnectionmanager.h"
#include "blackberryrunconfiguration.h"
#include "blackberrylogprocessrunner.h"
#include "blackberrydeviceinformation.h"
#include "qnxconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QTimer>
#include <QDir>

namespace {
bool parseRunningState(const QString &line)
{
    QTC_ASSERT(line.startsWith(QLatin1String("result::")), return false);
    return line.trimmed().mid(8) == QLatin1String("true");
}
}

using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryApplicationRunner::BlackBerryApplicationRunner(bool cppDebugMode, BlackBerryRunConfiguration *runConfiguration, QObject *parent)
    : QObject(parent)
    , m_cppDebugMode(cppDebugMode)
    , m_pid(-1)
    , m_appId(QString())
    , m_running(false)
    , m_stopping(false)
    , m_launchProcess(0)
    , m_stopProcess(0)
    , m_deviceInfo(0)
    , m_logProcessRunner(0)
    , m_runningStateTimer(new QTimer(this))
    , m_runningStateProcess(0)
{
    QTC_ASSERT(runConfiguration, return);

    Target *target = runConfiguration->target();
    BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    m_environment = buildConfig->environment();
    m_deployCmd = m_environment.searchInPath(QLatin1String(Constants::QNX_BLACKBERRY_DEPLOY_CMD));

    QFileInfo fi(target->kit()->autoDetectionSource());
    m_bbApiLevelVersion = BlackBerryVersionNumber::fromNdkEnvFileName(fi.baseName());

    m_device = BlackBerryDeviceConfiguration::device(target->kit());
    m_barPackage = runConfiguration->barPackage();

    // The BlackBerry device always uses key authentication
    m_sshParams = m_device->sshParameters();
    m_sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;

    m_runningStateTimer->setInterval(3000);
    m_runningStateTimer->setSingleShot(true);
    connect(m_runningStateTimer, SIGNAL(timeout()), this, SLOT(determineRunningState()));
    connect(this, SIGNAL(started()), this, SLOT(startLogProcessRunner()));

    connect(&m_launchStopProcessParser, SIGNAL(pidParsed(qint64)), this, SLOT(setPid(qint64)));
    connect(&m_launchStopProcessParser, SIGNAL(applicationIdParsed(QString)), this, SLOT(setApplicationId(QString)));
}

void BlackBerryApplicationRunner::start()
{
    if (!BlackBerryDeviceConnectionManager::instance()->isConnected(m_device->id())) {
        connect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(deviceConnected()),
                this, SLOT(checkDeployMode()));
        connect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(deviceDisconnected(Core::Id)),
                this, SLOT(disconnectFromDeviceSignals(Core::Id)));
        connect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(connectionOutput(Core::Id,QString)),
                this, SLOT(displayConnectionOutput(Core::Id,QString)));
        BlackBerryDeviceConnectionManager::instance()->connectDevice(m_device->id());
    } else {
        checkDeployMode();
    }
}

void BlackBerryApplicationRunner::startLogProcessRunner()
{
    if (!m_logProcessRunner) {
        m_logProcessRunner = new BlackBerryLogProcessRunner(this, m_appId, m_device);
        connect(m_logProcessRunner, SIGNAL(output(QString,Utils::OutputFormat)),
            this, SIGNAL(output(QString,Utils::OutputFormat)));
        connect(m_logProcessRunner, SIGNAL(finished()), this, SIGNAL(finished()));
    }

    m_logProcessRunner->start();
}

void BlackBerryApplicationRunner::displayConnectionOutput(Core::Id deviceId, const QString &msg)
{
    if (deviceId != m_device->id())
        return;

    if (msg.contains(QLatin1String("Info:")))
        emit output(msg, Utils::StdOutFormat);
    else if (msg.contains(QLatin1String("Error:")))
        emit output(msg, Utils::StdErrFormat);
}

void BlackBerryApplicationRunner::checkDeviceRuntimeVersion(int status)
{
    if (status != BlackBerryNdkProcess::Success) {
        emit output(tr("Cannot determine device runtime version."), Utils::StdErrFormat);
        return;
    }

    if (m_bbApiLevelVersion.isEmpty()) {
        emit output(tr("Cannot determine API level version."), Utils::StdErrFormat);
        launchApplication();
        return;
    }

    const QString runtimeVersion = m_deviceInfo->scmBundle();
    if (m_bbApiLevelVersion.toString() != runtimeVersion) {
        const QMessageBox::StandardButton answer =
                QMessageBox::question(Core::ICore::mainWindow(),
                                      tr("Confirmation"),
                                      tr("The device runtime version(%1) does not match "
                                         "the API level version(%2).\n"
                                         "This may cause unexpected behavior when debugging.\n"
                                         "Do you want to continue anyway?")
                                      .arg(runtimeVersion, m_bbApiLevelVersion.toString()),
                                      QMessageBox::Yes | QMessageBox::No);

        if (answer == QMessageBox::No) {
            emit startFailed(tr("API level version does not match Runtime version."));
            return;
        }
    }

    launchApplication();
}

void BlackBerryApplicationRunner::queryDeviceInformation()
{
    if (!m_deviceInfo) {
        m_deviceInfo = new BlackBerryDeviceInformation(this);
        connect(m_deviceInfo, SIGNAL(finished(int)),
                this, SLOT(checkDeviceRuntimeVersion(int)));
    }

    m_deviceInfo->setDeviceTarget(m_sshParams.host, m_sshParams.password);
    emit output(tr("Querying device runtime version..."), Utils::StdOutFormat);
}

void BlackBerryApplicationRunner::startFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit && m_pid > -1) {
        emit started();
    } else {
        m_running = false;
        m_runningStateTimer->stop();

        QTC_ASSERT(m_launchProcess, return);
        const QString errorString = (m_launchProcess->error() != QProcess::UnknownError)
                ? m_launchProcess->errorString() : tr("Launching application failed");
        emit startFailed(errorString);
        reset();
    }
}

ProjectExplorer::RunControl::StopResult BlackBerryApplicationRunner::stop()
{
    if (m_stopping)
        return ProjectExplorer::RunControl::AsynchronousStop;

    m_stopping = true;

    QStringList args;
    args << QLatin1String("-terminateApp");
    args << QLatin1String("-device") << m_sshParams.host;
    if (!m_sshParams.password.isEmpty())
        args << QLatin1String("-password") << m_sshParams.password;
    args << m_barPackage;

    if (!m_stopProcess) {
        m_stopProcess = new QProcess(this);
        connect(m_stopProcess, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
        connect(m_stopProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
        connect(m_stopProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(stopFinished(int,QProcess::ExitStatus)));

        m_stopProcess->setEnvironment(m_environment.toStringList());
    }

    m_stopProcess->start(m_deployCmd, args);
    return ProjectExplorer::RunControl::AsynchronousStop;
}

bool BlackBerryApplicationRunner::isRunning() const
{
    return m_running;
}

qint64 BlackBerryApplicationRunner::pid() const
{
    return m_pid;
}

void BlackBerryApplicationRunner::stopFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    reset();
}

void BlackBerryApplicationRunner::readStandardOutput()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    process->setReadChannel(QProcess::StandardOutput);
    while (process->canReadLine()) {
        QString line = QString::fromLocal8Bit(process->readLine());
        m_launchStopProcessParser.stdOutput(line);
        emit output(line, Utils::StdOutFormat);
    }
}

void BlackBerryApplicationRunner::readStandardError()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    process->setReadChannel(QProcess::StandardError);
    while (process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(process->readLine());
        m_launchStopProcessParser.stdError(line);
        emit output(line, Utils::StdErrFormat);
    }
}

void BlackBerryApplicationRunner::disconnectFromDeviceSignals(Core::Id deviceId)
{
    if (m_device->id() == deviceId) {
        disconnect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(deviceConnected()),
                   this, SLOT(launchApplication()));
        disconnect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(deviceDisconnected(Core::Id)),
                   this, SLOT(disconnectFromDeviceSignals(Core::Id)));
        disconnect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(connectionOutput(Core::Id,QString)),
                   this, SLOT(displayConnectionOutput(Core::Id,QString)));
    }
}

void BlackBerryApplicationRunner::setPid(qint64 pid)
{
    m_pid = pid;
}

void BlackBerryApplicationRunner::setApplicationId(const QString &applicationId)
{
    m_appId = applicationId;
}

void BlackBerryApplicationRunner::launchApplication()
{
    // If original device connection fails before launching, this method maybe triggered
    // if any other device is connected(?)
    if (!BlackBerryDeviceConnectionManager::instance()->isConnected(m_device->id()))
        return;

    QStringList args;
    args << QLatin1String("-launchApp");
    if (m_cppDebugMode)
        args << QLatin1String("-debugNative");
    args << QLatin1String("-device") << m_sshParams.host;
    if (!m_sshParams.password.isEmpty())
        args << QLatin1String("-password") << m_sshParams.password;
    args << QDir::toNativeSeparators(m_barPackage);

    if (!m_launchProcess) {
        m_launchProcess = new QProcess(this);
        connect(m_launchProcess, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
        connect(m_launchProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
        connect(m_launchProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(startFinished(int,QProcess::ExitStatus)));

        m_launchProcess->setEnvironment(m_environment.toStringList());
    }

    m_launchProcess->start(m_deployCmd, args);
    m_runningStateTimer->start();
    m_running = true;
}

void BlackBerryApplicationRunner::checkDeployMode()
{
    // If original device connection fails before launching, this method maybe triggered
    // if any other device is connected
    if (!BlackBerryDeviceConnectionManager::instance()->isConnected(m_device->id()))
        return;

    if (m_cppDebugMode)
        queryDeviceInformation(); // check API version vs Runtime version
    else
        launchApplication();
}

void BlackBerryApplicationRunner::startRunningStateTimer()
{
    if (m_running)
        m_runningStateTimer->start();
}

void BlackBerryApplicationRunner::determineRunningState()
{
    QStringList args;
    args << QLatin1String("-isAppRunning");
    args << QLatin1String("-device") << m_sshParams.host;
    if (!m_sshParams.password.isEmpty())
        args << QLatin1String("-password") << m_sshParams.password;
    args << m_barPackage;

    if (!m_runningStateProcess) {
        m_runningStateProcess = new QProcess(this);

        connect(m_runningStateProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(readRunningStateStandardOutput()));
        connect(m_runningStateProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(startRunningStateTimer()));
    }

    m_runningStateProcess->setEnvironment(m_environment.toStringList());

    m_runningStateProcess->start(m_deployCmd, args);
}

void BlackBerryApplicationRunner::readRunningStateStandardOutput()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    process->setReadChannel(QProcess::StandardOutput);
    while (process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(process->readLine());
        if (line.startsWith(QLatin1String("result"))) {
            m_running = parseRunningState(line);
            break;
        }
    }

    if (!m_running)
        reset();
}

void BlackBerryApplicationRunner::reset()
{
    m_pid = -1;
    m_appId.clear();
    m_running = false;
    m_stopping = false;

    m_runningStateTimer->stop();
    if (m_runningStateProcess) {
        m_runningStateProcess->terminate();
        if (!m_runningStateProcess->waitForFinished(1000))
            m_runningStateProcess->kill();
    }

    if (m_logProcessRunner) {
        m_logProcessRunner->stop();

        delete m_logProcessRunner;
        m_logProcessRunner = 0;
    } else {
        emit finished();
    }
}
