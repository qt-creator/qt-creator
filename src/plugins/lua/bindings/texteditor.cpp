// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luatr.h"

#include <texteditor/basehoverhandler.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>
#include <utils/stringutils.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>
#include <QToolBar>

#include "sol/sol.hpp"

using namespace Utils;

namespace {

class Suggestion
{
public:
    Suggestion(
        Text::Position start,
        Text::Position end,
        Text::Position position,
        const QString &text)
        : m_start(start)
        , m_end(end)
        , m_position(position)
        , m_text(text)
    {}

    Text::Position start() const { return m_start; }
    Text::Position end() const { return m_end; }
    Text::Position position() const { return m_position; }
    QString text() const { return m_text; }

private:
    Text::Position m_start;
    Text::Position m_end;
    Text::Position m_position;
    QString m_text;
};

QTextCursor toTextCursor(QTextDocument *doc, const Text::Position &position)
{
    QTextCursor cursor(doc);
    cursor.setPosition(position.toPositionInDocument(doc));
    return cursor;
}

QTextCursor toSelection(
    QTextDocument *doc, const Text::Position &start, const Text::Position &end)
{
    QTC_ASSERT(doc, return {});
    QTextCursor cursor = toTextCursor(doc, start);
    cursor.setPosition(end.toPositionInDocument(doc), QTextCursor::KeepAnchor);

    return cursor;
}

class CyclicSuggestion : public TextEditor::TextSuggestion
{
public:
    CyclicSuggestion(
        const QList<Suggestion> &suggestions, QTextDocument *origin, int current_suggestion = 0)
        : m_current_suggestion(current_suggestion)
        , m_suggestions(suggestions)
    {
        QTC_ASSERT(current_suggestion < suggestions.size(), return);
        const auto &suggestion = m_suggestions.at(m_current_suggestion);
        const auto start = suggestion.start();
        const auto end = suggestion.end();

        QString text = toTextCursor(origin, start).block().text();
        int length = text.length() - start.column;
        if (start.line == end.line)
            length = end.column - start.column;

        text.replace(start.column, length, suggestion.text());
        document()->setPlainText(text);

        m_start = toTextCursor(origin, suggestion.position());
        m_start.setKeepPositionOnInsert(true);
        setCurrentPosition(m_start.position());
    }

    virtual bool apply() override
    {
        QTC_ASSERT(m_current_suggestion < m_suggestions.size(), return false);
        reset();
        const auto &suggestion = m_suggestions.at(m_current_suggestion);
        QTextCursor cursor = toSelection(m_start.document(), suggestion.start(), suggestion.end());
        cursor.insertText(suggestion.text());
        return true;
    }

    // Returns true if the suggestion was applied completely, false if it was only partially applied.
    virtual bool applyWord(TextEditor::TextEditorWidget *widget) override
    {
        QTC_ASSERT(m_current_suggestion < m_suggestions.size(), return false);
        const auto &suggestion = m_suggestions.at(m_current_suggestion);
        QTextCursor cursor = toSelection(m_start.document(), suggestion.start(), suggestion.end());
        QTextCursor currentCursor = widget->textCursor();
        const QString text = suggestion.text();
        const int startPos = currentCursor.positionInBlock() - cursor.positionInBlock()
                             + (cursor.selectionEnd() - cursor.selectionStart());
        const int next = Utils::endOfNextWord(text, startPos);

        if (next == -1)
            return apply();

        // TODO: Allow adding more than one line
        QString subText = text.mid(startPos, next - startPos);
        subText = subText.left(subText.indexOf('\n'));
        if (subText.isEmpty())
            return false;

        currentCursor.insertText(subText);
        return false;
    }

    virtual void reset() override { m_start.removeSelectedText(); }

    virtual int position() override { return m_start.selectionEnd(); }

    const QList<Suggestion> &suggestions() const { return m_suggestions; }

    int currentSuggestion() const { return m_current_suggestion; }

private:
    int m_current_suggestion;
    QTextCursor m_start;
    QList<Suggestion> m_suggestions;
};

class SuggestionToolTip : public QToolBar
{
public:
    SuggestionToolTip(
        const QList<Suggestion> &suggestions,
        int currentSuggestion,
        TextEditor::TextEditorWidget *editor)
        : m_numberLabel(new QLabel)
        , m_suggestions(suggestions)
        , m_currentSuggestion(std::max(0, std::min<int>(currentSuggestion, suggestions.size() - 1)))
        , m_editor(editor)
    {
        QAction *prev
            = addAction(Utils::Icons::PREV_TOOLBAR.icon(), Lua::Tr::tr("Select Previous Suggestion"));
        prev->setEnabled(m_suggestions.size() > 1);
        addWidget(m_numberLabel);
        QAction *next
            = addAction(Utils::Icons::NEXT_TOOLBAR.icon(), Lua::Tr::tr("Select Next Suggestion"));
        next->setEnabled(m_suggestions.size() > 1);

        auto apply = addAction(Lua::Tr::tr("Apply (%1)").arg(QKeySequence(Qt::Key_Tab).toString()));
        auto applyWord = addAction(Lua::Tr::tr("Apply Word (%1)")
                                       .arg(QKeySequence(QKeySequence::MoveToNextWord).toString()));

        connect(prev, &QAction::triggered, this, &SuggestionToolTip::selectPrevious);
        connect(next, &QAction::triggered, this, &SuggestionToolTip::selectNext);
        connect(apply, &QAction::triggered, this, &SuggestionToolTip::apply);
        connect(applyWord, &QAction::triggered, this, &SuggestionToolTip::applyWord);

        updateLabels();
    }

private:
    void updateLabels()
    {
        m_numberLabel->setText(
            Lua::Tr::tr("%1 of %2").arg(m_currentSuggestion + 1).arg(m_suggestions.count()));
    }

