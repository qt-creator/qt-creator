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
    emit addToOutputWindow(this, tr("Cleaning up remote leftovers first ..."));
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
        emit addToOutputWindow(this, tr("Initial cleanup canceled by user."));
        emit finished();
    } else if (m_initialCleaner->hasError()) {
        handleError(tr("Error running initial cleanup: %1.")
                    .arg(m_initialCleaner->error()));
        emit finished();
    } else {
        emit addToOutputWindow(this, tr("Initial cleanup done."));
        startInternal();
    }
}

void AbstractMaemoRunControl::startDeployment(bool forDebugging)
{
    QTC_ASSERT(m_runConfig, return);

    if (m_stoppedByUser) {
        emit finished();
    } else {
        m_deployables.clear();
        if (m_runConfig->currentlyNeedsDeployment(m_devConfig.host)) {
            m_deployables.append(Deployable(executableFileName(),
                QFileInfo(executableOnHost()).canonicalPath(),
                &MaemoRunConfiguration::wasDeployed));
        }
        if (forDebugging
            && m_runConfig->debuggingHelpersNeedDeployment(m_devConfig.host)) {
            const QFileInfo &info(m_runConfig->dumperLib());
            m_deployables.append(Deployable(info.fileName(), info.canonicalPath(),
                &MaemoRunConfiguration::debuggingHelpersDeployed));
        }

        deploy();
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
        m_sshDeployer.reset(new MaemoSshDeployer(m_devConfig, deploySpecs));
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
    Deployable deployable = m_deployables.takeFirst();
    (m_runConfig->*deployable.updateTimestamp)(m_devConfig.host);
    m_progress.setProgressValue(m_progress.progressValue() + 1);
}

bool AbstractMaemoRunControl::isDeploying() const
{
    return m_sshDeployer && m_sshDeployer->isRunning();
}

bool AbstractMaemoRunControl::isCleaning() const
{
    return m_initialCleaner && m_initialCleaner->isRunning();
}

void AbstractMaemoRunControl::startExecution()
{
    m_sshRunner.reset(new MaemoSshRunner(m_devConfig, remoteCall()));
    connect(m_sshRunner.data(), SIGNAL(finished()),
            this, SLOT(handleRunThreadFinished()));
    connect(m_sshRunner.data(), SIGNAL(remoteOutput(QString)),
            this, SLOT(handleRemoteOutput(QString)));
    emit addToOutputWindow(this, tr("Starting remote application."));
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
    const QString remoteCall
        = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    QScopedPointer<MaemoSshRunner> &runner
        = initialCleanup ? m_initialCleaner : m_sshStopper;
    runner.reset(new MaemoSshRunner(m_devConfig, remoteCall));
    if (initialCleanup)
        connect(runner.data(), SIGNAL(finished()),
                this, SLOT(handleInitialCleanupFinished()));
    runner->start();
}

void AbstractMaemoRunControl::handleDeployThreadFinished()
{
    bool cancel;
    if (m_stoppedByUser) {
        emit addToOutputWindow(this, tr("Deployment canceled by user."));
        cancel = true;
    } else if (m_sshDeployer->hasError()) {
        handleError(tr("Deployment failed: %1").arg(m_sshDeployer->error()));
        cancel = true;
    } else {
        emit addToOutputWindow(this, tr("Deployment finished."));
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
        emit addToOutputWindow(this,
                 tr("Remote execution canceled due to user request."));
    } else if (m_sshRunner->hasError()) {
        emit addToOutputWindow(this, tr("Error running remote process: %1")
                                         .arg(m_sshRunner->error()));
    } else {
        emit addToOutputWindow(this, tr("Finished running remote process."));
    }
    emit finished();
}

const QString AbstractMaemoRunControl::executableOnHost() const
{
    qDebug("runconfig->executable: %s", qPrintable(m_runConfig->executable()));
    return m_runConfig->executable();
}

const QString AbstractMaemoRunControl::sshPort() const
{
    return m_devConfig.type == MaemoDeviceConfig::Physical
        ? QString::number(m_devConfig.sshPort)
        : m_runConfig->simulatorSshPort();
}

const QString AbstractMaemoRunControl::executableFileName() const
{
    return QFileInfo(executableOnHost()).fileName();
}

const QString AbstractMaemoRunControl::remoteDir() const
{
    return homeDirOnDevice(m_devConfig.uname);
}

const QStringList AbstractMaemoRunControl::options() const
{
    const bool usePassword
        = m_devConfig.authentication == MaemoDeviceConfig::Password;
    const QLatin1String opt("-o");
    QStringList optionList;
    if (!usePassword)
        optionList << QLatin1String("-i") << m_devConfig.keyFile;
    return optionList << opt
        << QString::fromLatin1("PasswordAuthentication=%1").
            arg(usePassword ? "yes" : "no") << opt
        << QString::fromLatin1("PubkeyAuthentication=%1").
            arg(usePassword ? "no" : "yes") << opt
        << QString::fromLatin1("ConnectTimeout=%1").arg(m_devConfig.timeout)
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

void MaemoRunControl::startInternal()
{
    startDeployment(false);
}

QString MaemoRunControl::remoteCall() const
{
    return QString::fromLocal8Bit("%1 %2 %3")
        .arg(targetCmdLinePrefix()).arg(executableOnTarget())
        .arg(m_runConfig->arguments().join(" "));
}

void MaemoRunControl::stopInternal()
{
    AbstractMaemoRunControl::stopRunning(false);
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
        = m_devConfig.host % QLatin1Char(':') % gdbServerPort();
    m_startParams->remoteArchitecture = QLatin1String("arm");
    m_startParams->sysRoot = m_runConfig->sysRoot();
    m_startParams->toolChainType = ToolChain::GCC_MAEMO;
    m_startParams->debuggerCommand = m_runConfig->gdbCmd();
    m_startParams->dumperLibrary = m_runConfig->dumperLib();
    m_startParams->remoteDumperLib = QString::fromLocal8Bit("%1/%2")
        .arg(remoteDir()).arg(QFileInfo(m_runConfig->dumperLib()).fileName());

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

void MaemoDebugRunControl::startInternal()
{
    m_inferiorPid = -1;
    startDeployment(true);
}

QString MaemoDebugRunControl::remoteCall() const
{
    return QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4")
        .arg(targetCmdLinePrefix()).arg(gdbServerPort())
        .arg(executableOnTarget()).arg(m_runConfig->arguments().join(" "));
}

void MaemoDebugRunControl::handleRemoteOutput(const QString &output)
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
        handleError(tr("Debugging failed: Could not parse gdbserver output."));
        m_debuggerManager->exitDebugger();
    } else {
        m_inferiorPid = pid;
        startDebugging();
    }
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
    emit addToOutputWindowInline(this, output);
}

QString MaemoDebugRunControl::gdbServerPort() const
{
    return m_devConfig.type == MaemoDeviceConfig::Physical
        ? QString::number(m_devConfig.gdbServerPort)
        : m_runConfig->simulatorGdbServerPort();
}

} // namespace Internal
} // namespace Qt4ProjectManager
