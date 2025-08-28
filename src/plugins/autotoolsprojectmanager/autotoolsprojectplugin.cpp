// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autogenstep.h"
#include "autoreconfstep.h"
#include "autotoolsbuildconfiguration.h"
#include "configurestep.h"
#include "makestep.h"

#include <extensionsystem/iplugin.h>

namespace AutotoolsProjectManager::Internal {

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
 * - AutotoolsBuildConfigurationFactory: Creates build configurations that
 *   contain the steps (make, autogen, autoreconf or configure) that will
 *   be executed in the build process)
 */

class AutotoolsProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AutotoolsProjectManager.json")

    void initialize() final
    {
        setupAutotoolsProject();
        setupAutogenStep();
        setupConfigureStep();
        setupAutoreconfStep();
        setupAutotoolsMakeStep();
    }
};

} // AutotoolsProjectManager::Internal

#include "autotoolsprojectplugin.moc"
