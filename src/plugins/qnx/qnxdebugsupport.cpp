/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxdebugsupport.h"

#include "qnxconstants.h"
#include "qnxdevice.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"
#include "qnxqtversion.h"
#include "qnxutils.h"

#include <coreplugin/icore.h>

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

static QStringList searchPaths(Kit *kit)
{
    auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitInformation::qtVersion(kit));
    if (!qtVersion)
        return {};

    const QDir pluginDir(qtVersion->qmakeProperty("QT_INSTALL_PLUGINS"));
    const QStringList pluginSubDirs = pluginDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QStringList searchPaths;

    for (const QString &dir : pluginSubDirs)
        searchPaths << qtVersion->qmakeProperty("QT_INSTALL_PLUGINS") + '/' + dir;

    searchPaths << qtVersion->qmakeProperty("QT_INSTALL_LIBS");
    searchPaths << qtVersion->qnxTarget() + '/' + qtVersion->cpuDir() + "/lib";
    searchPaths << qtVersion->qnxTarget() + '/' + qtVersion->cpuDir() + "/usr/lib";

    return searchPaths;
}

// QnxDebuggeeRunner

class QnxDebuggeeRunner : public ProjectExplorer::SimpleTargetRunner
{
public:
    QnxDebuggeeRunner(RunControl *runControl, GdbServerPortsGatherer *portsGatherer)
        : SimpleTargetRunner(runControl), m_portsGatherer(portsGatherer)
    {
        setDisplayName("QnxDebuggeeRunner");
    }

private:
    void start() final
    {
        StandardRunnable r = runnable().as<StandardRunnable>();
        QStringList arguments;
        if (m_portsGatherer->useGdbServer()) {
            Port pdebugPort = m_portsGatherer->gdbServerPort();
            r.executable = Constants::QNX_DEBUG_EXECUTABLE;
            arguments.append(pdebugPort.toString());
        }
        if (m_portsGatherer->useQmlServer()) {
            arguments.append(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                            m_portsGatherer->qmlServerPort()));
        }
        arguments.append(QtcProcess::splitArgs(r.commandLineArguments));
        r.commandLineArguments = QtcProcess::joinArgs(arguments);

        setRunnable(r);

        SimpleTargetRunner::start();
    }

    GdbServerPortsGatherer *m_portsGatherer;
};


// QnxDebugSupport

QnxDebugSupport::QnxDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setDisplayName("QnxDebugSupport");
    appendMessage(tr("Preparing remote side..."), LogMessageFormat);

    setUsePortsGatherer(isCppDebugging(), isQmlDebugging());

    auto debuggeeRunner = new QnxDebuggeeRunner(runControl, portsGatherer());
    debuggeeRunner->addStartDependency(portsGatherer());

    auto slog2InfoRunner = new Slog2InfoRunner(runControl);
    debuggeeRunner->addStartDependency(slog2InfoRunner);

    addStartDependency(debuggeeRunner);

    auto runConfig = qobject_cast<QnxRunConfiguration *>(runControl->runConfiguration());
    QTC_ASSERT(runConfig, return);
    Target *target = runConfig->target();
    Kit *k = target->kit();

    setStartMode(AttachToRemoteServer);
    setCloseMode(KillAtClose);
    setUseCtrlCStub(true);
    setSolibSearchPath(searchPaths(k));
    if (auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitInformation::qtVersion(k)))
        setSysRoot(qtVersion->qnxTarget());
    setSymbolFile(runConfig->localExecutableFilePath());
}


// QnxAttachDebugDialog

