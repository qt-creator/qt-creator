// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CppDocument.h>

#include <utils/filepath.h>
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
                   TextEditor::TextDocument *textDocument = nullptr,
                   const CPlusPlus::Document::Ptr &cppDocument = {})
        : m_cursor(cursor)
        , m_filePath(filePath)
        , m_editorWidget(editorWidget)
        , m_textDocument(textDocument)
        , m_cppDocument(cppDocument)
    {}
    CppEditorWidget *editorWidget() const { return m_editorWidget; }
    TextEditor::TextDocument *textDocument() const { return m_textDocument; }
    CPlusPlus::Document::Ptr cppDocument() const { return m_cppDocument; }
    const QTextCursor &cursor() const { return m_cursor; }
    const Utils::FilePath &filePath() const { return m_filePath; }
private:
    QTextCursor m_cursor;
    Utils::FilePath m_filePath;
    CppEditorWidget *m_editorWidget = nullptr;
    TextEditor::TextDocument * const m_textDocument;
    const CPlusPlus::Document::Ptr m_cppDocument;
};

} // namespace CppEditor
