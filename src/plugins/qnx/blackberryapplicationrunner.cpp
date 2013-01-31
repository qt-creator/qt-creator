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

#include "blackberryapplicationrunner.h"

#include "blackberrydeployconfiguration.h"
#include "blackberryrunconfiguration.h"

#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

#include <QTimer>
#include <QDir>

namespace {
const char DEPLOY_CMD[] = "blackberry-deploy";

qint64 parsePid(const QString &line)
{
    QTC_ASSERT(line.startsWith(QLatin1String("result::")), return -1);

    int pidIndex = -1;
    if (line.contains(QLatin1String("running"))) // "result::running,<pid>"
        pidIndex = 16;
    else // "result::<pid>"
        pidIndex = 8;

    bool ok;
    const qint64 pid = line.mid(pidIndex).toInt(&ok);
    if (!ok)
        return -1;
    return pid;
}

QString parseAppId(const QString &line)
{
    QTC_ASSERT(line.startsWith(QLatin1String("Info: Launching")), return QString());

    const int endOfId = line.indexOf(QLatin1String("..."));
    return line.mid(16, endOfId - 16);
}

bool parseRunningState(const QString &line)
{
    QTC_ASSERT(line.startsWith(QLatin1String("result::")), return false);
    return line.trimmed().mid(8) == QLatin1String("true");
}
}

using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryApplicationRunner::BlackBerryApplicationRunner(bool debugMode, BlackBerryRunConfiguration *runConfiguration, QObject *parent)
    : QObject(parent)
    , m_debugMode(debugMode)
    , m_slog2infoFound(false)
    , m_pid(-1)
    , m_appId(QString())
    , m_running(false)
    , m_stopping(false)
    , m_launchProcess(0)
    , m_stopProcess(0)
    , m_tailProcess(0)
    , m_testSlog2Process(0)
    , m_launchDateTimeProcess(0)
    , m_runningStateTimer(new QTimer(this))
    , m_runningStateProcess(0)
{
    QTC_ASSERT(runConfiguration, return);

    Target *target = runConfiguration->target();
    BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    m_environment = buildConfig->environment();
    m_deployCmd = m_environment.searchInPath(QLatin1String(DEPLOY_CMD));

    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(target->kit());
    m_deviceHost = device->sshParameters().host;
    m_password = device->sshParameters().password;
    m_barPackage = runConfiguration->barPackage();

    m_sshParams = device->sshParameters();
    // The BlackBerry device always uses key authentication
    m_sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationByKey;

    m_runningStateTimer->setInterval(3000);
    m_runningStateTimer->setSingleShot(true);
    connect(m_runningStateTimer, SIGNAL(timeout()), this, SLOT(determineRunningState()));
    connect(this, SIGNAL(started()), this, SLOT(checkSlog2Info()));
}

void BlackBerryApplicationRunner::start()
{
    QStringList args;
    args << QLatin1String("-launchApp");
    if (m_debugMode)
        args << QLatin1String("-debugNative");
    args << QLatin1String("-device") << m_deviceHost;
    if (!m_password.isEmpty())
        args << QLatin1String("-password") << m_password;
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

void BlackBerryApplicationRunner::checkSlog2Info()
{
    if (m_slog2infoFound) {
        readLaunchTime();
    } else if (!m_testSlog2Process) {
        m_testSlog2Process = new QSsh::SshRemoteProcessRunner(this);
        connect(m_testSlog2Process, SIGNAL(processClosed(int)),
                this, SLOT(handleSlog2InfoFound()));
        m_testSlog2Process->run("slog2info", m_sshParams);
    }
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

    if (m_testSlog2Process && m_testSlog2Process->isProcessRunning()) {
        m_testSlog2Process->cancel();
        delete m_testSlog2Process;
        m_testSlog2Process = 0;
    }

    QStringList args;
    args << QLatin1String("-terminateApp");
    args << QLatin1String("-device") << m_deviceHost;
    if (!m_password.isEmpty())
        args << QLatin1String("-password") << m_password;
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
        emit output(line, Utils::StdOutFormat);

        if (line.startsWith(QLatin1String("result::")))
            m_pid = parsePid(line);
        else if (line.startsWith(QLatin1String("Info: Launching")))
            m_appId = parseAppId(line);
    }
}

void BlackBerryApplicationRunner::readStandardError()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    process->setReadChannel(QProcess::StandardError);
    while (process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(process->readLine());
        emit output(line, Utils::StdErrFormat);
    }
}

