/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "maemosshthread.h"
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
#include <QtCore/QProcess>
#include <QtCore/QStringBuilder>

#include <QtGui/QMessageBox>

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

AbstractMaemoRunControl::~AbstractMaemoRunControl()
{
}

void AbstractMaemoRunControl::startDeployment(bool forDebugging)
{
    QTC_ASSERT(runConfig, return);

    if (devConfig.isValid()) {
        m_deployables.clear();
        if (runConfig->currentlyNeedsDeployment(devConfig.host)) {
            m_deployables.append(Deployable(executableFileName(),
                QFileInfo(executableOnHost()).canonicalPath(),
                &MaemoRunConfiguration::wasDeployed));
        }
        if (forDebugging
            && runConfig->debuggingHelpersNeedDeployment(devConfig.host)) {
            const QFileInfo &info(runConfig->dumperLib());
            m_deployables.append(Deployable(info.fileName(), info.canonicalPath(),
                &MaemoRunConfiguration::debuggingHelpersDeployed));
        }

        deploy();
    } else {
        handleError(tr("No device configuration set for run configuration."));
        handleDeploymentFinished(false);
    }
}


void AbstractMaemoRunControl::deploy()
{
    Core::ICore::instance()->progressManager()
        ->addTask(m_progress.future(), tr("Deploying"),
                  QLatin1String("Maemo.Deploy"));
    if (!m_deployables.isEmpty()) {
        QList<SshDeploySpec> deploySpecs;
        QStringList files;
        foreach (const Deployable &deployable, m_deployables) {
            const QString srcFilePath
                = deployable.dir % QDir::separator() % deployable.fileName;
            const QString tgtFilePath
                = remoteDir() % QDir::separator() % deployable.fileName;
            files << srcFilePath;
            deploySpecs << SshDeploySpec(srcFilePath, tgtFilePath);
        }
        emit addToOutputWindow(this, tr("Files to deploy: %1.").arg(files.join(" ")));
        m_sshDeployer.reset(new MaemoSshDeployer(devConfig, deploySpecs));
        connect(m_sshDeployer.data(), SIGNAL(finished()),
                this, SLOT(deployProcessFinished()));
        connect(m_sshDeployer.data(), SIGNAL(fileCopied(QString)),
                this, SLOT(handleFileCopied()));
        m_progress.setProgressRange(0, m_deployables.count());
        m_progress.setProgressValue(0);
        m_progress.reportStarted();
        emit started();
        m_sshDeployer->start();
    } else {
        m_progress.reportFinished();
        handleDeploymentFinished(true);
    }
}

void AbstractMaemoRunControl::handleFileCopied()
{
    Deployable deployable = m_deployables.takeFirst();
    (runConfig->*deployable.updateTimestamp)(devConfig.host);
    m_progress.setProgressValue(m_progress.progressValue() + 1);
}

void AbstractMaemoRunControl::stopDeployment()
{
    m_sshDeployer->stop();
}

bool AbstractMaemoRunControl::isDeploying() const
{
    return m_sshDeployer && m_sshDeployer->isRunning();
}

void AbstractMaemoRunControl::run(const QString &remoteCall)
{
    m_sshRunner.reset(new MaemoSshRunner(devConfig, remoteCall));
    handleExecutionAboutToStart(m_sshRunner.data());
    m_sshRunner->start();
}

bool AbstractMaemoRunControl::isRunning() const
{
    return isDeploying() || (m_sshRunner && m_sshRunner->isRunning());
}

void AbstractMaemoRunControl::stopRunning(bool forDebugging)
{
    if (m_sshRunner && m_sshRunner->isRunning()) {
        m_sshRunner->stop();
        QStringList apps(executableFileName());
        if (forDebugging)
            apps << QLatin1String("gdbserver");
        kill(apps);
    }
}

void AbstractMaemoRunControl::kill(const QStringList &apps)
{
    QString niceKill;
    QString brutalKill;
    foreach (const QString &app, apps) {
        niceKill += QString::fromLocal8Bit("pkill -x %1;").arg(app);
        brutalKill += QString::fromLocal8Bit("pkill -x -9 %1;").arg(app);
    }
    const QString remoteCall
        = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    m_sshStopper.reset(new MaemoSshRunner(devConfig, remoteCall));
    m_sshStopper->start();
}

