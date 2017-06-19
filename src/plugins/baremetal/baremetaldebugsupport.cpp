/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
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

#include "baremetaldebugsupport.h"
#include "baremetalrunconfiguration.h"
#include "baremetaldevice.h"
#include "baremetalgdbcommandsdeploystep.h"

#include "gdbserverprovider.h"
#include "gdbserverprovidermanager.h"

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerkitinformation.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

BareMetalDebugSupport::BareMetalDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
    , m_appLauncher(new ProjectExplorer::ApplicationLauncher(this))
{}

BareMetalDebugSupport::~BareMetalDebugSupport()
{
    setFinished();
}

void BareMetalDebugSupport::start()
{
    const auto rc = qobject_cast<BareMetalRunConfiguration *>(runControl()->runConfiguration());
    QTC_ASSERT(rc, reportFailure(); return);

    const QString bin = rc->localExecutableFilePath();
    if (bin.isEmpty()) {
        reportFailure(tr("Cannot debug: Local executable is not set."));
        return;
    }
    if (!QFile::exists(bin)) {
        reportFailure(tr("Cannot debug: Could not find executable for \"%1\".").arg(bin));
        return;
    }

    const Target *target = rc->target();
    QTC_ASSERT(target, reportFailure(); return);

    const Kit *kit = target->kit();
    QTC_ASSERT(kit, reportFailure(); return);

    auto dev = qSharedPointerCast<const BareMetalDevice>(DeviceKitInformation::device(kit));
    if (!dev) {
        reportFailure(tr("Cannot debug: Kit has no device."));
        return;
    }

    const GdbServerProvider *p = GdbServerProviderManager::findProvider(dev->gdbServerProviderId());
    if (!p) {
        reportFailure(tr("Cannot debug: Device has no GDB server provider configuration."));
        return;
    }

    Debugger::DebuggerStartParameters sp;

    if (const BuildConfiguration *bc = target->activeBuildConfiguration()) {
        if (BuildStepList *bsl = bc->stepList(BareMetalGdbCommandsDeployStep::stepId())) {
            foreach (const BareMetalGdbCommandsDeployStep *bs, bsl->allOfType<BareMetalGdbCommandsDeployStep>()) {
                if (!sp.commandsAfterConnect.endsWith("\n"))
                    sp.commandsAfterConnect.append("\n");
                sp.commandsAfterConnect.append(bs->gdbCommands());
            }
        }
    }

    sp.inferior.executable = bin;
    sp.inferior.commandLineArguments = rc->arguments();
    sp.symbolFile = bin;
    sp.startMode = AttachToRemoteServer;
    sp.commandsAfterConnect = p->initCommands();
    sp.commandsForReset = p->resetCommands();
    sp.remoteChannel = p->channel();
    sp.useContinueInsteadOfRun = true;

    setStartParameters(sp);

    connect(runControl(), &RunControl::finished,
            this, &BareMetalDebugSupport::debuggingFinished);

    if (p->startupMode() == GdbServerProvider::StartupOnNetwork) {
        m_state = StartingRunner;
        showMessage(tr("Starting GDB server...") + '\n', Debugger::LogStatus);

        connect(m_appLauncher, &ProjectExplorer::ApplicationLauncher::remoteStderr,
                this, &BareMetalDebugSupport::remoteErrorOutputMessage);
        connect(m_appLauncher, &ProjectExplorer::ApplicationLauncher::remoteStdout,
                this, &BareMetalDebugSupport::remoteOutputMessage);
        connect(m_appLauncher, &ProjectExplorer::ApplicationLauncher::remoteProcessStarted,
                this, &BareMetalDebugSupport::remoteProcessStarted);
        connect(m_appLauncher, &ProjectExplorer::ApplicationLauncher::finished,
                this, &BareMetalDebugSupport::appRunnerFinished);
        connect(m_appLauncher, &ProjectExplorer::ApplicationLauncher::reportProgress,
                this, &BareMetalDebugSupport::progressReport);
        connect(m_appLauncher, &ProjectExplorer::ApplicationLauncher::reportError,
                this, &BareMetalDebugSupport::appRunnerError);

        StandardRunnable r;
        r.executable = p->executable();
        // We need to wrap the command arguments depending on a host OS,
        // as the bare metal's GDB servers are launched on a host,
        // but not on a target.
        r.commandLineArguments = Utils::QtcProcess::joinArgs(p->arguments(), Utils::HostOsInfo::hostOs());
        m_appLauncher->start(r, dev);
    }

    DebuggerRunTool::start();
}

void BareMetalDebugSupport::debuggingFinished()
{
    setFinished();
    reset();
}

void BareMetalDebugSupport::remoteOutputMessage(const QByteArray &output)
{
    QTC_ASSERT(m_state == Inactive || m_state == Running, return);
    showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void BareMetalDebugSupport::remoteErrorOutputMessage(const QByteArray &output)
{
    QTC_ASSERT(m_state == Inactive, return);
    showMessage(QString::fromUtf8(output), Debugger::AppError);

    // FIXME: Should we here parse an outputn?
}

void BareMetalDebugSupport::remoteProcessStarted()
{
    QTC_ASSERT(m_state == StartingRunner, return);
    m_state = Running;
}

void BareMetalDebugSupport::appRunnerFinished(bool success)
{
    if (m_state == Inactive)
        return;

    if (m_state == Running) {
        if (!success)
            notifyInferiorIll();
    } else if (m_state == StartingRunner) {
        reportFailure(tr("Debugging failed."));
    }

    reset();
}

void BareMetalDebugSupport::progressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), Debugger::LogStatus);
}

void BareMetalDebugSupport::appRunnerError(const QString &error)
{
    if (m_state == Running) {
        showMessage(error, Debugger::AppError);
        notifyInferiorIll();
    } else if (m_state != Inactive) {
        adapterSetupFailed(error);
    }
}

void BareMetalDebugSupport::adapterSetupFailed(const QString &error)
{
    debuggingFinished();
    reportFailure(tr("Initial setup failed: %1").arg(error));
}

void BareMetalDebugSupport::setFinished()
{
    if (m_state == Inactive)
        return;
    if (m_state == Running)
        m_appLauncher->stop();
    m_state = Inactive;
}

void BareMetalDebugSupport::reset()
{
    m_appLauncher->disconnect(this);
    m_state = Inactive;
}

} // namespace Internal
} // namespace BareMetal