class QnxAttachDebugDialog : public DeviceProcessesDialog
{
public:
    QnxAttachDebugDialog(KitChooser *kitChooser)
        : DeviceProcessesDialog(kitChooser, Core::ICore::dialogParent())
    {
        auto sourceLabel = new QLabel(QnxDebugSupport::tr("Project source directory:"), this);
        m_projectSource = new PathChooser(this);
        m_projectSource->setExpectedKind(PathChooser::ExistingDirectory);

        auto binaryLabel = new QLabel(QnxDebugSupport::tr("Local executable:"), this);
        m_localExecutable = new PathChooser(this);
        m_localExecutable->setExpectedKind(PathChooser::File);

        auto formLayout = new QFormLayout;
        formLayout->addRow(sourceLabel, m_projectSource);
        formLayout->addRow(binaryLabel, m_localExecutable);

        auto mainLayout = dynamic_cast<QVBoxLayout*>(layout());
        QTC_ASSERT(mainLayout, return);
        mainLayout->insertLayout(mainLayout->count() - 2, formLayout);
    }

    QString projectSource() const { return m_projectSource->path(); }
    QString localExecutable() const { return m_localExecutable->path(); }

private:
    PathChooser *m_projectSource;
    PathChooser *m_localExecutable;
};


// QnxAttachDebugSupport

class PDebugRunner : public ProjectExplorer::SimpleTargetRunner
{
public:
    PDebugRunner(RunControl *runControl, GdbServerPortsGatherer *portsGatherer)
        : SimpleTargetRunner(runControl), m_portsGatherer(portsGatherer)
    {
        setDisplayName("PDebugRunner");
        addStartDependency(m_portsGatherer);
    }

private:
    void start() final
    {
        Port pdebugPort = m_portsGatherer->gdbServerPort();

        StandardRunnable r;
        r.executable = Constants::QNX_DEBUG_EXECUTABLE;
        r.commandLineArguments = pdebugPort.toString();
        setRunnable(r);

        SimpleTargetRunner::start();
    }

    GdbServerPortsGatherer *m_portsGatherer;
};

QnxAttachDebugSupport::QnxAttachDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setDisplayName("QnxAttachDebugSupport");

    setUsePortsGatherer(isCppDebugging(), isQmlDebugging());

    if (isCppDebugging()) {
        auto pdebugRunner = new PDebugRunner(runControl, portsGatherer());
        addStartDependency(pdebugRunner);
    }
}

void QnxAttachDebugSupport::showProcessesDialog()
{
    auto kitChooser = new KitChooser;
    kitChooser->setKitPredicate([](const Kit *k) {
        return k->isValid() && DeviceTypeKitInformation::deviceTypeId(k) == Constants::QNX_QNX_OS_TYPE;
    });

    QnxAttachDebugDialog dlg(kitChooser);
    dlg.addAcceptButton(DeviceProcessesDialog::tr("&Attach to Process"));
    dlg.showAllDevices();
    if (dlg.exec() == QDialog::Rejected)
        return;

    Kit *kit = kitChooser->currentKit();
    if (!kit)
        return;

    // FIXME: That should be somehow related to the selected kit.
    auto startRunConfig = RunConfiguration::startupRunConfiguration();
    auto runConfig = qobject_cast<QnxRunConfiguration *>(startRunConfig);
    if (!runConfig)
        return;

    DeviceProcessItem process = dlg.currentProcess();
    const int pid = process.pid;
//    QString projectSourceDirectory = dlg.projectSource();
    QString localExecutable = dlg.localExecutable();
    if (localExecutable.isEmpty())
        localExecutable = runConfig->localExecutableFilePath();

    auto runControl = new RunControl(runConfig, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new QnxAttachDebugSupport(runControl);
    debugger->setStartMode(AttachToRemoteServer);
    debugger->setCloseMode(DetachAtClose);
    debugger->setSymbolFile(localExecutable);
    debugger->setUseCtrlCStub(true);
    debugger->setAttachPid(pid);
//    setRunControlName(tr("Remote: \"%1\" - Process %2").arg(remoteChannel).arg(m_process.pid));
    debugger->setRunControlName(tr("Remote QNX process %1").arg(pid));
    debugger->setSolibSearchPath(searchPaths(kit));
    if (auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitInformation::qtVersion(kit)))
        debugger->setSysRoot(qtVersion->qnxTarget());
    debugger->setUseContinueInsteadOfRun(true);

    ProjectExplorerPlugin::startRunControl(runControl);
}

} // namespace Internal
} // namespace Qnx
