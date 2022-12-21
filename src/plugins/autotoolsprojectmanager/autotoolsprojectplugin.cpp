// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autotoolsprojectplugin.h"

#include "autogenstep.h"
#include "autoreconfstep.h"
#include "autotoolsbuildconfiguration.h"
#include "autotoolsbuildsystem.h"
#include "autotoolsprojectconstants.h"
#include "configurestep.h"
#include "makestep.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

namespace AutotoolsProjectManager::Internal {

/**
 * @brief Implementation of the ProjectExplorer::Project interface.
 *
 * Loads the autotools project and embeds it into the QtCreator project tree.
 * The class AutotoolsProject is the core of the autotools project plugin.
 * It is responsible to parse the Makefile.am files and do trigger project
 * updates if a Makefile.am file or a configure.ac file has been changed.
 */
class AutotoolsProject : public ProjectExplorer::Project
{
public:
    explicit AutotoolsProject(const Utils::FilePath &fileName)
        : Project(Constants::MAKEFILE_MIMETYPE, fileName)
    {
        setId(Constants::AUTOTOOLS_PROJECT_ID);
        setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
        setDisplayName(projectDirectory().fileName());

        setHasMakeInstallEquivalent(true);

        setBuildSystemCreator([](ProjectExplorer::Target *t) { return new AutotoolsBuildSystem(t); });
    }
};


class AutotoolsProjectPluginPrivate
{
public:
    AutotoolsBuildConfigurationFactory buildConfigurationFactory;
    MakeStepFactory makeStepFaactory;
    AutogenStepFactory autogenStepFactory;
    ConfigureStepFactory configureStepFactory;
    AutoreconfStepFactory autoreconfStepFactory;
};

AutotoolsProjectPlugin::~AutotoolsProjectPlugin()
{
    delete d;
}

void AutotoolsProjectPlugin::extensionsInitialized()
{ }

bool AutotoolsProjectPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new AutotoolsProjectPluginPrivate;
    ProjectExplorer::ProjectManager::registerProjectType<AutotoolsProject>(Constants::MAKEFILE_MIMETYPE);

    return true;
}

} // AutotoolsProjectManager::Internal
