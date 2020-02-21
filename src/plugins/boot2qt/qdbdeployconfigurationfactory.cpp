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

#include <projectexplorer/deploymentdataview.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinux_constants.h>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

QdbDeployConfigurationFactory::QdbDeployConfigurationFactory()
{
    setConfigBaseId(Constants::QdbDeployConfigurationId);
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
    setDefaultDisplayName(QCoreApplication::translate("Qdb::Internal::QdbDeployConfiguration",
                                                      "Deploy to Boot2Qt target"));
    setUseDeploymentDataView();

    addInitialStep(RemoteLinux::Constants::MakeInstallStepId, [](Target *target) {
        const Project * const prj = target->project();
        return prj->deploymentKnowledge() == DeploymentKnowledge::Bad
                && prj->hasMakeInstallEquivalent();
    });
    addInitialStep(RemoteLinux::Constants::CheckForFreeDiskSpaceId);
    addInitialStep(Qdb::Constants::QdbStopApplicationStepId);
    addInitialStep(RemoteLinux::Constants::DirectUploadStepId);
}

} // namespace Internal
} // namespace Qdb
