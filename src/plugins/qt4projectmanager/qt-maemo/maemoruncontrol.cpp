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

#include "maemodeploystep.h"
#include "maemoglobal.h"
#include "maemorunconfiguration.h"
#include "maemosshrunner.h"

#include <coreplugin/icore.h>
#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>

#include <QtGui/QMessageBox>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

#define USE_GDBSERVER

using ProjectExplorer::RunConfiguration;
using ProjectExplorer::ToolChain;
using namespace Debugger;


AbstractMaemoRunControl::AbstractMaemoRunControl(RunConfiguration *rc, QString mode)
    : RunControl(rc, mode)
    , m_runConfig(qobject_cast<MaemoRunConfiguration *>(rc))
    , m_devConfig(m_runConfig ? m_runConfig->deviceConfig() : MaemoDeviceConfig())
    , m_runner(new MaemoSshRunner(this, m_runConfig))
    , m_running(false)
{
}

AbstractMaemoRunControl::~AbstractMaemoRunControl()
{
}

void AbstractMaemoRunControl::start()
{
    if (!m_devConfig.isValid()) {
        handleError(tr("No device configuration set for run configuration."));
    } else {
        emit appendMessage(this, tr("Preparing remote side ..."), false);
        m_running = true;
        m_stopped = false;
        emit started();
        connect(m_runner, SIGNAL(error(QString)), this,
            SLOT(handleSshError(QString)));
        connect(m_runner, SIGNAL(readyForExecution()), this,
            SLOT(startExecution()));
        connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)), this,
            SLOT(handleRemoteErrorOutput(QByteArray)));
        connect(m_runner, SIGNAL(remoteOutput(QByteArray)), this,
            SLOT(handleRemoteOutput(QByteArray)));
        connect(m_runner, SIGNAL(remoteProcessStarted()), this,
            SLOT(handleRemoteProcessStarted()));
        connect(m_runner, SIGNAL(remoteProcessFinished(int)), this,
            SLOT(handleRemoteProcessFinished(int)));
        m_runner->start();
    }
}

void AbstractMaemoRunControl::stop()
{
    m_stopped = true;
    if (m_runner)
        m_runner->stop();
    stopInternal();
}

void AbstractMaemoRunControl::handleSshError(const QString &error)
{
    handleError(error);
    setFinished();
}

void AbstractMaemoRunControl::startExecution()
{
    emit appendMessage(this, tr("Starting remote process ..."), false);
    m_runner->startExecution(QString::fromLocal8Bit("%1 %2 %3")
        .arg(targetCmdLinePrefix()).arg(m_runConfig->remoteExecutableFilePath())
        .arg(arguments()).toUtf8());
}

void AbstractMaemoRunControl::handleRemoteProcessFinished(int exitCode)
{
    if (m_stopped)
        return;

    emit appendMessage(this,
        tr("Finished running remote process. Exit code was %1.").arg(exitCode),
        false);
    setFinished();
}

void AbstractMaemoRunControl::handleRemoteOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), false);
}

void AbstractMaemoRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), true);
}

bool AbstractMaemoRunControl::isRunning() const
{
    return m_running;
}

const QString AbstractMaemoRunControl::targetCmdLinePrefix() const
{
    return QString::fromLocal8Bit("%1 chmod a+x %2 && source /etc/profile && DISPLAY=:0.0 ")
        .arg(MaemoGlobal::remoteSudo())
        .arg(m_runConfig->remoteExecutableFilePath());
}

QString AbstractMaemoRunControl::arguments() const
{
    return m_runConfig->arguments().join(" ");
}

void AbstractMaemoRunControl::handleError(const QString &errString)
{
    QMessageBox::critical(0, tr("Remote Execution Failure"), errString);
    emit appendMessage(this, errString, true);
    stop();
}

void AbstractMaemoRunControl::setFinished()
{
    m_running = false;
    emit finished();
}


MaemoRunControl::MaemoRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration, ProjectExplorer::Constants::RUNMODE)
{
}

MaemoRunControl::~MaemoRunControl()
{
    stop();
}

void MaemoRunControl::stopInternal()
{
    setFinished();
}


