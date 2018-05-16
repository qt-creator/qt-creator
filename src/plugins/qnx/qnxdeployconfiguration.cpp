/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxdeployconfiguration.h"

#include "qnxconstants.h"
#include "qnxdevicefactory.h"

#include <projectexplorer/devicesupport/devicecheckbuildstep.h>
#include <projectexplorer/deploymentdataview.h>

#include <remotelinux/genericdirectuploadstep.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Qnx {
namespace Internal {

QnxDeployConfiguration::QnxDeployConfiguration(Target *target, Core::Id id)
    : DeployConfiguration(target, id)
{
}

void QnxDeployConfiguration::initialize()
{
    stepList()->appendStep(new DeviceCheckBuildStep(stepList()));
    stepList()->appendStep(new RemoteLinuxCheckForFreeDiskSpaceStep(stepList()));
    stepList()->appendStep(new GenericDirectUploadStep(stepList()));
}

NamedWidget *QnxDeployConfiguration::createConfigWidget()
{
    return new DeploymentDataView(target());
}

QnxDeployConfigurationFactory::QnxDeployConfigurationFactory()
{
    registerDeployConfiguration<QnxDeployConfiguration>
            (Constants::QNX_QNX_DEPLOYCONFIGURATION_ID);
    setDefaultDisplayName(QnxDeployConfiguration::tr("Deploy to QNX Device"));
    addSupportedTargetDeviceType(QnxDeviceFactory::deviceType());
}

} // namespace Internal
} // namespace Qnx
