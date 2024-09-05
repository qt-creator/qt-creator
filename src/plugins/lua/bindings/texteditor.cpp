// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luatr.h"

#include <texteditor/basehoverhandler.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>
#include <utils/layoutbuilder.h>
#include <utils/stringutils.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>
#include <QToolBar>

#include "sol/sol.hpp"

using namespace Utils;

namespace {

TextEditor::TextEditorWidget *getSuggestionReadyEditorWidget(TextEditor::TextDocument *document)
{
    const auto textEditor = TextEditor::BaseTextEditor::currentTextEditor();
    if (!textEditor || textEditor->document() != document)
        return nullptr;

    auto *widget = textEditor->editorWidget();
    if (widget->isReadOnly() || widget->multiTextCursor().hasMultipleCursors())
        return nullptr;

    return widget;
}

void addFloatingWidget(TextEditor::BaseTextEditor *editor, QWidget *widget, int position)
{
    widget->setParent(editor->editorWidget()->viewport());
    const auto editorWidget = editor->editorWidget();

    QTextCursor cursor = QTextCursor(editor->textDocument()->document());
    cursor.setPosition(position);
    const QRect cursorRect = editorWidget->cursorRect(cursor);

    QPoint widgetPos = cursorRect.bottomLeft();
    widgetPos.ry() += (cursorRect.top() - cursorRect.bottom()) / 2;

    widget->move(widgetPos);
    widget->show();
}

} // namespace

namespace Lua::Internal {

using TextEditorPtr = QPointer<TextEditor::BaseTextEditor>;
using TextDocumentPtr = QPointer<TextEditor::TextDocument>;

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

    void currentCursorChanged(TextEditor::BaseTextEditor *editor, MultiTextCursor cursor);

protected:
    TextEditorPtr m_currentTextEditor = nullptr;
};

