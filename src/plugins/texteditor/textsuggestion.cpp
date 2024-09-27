// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textsuggestion.h"

#include "textdocumentlayout.h"
#include "texteditor.h"
#include "texteditortr.h"

#include <QToolBar>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>

using namespace Utils;

namespace TextEditor {

TextSuggestion::TextSuggestion(const Data &suggestion, QTextDocument *sourceDocument)
    : m_suggestion(suggestion)
    , m_sourceDocument(sourceDocument)
{
    m_replacementDocument.setDocumentLayout(new TextDocumentLayout(&m_replacementDocument));
    m_replacementDocument.setDocumentMargin(0);
    replacementDocument()->setPlainText(suggestion.text);
    setCurrentPosition(suggestion.position.toPositionInDocument(sourceDocument));
}

TextSuggestion::~TextSuggestion() = default;

bool TextSuggestion::apply()
{
    QTextCursor c = m_suggestion.range.begin.toTextCursor(sourceDocument());
    c.setPosition(currentPosition(), QTextCursor::KeepAnchor);
    c.insertText(m_suggestion.text);
    return true;
}

bool TextSuggestion::applyWord(TextEditorWidget *widget)
{
    return applyPart(Word, widget);
}

bool TextSuggestion::applyLine(TextEditorWidget *widget)
{
    return applyPart(Line, widget);
}

void TextSuggestion::reset()
{
    QTextCursor c = m_suggestion.position.toTextCursor(sourceDocument());
    c.setPosition(currentPosition(), QTextCursor::KeepAnchor);
    c.removeSelectedText();
}

bool TextSuggestion::applyPart(Part part, TextEditorWidget *widget)
{
    const Text::Range range = m_suggestion.range;
    const QTextCursor cursor = range.toTextCursor(sourceDocument());
    QTextCursor currentCursor = widget->textCursor();
    const QString text = m_suggestion.text;
    const int startPos = currentCursor.positionInBlock() - cursor.positionInBlock()
                         + (cursor.selectionEnd() - cursor.selectionStart());
    int next = part == Word ? endOfNextWord(text, startPos) : text.indexOf('\n', startPos);

    if (next == -1)
        return apply();

    if (part == Line)
        ++next;
    QString subText = text.mid(startPos, next - startPos);
    if (subText.isEmpty())
        return false;

    currentCursor.insertText(subText);
    if (const int seperatorPos = subText.lastIndexOf('\n'); seperatorPos >= 0) {
        const QString newCompletionText = text.mid(startPos + seperatorPos + 1);
        if (!newCompletionText.isEmpty()) {
            const Text::Position newStart{int(range.begin.line + subText.count('\n')), 0};
            const Text::Position newEnd{newStart.line, int(subText.length() - seperatorPos - 1)};
            const Text::Range newRange{newStart, newEnd};
            const QList<Data> newSuggestion{{newRange, newEnd, newCompletionText}};
            widget->insertSuggestion(
                std::make_unique<CyclicSuggestion>(newSuggestion, widget->document(), 0));
        }
    }
    return false;
}

CyclicSuggestion::CyclicSuggestion(
    const QList<Data> &suggestions, QTextDocument *sourceDocument, int currentSuggestion)
    : TextSuggestion(
          QTC_GUARD(currentSuggestion < suggestions.size()) ? suggestions.at(currentSuggestion)
                                                            : Data(),
          sourceDocument)
    , m_suggestions(suggestions)
    , m_currentSuggestion(currentSuggestion)
{}

class SuggestionToolTip : public QToolBar
{
public:
    SuggestionToolTip(
        QList<TextSuggestion::Data> suggestions, int currentSuggestion, TextEditorWidget *editor)
        : m_numberLabel(new QLabel)
        , m_suggestions(suggestions)
        , m_currentSuggestion(std::max(0, std::min<int>(currentSuggestion, suggestions.size() - 1)))
        , m_editor(editor)
    {
        auto prev = addAction(
            Utils::Icons::PREV_TOOLBAR.icon(), Tr::tr("Select Previous Suggestion"));
        prev->setEnabled(m_suggestions.size() > 1);
        addWidget(m_numberLabel);
        auto next
            = addAction(Utils::Icons::NEXT_TOOLBAR.icon(), Tr::tr("Select Next Suggestion"));
        next->setEnabled(m_suggestions.size() > 1);

        auto apply = addAction(Tr::tr("Apply (%1)").arg(QKeySequence(Qt::Key_Tab).toString()));
        auto applyWord = addAction(
            Tr::tr("Apply Word (%1)").arg(QKeySequence(QKeySequence::MoveToNextWord).toString()));
        auto applyLine = addAction(Tr::tr("Apply Line"));

        connect(prev, &QAction::triggered, this, &SuggestionToolTip::selectPrevious);
        connect(next, &QAction::triggered, this, &SuggestionToolTip::selectNext);
        connect(apply, &QAction::triggered, this, &SuggestionToolTip::apply);
        connect(applyWord, &QAction::triggered, this, &SuggestionToolTip::applyWord);
        connect(applyLine, &QAction::triggered, this, &SuggestionToolTip::applyLine);

        updateLabels();
    }

private:
    void updateLabels()
    {
        m_numberLabel->setText(
            Tr::tr("%1 of %2").arg(m_currentSuggestion + 1).arg(m_suggestions.count()));
    }

