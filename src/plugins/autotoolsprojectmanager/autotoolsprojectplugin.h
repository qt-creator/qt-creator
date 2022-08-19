// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>

#include <extensionsystem/iplugin.h>

namespace AutotoolsProjectManager {
namespace Internal {

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

class AutotoolsProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AutotoolsProjectManager.json")

    ~AutotoolsProjectPlugin() final;

    void extensionsInitialized() final;
    bool initialize(const QStringList &arguments, QString *errorString) final;

    class AutotoolsProjectPluginPrivate *d;
};

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
    explicit AutotoolsProject(const Utils::FilePath &fileName);
};

} // namespace Internal
} // namespace AutotoolsProjectManager
