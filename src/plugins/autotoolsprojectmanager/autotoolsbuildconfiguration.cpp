// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autotoolsbuildconfiguration.h"

#include "autotoolsprojectconstants.h"
#include "autotoolsprojectmanagertr.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

// AutotoolsBuildConfiguration

class AutotoolsBuildConfiguration : public BuildConfiguration
{
public:
    AutotoolsBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        // /<foobar> is used so the un-changed check in setBuildDirectory() works correctly.
        // The leading / is to avoid the relative the path expansion in BuildConfiguration::buildDirectory.
        setBuildDirectory("/<foobar>");
        setBuildDirectoryHistoryCompleter("AutoTools.BuildDir.History");
        setConfigWidgetDisplayName(Tr::tr("Autotools Manager"));

        // ### Build Steps Build ###
        const FilePath autogenFile = target->project()->projectDirectory() / "autogen.sh";
        if (autogenFile.exists())
            appendInitialBuildStep(Constants::AUTOGEN_STEP_ID); // autogen.sh
        else
            appendInitialBuildStep(Constants::AUTORECONF_STEP_ID); // autoreconf

        appendInitialBuildStep(Constants::CONFIGURE_STEP_ID); // ./configure.
        appendInitialBuildStep(Constants::MAKE_STEP_ID); // make

        // ### Build Steps Clean ###
        appendInitialCleanStep(Constants::MAKE_STEP_ID); // make clean
    }
};

AutotoolsBuildConfigurationFactory::AutotoolsBuildConfigurationFactory()
{
    registerBuildConfiguration<AutotoolsBuildConfiguration>
            ("AutotoolsProjectManager.AutotoolsBuildConfiguration");

    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::MAKEFILE_MIMETYPE);

    setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
        BuildInfo info;
        info.typeName = ::ProjectExplorer::Tr::tr("Build");
        info.buildDirectory = forSetup
                ? FilePath::fromString(projectPath.toFileInfo().absolutePath()) : projectPath;
        if (forSetup) {
            //: The name of the build configuration created by default for a autotools project.
            info.displayName = ::ProjectExplorer::Tr::tr("Default");
        }
        return QList<BuildInfo>{info};
    });
}

} // AutotoolsProjectManager::Internal
