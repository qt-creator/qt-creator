// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericbuildconfiguration.h"

#include "genericmakestep.h"
#include "genericproject.h"
#include "genericprojectconstants.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/aspects.h>
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
    QtSupport::QtKitAspect::addHostBinariesToPath(kit(), env);
}

} // namespace Internal
} // namespace GenericProjectManager
