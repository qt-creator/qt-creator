// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <extensionsystem/iplugin.h>

using namespace ProjectExplorer;

namespace AutotoolsProjectManager::Internal {

/**
 * @brief Implementation of the ProjectExplorer::Project interface.
 *
 * Loads the autotools project and embeds it into the QtCreator project tree.
 * The class AutotoolsProject is the core of the autotools project plugin.
 * It is responsible to parse the Makefile.am files and do trigger project
 * updates if a Makefile.am file or a configure.ac file has been changed.
 */
class AutotoolsProject : public Project
{
public:
    explicit AutotoolsProject(const Utils::FilePath &fileName)
        : Project(Constants::MAKEFILE_MIMETYPE, fileName)
    {
        setId(Constants::AUTOTOOLS_PROJECT_ID);
        setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
        setDisplayName(projectDirectory().fileName());

        setHasMakeInstallEquivalent(true);

        setBuildSystemCreator([](Target *t) { return new AutotoolsBuildSystem(t); });
    }
};

/**
 * @brief Implementation of the ExtensionsSystem::IPlugin interface.
 *
 * The plugin creates the following components:
 *
 * - AutotoolsManager: Will manage the new autotools project and
 *   tell QtCreator for which MIME types the autotools project should
 *   be instantiated.
 *
 * - MakeStepFactory: This factory is used to create make steps.
 *
 * - AutogenStepFactory: This factory is used to create autogen steps.
 *
 * - AutoreconfStepFactory: This factory is used to create autoreconf
 *   steps.
 *
 * - ConfigureStepFactory: This factory is used to create configure steps.
 *
 * - MakefileEditorFactory: Provides a specialized editor with automatic
 *   syntax highlighting for Makefile.am files.
 *
 * - AutotoolsTargetFactory: Our current target is desktop.
 *
 * - AutotoolsBuildConfigurationFactory: Creates build configurations that
 *   contain the steps (make, autogen, autoreconf or configure) that will
 *   be executed in the build process)
 */

class AutotoolsProjectPluginPrivate
{
public:
    AutotoolsBuildConfigurationFactory buildConfigFactory;
    MakeStepFactory makeStepFactory;
    AutogenStepFactory autogenStepFactory;
    ConfigureStepFactory configureStepFactory;
    AutoreconfStepFactory autoreconfStepFactory;
};

class AutotoolsProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AutotoolsProjectManager.json")

    void initialize() final
    {
        ProjectManager::registerProjectType<AutotoolsProject>(Constants::MAKEFILE_MIMETYPE);
        d = std::make_unique<AutotoolsProjectPluginPrivate>();
    }

    std::unique_ptr<AutotoolsProjectPluginPrivate> d;
};

} // AutotoolsProjectManager::Internal

#include "autotoolsprojectplugin.moc"