    void selectPrevious()
    {
        m_currentSuggestion = (m_currentSuggestion - 1 + m_suggestions.size())
                              % m_suggestions.size();
        setCurrentSuggestion();
    }

    void selectNext()
    {
        m_currentSuggestion = (m_currentSuggestion + 1) % m_suggestions.size();
        setCurrentSuggestion();
    }

    void setCurrentSuggestion()
    {
        updateLabels();
        if (TextEditor::TextSuggestion *suggestion = m_editor->currentSuggestion())
            suggestion->reset();

        m_editor->insertSuggestion(std::make_unique<CyclicSuggestion>(
            m_suggestions, m_editor->document(), m_currentSuggestion));
    }

    void apply()
    {
        if (TextEditor::TextSuggestion *suggestion = m_editor->currentSuggestion()) {
            if (!suggestion->apply())
                return;
        }
        Utils::ToolTip::hide();
    }

    void applyWord()
    {
        if (TextEditor::TextSuggestion *suggestion = m_editor->currentSuggestion()) {
            if (!suggestion->applyWord(m_editor))
                return;
        }
        Utils::ToolTip::hide();
    }

    QLabel *m_numberLabel;
    QList<Suggestion> m_suggestions;
    int m_currentSuggestion;
    TextEditor::TextEditorWidget *m_editor;
};

class SuggestionHoverHandler final : public TextEditor::BaseHoverHandler
{
public:
    SuggestionHoverHandler() = default;

protected:
    void identifyMatch(
        TextEditor::TextEditorWidget *editorWidget, int pos, ReportPriority report) final
    {
        QScopeGuard cleanup([&] { report(Priority_None); });
        if (!editorWidget->suggestionVisible())
            return;

        QTextCursor cursor(editorWidget->document());
        cursor.setPosition(pos);
        m_block = cursor.block();

        auto *cyclic_suggestion = dynamic_cast<CyclicSuggestion *>(
            TextEditor::TextDocumentLayout::suggestion(m_block));
        if (!cyclic_suggestion || cyclic_suggestion->suggestions().isEmpty())
            return;

        cleanup.dismiss();
        report(Priority_Suggestion);
    }

    void operateTooltip(TextEditor::TextEditorWidget *editorWidget, const QPoint &point) final
    {
        Q_UNUSED(point)

        auto *cyclic_suggestion = dynamic_cast<CyclicSuggestion *>(
            TextEditor::TextDocumentLayout::suggestion(m_block));
        if (!cyclic_suggestion)
            return;

        auto tooltipWidget = new SuggestionToolTip(
            cyclic_suggestion->suggestions(), cyclic_suggestion->currentSuggestion(), editorWidget);

        const QRect cursorRect = editorWidget->cursorRect(editorWidget->textCursor());
        QPoint pos = editorWidget->viewport()->mapToGlobal(cursorRect.topLeft())
                     - Utils::ToolTip::offsetFromPosition();
        pos.ry() -= tooltipWidget->sizeHint().height();
        Utils::ToolTip::show(pos, tooltipWidget, editorWidget);
    }

private:
    QTextBlock m_block;
};

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

} // namespace

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

    void currentCursorChanged(TextEditor::BaseTextEditor *editor, MultiTextCursor cursor);

protected:
    QPointer<TextEditor::BaseTextEditor> m_currentTextEditor = nullptr;
};

void setupTextEditorModule()
{
    TextEditorRegistry::instance();

    registerProvider("TextEditor", [](sol::state_view lua) -> sol::object {
        sol::table result = lua.create_table();

        result["currentEditor"] = []() -> TextEditor::BaseTextEditor * {
            return TextEditor::BaseTextEditor::currentTextEditor();
        };

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
            &TextEditor::BaseTextEditor::textDocument,
            "cursor",
            [](TextEditor::BaseTextEditor *textEditor) {
                return textEditor->editorWidget()->multiTextCursor();
            });

        result.new_usertype<Suggestion>(
            "Suggestion",
            "create",
            [](int start_line,
               int start_character,
               int end_line,
               int end_character,
               const QString &text) -> Suggestion {
                auto one_based = [](int zero_based) { return zero_based + 1; };
                Text::Position start_pos = {one_based(start_line), start_character};
                Text::Position end_pos = {one_based(end_line), end_character};
                return {start_pos, end_pos, start_pos, text};
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
            [](TextEditor::TextDocument *document) { return document->document()->blockCount(); },
            "setSuggestions",
            [](TextEditor::TextDocument *document, QList<Suggestion> suggestions) {
                if (suggestions.isEmpty())
                    return;

                auto widget = getSuggestionReadyEditorWidget(document);
                if (!widget)
                    return;

                widget->insertSuggestion(
                    std::make_unique<CyclicSuggestion>(suggestions, document->document()));

                static SuggestionHoverHandler hover_handler;
                widget->addHoverHandler(&hover_handler);
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
