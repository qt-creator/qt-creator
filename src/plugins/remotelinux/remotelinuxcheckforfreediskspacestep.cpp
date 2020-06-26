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

#include "remotelinuxcheckforfreediskspacestep.h"

#include "remotelinuxcheckforfreediskspaceservice.h"

#include <projectexplorer/projectconfigurationaspects.h>

#include <limits>

using namespace ProjectExplorer;

namespace RemoteLinux {

RemoteLinuxCheckForFreeDiskSpaceStep::RemoteLinuxCheckForFreeDiskSpaceStep
    (BuildStepList *bsl, Utils::Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
{
    setDefaultDisplayName(displayName());

    auto service = createDeployService<RemoteLinuxCheckForFreeDiskSpaceService>();

    auto pathToCheckAspect = addAspect<BaseStringAspect>();
    pathToCheckAspect->setSettingsKey("RemoteLinux.CheckForFreeDiskSpaceStep.PathToCheck");
    pathToCheckAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    pathToCheckAspect->setValue("/");
    pathToCheckAspect->setLabelText(tr("Remote path to check for free space:"));

    auto requiredSpaceAspect = addAspect<BaseIntegerAspect>();
    requiredSpaceAspect->setSettingsKey("RemoteLinux.CheckForFreeDiskSpaceStep.RequiredSpace");
    requiredSpaceAspect->setLabel(tr("Required disk space:"));
    requiredSpaceAspect->setDisplayScaleFactor(1024*1024);
    requiredSpaceAspect->setValue(5*1024*1024);
    requiredSpaceAspect->setSuffix(tr("MB"));
    requiredSpaceAspect->setRange(1, std::numeric_limits<int>::max());

    setInternalInitializer([service, pathToCheckAspect, requiredSpaceAspect] {
        service->setPathToCheck(pathToCheckAspect->value());
        service->setRequiredSpaceInBytes(requiredSpaceAspect->value());
        return CheckResult::success();
    });
}

RemoteLinuxCheckForFreeDiskSpaceStep::~RemoteLinuxCheckForFreeDiskSpaceStep() = default;

Utils::Id RemoteLinuxCheckForFreeDiskSpaceStep::stepId()
{
    return "RemoteLinux.CheckForFreeDiskSpaceStep";
}

QString RemoteLinuxCheckForFreeDiskSpaceStep::displayName()
{
    return tr("Check for free disk space");
}

} // namespace RemoteLinux
