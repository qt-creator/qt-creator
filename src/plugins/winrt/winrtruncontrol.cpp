/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "winrtruncontrol.h"

#include "winrtdevice.h"
#include "winrtrunconfiguration.h"
#include "winrtrunnerhelper.h"

#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/utilsicons.h>

#include <QTimer>

using ProjectExplorer::DeviceKitInformation;
using ProjectExplorer::IDevice;
using ProjectExplorer::RunControl;
using Core::Id;
using ProjectExplorer::Target;

namespace WinRt {
namespace Internal {

WinRtRunControl::WinRtRunControl(WinRtRunConfiguration *runConfiguration, Core::Id mode)
    : RunControl(runConfiguration, mode)
    , m_runConfiguration(runConfiguration)
{
    setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);
}

void WinRtRunControl::start()
{
    if (m_state != StoppedState)
        return;
    if (!startWinRtRunner())
        m_state = StoppedState;
}

RunControl::StopResult WinRtRunControl::stop()
{
    if (m_state == StoppedState)
        return StoppedSynchronously;

    m_runner->stop();
    return AsynchronousStop;
}

bool WinRtRunControl::isRunning() const
{
    return m_state == StartedState;
}

void WinRtRunControl::onProcessStarted()
{
    QTC_CHECK(m_state == StartingState);
    m_state = StartedState;
    emit started();
}

void WinRtRunControl::onProcessFinished()
{
    QTC_CHECK(m_state == StartedState);
    onProcessError();
}

void WinRtRunControl::onProcessError()
{
    QTC_ASSERT(m_runner, return);
    m_runner->disconnect();
    m_runner->deleteLater();
    m_runner = 0;
    m_state = StoppedState;
    emit finished();
}

bool WinRtRunControl::startWinRtRunner()
{
    QTC_ASSERT(!m_runner, return false);
    m_runner = new WinRtRunnerHelper(this);
    connect(m_runner, &WinRtRunnerHelper::started, this, &WinRtRunControl::onProcessStarted);
    connect(m_runner, &WinRtRunnerHelper::finished, this, &WinRtRunControl::onProcessFinished);
    connect(m_runner, &WinRtRunnerHelper::error, this, &WinRtRunControl::onProcessError);
    m_state = StartingState;
    m_runner->start();
    return true;
}

} // namespace Internal
} // namespace WinRt