void setupTextEditorModule()
{
    TextEditorRegistry::instance();

    registerProvider("TextEditor", [](sol::state_view lua) -> sol::object {
        sol::table result = lua.create_table();

        result["currentEditor"] = []() -> TextEditorPtr {
            return TextEditor::BaseTextEditor::currentTextEditor();
        };

        result["currentSuggestion"] = []() -> TextEditor::CyclicSuggestion * {
            if (const auto *textEditor = TextEditor::BaseTextEditor::currentTextEditor()) {
                if (const TextEditor::TextEditorWidget *widget = textEditor->editorWidget())
                    return dynamic_cast<TextEditor::CyclicSuggestion *>(widget->currentSuggestion());
            }
            return nullptr;
        };

        result.new_usertype<TextEditor::CyclicSuggestion>("CyclicSuggestion", sol::no_constructor);

        result.new_usertype<MultiTextCursor>(
            "MultiTextCursor",
            sol::no_constructor,
            "mainCursor",
            &MultiTextCursor::mainCursor,
            "cursors",
            &MultiTextCursor::cursors);

        result.new_usertype<QTextCursor>(
            "TextCursor",
            sol::no_constructor,
            "position",
            &QTextCursor::position,
            "blockNumber",
            &QTextCursor::blockNumber,
            "columnNumber",
            &QTextCursor::columnNumber,
            "hasSelection",
            &QTextCursor::hasSelection);

        result.new_usertype<TextEditor::BaseTextEditor>(
            "TextEditor",
            sol::no_constructor,
            "document",
            [](const TextEditorPtr &textEditor) -> TextDocumentPtr {
                QTC_ASSERT(textEditor, throw sol::error("TextEditor is not valid"));
                return textEditor->textDocument();
            },
            "addFloatingWidget",
            sol::overload(
                [](const TextEditorPtr &textEditor, QWidget *widget, int position) {
                    QTC_ASSERT(textEditor, throw sol::error("TextEditor is not valid"));
                    addFloatingWidget(textEditor, widget, position);
                },
                [](const TextEditorPtr &textEditor, Layouting::Widget *widget, int position) {
                    QTC_ASSERT(textEditor, throw sol::error("TextEditor is not valid"));
                    addFloatingWidget(textEditor, widget->emerge(), position);
                },
                [](const TextEditorPtr &textEditor, Layouting::Layout *layout, int position) {
                    QTC_ASSERT(textEditor, throw sol::error("TextEditor is not valid"));
                    addFloatingWidget(textEditor, layout->emerge(), position);
                }),
            "cursor",
            [](const TextEditorPtr &textEditor) {
                QTC_ASSERT(textEditor, throw sol::error("TextEditor is not valid"));
                return textEditor->editorWidget()->multiTextCursor();
            });

        result.new_usertype<TextEditor::CyclicSuggestion::Data>(
            "Suggestion",
            "create",
            [](int start_line,
               int start_character,
               int end_line,
               int end_character,
               const QString &text) -> TextEditor::CyclicSuggestion::Data {
                auto one_based = [](int zero_based) { return zero_based + 1; };
                Text::Position start_pos = {one_based(start_line), start_character};
                Text::Position end_pos = {one_based(end_line), end_character};
                return {Text::Range{start_pos, end_pos}, start_pos, text};
            });

        result.new_usertype<TextEditor::TextDocument>(
            "TextDocument",
            sol::no_constructor,
            "file",
            [](const TextDocumentPtr &document) {
                QTC_ASSERT(document, throw sol::error("TextDocument is not valid"));
                return document->filePath();
            },
            "blockAndColumn",
            [](const TextDocumentPtr &document,
               int position) -> std::optional<std::pair<int, int>> {
                QTC_ASSERT(document, throw sol::error("TextDocument is not valid"));
                QTextBlock block = document->document()->findBlock(position);
                if (!block.isValid())
                    return std::nullopt;

                int column = position - block.position();

                return std::make_pair(block.blockNumber() + 1, column + 1);
            },
            "blockCount",
            [](const TextDocumentPtr &document) {
                QTC_ASSERT(document, throw sol::error("TextDocument is not valid"));
                return document->document()->blockCount();
            },
            "setSuggestions",
            [](const TextDocumentPtr &document, QList<TextEditor::CyclicSuggestion::Data> suggestions) {
                QTC_ASSERT(document, throw sol::error("TextDocument is not valid"));

                if (suggestions.isEmpty())
                    return;

                auto widget = getSuggestionReadyEditorWidget(document);
                if (!widget)
                    return;

                widget->insertSuggestion(
                    std::make_unique<TextEditor::CyclicSuggestion>(suggestions, document->document()));
            });

        return result;
    });

    registerHook("editors.text.currentChanged", [](sol::function func, QObject *guard) {
        QObject::connect(
            TextEditorRegistry::instance(),
            &TextEditorRegistry::currentEditorChanged,
            guard,
            [func](TextEditor::BaseTextEditor *editor) {
                Utils::expected_str<void> res = void_safe_call(func, editor);
                QTC_CHECK_EXPECTED(res);
            });
    });

    registerHook("editors.text.contentsChanged", [](sol::function func, QObject *guard) {
        QObject::connect(
            TextEditorRegistry::instance(),
            &TextEditorRegistry::documentContentsChanged,
            guard,
            [func](TextEditor::TextDocument *document, int position, int charsRemoved, int charsAdded) {
                Utils::expected_str<void> res
                    = void_safe_call(func, document, position, charsRemoved, charsAdded);
                QTC_CHECK_EXPECTED(res);
            });
    });

    registerHook("editors.text.cursorChanged", [](sol::function func, QObject *guard) {
        QObject::connect(
            TextEditorRegistry::instance(),
            &TextEditorRegistry::currentCursorChanged,
            guard,
            [func](TextEditor::BaseTextEditor *editor, const MultiTextCursor &cursor) {
                Utils::expected_str<void> res = void_safe_call(func, editor, cursor);
                QTC_CHECK_EXPECTED(res);
            });
    });
}

} // namespace Lua::Internal

#include "texteditor.moc"
