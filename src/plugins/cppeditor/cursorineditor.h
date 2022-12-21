// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
#include <utils/link.h>

#include <QTextCursor>

#include <functional>

namespace TextEditor { class TextDocument; }

namespace CppEditor {
class CppEditorWidget;

// TODO: Move to a better place.
using RenameCallback = std::function<void(const QString &, const Utils::Links &, int)>;

class CursorInEditor
{
public:
    CursorInEditor(const QTextCursor &cursor, const Utils::FilePath &filePath,
                 CppEditorWidget *editorWidget = nullptr,
                 TextEditor::TextDocument *textDocument = nullptr)
        : m_cursor(cursor)
        , m_filePath(filePath)
        , m_editorWidget(editorWidget)
        , m_textDocument(textDocument)
    {}
    CppEditorWidget *editorWidget() const { return m_editorWidget; }
    TextEditor::TextDocument *textDocument() const { return m_textDocument; }
    const QTextCursor &cursor() const { return m_cursor; }
    const Utils::FilePath &filePath() const { return m_filePath; }
private:
    QTextCursor m_cursor;
    Utils::FilePath m_filePath;
    CppEditorWidget *m_editorWidget = nullptr;
    TextEditor::TextDocument * const m_textDocument;
};

} // namespace CppEditor
