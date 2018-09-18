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

#include "remotelinuxcustomcommanddeploymentstep.h"
#include "remotelinuxcustomcommanddeployservice.h"

#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxCustomCommandDeploymentStepPrivate
{
public:
    BaseStringAspect *commandLineAspect;
    RemoteLinuxCustomCommandDeployService service;
};

} // namespace Internal

RemoteLinuxCustomCommandDeploymentStep::RemoteLinuxCustomCommandDeploymentStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    d = new Internal::RemoteLinuxCustomCommandDeploymentStepPrivate;
    d->commandLineAspect = addAspect<BaseStringAspect>();
    d->commandLineAspect->setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
    d->commandLineAspect->setLabelText(tr("Command line:"));
    d->commandLineAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    setDefaultDisplayName(displayName());
}

RemoteLinuxCustomCommandDeploymentStep::~RemoteLinuxCustomCommandDeploymentStep()
{
    delete d;
}

bool RemoteLinuxCustomCommandDeploymentStep::initInternal(QString *error)
{
    d->service.setCommandLine(d->commandLineAspect->value().trimmed());
    return d->service.isDeploymentPossible(error);
}

BuildStepConfigWidget *RemoteLinuxCustomCommandDeploymentStep::createConfigWidget()
{
    return BuildStep::createConfigWidget();
}

AbstractRemoteLinuxDeployService *RemoteLinuxCustomCommandDeploymentStep::deployService() const
{
    return &d->service;
}

Core::Id RemoteLinuxCustomCommandDeploymentStep::stepId()
{
    return "RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep";
}

QString RemoteLinuxCustomCommandDeploymentStep::displayName()
{
    return tr("Run custom remote command");
}

} // namespace RemoteLinux
