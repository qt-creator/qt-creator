// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <utils/fileutils.h>

namespace ProjectExplorer { class Project; }

namespace ConanPackageManager {
namespace Internal {

class ConanSettings;

class ConanPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Conan.json")

public:
    static ConanSettings *conanSettings();
    static Utils::FilePath conanFilePath(ProjectExplorer::Project *project,
                           const Utils::FilePath &defaultFilePath = Utils::FilePath());

private:
    ~ConanPlugin() final;
    void projectAdded(ProjectExplorer::Project *project);

    void extensionsInitialized() final;
    bool initialize(const QStringList &arguments, QString *errorString) final;

    class ConanPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace ConanPackageManager
