/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

namespace BareMetal {
namespace Internal {

BareMetalDebugSupport::BareMetalDebugSupport(
        const ProjectExplorer::IDevice::ConstPtr device,
        Debugger::DebuggerRunControl *runControl)
    : QObject(runControl)
    , m_appRunner(new ProjectExplorer::DeviceApplicationRunner(this))
    , m_runControl(runControl)
    , m_device(device)
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
    auto dev = qSharedPointerCast<const BareMetalDevice>(m_device);
    QTC_ASSERT(dev, return);

    const GdbServerProvider *p = GdbServerProviderManager::instance()->findProvider(
                dev->gdbServerProviderId());
    QTC_ASSERT(p, return);

    m_state = StartingRunner;
    showMessage(tr("Starting GDB server...") + QLatin1Char('\n'), Debugger::LogStatus);

    connect(m_appRunner.data(), &ProjectExplorer::DeviceApplicationRunner::remoteStderr,
            this, &BareMetalDebugSupport::remoteErrorOutputMessage);
    connect(m_appRunner.data(), &ProjectExplorer::DeviceApplicationRunner::remoteStdout,
            this, &BareMetalDebugSupport::remoteOutputMessage);
    connect(m_appRunner.data(), &ProjectExplorer::DeviceApplicationRunner::remoteProcessStarted,
            this, &BareMetalDebugSupport::remoteProcessStarted);
    connect(m_appRunner.data(), &ProjectExplorer::DeviceApplicationRunner::finished,
            this, &BareMetalDebugSupport::appRunnerFinished);
    connect(m_appRunner.data(), &ProjectExplorer::DeviceApplicationRunner::reportProgress,
            this, &BareMetalDebugSupport::progressReport);
    connect(m_appRunner.data(), &ProjectExplorer::DeviceApplicationRunner::reportError,
            this, &BareMetalDebugSupport::appRunnerError);

    const QString cmd = p->executable();
    const QStringList args = p->arguments();
    m_appRunner->start(m_device, cmd, args);
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
