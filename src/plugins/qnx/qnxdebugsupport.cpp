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
    void start() override
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

    m_portsGatherer = new GdbServerPortsGatherer(runControl);
    m_portsGatherer->setUseGdbServer(isCppDebugging());
    m_portsGatherer->setUseQmlServer(isQmlDebugging());

    auto debuggeeRunner = new QnxDebuggeeRunner(runControl, m_portsGatherer);
    debuggeeRunner->addStartDependency(m_portsGatherer);

    auto slog2InfoRunner = new Slog2InfoRunner(runControl);
    debuggeeRunner->addStartDependency(slog2InfoRunner);

    addStartDependency(debuggeeRunner);
}

void QnxDebugSupport::start()
{
    auto runConfig = qobject_cast<QnxRunConfiguration *>(runControl()->runConfiguration());
    QTC_ASSERT(runConfig, return);
    Target *target = runConfig->target();
    Kit *k = target->kit();

    auto inferior = runConfig->runnable().as<StandardRunnable>();
    inferior.executable = runConfig->remoteExecutableFilePath();
    inferior.commandLineArguments = runConfig->arguments();

    setStartMode(AttachToRemoteServer);
    setCloseMode(KillAtClose);
    setUseCtrlCStub(true);
    setRemoteChannel(m_portsGatherer->gdbServerChannel());
    setQmlServer(m_portsGatherer->qmlServer());
    setInferior(inferior);
    setSolibSearchPath(searchPaths(k));

    DebuggerRunTool::start();
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

QnxAttachDebugSupport::QnxAttachDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setDisplayName("QnxAttachDebugSupport");

    m_portsGatherer = new GdbServerPortsGatherer(runControl);
    m_portsGatherer->setUseGdbServer(isCppDebugging());
    m_portsGatherer->setUseQmlServer(isQmlDebugging());

    m_pdebugRunner = new PDebugRunner(runControl, m_portsGatherer);
    m_pdebugRunner->addStartDependency(m_portsGatherer);

    addStartDependency(m_pdebugRunner);

//    connect(m_launcher, &ApplicationLauncher::remoteProcessStarted,
//            this, &QnxAttachDebugSupport::attachToProcess);
//    connect(m_launcher, &ApplicationLauncher::reportError,
//            this, &QnxAttachDebugSupport::handleError);
//    connect(m_launcher, &ApplicationLauncher::reportProgress,
//            this, &QnxAttachDebugSupport::handleProgressReport);
//    connect(m_launcher, &ApplicationLauncher::remoteStdout,
//            this, &QnxAttachDebugSupport::handleRemoteOutput);
//    connect(m_launcher, &ApplicationLauncher::remoteStderr,
//            this, &QnxAttachDebugSupport::handleRemoteOutput);
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
    auto runConfig = RunConfiguration::startupRunConfiguration();
    if (!runConfig)
        return;

    DeviceProcessItem process = dlg.currentProcess();
    const int pid = process.pid;

    auto runControl = new RunControl(runConfig, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new QnxAttachDebugSupport(runControl);
    debugger->setAttachPid(pid);
    debugger->setSymbolFile(dlg.localExecutable());
//    QString projectSourceDirectory = dlg.projectSource();

//    auto runConfig = qobject_cast<QnxRunConfiguration *>(runControl()->runConfiguration());
//    QTC_ASSERT(runConfig, return);
//    Target *target = runConfig->target();

//    StandardRunnable inferior;
//    inferior.executable = runConfig->remoteExecutableFilePath();
//    inferior.executable = m_localExecutablePath;
//    inferior.commandLineArguments = runConfig->arguments();

    debugger->setStartMode(AttachToRemoteServer);
    debugger->setCloseMode(DetachAtClose);
    debugger->setUseCtrlCStub(true);
//    setInferior(inferior);
//    setRunControlName(tr("Remote: \"%1\" - Process %2").arg(remoteChannel).arg(m_process.pid));
    debugger->setRunControlName(tr("Remote QNX process %1").arg(pid));
    debugger->setSolibSearchPath(searchPaths(kit));

    (void) new QnxAttachDebugSupport(runControl);
    ProjectExplorerPlugin::startRunControl(runControl);
}

void QnxAttachDebugSupport::handleDebuggerStateChanged(Debugger::DebuggerState state)
{
    if (state == Debugger::DebuggerFinished)
        stopPDebug();
}

void QnxAttachDebugSupport::handleError(const QString &message)
{
    showMessage(message, Debugger::AppError);
}

void QnxAttachDebugSupport::handleProgressReport(const QString &message)
{
    showMessage(message, Debugger::AppStuff);
}

void QnxAttachDebugSupport::handleRemoteOutput(const QString &output)
{
    showMessage(output, Debugger::AppOutput);
}

void QnxAttachDebugSupport::stopPDebug()
{
//    m_launcher->stop();
}

void QnxAttachDebugSupport::start()
{
    setRemoteChannel(m_portsGatherer->gdbServerChannel());
    setQmlServer(m_portsGatherer->qmlServer());

    DebuggerRunTool::start();
}

} // namespace Internal
} // namespace Qnx
