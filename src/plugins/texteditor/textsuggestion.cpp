// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textsuggestion.h"

#include "textdocumentlayout.h"
#include "texteditor.h"

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

using namespace Utils;

namespace TextEditor {

TextSuggestion::TextSuggestion()
{
    m_replacementDocument.setDocumentLayout(new TextDocumentLayout(&m_replacementDocument));
    m_replacementDocument.setDocumentMargin(0);
}

TextSuggestion::~TextSuggestion() = default;

CyclicSuggestion::CyclicSuggestion(const QList<Data> &suggestions, QTextDocument *sourceDocument, int currentSuggestion)
    : m_suggestions(suggestions)
    , m_currentSuggestion(currentSuggestion)
    , m_sourceDocument(sourceDocument)
{
    if (QTC_GUARD(!suggestions.isEmpty())) {
        QTC_ASSERT(
            m_currentSuggestion >= 0 && m_currentSuggestion < suggestions.size(),
            m_currentSuggestion = 0);
        Data current = suggestions.at(m_currentSuggestion);
        document()->setPlainText(current.text);
        setCurrentPosition(current.position.toPositionInDocument(sourceDocument));
    }
}

bool CyclicSuggestion::apply()
{
    const Data &suggestion = m_suggestions.value(m_currentSuggestion);
    QTextCursor c = suggestion.range.begin.toTextCursor(m_sourceDocument);
    c.setPosition(currentPosition(), QTextCursor::KeepAnchor);
    c.insertText(suggestion.text);
    return true;
}

bool CyclicSuggestion::applyWord(TextEditorWidget *widget)
{
    return applyPart(Word, widget);
}

bool CyclicSuggestion::applyLine(TextEditorWidget *widget)
{
    return applyPart(Line, widget);
}

void CyclicSuggestion::reset()
{
    const Data &suggestion = m_suggestions.value(m_currentSuggestion);
    QTextCursor c = suggestion.position.toTextCursor(m_sourceDocument);
    c.setPosition(currentPosition(), QTextCursor::KeepAnchor);
    c.removeSelectedText();
}

bool CyclicSuggestion::applyPart(Part part, TextEditorWidget *widget)
{
    const Data suggestion = m_suggestions.value(m_currentSuggestion);
    const Text::Range range = suggestion.range;
    const QTextCursor cursor = range.toTextCursor(m_sourceDocument);
    QTextCursor currentCursor = widget->textCursor();
    const QString text = suggestion.text;
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

} // namespace TextEditor