void AbstractMaemoRunControl::deployProcessFinished()
{
    const bool success = !m_sshDeployer->hasError();
    if (success) {
        emit addToOutputWindow(this, tr("Deployment finished."));
    } else {
        handleError(tr("Deployment failed: %1").arg(m_sshDeployer->error()));
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

void AbstractMaemoRunControl::handleError(const QString &errString)
{
    QMessageBox::critical(0, tr("Remote Execution Failure"), errString);
    emit error(this, errString);
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
    m_stoppedByUser = false;
    startDeployment(false);
}

void MaemoRunControl::handleDeploymentFinished(bool success)
{
    if (success)
        startExecution();
    else
        emit finished();
}

void MaemoRunControl::handleExecutionAboutToStart(const MaemoSshRunner *runner)
{
    connect(runner, SIGNAL(remoteOutput(QString)),
            this, SLOT(handleRemoteOutput(QString)));
    connect(runner, SIGNAL(finished()),
            this, SLOT(executionFinished()));
    emit addToOutputWindow(this, tr("Starting remote application."));
    emit started();
}

void MaemoRunControl::startExecution()
{
    const QString remoteCall = QString::fromLocal8Bit("%1 %2 %3")
        .arg(targetCmdLinePrefix()).arg(executableOnTarget())
        .arg(runConfig->arguments().join(" "));
    run(remoteCall);
}

void MaemoRunControl::executionFinished()
{
    MaemoSshRunner *runner = static_cast<MaemoSshRunner *>(sender());
    if (m_stoppedByUser) {
        emit addToOutputWindow(this, tr("Remote process stopped by user."));
    } else if (runner->hasError()) {
        emit addToOutputWindow(this, tr("Remote process exited with error: %1")
                                         .arg(runner->error()));
    } else {
        emit addToOutputWindow(this, tr("Remote process finished successfully."));
    }
    emit finished();
}

void MaemoRunControl::stop()
{
    if (!isRunning())
        return;

    m_stoppedByUser = true;
    if (isDeploying()) {
        stopDeployment();
    } else {
        AbstractMaemoRunControl::stopRunning(false);
    }
}

void MaemoRunControl::handleRemoteOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}


MaemoDebugRunControl::MaemoDebugRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
    , m_debuggerManager(ExtensionSystem::PluginManager::instance()
                      ->getObject<Debugger::DebuggerManager>())
    , m_startParams(new Debugger::DebuggerStartParameters)
{
    QTC_ASSERT(m_debuggerManager != 0, return);
    m_startParams->startMode = Debugger::StartRemote;
    m_startParams->executable = executableOnHost();
    m_startParams->remoteChannel
        = devConfig.host % QLatin1Char(':') % gdbServerPort();
    m_startParams->remoteArchitecture = QLatin1String("arm");
    m_startParams->sysRoot = runConfig->sysRoot();
    m_startParams->toolChainType = ToolChain::GCC_MAEMO;
    m_startParams->debuggerCommand = runConfig->gdbCmd();
    m_startParams->dumperLibrary = runConfig->dumperLib();
    m_startParams->remoteDumperLib = QString::fromLocal8Bit("%1/%2")
        .arg(remoteDir()).arg(QFileInfo(runConfig->dumperLib()).fileName());

    connect(m_debuggerManager, SIGNAL(debuggingFinished()), this,
        SLOT(debuggingFinished()), Qt::QueuedConnection);
    connect(m_debuggerManager, SIGNAL(applicationOutputAvailable(QString)),
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

void MaemoDebugRunControl::handleExecutionAboutToStart(const MaemoSshRunner *runner)
{
    m_inferiorPid = -1;
    connect(runner, SIGNAL(remoteOutput(QString)),
            this, SLOT(gdbServerStarted(QString)));
}

void MaemoDebugRunControl::startGdbServer()
{
    const QString remoteCall(QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4").
        arg(targetCmdLinePrefix()).arg(gdbServerPort())
        .arg(executableOnTarget()).arg(runConfig->arguments().join(" ")));
    run(remoteCall);
}

void MaemoDebugRunControl::gdbServerStartFailed(const QString &reason)
{
    handleError(tr("Debugging failed: %1").arg(reason));
    m_debuggerManager->exitDebugger();
    emit finished();
}

void MaemoDebugRunControl::gdbServerStarted(const QString &output)
{
    qDebug("gdbserver's stderr output: %s", output.toLatin1().data());
    if (m_inferiorPid != -1)
        return;
    const QString searchString("pid = ");
    const int searchStringLength = searchString.length();
    int pidStartPos = output.indexOf(searchString);
    const int pidEndPos = output.indexOf("\n", pidStartPos + searchStringLength);
    if (pidStartPos == -1 || pidEndPos == -1)
        return; // gdbserver has not started yet.
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
    m_inferiorPid = pid;
    qDebug("inferiorPid = %d", m_inferiorPid);

    startDebugging();
}

void MaemoDebugRunControl::startDebugging()
{
    m_debuggerManager->startNewDebugger(m_startParams);
}

void MaemoDebugRunControl::stop()
{
    if (!isRunning())
        return;
    emit addToOutputWindow(this, tr("Stopping debugging operation ..."));
    if (isDeploying()) {
        stopDeployment();
    } else {
        m_debuggerManager->exitDebugger();
    }
}

bool MaemoDebugRunControl::isRunning() const
{
    return AbstractMaemoRunControl::isRunning()
        || m_debuggerManager->state() != Debugger::DebuggerNotReady;
}

void MaemoDebugRunControl::debuggingFinished()
{
    AbstractMaemoRunControl::stopRunning(true);
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
