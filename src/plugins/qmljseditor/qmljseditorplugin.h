// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace QmlJS { class JsonSchemaManager; }

namespace QmlJSEditor {
class QuickToolBar;
namespace Internal {

class QmlJSQuickFixAssistProvider;

class QmlJSEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlJSEditor.json")

public:
    QmlJSEditorPlugin();
    ~QmlJSEditorPlugin() final;

    static QmlJSQuickFixAssistProvider *quickFixAssistProvider();
    static QmlJS::JsonSchemaManager *jsonManager();

private:
    void initialize() final;
    void extensionsInitialized() final;

    class QmlJSEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace QmlJSEditor
