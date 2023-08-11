// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericbuildconfiguration.h"

#include "genericproject.h"
#include "genericprojectconstants.h"
#include "genericprojectmanagertr.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/aspects.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace GenericProjectManager::Internal {

class GenericBuildConfiguration final : public BuildConfiguration
{
public:
    GenericBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        setConfigWidgetDisplayName(GenericProjectManager::Tr::tr("Generic Manager"));
        setBuildDirectoryHistoryCompleter("Generic.BuildDir.History");

        setInitializer([this](const BuildInfo &) {
            buildSteps()->appendStep(Constants::GENERIC_MS_ID);
            cleanSteps()->appendStep(Constants::GENERIC_MS_ID);
            updateCacheAndEmitEnvironmentChanged();
        });

        updateCacheAndEmitEnvironmentChanged();
    }

    void addToEnvironment(Environment &env) const final
    {
        QtSupport::QtKitAspect::addHostBinariesToPath(kit(), env);
    }
};


// GenericBuildConfigurationFactory

GenericBuildConfigurationFactory::GenericBuildConfigurationFactory()
{
    registerBuildConfiguration<GenericBuildConfiguration>
        ("GenericProjectManager.GenericBuildConfiguration");

    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::GENERICMIMETYPE);

    setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
        BuildInfo info;
        info.typeName = ProjectExplorer::Tr::tr("Build");
        info.buildDirectory = forSetup ? Project::projectDirectory(projectPath) : projectPath;

        if (forSetup)  {
            //: The name of the build configuration created by default for a generic project.
            info.displayName = ProjectExplorer::Tr::tr("Default");
        }

        return QList<BuildInfo>{info};
    });
}

} // GenericProjectManager::Internal
