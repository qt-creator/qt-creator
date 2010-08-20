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

#include "maemopackagecreationstep.h"
#include "maemosshthread.h"
#include "maemorunconfiguration.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <debugger/debuggermanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/toolchain.h>
#include <utils/qtcassert.h>

#include <QtCore/QCryptographicHash>
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
    , m_runConfig(qobject_cast<MaemoRunConfiguration *>(rc))
    , m_devConfig(m_runConfig ? m_runConfig->deviceConfig() : MaemoDeviceConfig())
{
}

AbstractMaemoRunControl::~AbstractMaemoRunControl()
{
}

void AbstractMaemoRunControl::start()
{
    m_stoppedByUser = false;
    if (!m_devConfig.isValid()) {
        handleError(tr("No device configuration set for run configuration."));
    } else {
        emit started();
        startInitialCleanup();
    }
}

void AbstractMaemoRunControl::startInitialCleanup()
{   
    emit appendMessage(this, tr("Cleaning up remote leftovers first ..."), false);
    const QStringList appsToKill
        = QStringList() << executableFileName() << QLatin1String("gdbserver");
    killRemoteProcesses(appsToKill, true);
}

void AbstractMaemoRunControl::stop()
{
    m_stoppedByUser = true;
    if (isCleaning())
        m_initialCleaner->stop();
    else if (isDeploying())
        m_sshDeployer->stop();
    else
        stopInternal();
}

void AbstractMaemoRunControl::handleInitialCleanupFinished()
{
    if (m_stoppedByUser) {
        emit appendMessage(this, tr("Initial cleanup canceled by user."), false);
        emit finished();
    } else if (m_initialCleaner->hasError()) {
        handleError(tr("Error running initial cleanup: %1.")
                    .arg(m_initialCleaner->error()));
        emit finished();
    } else {
        emit appendMessage(this, tr("Initial cleanup done."), false);
        startInternal();
    }
}

void AbstractMaemoRunControl::startDeployment(bool forDebugging)
{
    QTC_ASSERT(m_runConfig, return);

    if (m_stoppedByUser) {
        emit finished();
    } else {
        m_needsInstall = false;
        m_deployables.clear();
        m_remoteLinks.clear();
        const MaemoPackageCreationStep * const packageStep
            = m_runConfig->packageStep();
        if (packageStep->isPackagingEnabled()) {
            const MaemoDeployable d(packageFilePath(),
                remoteDir() + '/' + packageFileName());
            m_needsInstall = addDeployableIfNeeded(d);
        } else {
            const MaemoPackageContents * const packageContents
                = packageStep->packageContents();
            for (int i = 0; i < packageContents->rowCount(); ++i) {
                const MaemoDeployable &d = packageContents->deployableAt(i);
                if (addDeployableIfNeeded(d))
                    m_needsInstall = true;
            }
        }

        if (forDebugging) {
            QFileInfo dumperInfo(m_runConfig->dumperLib());
            if (dumperInfo.exists()) {
                const MaemoDeployable d(m_runConfig->dumperLib(),
                    remoteDir() + '/' + dumperInfo.fileName());
                m_needsInstall |= addDeployableIfNeeded(d);
            }
        }
        deploy();
    }
}

bool AbstractMaemoRunControl::addDeployableIfNeeded(const MaemoDeployable &deployable)
{
    if (m_runConfig->currentlyNeedsDeployment(m_devConfig.server.host,
        deployable)) {
        const QString fileName
            = QFileInfo(deployable.remoteFilePath).fileName();
        const QString sftpTargetFilePath = remoteDir() + '/' + fileName + '.'
            + QCryptographicHash::hash(deployable.remoteFilePath.toUtf8(),
                  QCryptographicHash::Md5).toHex();
        m_deployables.append(MaemoDeployable(deployable.localFilePath,
            sftpTargetFilePath));
        m_remoteLinks.insert(sftpTargetFilePath, deployable.remoteFilePath);
        return true;
    } else {
        return false;
    }
}

void AbstractMaemoRunControl::deploy()
{
    Core::ICore::instance()->progressManager()
        ->addTask(m_progress.future(), tr("Deploying"),
                  QLatin1String("Maemo.Deploy"));
    if (!m_deployables.isEmpty()) {
        QList<Core::SftpTransferInfo> deploySpecs;
        QStringList files;
        foreach (const MaemoDeployable &deployable, m_deployables) {
            files << deployable.localFilePath;
            deploySpecs << Core::SftpTransferInfo(deployable.localFilePath,
                deployable.remoteFilePath.toUtf8(),
                Core::SftpTransferInfo::Upload);
        }
        emit appendMessage(this, tr("Files to deploy: %1.").arg(files.join(" ")), false);
        m_sshDeployer.reset(new MaemoSshDeployer(m_devConfig.server, deploySpecs));
        connect(m_sshDeployer.data(), SIGNAL(finished()),
                this, SLOT(handleDeployThreadFinished()));
        connect(m_sshDeployer.data(), SIGNAL(fileCopied(QString)),
                this, SLOT(handleFileCopied()));
        m_progress.setProgressRange(0, m_deployables.count());
        m_progress.setProgressValue(0);
        m_progress.reportStarted();
        m_sshDeployer->start();
    } else {
        m_progress.reportFinished();
        startExecution();
    }
}