    void selectPrevious()
    {
        --m_currentSuggestion;
        if (m_currentSuggestion < 0)
            m_currentSuggestion = m_suggestions.size() - 1;
        setCurrentSuggestion();
    }

    void selectNext()
    {
        ++m_currentSuggestion;
        if (m_currentSuggestion >= m_suggestions.size())
            m_currentSuggestion = 0;
        setCurrentSuggestion();
    }

    void setCurrentSuggestion()
    {
        updateLabels();
        if (TextSuggestion *suggestion = m_editor->currentSuggestion())
            suggestion->reset();
        m_editor->insertSuggestion(std::make_unique<CyclicSuggestion>(
            m_suggestions, m_editor->document(), m_currentSuggestion));
    }

    void apply()
    {
        if (TextSuggestion *suggestion = m_editor->currentSuggestion()) {
            if (!suggestion->apply())
                return;
        }
        ToolTip::hide();
    }

    void applyWord()
    {
        if (TextSuggestion *suggestion = m_editor->currentSuggestion()) {
            if (!suggestion->applyWord(m_editor))
                return;
        }
        ToolTip::hide();
    }

    void applyLine()
    {
        if (TextSuggestion *suggestion = m_editor->currentSuggestion()) {
            if (!suggestion->applyLine(m_editor))
                return;
        }
        ToolTip::hide();
    }

    QLabel *m_numberLabel;
    QList<TextSuggestion::Data> m_suggestions;
    int m_currentSuggestion = 0;
    TextEditorWidget *m_editor;
};

void SuggestionHoverHandler::identifyMatch(
    TextEditorWidget *editorWidget, int pos, ReportPriority report)
{
    QScopeGuard cleanup([&] { report(Priority_None); });
    if (!editorWidget->suggestionVisible())
        return;

    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(pos);
    m_block = cursor.block();
    auto *suggestion = dynamic_cast<CyclicSuggestion *>(TextDocumentLayout::suggestion(m_block));

    if (!suggestion)
        return;

    const QList<TextSuggestion::Data> suggestions = suggestion->suggestions();
    if (suggestions.isEmpty())
        return;

    cleanup.dismiss();
    report(Priority_Suggestion);
}

void SuggestionHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    Q_UNUSED(point)
    auto *suggestion = dynamic_cast<CyclicSuggestion *>(TextDocumentLayout::suggestion(m_block));

    if (!suggestion)
        return;

    auto tooltipWidget = new SuggestionToolTip(
        suggestion->suggestions(), suggestion->currentSuggestion(), editorWidget);

    const QRect cursorRect = editorWidget->cursorRect(editorWidget->textCursor());
    QPoint pos = editorWidget->viewport()->mapToGlobal(cursorRect.topLeft())
                 - Utils::ToolTip::offsetFromPosition();
    pos.ry() -= tooltipWidget->sizeHint().height();
    ToolTip::show(pos, tooltipWidget, editorWidget);
}

} // namespace TextEditor
