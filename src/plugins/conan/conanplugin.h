// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <utils/filepath.h>

namespace ProjectExplorer { class Project; }

namespace Conan::Internal {

class ConanSettings;

class ConanPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Conan.json")

public:
    static ConanSettings *conanSettings();
    static Utils::FilePath conanFilePath(ProjectExplorer::Project *project,
                           const Utils::FilePath &defaultFilePath = {});

private:
    ~ConanPlugin() final;
    void projectAdded(ProjectExplorer::Project *project);

    void initialize() final;

    class ConanPluginPrivate *d = nullptr;
};

} // Conan::Internal
