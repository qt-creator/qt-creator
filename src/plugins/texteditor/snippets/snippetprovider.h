// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include <QString>

#include <functional>

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT SnippetProvider
{
public:
    SnippetProvider() = default;

    using EditorDecorator = std::function<void(TextEditorWidget *)>;

    static const QList<SnippetProvider> &snippetProviders();
    static void registerGroup(const QString &groupId, const QString &displayName,
                              EditorDecorator editorDecorator = EditorDecorator());

    QString groupId() const;
    QString displayName() const;

    static void decorateEditor(TextEditorWidget *editor, const QString &groupId);

private:
    QString m_groupId;
    QString m_displayName;
    EditorDecorator m_editorDecorator;
};

} // TextEditor
