/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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
    , m_state(StoppedState)
    , m_runner(0)
{
    setIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
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
    connect(m_runner, SIGNAL(started()), SLOT(onProcessStarted()));
    connect(m_runner, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onProcessFinished()));
    connect(m_runner, SIGNAL(error(QProcess::ProcessError)), SLOT(onProcessError()));
    m_state = StartingState;
    m_runner->start();
    return true;
}

} // namespace Internal
} // namespace WinRt
