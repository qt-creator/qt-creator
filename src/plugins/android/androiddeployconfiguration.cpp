/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddeployconfiguration.h"

#include "androidconstants.h"
#include "androiddeployqtstep.h"
#include "androidmanager.h"
#include "androidqtsupport.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidDeployConfiguration::AndroidDeployConfiguration(Target *parent, Core::Id id)
    : DeployConfiguration(parent, id)
{}

void AndroidDeployConfiguration::initialize()
{
    stepList()->appendStep(new AndroidDeployQtStep(stepList()));
}

AndroidDeployConfigurationFactory::AndroidDeployConfigurationFactory()
{
    registerDeployConfiguration<AndroidDeployConfiguration>
            ("Qt4ProjectManager.AndroidDeployConfiguration2");
    addSupportedTargetDeviceType(Constants::ANDROID_DEVICE_TYPE);
    setDefaultDisplayName(AndroidDeployConfiguration::tr("Deploy to Android device"));
}

QList<QString> AndroidDeployConfigurationFactory::availableBuildTargets(Target *parent) const
{
    if (!parent->project()->id().name().startsWith("QmlProjectManager.QmlProject")) {
        // Avoid tool chain check for QML Project
        Core::Id cxxLangId(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        ToolChain *tc = ToolChainKitInformation::toolChain(parent->kit(), cxxLangId);
        if (!tc || tc->targetAbi().osFlavor() != Abi::AndroidLinuxFlavor)
            return {};
    }

    QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(parent->kit());
    if (!qt || qt->type() != Constants::ANDROIDQT)
        return {};
    return {QString()};
}

} // namespace Internal
} // namespace Android
