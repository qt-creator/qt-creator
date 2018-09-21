/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "remotelinuxkillappstep.h"

#include "remotelinuxkillappservice.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QString>

using namespace ProjectExplorer;

namespace RemoteLinux {

RemoteLinuxKillAppStep::RemoteLinuxKillAppStep(BuildStepList *bsl, Core::Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id), m_service(new RemoteLinuxKillAppService(this))
{
    setDefaultDisplayName(displayName());
}

bool RemoteLinuxKillAppStep::initInternal(QString *error)
{
    Q_UNUSED(error);
    Target * const theTarget = target();
    QTC_ASSERT(theTarget, return false);
    RunConfiguration * const rc = theTarget->activeRunConfiguration();
    const QString remoteExe = rc ? rc->runnable().executable : QString();
    m_service->setRemoteExecutable(remoteExe);
    return true;
}

AbstractRemoteLinuxDeployService *RemoteLinuxKillAppStep::deployService() const
{
    return m_service;
}

Core::Id RemoteLinuxKillAppStep::stepId()
{
    return "RemoteLinux.KillAppStep";
}

QString RemoteLinuxKillAppStep::displayName()
{
    return tr("Kill current application instance");
}

} // namespace RemoteLinux
