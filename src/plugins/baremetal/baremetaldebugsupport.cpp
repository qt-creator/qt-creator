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

#include "gdbserverprovider.h"
#include "gdbserverprovidermanager.h"

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/runnables.h>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

BareMetalDebugSupport::BareMetalDebugSupport(Debugger::DebuggerRunControl *runControl)
    : QObject(runControl)
    , m_appRunner(new ProjectExplorer::DeviceApplicationRunner(this))
    , m_runControl(runControl)
    , m_state(BareMetalDebugSupport::Inactive)
{
    Q_ASSERT(runControl);
    connect(m_runControl.data(), &Debugger::DebuggerRunControl::requestRemoteSetup,
            this, &BareMetalDebugSupport::remoteSetupRequested);
    connect(runControl, &Debugger::DebuggerRunControl::finished,
            this, &BareMetalDebugSupport::debuggingFinished);
}

BareMetalDebugSupport::~BareMetalDebugSupport()
{
    setFinished();
}

void BareMetalDebugSupport::remoteSetupRequested()
{
    QTC_ASSERT(m_state == Inactive, return);
    startExecution();
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
    adapterSetupDone();
}

void BareMetalDebugSupport::appRunnerFinished(bool success)
{
    if (m_state == Inactive)
        return;

    if (m_state == Running) {
        if (!success)
            m_runControl->notifyInferiorIll();
    } else if (m_state == StartingRunner) {
        Debugger::RemoteSetupResult result;
        result.success = false;
        result.reason = tr("Debugging failed.");
        m_runControl->notifyEngineRemoteSetupFinished(result);
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
        m_runControl->notifyInferiorIll();
    } else if (m_state != Inactive) {
        adapterSetupFailed(error);
    }
}

void BareMetalDebugSupport::adapterSetupDone()
{
    m_state = Running;
    Debugger::RemoteSetupResult result;
    result.success = true;
    m_runControl->notifyEngineRemoteSetupFinished(result);
}

void BareMetalDebugSupport::adapterSetupFailed(const QString &error)
{
    debuggingFinished();

    Debugger::RemoteSetupResult result;
    result.success = false;
    result.reason = tr("Initial setup failed: %1").arg(error);
    m_runControl->notifyEngineRemoteSetupFinished(result);
}

void BareMetalDebugSupport::startExecution()
{
    auto dev = qSharedPointerCast<const BareMetalDevice>(m_runControl->device());
    QTC_ASSERT(dev, return);

    const GdbServerProvider *p = GdbServerProviderManager::instance()->findProvider(
                dev->gdbServerProviderId());
    QTC_ASSERT(p, return);

    m_state = StartingRunner;
    showMessage(tr("Starting GDB server...") + QLatin1Char('\n'), Debugger::LogStatus);

    connect(m_appRunner, &ProjectExplorer::DeviceApplicationRunner::remoteStderr,
            this, &BareMetalDebugSupport::remoteErrorOutputMessage);
    connect(m_appRunner, &ProjectExplorer::DeviceApplicationRunner::remoteStdout,
            this, &BareMetalDebugSupport::remoteOutputMessage);
    connect(m_appRunner, &ProjectExplorer::DeviceApplicationRunner::remoteProcessStarted,
            this, &BareMetalDebugSupport::remoteProcessStarted);
    connect(m_appRunner, &ProjectExplorer::DeviceApplicationRunner::finished,
            this, &BareMetalDebugSupport::appRunnerFinished);
    connect(m_appRunner, &ProjectExplorer::DeviceApplicationRunner::reportProgress,
            this, &BareMetalDebugSupport::progressReport);
    connect(m_appRunner, &ProjectExplorer::DeviceApplicationRunner::reportError,
            this, &BareMetalDebugSupport::appRunnerError);

    StandardRunnable r;
    r.executable = p->executable();
    r.commandLineArguments = Utils::QtcProcess::joinArgs(p->arguments(), Utils::OsTypeLinux);
    m_appRunner->start(dev, r);
}

void BareMetalDebugSupport::setFinished()
{
    if (m_state == Inactive)
        return;
    if (m_state == Running)
        m_appRunner->stop();
    m_state = Inactive;
}

void BareMetalDebugSupport::reset()
{
    m_appRunner->disconnect(this);
    m_state = Inactive;
}

void BareMetalDebugSupport::showMessage(const QString &msg, int channel)
{
    if (m_state != Inactive)
        m_runControl->showMessage(msg, channel);
}

} // namespace Internal
} // namespace BareMetal
