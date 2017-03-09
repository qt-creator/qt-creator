/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company
** Contact: info@kdab.com
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qnxattachdebugsupport.h"

#include "qnxattachdebugdialog.h"
#include "qnxconstants.h"
#include "qnxqtversion.h"
#include "qnxutils.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

QnxAttachDebugSupport::QnxAttachDebugSupport(QObject *parent)
    : QObject(parent)
{
    m_launcher = new ApplicationLauncher(this);
    m_portsGatherer = new DeviceUsedPortsGatherer(this);

    connect(m_portsGatherer, &DeviceUsedPortsGatherer::portListReady,
            this, &QnxAttachDebugSupport::launchPDebug);
    connect(m_portsGatherer, &DeviceUsedPortsGatherer::error,
            this, &QnxAttachDebugSupport::handleError);
    connect(m_launcher, &ApplicationLauncher::remoteProcessStarted,
            this, &QnxAttachDebugSupport::attachToProcess);
    connect(m_launcher, &ApplicationLauncher::reportError,
            this, &QnxAttachDebugSupport::handleError);
    connect(m_launcher, &ApplicationLauncher::reportProgress,
            this, &QnxAttachDebugSupport::handleProgressReport);
    connect(m_launcher, &ApplicationLauncher::remoteStdout,
            this, &QnxAttachDebugSupport::handleRemoteOutput);
    connect(m_launcher, &ApplicationLauncher::remoteStderr,
            this, &QnxAttachDebugSupport::handleRemoteOutput);
}

void QnxAttachDebugSupport::showProcessesDialog()
{
    auto kitChooser = new KitChooser;
    kitChooser->setKitPredicate([](const Kit *k){
        return k->isValid() && DeviceTypeKitInformation::deviceTypeId(k) == Core::Id(Constants::QNX_QNX_OS_TYPE);
    });

    QnxAttachDebugDialog dlg(kitChooser, 0);
    dlg.addAcceptButton(DeviceProcessesDialog::tr("&Attach to Process"));
    dlg.showAllDevices();
    if (dlg.exec() == QDialog::Rejected)
        return;

    m_kit = kitChooser->currentKit();
    if (!m_kit)
        return;

    m_device = DeviceKitInformation::device(m_kit);
    QTC_ASSERT(m_device, return);
    m_process = dlg.currentProcess();

    m_projectSourceDirectory = dlg.projectSource();
    m_localExecutablePath = dlg.localExecutable();

    m_portsGatherer->start(m_device);
}

void QnxAttachDebugSupport::launchPDebug()
{
    Utils::PortList portList = m_device->freePorts();
    m_pdebugPort = m_portsGatherer->getNextFreePort(&portList);
    if (!m_pdebugPort.isValid()) {
        handleError(tr("No free ports for debugging."));
        return;
    }

    StandardRunnable r;
    r.executable = QLatin1String("pdebug");
    r.commandLineArguments = QString::number(m_pdebugPort.number());
    m_launcher->start(r, m_device);
}

void QnxAttachDebugSupport::attachToProcess()
{
    Debugger::DebuggerStartParameters sp;
    sp.attachPID = Utils::ProcessHandle(m_process.pid);
    sp.startMode = Debugger::AttachToRemoteServer;
    sp.closeMode = Debugger::DetachAtClose;
    sp.connParams.port = m_pdebugPort.number();
    sp.remoteChannel = m_device->sshParameters().host + QLatin1Char(':') +
            QString::number(m_pdebugPort.number());
    sp.displayName = tr("Remote: \"%1:%2\" - Process %3").arg(sp.connParams.host)
            .arg(m_pdebugPort.number()).arg(m_process.pid);
    sp.inferior.executable = m_localExecutablePath;
    sp.useCtrlCStub = true;

    QnxQtVersion *qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitInformation::qtVersion(m_kit));
    if (qtVersion)
        sp.solibSearchPath = QnxUtils::searchPaths(qtVersion);

    QString errorMessage;
    Debugger::DebuggerRunControl *runControl = Debugger::createDebuggerRunControl(sp, 0, &errorMessage);
    if (!errorMessage.isEmpty()) {
        handleError(errorMessage);
        stopPDebug();
        return;
    }
    if (!runControl) {
        handleError(tr("Attaching failed."));
        stopPDebug();
        return;
    }
    connect(runControl, &Debugger::DebuggerRunControl::stateChanged,
            this, &QnxAttachDebugSupport::handleDebuggerStateChanged);
    ProjectExplorerPlugin::startRunControl(runControl, ProjectExplorer::Constants::DEBUG_RUN_MODE);
}

void QnxAttachDebugSupport::handleDebuggerStateChanged(Debugger::DebuggerState state)
{
    if (state == Debugger::DebuggerFinished)
        stopPDebug();
}

void QnxAttachDebugSupport::handleError(const QString &message)
{
    if (m_runControl)
        m_runControl->showMessage(message, Debugger::AppError);
}

void QnxAttachDebugSupport::handleProgressReport(const QString &message)
{
    if (m_runControl)
        m_runControl->showMessage(message + QLatin1Char('\n'), Debugger::AppStuff);
}

void QnxAttachDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    if (m_runControl)
        m_runControl->showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void QnxAttachDebugSupport::stopPDebug()
{
    m_launcher->stop();
}

} // namespace Internal
} // namespace Qnx
