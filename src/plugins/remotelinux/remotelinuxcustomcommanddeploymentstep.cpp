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

#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxCustomCommandDeploymentStepPrivate
{
public:
    BaseStringAspect *commandLineAspect;
};

class GenericRemoteLinuxCustomCommandDeploymentStepPrivate
{
public:
    RemoteLinuxCustomCommandDeployService service;
};

} // namespace Internal

using namespace Internal;


AbstractRemoteLinuxCustomCommandDeploymentStep::AbstractRemoteLinuxCustomCommandDeploymentStep
        (BuildStepList *bsl, Core::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    d = new AbstractRemoteLinuxCustomCommandDeploymentStepPrivate;

    d->commandLineAspect = addAspect<BaseStringAspect>();
    d->commandLineAspect->setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
    d->commandLineAspect->setLabelText(tr("Command line:"));
    d->commandLineAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
}

AbstractRemoteLinuxCustomCommandDeploymentStep::~AbstractRemoteLinuxCustomCommandDeploymentStep()
{
    delete d;
}

bool AbstractRemoteLinuxCustomCommandDeploymentStep::initInternal(QString *error)
{
    deployService()->setCommandLine(d->commandLineAspect->value().trimmed());
    return deployService()->isDeploymentPossible(error);
}

BuildStepConfigWidget *AbstractRemoteLinuxCustomCommandDeploymentStep::createConfigWidget()
{
    return BuildStep::createConfigWidget();
}


GenericRemoteLinuxCustomCommandDeploymentStep::GenericRemoteLinuxCustomCommandDeploymentStep(BuildStepList *bsl)
    : AbstractRemoteLinuxCustomCommandDeploymentStep(bsl, stepId())
{
    d = new GenericRemoteLinuxCustomCommandDeploymentStepPrivate;
    setDefaultDisplayName(displayName());
}

GenericRemoteLinuxCustomCommandDeploymentStep::~GenericRemoteLinuxCustomCommandDeploymentStep()
{
    delete d;
}

RemoteLinuxCustomCommandDeployService *GenericRemoteLinuxCustomCommandDeploymentStep::deployService() const
{
    return &d->service;
}

Core::Id GenericRemoteLinuxCustomCommandDeploymentStep::stepId()
{
    return "RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep";
}

QString GenericRemoteLinuxCustomCommandDeploymentStep::displayName()
{
    return tr("Run custom remote command");
}

} // namespace RemoteLinux
