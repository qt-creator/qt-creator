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
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/session.h>
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
    auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitAspect::qtVersion(kit));
    if (!qtVersion)
        return {};

    const QDir pluginDir(qtVersion->pluginPath().toString());
    const QStringList pluginSubDirs = pluginDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QStringList searchPaths;

    for (const QString &dir : pluginSubDirs)
        searchPaths << qtVersion->pluginPath().toString() + '/' + dir;

    searchPaths << qtVersion->libraryPath().toString();
    searchPaths << qtVersion->qnxTarget().pathAppended(qtVersion->cpuDir() + "/lib").toString();
    searchPaths << qtVersion->qnxTarget().pathAppended(qtVersion->cpuDir() + "/usr/lib").toString();

    return searchPaths;
}

// QnxDebuggeeRunner

class QnxDebuggeeRunner : public ProjectExplorer::SimpleTargetRunner
{
public:
    QnxDebuggeeRunner(RunControl *runControl, DebugServerPortsGatherer *portsGatherer)
        : SimpleTargetRunner(runControl)
    {
        setId("QnxDebuggeeRunner");

        setStarter([this, runControl, portsGatherer] {
            Runnable r = runControl->runnable();
            QStringList arguments;
            if (portsGatherer->useGdbServer()) {
                int pdebugPort = portsGatherer->gdbServer().port();
                r.executable = FilePath::fromString(Constants::QNX_DEBUG_EXECUTABLE);
                arguments.append(QString::number(pdebugPort));
            }
            if (portsGatherer->useQmlServer()) {
                arguments.append(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                                portsGatherer->qmlServer()));
            }
            arguments.append(QtcProcess::splitArgs(r.commandLineArguments));
            r.commandLineArguments = QtcProcess::joinArgs(arguments);

            doStart(r, runControl->device());
        });
    }
};


// QnxDebugSupport

QnxDebugSupport::QnxDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setId("QnxDebugSupport");
    appendMessage(tr("Preparing remote side..."), LogMessageFormat);

    setUsePortsGatherer(isCppDebugging(), isQmlDebugging());

    auto debuggeeRunner = new QnxDebuggeeRunner(runControl, portsGatherer());
    debuggeeRunner->addStartDependency(portsGatherer());

    auto slog2InfoRunner = new Slog2InfoRunner(runControl);
    debuggeeRunner->addStartDependency(slog2InfoRunner);

    addStartDependency(debuggeeRunner);

    Kit *k = runControl->kit();

    setStartMode(AttachToRemoteServer);
    setCloseMode(KillAtClose);
    setUseCtrlCStub(true);
    setSolibSearchPath(searchPaths(k));
    if (auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitAspect::qtVersion(k)))
        setSysRoot(qtVersion->qnxTarget());
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

    QString projectSource() const { return m_projectSource->filePath().toString(); }
    FilePath localExecutable() const { return m_localExecutable->filePath(); }

private:
    PathChooser *m_projectSource;
    PathChooser *m_localExecutable;
};


// QnxAttachDebugSupport

class PDebugRunner : public ProjectExplorer::SimpleTargetRunner
{
public:
    PDebugRunner(RunControl *runControl, DebugServerPortsGatherer *portsGatherer)
        : SimpleTargetRunner(runControl)
    {
        setId("PDebugRunner");
        addStartDependency(portsGatherer);

        setStarter([this, runControl, portsGatherer] {
            const int pdebugPort = portsGatherer->gdbServer().port();

            Runnable r;
            r.executable = FilePath::fromString(Constants::QNX_DEBUG_EXECUTABLE);
            r.commandLineArguments = QString::number(pdebugPort);
            doStart(r, runControl->device());
        });
    }
};

QnxAttachDebugSupport::QnxAttachDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setId("QnxAttachDebugSupport");

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
        return k->isValid() && DeviceTypeKitAspect::deviceTypeId(k) == Constants::QNX_QNX_OS_TYPE;
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
    auto startRunConfig = SessionManager::startupRunConfiguration();
    auto runConfig = qobject_cast<QnxRunConfiguration *>(startRunConfig);
    if (!runConfig)
        return;

    DeviceProcessItem process = dlg.currentProcess();
    const int pid = process.pid;
//    QString projectSourceDirectory = dlg.projectSource();
    FilePath localExecutable = dlg.localExecutable();
    if (localExecutable.isEmpty()) {
        if (auto aspect = runConfig->aspect<SymbolFileAspect>())
            localExecutable = aspect->filePath();
    }

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setRunConfiguration(runConfig);
    auto debugger = new QnxAttachDebugSupport(runControl);
    debugger->setStartMode(AttachToRemoteServer);
    debugger->setCloseMode(DetachAtClose);
    debugger->setSymbolFile(localExecutable);
    debugger->setUseCtrlCStub(true);
    debugger->setAttachPid(pid);
//    setRunControlName(tr("Remote: \"%1\" - Process %2").arg(remoteChannel).arg(m_process.pid));
    debugger->setRunControlName(tr("Remote QNX process %1").arg(pid));
    debugger->setSolibSearchPath(searchPaths(kit));
    if (auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitAspect::qtVersion(kit)))
        debugger->setSysRoot(qtVersion->qnxTarget());
    debugger->setUseContinueInsteadOfRun(true);

    ProjectExplorerPlugin::startRunControl(runControl);
}

} // namespace Internal
} // namespace Qnx
