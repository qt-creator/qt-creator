/**************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company
** Contact: info@kdab.com
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxAttachDebugSupport::QnxAttachDebugSupport(QObject *parent)
    : QObject(parent)
    , m_kit(0)
    , m_runControl(0)
    , m_pdebugPort(-1)
{
    m_runner = new ProjectExplorer::DeviceApplicationRunner(this);
    m_portsGatherer = new ProjectExplorer::DeviceUsedPortsGatherer(this);

    connect(m_portsGatherer, SIGNAL(portListReady()), this, SLOT(launchPDebug()));
    connect(m_portsGatherer, SIGNAL(error(QString)), this, SLOT(handleError(QString)));
    connect(m_runner, SIGNAL(remoteProcessStarted()), this, SLOT(attachToProcess()));
    connect(m_runner, SIGNAL(reportError(QString)), this, SLOT(handleError(QString)));
    connect(m_runner, SIGNAL(reportProgress(QString)), this, SLOT(handleProgressReport(QString)));
    connect(m_runner, SIGNAL(remoteStdout(QByteArray)), this, SLOT(handleRemoteOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteStderr(QByteArray)), this, SLOT(handleRemoteOutput(QByteArray)));
}

void QnxAttachDebugSupport::showProcessesDialog()
{
    auto kitChooser = new ProjectExplorer::KitChooser;
    kitChooser->setKitMatcher([](const ProjectExplorer::Kit *k){
        return k->isValid() && ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k) == Core::Id(Constants::QNX_QNX_OS_TYPE);
    });

    QnxAttachDebugDialog dlg(kitChooser, 0);
    dlg.addAcceptButton(ProjectExplorer::DeviceProcessesDialog::tr("&Attach to Process"));
    dlg.showAllDevices();
    if (dlg.exec() == QDialog::Rejected)
        return;

    m_kit = kitChooser->currentKit();
    if (!m_kit)
        return;

    m_device = ProjectExplorer::DeviceKitInformation::device(m_kit);
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
    if (m_pdebugPort == -1) {
        handleError(tr("No free ports for debugging."));
        return;
    }

    const QString remoteCommand = QLatin1String("pdebug");
    QStringList arguments;
    arguments << QString::number(m_pdebugPort);
    m_runner->start(m_device, remoteCommand, arguments);
}

void QnxAttachDebugSupport::attachToProcess()
{
    Debugger::DebuggerStartParameters sp;
    sp.attachPID = m_process.pid;
    sp.startMode = Debugger::AttachToRemoteServer;
    sp.closeMode = Debugger::DetachAtClose;
    sp.connParams.port = m_pdebugPort;
    sp.remoteChannel = m_device->sshParameters().host + QLatin1Char(':') + QString::number(m_pdebugPort);
    sp.displayName = tr("Remote: \"%1:%2\" - Process %3").arg(sp.connParams.host).arg(m_pdebugPort).arg(m_process.pid);
    sp.executable = m_localExecutablePath;
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
    connect(runControl, &Debugger::DebuggerRunControl::stateChanged,
            this, &QnxAttachDebugSupport::handleDebuggerStateChanged);
    ProjectExplorer::ProjectExplorerPlugin::startRunControl(runControl, ProjectExplorer::Constants::DEBUG_RUN_MODE);
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
    m_runner->stop();
}
