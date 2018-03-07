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

#include "iosrunfactories.h"

#include "iosconstants.h"
#include "iosrunconfiguration.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

IosRunConfigurationFactory::IosRunConfigurationFactory(QObject *parent)
    : QmakeRunConfigurationFactory(parent)
{
    setObjectName("IosRunConfigurationFactory");
    registerRunConfiguration<IosRunConfiguration>(Constants::IOS_RC_ID_PREFIX);
    setSupportedTargetDeviceTypes({Constants::IOS_DEVICE_TYPE, Constants::IOS_SIMULATOR_TYPE});
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
}

bool IosRunConfigurationFactory::canCreateHelper(Target *parent, const QString &buildTarget) const
{
    const QList<BuildTargetInfo> buildTargets = availableBuildTargets(parent, UserCreate);
    return Utils::contains(buildTargets, [buildTarget](const BuildTargetInfo &bti) {
        return bti.targetName == buildTarget;
    });
}

QList<BuildTargetInfo>
    IosRunConfigurationFactory::availableBuildTargets(Target *parent, CreationMode mode) const
{
    auto project = qobject_cast<QmakeProject *>(parent->project());
    QTC_ASSERT(project, return {});
    return project->buildTargets(mode, {ProjectType::ApplicationTemplate,
                                        ProjectType::SharedLibraryTemplate,
                                        ProjectType::AuxTemplate});
}

bool IosRunConfigurationFactory::hasRunConfigForProFile(RunConfiguration *rc, const Utils::FileName &n) const
{
    auto iosRc = qobject_cast<IosRunConfiguration *>(rc);
    return iosRc && iosRc->profilePath() == n;
}

} // namespace Internal
} // namespace Ios
