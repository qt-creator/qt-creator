/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbdeployconfigurationfactory.h"

#include "qdbconstants.h"
#include "qdbstopapplicationstep.h"

#include <projectexplorer/deploymentdataview.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <remotelinux/genericdirectuploadstep.h>
#include <remotelinux/makeinstallstep.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>
#include <remotelinux/remotelinuxdeployconfiguration.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Qdb {
namespace Internal {

QdbDeployConfigurationFactory::QdbDeployConfigurationFactory()
{
    setConfigBaseId(Constants::QdbDeployConfigurationId);
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
    setDefaultDisplayName(QCoreApplication::translate("Qdb::Internal::QdbDeployConfiguration",
                                                      "Deploy to Boot2Qt target"));
    setUseDeploymentDataView();

    addInitialStep(RemoteLinux::MakeInstallStep::stepId(), [](Target *target) {
        const Project * const prj = target->project();
        return prj->deploymentKnowledge() == DeploymentKnowledge::Bad
                && prj->hasMakeInstallEquivalent();
    });
    addInitialStep(RemoteLinuxCheckForFreeDiskSpaceStep::stepId());
    addInitialStep(QdbStopApplicationStep::stepId());
    addInitialStep(GenericDirectUploadStep::stepId());
}

} // namespace Internal
} // namespace Qdb
