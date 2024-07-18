// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/textdocument.h>

#include "sol/sol.hpp"

namespace Lua::Internal {

class TextDocumentRegistry : public QObject
{
    Q_OBJECT

public:
    static TextDocumentRegistry *instance()
    {
        static TextDocumentRegistry *instance = new TextDocumentRegistry();
        return instance;
    }

    TextDocumentRegistry()
    {
        connect(
            Core::EditorManager::instance(),
            &Core::EditorManager::documentOpened,
            this,
            &TextDocumentRegistry::onDocumentOpened);

        connect(
            Core::EditorManager::instance(),
            &Core::EditorManager::documentClosed,
            this,
            &TextDocumentRegistry::onDocumentClosed);

        connect(
            Core::EditorManager::instance(),
            &Core::EditorManager::currentEditorChanged,
            this,
            [this](Core::IEditor *editor) {
                if (!editor) {
                    emit currentDocumentChanged(nullptr);
                    return;
                }
                auto document = editor->document();
                auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
                if (textDocument) {
                    emit currentDocumentChanged(textDocument);
                    return;
                }
                emit currentDocumentChanged(nullptr);
            });
    }

    void onDocumentOpened(Core::IDocument *document)
    {
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
        if (!textDocument)
            return;

        connect(
            textDocument,
            &TextEditor::TextDocument::contentsChangedWithPosition,
            this,
            [this, textDocument](int position, int charsRemoved, int charsAdded) {
                emit documentContentsChanged(textDocument, position, charsRemoved, charsAdded);
            });

        emit documentOpened(textDocument);
    }

    void onDocumentClosed(Core::IDocument *document)
    {
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
        if (!textDocument)
            return;

        document->disconnect(this);
        emit documentClosed(textDocument);
    }

signals:
    void documentOpened(TextEditor::TextDocument *document);
    void currentDocumentChanged(TextEditor::TextDocument *document);
    void documentClosed(TextEditor::TextDocument *document);
    void documentContentsChanged(
        TextEditor::TextDocument *document, int position, int charsRemoved, int charsAdded);
};

void addTextDocumentsModule()
{
    TextDocumentRegistry::instance();

    LuaEngine::registerProvider("TextDocument", [](sol::state_view lua) -> sol::object {
        sol::table documents = lua.create_table();

        documents.new_usertype<TextEditor::TextDocument>(
            "TextDocument",
            sol::no_constructor,
            "file",
            &TextEditor::TextDocument::filePath,
            "lineAndColumn",
            [](TextEditor::TextDocument *document,
               int position) -> std::optional<std::pair<int, int>> {
                QTextBlock block = document->document()->findBlock(position);
                if (!block.isValid())
                    return std::nullopt;

                int column = position - block.position();

                return std::make_pair(block.blockNumber() + 1, column + 1);
            });

        return documents;
    });

    LuaEngine::registerHook("editors.text.opened", [](sol::function func) {
        QObject::connect(
            TextDocumentRegistry::instance(),
            &TextDocumentRegistry::documentOpened,
            [func](TextEditor::TextDocument *document) {
                Utils::expected_str<void> res = LuaEngine::void_safe_call(func, document);
                QTC_CHECK_EXPECTED(res);
            });
    });

    LuaEngine::registerHook("editors.text.closed", [](sol::function func) {
        QObject::connect(
            TextDocumentRegistry::instance(),
            &TextDocumentRegistry::documentClosed,
            [func](TextEditor::TextDocument *document) {
                Utils::expected_str<void> res = LuaEngine::void_safe_call(func, document);
                QTC_CHECK_EXPECTED(res);
            });
    });

    LuaEngine::registerHook("editors.text.contentsChanged", [](sol::function func) {
        QObject::connect(
            TextDocumentRegistry::instance(),
            &TextDocumentRegistry::documentContentsChanged,
            [func](TextEditor::TextDocument *document, int position, int charsRemoved, int charsAdded) {
                Utils::expected_str<void> res
                    = LuaEngine::void_safe_call(func, document, position, charsRemoved, charsAdded);
                QTC_CHECK_EXPECTED(res);
            });
    });

    LuaEngine::registerHook("editors.text.currentChanged", [](sol::function func) {
        QObject::connect(
            TextDocumentRegistry::instance(),
            &TextDocumentRegistry::currentDocumentChanged,
            [func](TextEditor::TextDocument *document) {
                Utils::expected_str<void> res = LuaEngine::void_safe_call(func, document);
                QTC_CHECK_EXPECTED(res);
            });
    });
}

} // namespace Lua::Internal

#include "textdocument.moc"
