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

#include "genericbuildconfiguration.h"

#include "genericmakestep.h"
#include "genericproject.h"
#include "genericprojectconstants.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>


using namespace ProjectExplorer;
using namespace Utils;

namespace GenericProjectManager {
namespace Internal {

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent, Utils::Id id)
    : BuildConfiguration(parent, id)
{
    setConfigWidgetDisplayName(tr("Generic Manager"));
    setBuildDirectoryHistoryCompleter("Generic.BuildDir.History");

    setInitializer([this](const BuildInfo &) {
        buildSteps()->appendStep(Constants::GENERIC_MS_ID);
        cleanSteps()->appendStep(Constants::GENERIC_MS_ID);
        updateCacheAndEmitEnvironmentChanged();
    });

    updateCacheAndEmitEnvironmentChanged();
}


// GenericBuildConfigurationFactory

GenericBuildConfigurationFactory::GenericBuildConfigurationFactory()
{
    registerBuildConfiguration<GenericBuildConfiguration>
        ("GenericProjectManager.GenericBuildConfiguration");

    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::GENERICMIMETYPE);

    setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
        BuildInfo info;
        info.typeName = BuildConfiguration::tr("Build");
        info.buildDirectory = forSetup ? Project::projectDirectory(projectPath) : projectPath;

        if (forSetup)  {
            //: The name of the build configuration created by default for a generic project.
            info.displayName = BuildConfiguration::tr("Default");
        }

        return QList<BuildInfo>{info};
    });
}

void GenericBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    prependCompilerPathToEnvironment(target()->kit(), env);
    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(target()->kit());
    if (qt)
        env.prependOrSetPath(qt->hostBinPath().toString());
}

} // namespace Internal
} // namespace GenericProjectManager
