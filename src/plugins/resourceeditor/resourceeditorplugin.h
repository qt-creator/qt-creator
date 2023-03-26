// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace ResourceEditor::Internal {

class ResourceEditorW;

class ResourceEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ResourceEditor.json")

    ~ResourceEditorPlugin() final;

public:
    void onUndoStackChanged(ResourceEditorW const *editor, bool canUndo, bool canRedo);

private:
    void initialize() final;
    void extensionsInitialized() override;

    class ResourceEditorPluginPrivate *d = nullptr;
};

} // ResourceEditor::Internal