void AbstractMaemoRunControl::handleFileCopied()
{
    const MaemoDeployable &deployable = m_deployables.takeFirst();
    m_runConfig->setDeployed(m_devConfig.server.host,
        MaemoDeployable(deployable.localFilePath,
        m_remoteLinks.value(deployable.remoteFilePath)));
    m_progress.setProgressValue(m_progress.progressValue() + 1);
}

bool AbstractMaemoRunControl::isDeploying() const
{
    return m_sshDeployer && m_sshDeployer->isRunning();
}

QString AbstractMaemoRunControl::packageFileName() const
{
    return QFileInfo(packageFilePath()).fileName();
}

QString AbstractMaemoRunControl::packageFilePath() const
{
    return m_runConfig->packageStep()->packageFilePath();
}

QString AbstractMaemoRunControl::executableFilePathOnTarget() const
{
    return m_runConfig->packageStep()->packageContents()->remoteExecutableFilePath();
}

bool AbstractMaemoRunControl::isCleaning() const
{
    return m_initialCleaner && m_initialCleaner->isRunning();
}

void AbstractMaemoRunControl::startExecution()
{
    m_sshRunner.reset(new MaemoSshRunner(m_devConfig.server, remoteCall()));
    connect(m_sshRunner.data(), SIGNAL(finished()),
            this, SLOT(handleRunThreadFinished()));
    connect(m_sshRunner.data(), SIGNAL(remoteOutput(QString)),
            this, SLOT(handleRemoteOutput(QString)));
    emit appendMessage(this, tr("Starting remote application."), false);
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
        killRemoteProcesses(apps, false);
    }
}

void AbstractMaemoRunControl::killRemoteProcesses(const QStringList &apps,
                                                  bool initialCleanup)
{
    QString niceKill;
    QString brutalKill;
    foreach (const QString &app, apps) {
        niceKill += QString::fromLocal8Bit("pkill -x %1;").arg(app);
        brutalKill += QString::fromLocal8Bit("pkill -x -9 %1;").arg(app);
    }
    QString remoteCall = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    remoteCall.remove(remoteCall.count() - 1, 1); // Get rid of trailing semicolon.
    QScopedPointer<MaemoSshRunner> &runner
        = initialCleanup ? m_initialCleaner : m_sshStopper;
    runner.reset(new MaemoSshRunner(m_devConfig.server, remoteCall));
    if (initialCleanup)
        connect(runner.data(), SIGNAL(finished()),
                this, SLOT(handleInitialCleanupFinished()));
    runner->start();
}

void AbstractMaemoRunControl::handleDeployThreadFinished()
{
    bool cancel;
    if (m_stoppedByUser) {
        emit appendMessage(this, tr("Deployment canceled by user."), false);
        cancel = true;
    } else if (m_sshDeployer->hasError()) {
        handleError(tr("Deployment failed: %1").arg(m_sshDeployer->error()));
        cancel = true;
    } else {
        emit appendMessage(this, tr("Deployment finished."), false);
        cancel = false;
    }

    if (cancel) {
        m_progress.reportCanceled();
        m_progress.reportFinished();
        emit finished();
    } else {
        m_progress.reportFinished();
        startExecution();
    }
}

void AbstractMaemoRunControl::handleRunThreadFinished()
{
    if (m_stoppedByUser) {
        emit appendMessage(this,
                 tr("Remote execution canceled due to user request."),
                 false);
    } else if (m_sshRunner->hasError()) {
        emit appendMessage(this, tr("Error running remote process: %1")
                                         .arg(m_sshRunner->error()),
                               true);
    } else {
        emit appendMessage(this, tr("Finished running remote process."),
                               false);
    }
    emit finished();
}

const QString AbstractMaemoRunControl::executableOnHost() const
{
    return m_runConfig->executable();
}

const QString AbstractMaemoRunControl::executableFileName() const
{
    return QFileInfo(executableOnHost()).fileName();
}

const QString AbstractMaemoRunControl::remoteDir() const
{
    return homeDirOnDevice(m_devConfig.server.uname);
}

QString AbstractMaemoRunControl::remoteSudo() const
{
    return QLatin1String("/usr/lib/mad-developer/devrootsh");
}

QString AbstractMaemoRunControl::remoteInstallCommand() const
{
    Q_ASSERT(m_needsInstall);
    QString cmd;
    for (QMap<QString, QString>::ConstIterator it = m_remoteLinks.begin();
    it != m_remoteLinks.end(); ++it) {
        cmd += QString::fromLocal8Bit("%1 ln -sf %2 %3 && ")
            .arg(remoteSudo(), it.key(), it.value());
    }
    if (m_runConfig->packageStep()->isPackagingEnabled()) {
        cmd += QString::fromLocal8Bit("%1 dpkg -i %2").arg(remoteSudo())
            .arg(packageFileName());
    } else if (!m_remoteLinks.isEmpty()) {
           return cmd.remove(cmd.length() - 4, 4); // Trailing " && "
    }

    return cmd;
}

