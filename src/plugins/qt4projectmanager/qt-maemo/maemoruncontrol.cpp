/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemoruncontrol.h"

#include "maemorunconfiguration.h"

#ifndef USE_SSH_LIB
#include "qt4buildconfiguration.h"
#include "qt4project.h"
#endif // USE_SSH_LIB

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <debugger/debuggermanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFuture>
#include <QtCore/QStringBuilder>

namespace Qt4ProjectManager {
namespace Internal {

using ProjectExplorer::Environment;
using ProjectExplorer::RunConfiguration;
using ProjectExplorer::ToolChain;

AbstractMaemoRunControl::AbstractMaemoRunControl(RunConfiguration *rc)
    : RunControl(rc)
    , runConfig(qobject_cast<MaemoRunConfiguration *>(rc))
    , devConfig(runConfig ? runConfig->deviceConfig() : MaemoDeviceConfig())
{
#ifndef USE_SSH_LIB
    setProcessEnvironment(deployProcess);
    connect(&deployProcess, SIGNAL(readyReadStandardError()), this,
        SLOT(readStandardError()));
    connect(&deployProcess, SIGNAL(readyReadStandardOutput()), this,
        SLOT(readStandardOutput()));
    connect(&deployProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(deployProcessFinished()));
#endif // USE_SSH_LIB
}

void AbstractMaemoRunControl::startDeployment(bool forDebugging)
{
    QTC_ASSERT(runConfig, return);

#ifndef USE_SSH_LIB
    Core::ICore::instance()->progressManager()->addTask(m_progress.future(),
        tr("Deploying"), QLatin1String("Maemo.Deploy"));
#endif // USE_SSH_LIB
    if (devConfig.isValid()) {
        deployables.clear();
        if (runConfig->currentlyNeedsDeployment()) {
            deployables.append(Deployable(executableFileName(),
                QFileInfo(executableOnHost()).canonicalPath(),
                &MaemoRunConfiguration::wasDeployed));
        }
        if (forDebugging && runConfig->debuggingHelpersNeedDeployment()) {
            const QFileInfo &info(runConfig->dumperLib());
            deployables.append(Deployable(info.fileName(), info.canonicalPath(),
                &MaemoRunConfiguration::debuggingHelpersDeployed));
        }

#ifndef USE_SSH_LIB
        m_progress.setProgressRange(0, deployables.count());
        m_progress.setProgressValue(0);
        m_progress.reportStarted();
#endif // USE_SSH_LIB
        deploy();
    } else {
#ifdef USE_SSH_LIB
        handleDeploymentFinished(false);
#else
        deploymentFinished(false);
#endif // USE_SSH_LIB
    }
}

void AbstractMaemoRunControl::deploy()
{
#ifdef USE_SSH_LIB
    if (!deployables.isEmpty()) {
        QStringList files;
        QStringList targetDirs;
        foreach (const Deployable &deployable, deployables) {
            files << deployable.dir + QDir::separator() + deployable.fileName;
            targetDirs << remoteDir();
        }
        emit addToOutputWindow(this, tr("Files to deploy: %1.").arg(files.join(" ")));
        sshDeployer.reset(new MaemoSshDeployer(devConfig, files, targetDirs));
        connect(sshDeployer.data(), SIGNAL(finished()),
                this, SLOT(deployProcessFinished()));
        connect(sshDeployer.data(), SIGNAL(fileCopied(QString)),
                this, SLOT(handleFileCopied()));
        Core::ICore::instance()->progressManager()
                ->addTask(m_progress.future(), tr("Deploying"),
                          QLatin1String("Maemo.Deploy"));
        m_progress.setProgressRange(0, deployables.count());
        m_progress.setProgressValue(0);
        m_progress.reportStarted();
        emit started();
        sshDeployer->start();
    } else {
        handleDeploymentFinished(true);
    }
#else
    if (!deployables.isEmpty()) {
        const Deployable &deployable = deployables.first();
        emit addToOutputWindow(this, tr("File to deploy: %1.").arg(deployable.fileName));

        QStringList cmdArgs;
        cmdArgs << "-P" << port() << options() << deployable.fileName
            << (devConfig.uname + "@" + devConfig.host + ":" + remoteDir());
        deployProcess.setWorkingDirectory(deployable.dir);

        deployProcess.start(runConfig->scpCmd(), cmdArgs);
        if (!deployProcess.waitForStarted()) {
            emit error(this, tr("Could not start scp. Deployment failed."));
            deployProcess.kill();
        } else {
            emit started();
        }
    } else {
        deploymentFinished(true);
    }
#endif // USE_SSH_LIB
}

#ifdef USE_SSH_LIB
void AbstractMaemoRunControl::handleFileCopied()
{
    Deployable deployable = deployables.takeFirst();
    (runConfig->*deployable.updateTimestamp)();
    m_progress.setProgressValue(m_progress.progressValue() + 1);
}
#endif // USE_SSH_LIB

void AbstractMaemoRunControl::stopDeployment()
{
#ifdef USE_SSH_LIB
    sshDeployer->stop();
#else
    deployProcess.kill();
#endif // USE_SSH_LIB
}

bool AbstractMaemoRunControl::isDeploying() const
{
#ifdef USE_SSH_LIB
    return !sshDeployer.isNull() && sshDeployer->isRunning();
#else
    return deployProcess.state() != QProcess::NotRunning;
#endif // USE_SSH_LIB
}

void AbstractMaemoRunControl::deployProcessFinished()
{
#if USE_SSH_LIB
    const bool success = !sshDeployer->hasError();
    if (success) {
        emit addToOutputWindow(this, tr("Deployment finished."));
    } else {
        emit error(this, tr("Deployment failed: ") % sshDeployer->error());
        m_progress.reportCanceled();
    }
    m_progress.reportFinished();
    handleDeploymentFinished(success);
#else
    bool success;
    if (deployProcess.exitCode() == 0) {
        emit addToOutputWindow(this, tr("Target deployed."));
        success = true;
        Deployable deployable = deployables.takeFirst();
        (runConfig->*deployable.updateTimestamp)();
        m_progress.setProgressValue(m_progress.progressValue() + 1);
    } else {
        emit error(this, tr("Deployment failed."));
        success = false;
    }
    if (deployables.isEmpty() || !success)
        deploymentFinished(success);
    else
        deploy();
#endif // USE_SSH_LIB
}

#ifndef USE_SSH_LIB
void AbstractMaemoRunControl::deploymentFinished(bool success)
{
    m_progress.reportFinished();
    handleDeploymentFinished(success);
}
#endif // USE_SSH_LIB

const QString AbstractMaemoRunControl::executableOnHost() const
{
    return runConfig->executable();
}

const QString AbstractMaemoRunControl::port() const
{
    return QString::number(devConfig.port);
}

const QString AbstractMaemoRunControl::executableFileName() const
{
    return QFileInfo(executableOnHost()).fileName();
}

const QString AbstractMaemoRunControl::remoteDir() const
{
    return devConfig.uname == QString::fromLocal8Bit("root")
        ? QString::fromLocal8Bit("/root")
        : QString::fromLocal8Bit("/home/") + devConfig.uname;
}

const QStringList AbstractMaemoRunControl::options() const
{
    const bool usePassword
        = devConfig.authentication == MaemoDeviceConfig::Password;
    const QLatin1String opt("-o");
    QStringList optionList;
    if (!usePassword)
        optionList << QLatin1String("-i") << devConfig.keyFile;
    return optionList << opt
        << QString::fromLatin1("PasswordAuthentication=%1").
            arg(usePassword ? "yes" : "no") << opt
        << QString::fromLatin1("PubkeyAuthentication=%1").
            arg(usePassword ? "no" : "yes") << opt
        << QString::fromLatin1("ConnectTimeout=%1").arg(devConfig.timeout)
        << opt << QLatin1String("CheckHostIP=no")
        << opt << QLatin1String("StrictHostKeyChecking=no");
}

const QString AbstractMaemoRunControl::executableOnTarget() const
{
    return QString::fromLocal8Bit("%1/%2").arg(remoteDir()).
        arg(executableFileName());
}

const QString AbstractMaemoRunControl::targetCmdLinePrefix() const
{
    return QString::fromLocal8Bit("chmod u+x %1; source /etc/profile; ").
        arg(executableOnTarget());
}

#ifndef USE_SSH_LIB
bool AbstractMaemoRunControl::setProcessEnvironment(QProcess &process)
{
    QTC_ASSERT(runConfig, return false);
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration*>
        (runConfig->project()->activeBuildConfiguration());
    QTC_ASSERT(qt4bc, return false);
    Environment env = Environment::systemEnvironment();
    qt4bc->toolChain()->addToEnvironment(env);
    process.setEnvironment(env.toStringList());

    return true;
}

void AbstractMaemoRunControl::readStandardError()
{
    QProcess *process = static_cast<QProcess *>(sender());
    const QByteArray &data = process->readAllStandardError();
    emit addToOutputWindow(this, QString::fromLocal8Bit(data.constData(),
        data.length()));
}

void AbstractMaemoRunControl::readStandardOutput()
{
    QProcess *process = static_cast<QProcess *>(sender());
    const QByteArray &data = process->readAllStandardOutput();
    emit addToOutputWindow(this, QString::fromLocal8Bit(data.constData(),
        data.length()));
}
#endif // USE_SSH_LIB

// #pragma mark -- MaemoRunControl


MaemoRunControl::MaemoRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
{
#ifndef USE_SSH_LIB
    setProcessEnvironment(sshProcess);
    setProcessEnvironment(stopProcess);

    connect(&sshProcess, SIGNAL(readyReadStandardError()), this,
        SLOT(readStandardError()));
    connect(&sshProcess, SIGNAL(readyReadStandardOutput()), this,
        SLOT(readStandardOutput()));
    connect(&sshProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(executionFinished()));
#endif // USE_SSH_LIB
}

MaemoRunControl::~MaemoRunControl()
{
    stop();
#ifndef USE_SSH_LIB
    stopProcess.waitForFinished(5000);
#endif
}

void MaemoRunControl::start()
{
    stoppedByUser = false;
    startDeployment(false);
}

void MaemoRunControl::handleDeploymentFinished(bool success)
{
    if (success)
        startExecution();
    else
        emit finished();
}

void MaemoRunControl::startExecution()
{
#ifdef USE_SSH_LIB
    const QString remoteCall = QString::fromLocal8Bit("%1 %2 %3")
        .arg(targetCmdLinePrefix()).arg(executableOnTarget())
        .arg(runConfig->arguments().join(" "));
    sshRunner.reset(new MaemoSshRunner(devConfig, remoteCall));
    connect(sshRunner.data(), SIGNAL(remoteOutput(QString)),
            this, SLOT(handleRemoteOutput(QString)));
    connect(sshRunner.data(), SIGNAL(finished()),
            this, SLOT(executionFinished()));
    emit addToOutputWindow(this, tr("Starting remote application."));
    emit started();
    sshRunner->start();
#else
    const QString remoteCall = QString::fromLocal8Bit("%1 %2 %3")
        .arg(targetCmdLinePrefix()).arg(executableOnTarget())
        .arg(runConfig->arguments().join(" "));

    QStringList cmdArgs;
    cmdArgs << "-n" << "-p" << port() << "-l" << devConfig.uname
        << options() << devConfig.host << remoteCall;
    sshProcess.start(runConfig->sshCmd(), cmdArgs);

    sshProcess.start(runConfig->sshCmd(), cmdArgs);
    emit addToOutputWindow(this, tr("Starting remote application."));
    if (sshProcess.waitForStarted()) {
        emit started();
    } else {
        emit error(this, tr("Could not start ssh!"));
        sshProcess.kill();
    }
#endif // USE_SSH_LIB
}

void MaemoRunControl::executionFinished()
{
    if (stoppedByUser)
        emit addToOutputWindow(this, tr("Remote process stopped by user."));
#ifdef USE_SSH_LIB
    else if (sshRunner->hasError())
        emit addToOutputWindow(this, tr("Remote process exited with error: ")
                               % sshRunner->error());
#else
    else if (sshProcess.exitCode() != 0)
        emit addToOutputWindow(this, tr("Remote process exited with error."));
#endif // USE_SSH_LIB
    else
        emit addToOutputWindow(this, tr("Remote process finished successfully."));
    emit finished();
}

void MaemoRunControl::stop()
{
    if (!isRunning())
        return;

    stoppedByUser = true;
    if (isDeploying()) {
        stopDeployment();
    } else {
#ifdef USE_SSH_LIB
        sshRunner->stop();
        const QString remoteCall = QString::fromLocal8Bit("pkill -x %1; "
            "sleep 1; pkill -x -9 %1").arg(executableFileName());
        sshStopper.reset(new MaemoSshRunner(devConfig, remoteCall));
        sshStopper->start();
#else
        stopProcess.kill();
        QStringList cmdArgs;
        const QString remoteCall = QString::fromLocal8Bit("pkill -x %1; "
            "sleep 1; pkill -x -9 %1").arg(executableFileName());
        cmdArgs << "-n" << "-p" << port() << "-l" << devConfig.uname
            << options() << devConfig.host << remoteCall;
        stopProcess.start(runConfig->sshCmd(), cmdArgs);
#endif // USE_SSH_LIB
    }
}

bool MaemoRunControl::isRunning() const
{
    return isDeploying()
#ifdef USE_SSH_LIB
        || (!sshRunner.isNull() && sshRunner->isRunning());
#else
        || sshProcess.state() != QProcess::NotRunning;
#endif // USE_SSH_LIB
}

#ifdef USE_SSH_LIB
void MaemoRunControl::handleRemoteOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}

#endif // USE_SSH_LIB

// #pragma mark -- MaemoDebugRunControl


MaemoDebugRunControl::MaemoDebugRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
    , gdbServerPort("10000"), debuggerManager(0)
    , startParams(new Debugger::DebuggerStartParameters)
{
#ifndef USE_SSH_LIB
    setProcessEnvironment(gdbServer);
    setProcessEnvironment(stopProcess);
#endif // USE_SSH_LIB

    debuggerManager = ExtensionSystem::PluginManager::instance()
        ->getObject<Debugger::DebuggerManager>();

    QTC_ASSERT(debuggerManager != 0, return);
    startParams->startMode = Debugger::StartRemote;
    startParams->executable = executableOnHost();
    startParams->remoteChannel = devConfig.host + ":" + gdbServerPort;
    startParams->remoteArchitecture = "arm";
    startParams->sysRoot = runConfig->sysRoot();
    startParams->toolChainType = ToolChain::GCC_MAEMO;
    startParams->debuggerCommand = runConfig->gdbCmd();
    startParams->dumperLibrary = runConfig->dumperLib();
    startParams->remoteDumperLib = QString::fromLocal8Bit("%1/%2")
        .arg(remoteDir()).arg(QFileInfo(runConfig->dumperLib()).fileName());

    connect(this, SIGNAL(stopRequested()), debuggerManager, SLOT(exitDebugger()));
    connect(debuggerManager, SIGNAL(debuggingFinished()), this,
        SLOT(debuggingFinished()), Qt::QueuedConnection);
    connect(debuggerManager, SIGNAL(applicationOutputAvailable(QString)),
        this, SLOT(debuggerOutput(QString)), Qt::QueuedConnection);
}

MaemoDebugRunControl::~MaemoDebugRunControl()
{
    disconnect(SIGNAL(addToOutputWindow(RunControl*,QString)));
    disconnect(SIGNAL(addToOutputWindowInline(RunControl*,QString)));
    stop();
    debuggingFinished();
}

void MaemoDebugRunControl::start()
{
    startDeployment(true);
}

void MaemoDebugRunControl::handleDeploymentFinished(bool success)
{
    if (success) {
        startGdbServer();
    } else {
        emit finished();
    }
}

void MaemoDebugRunControl::startGdbServer()
{
    const QString remoteCall(QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4").
        arg(targetCmdLinePrefix()).arg(gdbServerPort). arg(executableOnTarget())
        .arg(runConfig->arguments().join(" ")));
    inferiorPid = -1;
#ifdef USE_SSH_LIB
    sshRunner.reset(new MaemoSshRunner(devConfig, remoteCall));
    connect(sshRunner.data(), SIGNAL(remoteOutput(QString)),
            this, SLOT(gdbServerStarted(QString)));
    sshRunner->start();
#else
    QStringList sshArgs;
    sshArgs << "-t" << "-n" << "-l" << devConfig.uname << "-p"
        << port() << options() << devConfig.host << remoteCall;
    disconnect(&gdbServer, SIGNAL(readyReadStandardError()), 0, 0);
    connect(&gdbServer, SIGNAL(readyReadStandardError()), this,
        SLOT(gdbServerStarted()));
    gdbServer.start(runConfig->sshCmd(), sshArgs);
#endif // USE_SSH_LIB
}

void MaemoDebugRunControl::gdbServerStartFailed(const QString &reason)
{
    emit addToOutputWindow(this, tr("Debugging failed: %1").arg(reason));
    emit stopRequested();
    emit finished();
}

#ifdef USE_SSH_LIB
void MaemoDebugRunControl::gdbServerStarted(const QString &output)
{
    qDebug("gdbserver's stderr output: %s", output.toLatin1().data());
    if (inferiorPid != -1)
        return;
    const QString searchString("pid = ");
    const int searchStringLength = searchString.length();
    int pidStartPos = output.indexOf(searchString);
    const int pidEndPos = output.indexOf("\n", pidStartPos + searchStringLength);
    if (pidStartPos == -1 || pidEndPos == -1) {
        gdbServerStartFailed(output);
        return;
    }
    pidStartPos += searchStringLength;
    QString pidString = output.mid(pidStartPos, pidEndPos - pidStartPos);
    qDebug("pidString = %s", pidString.toLatin1().data());
    bool ok;
    const int pid = pidString.toInt(&ok);
    if (!ok) {
        gdbServerStartFailed(tr("Debugging failed, could not parse gdb "
            "server pid!"));
        return;
    }
    inferiorPid = pid;
    qDebug("inferiorPid = %d", inferiorPid);

    disconnect(sshRunner.data(), SIGNAL(remoteOutput(QString)), 0, 0);
    startDebugging();
}
#else
void MaemoDebugRunControl::gdbServerStarted()
{
    const QByteArray output = gdbServer.readAllStandardError();
    qDebug("gdbserver's stderr output: %s", output.data());

    const QByteArray searchString("pid = ");
    const int searchStringLength = searchString.length();

    int pidStartPos = output.indexOf(searchString);
    const int pidEndPos = output.indexOf("\n", pidStartPos + searchStringLength);
    if (pidStartPos == -1 || pidEndPos == -1) {
        gdbServerStartFailed(output.data());
        return;
    }

    pidStartPos += searchStringLength;
    QByteArray pidString = output.mid(pidStartPos, pidEndPos - pidStartPos);
    qDebug("pidString = %s", pidString.data());

    bool ok;
    const int pid = pidString.toInt(&ok);
    if (!ok) {
        gdbServerStartFailed(tr("Debugging failed, could not parse gdb "
            "server pid!"));
        return;
    }

    inferiorPid = pid;
    qDebug("inferiorPid = %d", inferiorPid);

    disconnect(&gdbServer, SIGNAL(readyReadStandardError()), 0, 0);
    connect(&gdbServer, SIGNAL(readyReadStandardError()), this,
        SLOT(readStandardError()));
    startDebugging();
}
#endif // USE_SSH_LIB

void MaemoDebugRunControl::startDebugging()
{
    debuggerManager->startNewDebugger(startParams);
}

void MaemoDebugRunControl::stop()
{
    if (!isRunning())
        return;
    emit addToOutputWindow(this, tr("Stopping debugging operation ..."));
    if (isDeploying()) {
        stopDeployment();
    } else {
        emit stopRequested();
    }
}

bool MaemoDebugRunControl::isRunning() const
{
    return isDeploying()
#ifdef USE_SSH_LIB
        || (!sshRunner.isNull() && sshRunner->isRunning())
#else
        || gdbServer.state() != QProcess::NotRunning
#endif // USE_SSH_LIB
        || debuggerManager->state() != Debugger::DebuggerNotReady;
}

void MaemoDebugRunControl::debuggingFinished()
{
#ifdef USE_SSH_LIB
    if (!sshRunner.isNull() && sshRunner->isRunning()) {
        sshRunner->stop();
        const QString remoteCall = QString::fromLocal8Bit("kill %1; sleep 1; "
            "kill -9 %1; pkill -x -9 gdbserver").arg(inferiorPid);
        sshStopper.reset(new MaemoSshRunner(devConfig, remoteCall));
        sshStopper->start();
    }
#else
    if (gdbServer.state() != QProcess::NotRunning) {
        stopProcess.kill();
        const QString remoteCall = QString::fromLocal8Bit("kill %1; sleep 1; "
            "kill -9 %1; pkill -x -9 gdbserver").arg(inferiorPid);
        QStringList sshArgs;
        sshArgs << "-n" << "-l" << devConfig.uname << "-p" << port()
            << options() << devConfig.host << remoteCall;
        stopProcess.start(runConfig->sshCmd(), sshArgs);
    }
    qDebug("ssh return code is %d", gdbServer.exitCode());
#endif // USE_SSH_LIB
    emit addToOutputWindow(this, tr("Debugging finished."));
    emit finished();
}

void MaemoDebugRunControl::debuggerOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}

} // namespace Internal
} // namespace Qt4ProjectManager
