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

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <debugger/debuggermanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/toolchain.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFuture>
#include <QtCore/QStringBuilder>

namespace Qt4ProjectManager {
namespace Internal {

using ProjectExplorer::RunConfiguration;
using ProjectExplorer::ToolChain;

AbstractMaemoRunControl::AbstractMaemoRunControl(RunConfiguration *rc)
    : RunControl(rc)
    , runConfig(qobject_cast<MaemoRunConfiguration *>(rc))
    , devConfig(runConfig ? runConfig->deviceConfig() : MaemoDeviceConfig())
{
}

void AbstractMaemoRunControl::startDeployment(bool forDebugging)
{
    QTC_ASSERT(runConfig, return);

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

        deploy();
    } else {
        handleDeploymentFinished(false);
    }
}

void AbstractMaemoRunControl::deploy()
{
    if (!deployables.isEmpty()) {
        QList<SshDeploySpec> deploySpecs;
        QStringList files;
        foreach (const Deployable &deployable, deployables) {
            const QString srcFilePath
                = deployable.dir % QDir::separator() % deployable.fileName;
            const QString tgtFilePath
                = remoteDir() % QDir::separator() % deployable.fileName;
            files << srcFilePath;
            deploySpecs << SshDeploySpec(srcFilePath, tgtFilePath);
        }
        emit addToOutputWindow(this, tr("Files to deploy: %1.").arg(files.join(" ")));
        sshDeployer.reset(new MaemoSshDeployer(devConfig, deploySpecs));
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
}

void AbstractMaemoRunControl::handleFileCopied()
{
    Deployable deployable = deployables.takeFirst();
    (runConfig->*deployable.updateTimestamp)();
    m_progress.setProgressValue(m_progress.progressValue() + 1);
}

void AbstractMaemoRunControl::stopDeployment()
{
    sshDeployer->stop();
}

bool AbstractMaemoRunControl::isDeploying() const
{
    return !sshDeployer.isNull() && sshDeployer->isRunning();
}

void AbstractMaemoRunControl::deployProcessFinished()
{
    const bool success = !sshDeployer->hasError();
    if (success) {
        emit addToOutputWindow(this, tr("Deployment finished."));
    } else {
        emit error(this, tr("Deployment failed: %1").arg(sshDeployer->error()));
        m_progress.reportCanceled();
    }
    m_progress.reportFinished();
    handleDeploymentFinished(success);
}

const QString AbstractMaemoRunControl::executableOnHost() const
{
    qDebug("runconfig->executable: %s", qPrintable(runConfig->executable()));
    return runConfig->executable();
}

const QString AbstractMaemoRunControl::sshPort() const
{
    return devConfig.type == MaemoDeviceConfig::Physical
        ? QString::number(devConfig.sshPort)
        : runConfig->simulatorSshPort();
}

const QString AbstractMaemoRunControl::executableFileName() const
{
    return QFileInfo(executableOnHost()).fileName();
}

const QString AbstractMaemoRunControl::remoteDir() const
{
    return homeDirOnDevice(devConfig.uname);
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


MaemoRunControl::MaemoRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
{
}

MaemoRunControl::~MaemoRunControl()
{
    stop();
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
}

void MaemoRunControl::executionFinished()
{
    if (stoppedByUser) {
        emit addToOutputWindow(this, tr("Remote process stopped by user."));
    } else if (sshRunner->hasError()) {
        emit addToOutputWindow(this, tr("Remote process exited with error: %1").arg(sshRunner->error()));
    } else {
        emit addToOutputWindow(this, tr("Remote process finished successfully."));
    }
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
        sshRunner->stop();
        const QString remoteCall = QString::fromLocal8Bit("pkill -x %1; "
            "sleep 1; pkill -x -9 %1").arg(executableFileName());
        sshStopper.reset(new MaemoSshRunner(devConfig, remoteCall));
        sshStopper->start();
    }
}

bool MaemoRunControl::isRunning() const
{
    return isDeploying()
        || (!sshRunner.isNull() && sshRunner->isRunning());
}

void MaemoRunControl::handleRemoteOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}


MaemoDebugRunControl::MaemoDebugRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
    , debuggerManager(0)
    , startParams(new Debugger::DebuggerStartParameters)
{
    debuggerManager = ExtensionSystem::PluginManager::instance()
        ->getObject<Debugger::DebuggerManager>();

    QTC_ASSERT(debuggerManager != 0, return);
    startParams->startMode = Debugger::StartRemote;
    startParams->executable = executableOnHost();
    startParams->remoteChannel
        = devConfig.host % QLatin1Char(':') % gdbServerPort();
    startParams->remoteArchitecture = QLatin1String("arm");
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
        arg(targetCmdLinePrefix()).arg(gdbServerPort())
        .arg(executableOnTarget()).arg(runConfig->arguments().join(" ")));
    inferiorPid = -1;
    sshRunner.reset(new MaemoSshRunner(devConfig, remoteCall));
    connect(sshRunner.data(), SIGNAL(remoteOutput(QString)),
            this, SLOT(gdbServerStarted(QString)));
    sshRunner->start();
}

void MaemoDebugRunControl::gdbServerStartFailed(const QString &reason)
{
    emit addToOutputWindow(this, tr("Debugging failed: %1").arg(reason));
    emit stopRequested();
    emit finished();
}

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
        || (!sshRunner.isNull() && sshRunner->isRunning())
        || debuggerManager->state() != Debugger::DebuggerNotReady;
}

void MaemoDebugRunControl::debuggingFinished()
{
    if (!sshRunner.isNull() && sshRunner->isRunning()) {
        sshRunner->stop();
        const QString remoteCall = QString::fromLocal8Bit("kill %1; sleep 1; "
            "kill -9 %1; pkill -x -9 gdbserver").arg(inferiorPid);
        sshStopper.reset(new MaemoSshRunner(devConfig, remoteCall));
        sshStopper->start();
    }
    emit addToOutputWindow(this, tr("Debugging finished."));
    emit finished();
}

void MaemoDebugRunControl::debuggerOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}

QString MaemoDebugRunControl::gdbServerPort() const
{
    return devConfig.type == MaemoDeviceConfig::Physical
        ? QString::number(devConfig.gdbServerPort)
        : runConfig->simulatorGdbServerPort();
}

} // namespace Internal
} // namespace Qt4ProjectManager