const QString AbstractMaemoRunControl::targetCmdLinePrefix() const
{
    const QString &installPrefix = m_needsInstall
        ? remoteInstallCommand() + QLatin1String(" && ")
        : QString();
    return QString::fromLocal8Bit("%1%2 chmod a+x %3 && source /etc/profile && DISPLAY=:0.0 ")
        .arg(installPrefix).arg(remoteSudo()).arg(executableFilePathOnTarget());
}

QString AbstractMaemoRunControl::targetCmdLineSuffix() const
{
    return m_runConfig->arguments().join(" ");
}

void AbstractMaemoRunControl::handleError(const QString &errString)
{
    QMessageBox::critical(0, tr("Remote Execution Failure"), errString);
    emit appendMessage(this, errString, true);
}


MaemoRunControl::MaemoRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration)
{
}

MaemoRunControl::~MaemoRunControl()
{
    stop();
}

void MaemoRunControl::startInternal()
{
    startDeployment(false);
}

QString MaemoRunControl::remoteCall() const
{
    return QString::fromLocal8Bit("%1 %2 %3").arg(targetCmdLinePrefix())
        .arg(executableFilePathOnTarget()).arg(targetCmdLineSuffix());
}

void MaemoRunControl::stopInternal()
{
    AbstractMaemoRunControl::stopRunning(false);
}

void MaemoRunControl::handleRemoteOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output, false);
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
        = m_devConfig.server.host % QLatin1Char(':') % gdbServerPort();
    m_startParams->remoteArchitecture = QLatin1String("arm");
    m_startParams->sysRoot = m_runConfig->sysRoot();
    m_startParams->toolChainType = ToolChain::GCC_MAEMO;
    m_startParams->debuggerCommand = m_runConfig->gdbCmd();
    m_startParams->dumperLibrary = m_runConfig->dumperLib();
    m_startParams->remoteDumperLib = QString::fromLocal8Bit("%1/%2")
        .arg(remoteDir()).arg(QFileInfo(m_runConfig->dumperLib()).fileName());

    connect(m_debuggerManager, SIGNAL(debuggingFinished()), this,
        SLOT(debuggingFinished()), Qt::QueuedConnection);
    connect(m_debuggerManager, SIGNAL(applicationOutputAvailable(QString, bool)),
        this, SLOT(debuggerOutput(QString)), Qt::QueuedConnection);
}

MaemoDebugRunControl::~MaemoDebugRunControl()
{
    disconnect(SIGNAL(addToOutputWindow(RunControl*,QString, bool)));
    disconnect(SIGNAL(addToOutputWindowInline(RunControl*,QString, bool)));
    stop();
    debuggingFinished();
}

void MaemoDebugRunControl::startInternal()
{
    m_debuggingStarted = false;
    m_remoteOutput.clear();
    startDeployment(true);
}

QString MaemoDebugRunControl::remoteCall() const
{
    return QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4")
        .arg(targetCmdLinePrefix()).arg(gdbServerPort())
        .arg(executableFilePathOnTarget()).arg(targetCmdLineSuffix());
}

void MaemoDebugRunControl::handleRemoteOutput(const QString &output)
{
    if (!m_debuggingStarted) {
        m_remoteOutput += output;
        if (m_remoteOutput.contains(QLatin1String("Listening on port"))) {
            m_debuggingStarted = true;
            startDebugging();
        }
    }
    emit addToOutputWindowInline(this, output, false);
}

void MaemoDebugRunControl::startDebugging()
{
    m_debuggerManager->startNewDebugger(m_startParams);
}

void MaemoDebugRunControl::stopInternal()
{
    m_debuggerManager->exitDebugger();
}

bool MaemoDebugRunControl::isRunning() const
{
    return AbstractMaemoRunControl::isRunning()
        || m_debuggerManager->state() != Debugger::DebuggerNotReady;
}

void MaemoDebugRunControl::debuggingFinished()
{
    AbstractMaemoRunControl::stopRunning(true);
}

void MaemoDebugRunControl::debuggerOutput(const QString &output)
{
    emit appendMessage(this, QLatin1String("[gdb says:] ") + output, true);
}

QString MaemoDebugRunControl::gdbServerPort() const
{
    return m_devConfig.type == MaemoDeviceConfig::Physical
        ? QString::number(m_devConfig.gdbServerPort)
        : m_runConfig->runtimeGdbServerPort();  // During configuration we don't
        // know which port to use, so we display something in the config dialog,
        // but we will make sure we use the right port from the information file.
}

    } // namespace Internal
} // namespace Qt4ProjectManager