MaemoDebugRunControl::MaemoDebugRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration, ProjectExplorer::Constants::DEBUGMODE)
    , m_debuggerRunControl(0)
    , m_startParams(new DebuggerStartParameters)
    , m_uploadJob(SftpInvalidJob)
{
#ifdef USE_GDBSERVER
    m_startParams->startMode = AttachToRemote;
    m_startParams->executable = m_runConfig->localExecutableFilePath();
    m_startParams->debuggerCommand = m_runConfig->gdbCmd();
    m_startParams->remoteChannel
        = m_devConfig.server.host % QLatin1Char(':') % gdbServerPort();
    m_startParams->remoteArchitecture = QLatin1String("arm");
    m_runner->addProcsToKill(QStringList() << QLatin1String("gdbserver"));
#else
    m_startParams->startMode = StartRemoteGdb;
    m_startParams->executable = m_runConfig->remoteExecutableFilePath();
    m_startParams->debuggerCommand = targetCmdLinePrefix()
        + QLatin1String(" /usr/bin/gdb");
    m_startParams->connParams = m_devConfig.server;
    m_runner->addProcsToKill(QStringList() << QLatin1String("gdb"));
#endif
    m_startParams->processArgs = m_runConfig->arguments();
    m_startParams->sysRoot = m_runConfig->sysRoot();
    m_startParams->toolChainType = ToolChain::GCC_MAEMO;
    m_startParams->dumperLibrary = m_runConfig->dumperLib();
    m_startParams->remoteDumperLib = uploadDir().toUtf8() + '/'
        + QFileInfo(m_runConfig->dumperLib()).fileName().toUtf8();

    m_debuggerRunControl = DebuggerPlugin::createDebugger(*m_startParams.data());
    connect(m_debuggerRunControl, SIGNAL(finished()), this,
        SLOT(debuggingFinished()), Qt::QueuedConnection);
}

MaemoDebugRunControl::~MaemoDebugRunControl()
{
    disconnect(SIGNAL(addToOutputWindow(ProjectExplorer::RunControl*,QString, bool)));
    disconnect(SIGNAL(addToOutputWindowInline(ProjectExplorer::RunControl*,QString, bool)));
    stop();
}

void MaemoDebugRunControl::startExecution()
{
    const QString &dumperLib = m_runConfig->dumperLib();
    if (!dumperLib.isEmpty()
        && m_runConfig->deployStep()->currentlyNeedsDeployment(m_devConfig.server.host,
            MaemoDeployable(dumperLib, uploadDir()))) {
        m_uploader = m_runner->connection()->createSftpChannel();
        connect(m_uploader.data(), SIGNAL(initialized()), this,
            SLOT(handleSftpChannelInitialized()));
        connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
            SLOT(handleSftpChannelInitializationFailed(QString)));
        connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
            this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
        m_uploader->initialize();
    } else {
        startDebugging();
    }
}

void MaemoDebugRunControl::handleSftpChannelInitialized()
{
    if (m_stopped)
        return;

    const QString dumperLib = m_runConfig->dumperLib();
    const QString fileName = QFileInfo(dumperLib).fileName();
    const QString remoteFilePath = uploadDir() + '/' + fileName;
    m_uploadJob = m_uploader->uploadFile(dumperLib, remoteFilePath,
        SftpOverwriteExisting);
    if (m_uploadJob == SftpInvalidJob) {
        handleError(tr("Upload failed: Could not open file '%1'")
            .arg(dumperLib));
    } else {
        emit appendMessage(this,
            tr("Started uploading debugging helpers ('%1').").arg(dumperLib),
            false);
    }
}

void MaemoDebugRunControl::handleSftpChannelInitializationFailed(const QString &error)
{
    handleError(error);
}

void MaemoDebugRunControl::handleSftpJobFinished(Core::SftpJobId job,
    const QString &error)
{
    if (m_stopped)
        return;

    if (job != m_uploadJob) {
        qWarning("Warning: Unknown debugging helpers upload job %d finished.", job);
        return;
    }

    if (!error.isEmpty()) {
        handleError(tr("Error: Could not upload debugging helpers."));
    } else {
        m_runConfig->deployStep()->setDeployed(m_devConfig.server.host,
            MaemoDeployable(m_runConfig->dumperLib(), uploadDir()));
        emit appendMessage(this,
            tr("Finished uploading debugging helpers."), false);
        startDebugging();
    }
}

void MaemoDebugRunControl::startDebugging()
{
#ifdef USE_GDBSERVER
    m_runner->startExecution(QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4")
        .arg(targetCmdLinePrefix()).arg(gdbServerPort())
        .arg(m_runConfig->remoteExecutableFilePath())
        .arg(arguments()).toUtf8());
#else
    DebuggerPlugin::startDebugger(m_debuggerRunControl);
#endif
}

bool MaemoDebugRunControl::isDeploying() const
{
    return m_uploader && m_uploadJob != SftpInvalidJob;
}

void MaemoDebugRunControl::stopInternal()
{
    if (isDeploying()) {
        disconnect(m_uploader.data(), 0, this, 0);
        m_uploader->closeChannel();
        m_uploadJob = SftpInvalidJob;
        setFinished();
    } else if (m_debuggerRunControl && m_debuggerRunControl->engine()) {
        m_debuggerRunControl->engine()->quitDebugger();
    } else {
        setFinished();
    }
}

void MaemoDebugRunControl::debuggingFinished()
{
#ifdef USE_GDBSERVER
    m_runner->stop();
#endif
    setFinished();
}

void MaemoDebugRunControl::handleRemoteProcessStarted()
{
    DebuggerPlugin::startDebugger(m_debuggerRunControl);
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

QString MaemoDebugRunControl::uploadDir() const
{
    return MaemoGlobal::homeDirOnDevice(m_devConfig.server.uname);
}

} // namespace Internal
} // namespace Qt4ProjectManager
