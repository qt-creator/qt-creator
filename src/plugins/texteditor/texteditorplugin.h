// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace TextEditor {
namespace Internal {

class LineNumberFilter;

class TextEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "TextEditor.json")

public:
    TextEditorPlugin();
    ~TextEditorPlugin() final;

    static TextEditorPlugin *instance();
    static LineNumberFilter *lineNumberFilter();

    ShutdownFlag aboutToShutdown() override;

private:
    void initialize() final;
    void extensionsInitialized() final;

    class TextEditorPluginPrivate *d = nullptr;

#ifdef WITH_TESTS
private slots:
    void testSnippetParsing_data();
    void testSnippetParsing();

    void testIndentationClean_data();
    void testIndentationClean();

    void testFormatting_data();
    void testFormatting();

    void testDeletingMarkOnReload();
#endif
};

} // namespace Internal
} // namespace TextEditor
