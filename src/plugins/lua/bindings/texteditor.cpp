// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include "sol/sol.hpp"

namespace Lua::Internal {

class TextEditorRegistry : public QObject
{
    Q_OBJECT

public:
    static TextEditorRegistry *instance()
    {
        static TextEditorRegistry *instance = new TextEditorRegistry();
        return instance;
    }

    TextEditorRegistry()
    {
        connect(
            Core::EditorManager::instance(),
            &Core::EditorManager::currentEditorChanged,
            this,
            [this](Core::IEditor *editor) {
                if (!editor) {
                    emit currentEditorChanged(nullptr);
                    return;
                }

                if (m_currentTextEditor) {
                    m_currentTextEditor->disconnect(this);
                    m_currentTextEditor->editorWidget()->disconnect(this);
                    m_currentTextEditor->document()->disconnect(this);
                    m_currentTextEditor = nullptr;
                }

                m_currentTextEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);

                if (m_currentTextEditor) {
                    if (!connectTextEditor(m_currentTextEditor))
                        m_currentTextEditor = nullptr;
                }

                emit currentEditorChanged(m_currentTextEditor);
            });
    }

    bool connectTextEditor(TextEditor::BaseTextEditor *editor)
    {
        auto textEditorWidget = editor->editorWidget();
        if (!textEditorWidget)
            return false;

        TextEditor::TextDocument *textDocument = editor->textDocument();
        if (!textDocument)
            return false;

        connect(
            textEditorWidget,
            &TextEditor::TextEditorWidget::cursorPositionChanged,
            this,
            [editor, textEditorWidget, this]() {
                emit currentCursorChanged(editor, textEditorWidget->multiTextCursor());
            });

        connect(
            textDocument,
            &TextEditor::TextDocument::contentsChangedWithPosition,
            this,
            [this, textDocument](int position, int charsRemoved, int charsAdded) {
                emit documentContentsChanged(textDocument, position, charsRemoved, charsAdded);
            });

        return true;
    }

signals:
    void currentEditorChanged(TextEditor::BaseTextEditor *editor);
    void documentContentsChanged(
        TextEditor::TextDocument *document, int position, int charsRemoved, int charsAdded);

    void currentCursorChanged(TextEditor::BaseTextEditor *editor, Utils::MultiTextCursor cursor);

protected:
    QPointer<TextEditor::BaseTextEditor> m_currentTextEditor = nullptr;
};

void addTextEditorModule()
{
    TextEditorRegistry::instance();

    LuaEngine::registerProvider("TextEditor", [](sol::state_view lua) -> sol::object {
        sol::table result = lua.create_table();

        result["currentEditor"] = []() -> TextEditor::BaseTextEditor * {
            return TextEditor::BaseTextEditor::currentTextEditor();
        };

        result.new_usertype<Utils::MultiTextCursor>(
            "MultiTextCursor",
            sol::no_constructor,
            "mainCursor",
            &Utils::MultiTextCursor::mainCursor,
            "cursors",
            &Utils::MultiTextCursor::cursors);

        result.new_usertype<QTextCursor>(
            "TextCursor",
            sol::no_constructor,
            "position",
            &QTextCursor::position,
            "blockNumber",
            &QTextCursor::blockNumber,
            "columnNumber",
            &QTextCursor::columnNumber);

        result.new_usertype<TextEditor::BaseTextEditor>(
            "TextEditor",
            sol::no_constructor,
            "document",
            &TextEditor::BaseTextEditor::textDocument,
            "cursor",
            [](TextEditor::BaseTextEditor *textEditor) {
                return textEditor->editorWidget()->multiTextCursor();
            });

        result.new_usertype<TextEditor::TextDocument>(
            "TextDocument",
            sol::no_constructor,
            "file",
            &TextEditor::TextDocument::filePath,
            "blockAndColumn",
            [](TextEditor::TextDocument *document,
               int position) -> std::optional<std::pair<int, int>> {
                QTextBlock block = document->document()->findBlock(position);
                if (!block.isValid())
                    return std::nullopt;

                int column = position - block.position();

                return std::make_pair(block.blockNumber() + 1, column + 1);
            },
            "blockCount",
            [](TextEditor::TextDocument *document) { return document->document()->blockCount(); });

        return result;
    });

    LuaEngine::registerHook("editors.text.currentChanged", [](sol::function func, QObject *guard) {
        QObject::connect(
            TextEditorRegistry::instance(),
            &TextEditorRegistry::currentEditorChanged,
            guard,
            [func](TextEditor::BaseTextEditor *editor) {
                Utils::expected_str<void> res = LuaEngine::void_safe_call(func, editor);
                QTC_CHECK_EXPECTED(res);
            });
    });

    LuaEngine::registerHook("editors.text.contentsChanged", [](sol::function func, QObject *guard) {
        QObject::connect(
            TextEditorRegistry::instance(),
            &TextEditorRegistry::documentContentsChanged,
            guard,
            [func](TextEditor::TextDocument *document, int position, int charsRemoved, int charsAdded) {
                Utils::expected_str<void> res
                    = LuaEngine::void_safe_call(func, document, position, charsRemoved, charsAdded);
                QTC_CHECK_EXPECTED(res);
            });
    });

    LuaEngine::registerHook("editors.text.cursorChanged", [](sol::function func, QObject *guard) {
        QObject::connect(
            TextEditorRegistry::instance(),
            &TextEditorRegistry::currentCursorChanged,
            guard,
            [func](TextEditor::BaseTextEditor *editor, const Utils::MultiTextCursor &cursor) {
                Utils::expected_str<void> res = LuaEngine::void_safe_call(func, editor, cursor);
                QTC_CHECK_EXPECTED(res);
            });
    });
}

} // namespace Lua::Internal

#include "texteditor.moc"
