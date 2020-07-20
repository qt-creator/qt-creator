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

RemoteLinuxCustomCommandDeploymentStep::RemoteLinuxCustomCommandDeploymentStep
        (BuildStepList *bsl, Utils::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<RemoteLinuxCustomCommandDeployService>();

    auto commandLine = addAspect<BaseStringAspect>();
    commandLine->setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
    commandLine->setLabelText(tr("Command line:"));
    commandLine->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    commandLine->setHistoryCompleter("RemoteLinuxCustomCommandDeploymentStep.History");

    setDefaultDisplayName(displayName());

    setInternalInitializer([service, commandLine] {
        service->setCommandLine(commandLine->value().trimmed());
        return service->isDeploymentPossible();
    });

    addMacroExpander();
}

RemoteLinuxCustomCommandDeploymentStep::~RemoteLinuxCustomCommandDeploymentStep() = default;

Utils::Id RemoteLinuxCustomCommandDeploymentStep::stepId()
{
    return "RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep";
}

QString RemoteLinuxCustomCommandDeploymentStep::displayName()
{
    return tr("Run custom remote command");
}

} // namespace RemoteLinux