void BlackBerryApplicationRunner::killTailProcess()
{
    QSsh::SshRemoteProcessRunner *slayProcess = new QSsh::SshRemoteProcessRunner(this);
    connect(slayProcess, SIGNAL(processClosed(int)), this, SIGNAL(finished()));

    if (m_slog2infoFound)
        slayProcess->run("slay slog2info", m_sshParams);
    else
        slayProcess->run("slay tail", m_sshParams);

    // Not supported by OpenSSH server
    //m_tailProcess->sendSignalToProcess(Utils::SshRemoteProcess::KillSignal);
    m_tailProcess->cancel();

    delete m_tailProcess;
    m_tailProcess = 0;
}

void BlackBerryApplicationRunner::tailApplicationLog()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    m_launchDateTime = QDateTime::fromString(QString::fromLatin1(process->readAllStandardOutput()).trimmed(),
                                             QString::fromLatin1("dd HH:mm:ss"));

    if (m_stopping || (m_tailProcess && m_tailProcess->isProcessRunning()))
        return;

    QTC_CHECK(!m_appId.isEmpty());

    if (!m_tailProcess) {
        m_tailProcess = new QSsh::SshRemoteProcessRunner(this);

        connect(m_tailProcess, SIGNAL(readyReadStandardOutput()),
                this, SLOT(handleTailOutput()));
        connect(m_tailProcess, SIGNAL(readyReadStandardError()),
                this, SLOT(handleTailError()));
        connect(m_tailProcess, SIGNAL(connectionError()),
                this, SLOT(handleTailConnectionError()));
    }

    QString command;
    if (m_slog2infoFound) {
        command = QString::fromLatin1("slog2info -w -b ") + m_appId;
    } else {
        command = QLatin1String("tail -c +1 -f /accounts/1000/appdata/") + m_appId
                + QLatin1String("/logs/log");
    }
    m_tailProcess->run(command.toLatin1(), m_sshParams);
}

void BlackBerryApplicationRunner::handleSlog2InfoFound()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    m_slog2infoFound = (process->processExitCode() == 0);

    tailApplicationLog();
}

void BlackBerryApplicationRunner::readLaunchTime()
{
    m_launchDateTimeProcess = new QSsh::SshRemoteProcessRunner(this);
    connect(m_launchDateTimeProcess, SIGNAL(processClosed(int)),
            this, SLOT(tailApplicationLog()));

    m_launchDateTimeProcess->run("date +\"%d %H:%M:%S\"", m_sshParams);
}

void BlackBerryApplicationRunner::handleTailOutput()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    const QString message = QString::fromLatin1(process->readAllStandardOutput());
    if (m_slog2infoFound) {
        const QStringList multiLine = message.split(QLatin1Char('\n'));
        Q_FOREACH (const QString &line, multiLine) {
            QDateTime dateTime = QDateTime::fromString(line.split(m_appId).first().mid(4).trimmed(),
                                                       QString::fromLatin1("dd HH:mm:ss.zzz"));
            if (dateTime >= m_launchDateTime) {
                QStringList validLineBeginnings;
                validLineBeginnings << QLatin1String("qt-msg      0  ")
                                    << QLatin1String("qt-msg*     0  ")
                                    << QLatin1String("default*  9000  ")
                                    << QLatin1String("default   9000  ")
                                    << QLatin1String("                           0  ");
                Q_FOREACH (const QString &beginning, validLineBeginnings) {
                    if (showQtMessage(beginning, line))
                        break;
                }
            }
        }
        return;
    }
    emit output(message, Utils::StdOutFormat);
}

bool BlackBerryApplicationRunner::showQtMessage(const QString& pattern, const QString& line)
{
    const int index = line.indexOf(pattern);
    if (index != -1) {
        const QString str = line.right(line.length()-index-pattern.length()) + QLatin1Char('\n');
        emit output(str, Utils::StdOutFormat);
        return true;
    }
    return false;
}

void BlackBerryApplicationRunner::handleTailError()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    const QString message = QString::fromLatin1(process->readAllStandardError());
    emit output(message, Utils::StdErrFormat);
}

void BlackBerryApplicationRunner::handleTailConnectionError()
{
    emit output(tr("Cannot show debug output. Error: %1").arg(m_tailProcess->lastConnectionErrorString()),
                Utils::StdErrFormat);
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
    args << QLatin1String("-device") << m_deviceHost;
    if (!m_password.isEmpty())
        args << QLatin1String("-password") << m_password;
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

    if (m_tailProcess && m_tailProcess->isProcessRunning())
        killTailProcess();
    else
        emit finished();
}
