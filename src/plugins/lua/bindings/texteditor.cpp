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

class CyclicSuggestion : public QObject, public TextEditor::TextSuggestion
{
    Q_OBJECT

public:
    CyclicSuggestion(
        const QList<TextEditor::CyclicSuggestion::Data> &suggestions,
        TextEditor::TextDocument *origin_document,
        int current_suggestion = 0,
        bool is_locked = false)
        : m_currentSuggestion(current_suggestion)
        , m_suggestions(suggestions)
        , m_originDocument(origin_document)
        , m_locked(is_locked)
    {
        QTC_ASSERT(current_suggestion < m_suggestions.size(), return);
        const auto &suggestion = m_suggestions.at(m_currentSuggestion);
        const auto start = suggestion.range.begin;
        const auto end = suggestion.range.end;

        QString text = start.toTextCursor(origin_document->document()).block().text();
        int length = text.length() - start.column;
        if (start.line == end.line)
            length = end.column - start.column;

        text.replace(start.column, length, suggestion.text);
        document()->setPlainText(text);

        m_start = suggestion.position.toTextCursor(origin_document->document());
        m_start.setKeepPositionOnInsert(true);
        setCurrentPosition(m_start.position());

        connect(
            origin_document,
            &::TextEditor::TextDocument::contentsChangedWithPosition,
            this,
            &CyclicSuggestion::documentChanged);
    }

    virtual bool apply() override
    {
        QTC_ASSERT(m_currentSuggestion < m_suggestions.size(), return false);
        reset();
        const auto &suggestion = m_suggestions.at(m_currentSuggestion);
        QTextCursor cursor = suggestion.range.toTextCursor(m_start.document());
        cursor.insertText(suggestion.text);
        return true;
    }

    // Returns true if the suggestion was applied completely, false if it was only partially applied.
    virtual bool applyWord(TextEditor::TextEditorWidget *widget) override
    {
        QTC_ASSERT(m_currentSuggestion < m_suggestions.size(), return true);

        lockCurrentSuggestion();
        const auto &suggestion = m_suggestions.at(m_currentSuggestion);
        QTextCursor cursor = suggestion.range.toTextCursor(m_start.document());
        QTextCursor currentCursor = widget->textCursor();
        const QString text = suggestion.text;
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

    virtual bool applyLine(TextEditor::TextEditorWidget * /*widget*/) override { return false; }

    virtual void reset() override { m_start.removeSelectedText(); }

    qsizetype size() const { return m_suggestions.size(); }

    bool isEmpty() const { return m_suggestions.isEmpty(); }

    bool isLocked() const { return m_locked; }

    int currentSuggestion() const { return m_currentSuggestion; }

    void selectPrevious()
    {
        if (m_suggestions.size() <= 1)
            return;

        m_currentSuggestion = (m_currentSuggestion - 1 + m_suggestions.size())
                              % m_suggestions.size();
        emit update();
        refreshTextEditorSuggestion();
    }

    void selectNext()
    {
        if (m_suggestions.size() <= 1)
            return;

        m_currentSuggestion = (m_currentSuggestion + 1) % m_suggestions.size();
        emit update();
        refreshTextEditorSuggestion();
    }

signals:
    void update();

private slots:
    void documentChanged(int /* position */, int /* charsRemoved */, int /* charsAdded */)
    {
        // When the document is changed, the suggestion will be either destroyed or must be locked.
        if (!m_locked)
            lockCurrentSuggestion();
    }

private:
    // Be causious with this function, it should be the last called in the chain
    // since it replaces this object.
    // Potential alternative solution: implement update on TextDocument site if possible.
    void refreshTextEditorSuggestion()
    {
        // Reinserting itself for update purpose.
        m_originDocument->insertSuggestion(std::make_unique<CyclicSuggestion>(
            m_suggestions, m_originDocument, m_currentSuggestion, m_locked));
    }

    void lockCurrentSuggestion()
    {
        m_locked = true;
        if (m_suggestions.size() <= 1) {
            emit update();
            return;
        }

        m_suggestions = QList<TextEditor::CyclicSuggestion::Data>{m_suggestions.at(m_currentSuggestion)};
        m_currentSuggestion = 0;
        emit update();
    }

private:
    int m_currentSuggestion;
    QTextCursor m_start;
    QList<TextEditor::CyclicSuggestion::Data> m_suggestions;
    TextEditor::TextDocument *m_originDocument;
    bool m_locked = false;
}; // class CyclicSuggestion

class SuggestionToolTip : public QToolBar
{
public:
    SuggestionToolTip(QTextBlock block, TextEditor::TextEditorWidget *editor)
        : m_numberLabel(new QLabel)
        , m_editor(editor)
        , m_block(block)
    {
        auto suggestion = currentSuggestion();
        QTC_ASSERT(suggestion, return);
        const auto suggestions_count = suggestion->size();

        m_prev
            = addAction(Utils::Icons::PREV_TOOLBAR.icon(), Lua::Tr::tr("Select Previous Suggestion"));
        m_prev->setEnabled(suggestions_count > 1);
        addWidget(m_numberLabel);
        m_next = addAction(Utils::Icons::NEXT_TOOLBAR.icon(), Lua::Tr::tr("Select Next Suggestion"));
        m_next->setEnabled(suggestions_count > 1);

        auto apply = addAction(Lua::Tr::tr("Apply (%1)").arg(QKeySequence(Qt::Key_Tab).toString()));
        auto applyWord = addAction(Lua::Tr::tr("Apply Word (%1)")
                                       .arg(QKeySequence(QKeySequence::MoveToNextWord).toString()));

        connect(m_prev, &QAction::triggered, [this]() {
            if (auto cs = currentSuggestion())
                cs->selectPrevious();
        });
        connect(m_next, &QAction::triggered, [this]() {
            if (auto cs = currentSuggestion())
                cs->selectNext();
        });
        connect(apply, &QAction::triggered, this, &SuggestionToolTip::apply);
        connect(applyWord, &QAction::triggered, this, &SuggestionToolTip::applyWord);

        updateLabels();
    }

private:
    CyclicSuggestion *currentSuggestion()
    {
        CyclicSuggestion *cyclic_suggestion = dynamic_cast<CyclicSuggestion *>(
            TextEditor::TextDocumentLayout::suggestion(m_block));

        if (!cyclic_suggestion)
            return nullptr;

        if (cyclic_suggestion != m_connectedSuggestion) {
            connect(
                cyclic_suggestion,
                &CyclicSuggestion::update,
                this,
                &SuggestionToolTip::updateLabels);
            m_connectedSuggestion = cyclic_suggestion;
        }

        return cyclic_suggestion;
    }

