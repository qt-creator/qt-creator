// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace ModelEditor {
namespace Internal {

class ModelsManager;

class ModelEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ModelEditor.json")

public:
    ModelEditorPlugin();
    ~ModelEditorPlugin();

    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

    static ModelsManager *modelsManager();

private:
    class ModelEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace ModelEditor
