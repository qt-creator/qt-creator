// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericbuildconfiguration.h"
#include "genericmakestep.h"
#include "genericproject.h"
#include "genericprojectfileseditor.h"
#include "genericprojectwizard.h"

#include <extensionsystem/iplugin.h>

namespace GenericProjectManager::Internal {

class GenericProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GenericProjectManager.json")

    void initialize() final
    {
        setupGenericProject(this);
        setupGenericProjectWizard();
        setupGenericProjectFiles();
        setupGenericMakeStep();
        setupGenericBuildConfiguration();
    }
};

} // GenericProjectManager::Internal

#include "genericprojectplugin.moc"
