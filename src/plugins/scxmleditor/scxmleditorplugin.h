// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <extensionsystem/iplugin.h>

namespace ScxmlEditor {
namespace Internal {

class ScxmlEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ScxmlEditor.json")

public:
    ScxmlEditorPlugin() = default;
    ~ScxmlEditorPlugin();

private:
    bool initialize(const QStringList &arguments, QString *errorString) final;
    void extensionsInitialized() final;

    class ScxmlEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace ScxmlEditor