    void updateLabels()
    {
        if (auto cs = currentSuggestion()) {
            if (cs->isLocked())
                m_numberLabel->setText("         ");
            else
                m_numberLabel->setText(
                    Lua::Tr::tr("%1 of %2").arg(cs->currentSuggestion() + 1).arg(cs->size()));
        }
    }

    void apply()
    {
        if (auto cs = currentSuggestion()) {
            if (!cs->apply())
                return;
        }

        Utils::ToolTip::hide();
    }

    void lockOnSingleSuggestion()
    {
        m_prev->setEnabled(false);
        m_next->setEnabled(false);
    }

    void applyWord()
    {
        if (auto cs = currentSuggestion()) {
            if (cs->size() > 1)
                lockOnSingleSuggestion();

            if (!cs->applyWord(m_editor))
                return;
        }

        Utils::ToolTip::hide();
    }

    QLabel *m_numberLabel;
    TextEditor::TextEditorWidget *m_editor;
    QAction *m_prev = nullptr;
    QAction *m_next = nullptr;
    QTextBlock m_block;
    CyclicSuggestion *m_connectedSuggestion = nullptr;
}; // class SuggestionToolTip

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
        if (!cyclic_suggestion || cyclic_suggestion->isEmpty())
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

        auto tooltipWidget = new SuggestionToolTip(m_block, editorWidget);
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

        result["currentSuggestion"] = []() -> CyclicSuggestion * {
            if (const auto *textEditor = TextEditor::BaseTextEditor::currentTextEditor()) {
                if (const TextEditor::TextEditorWidget *widget = textEditor->editorWidget())
                    return dynamic_cast<CyclicSuggestion *>(widget->currentSuggestion());
            }
            return nullptr;
        };

        result.new_usertype<CyclicSuggestion>(
            "CyclicSuggestion", sol::no_constructor, "isLocked", &CyclicSuggestion::isLocked);

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

                static SuggestionHoverHandler hover_handler;

                widget->removeHoverHandler(&hover_handler);
                widget->clearSuggestion();

                widget->insertSuggestion(std::make_unique<CyclicSuggestion>(suggestions, document));
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
