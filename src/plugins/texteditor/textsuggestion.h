// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "basehoverhandler.h"

#include <utils/textutils.h>

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT TextSuggestion
{
public:
    class TEXTEDITOR_EXPORT Data
    {
    public:
        Utils::Text::Range range;
        Utils::Text::Position position;
        QString text;
    };

    TextSuggestion(const Data &suggestion, QTextDocument *sourceDocument);
    virtual ~TextSuggestion();
    // Returns true if the suggestion was applied completely, false if it was only partially applied.
    virtual bool apply();
    // Returns true if the suggestion was applied completely, false if it was only partially applied.
    virtual bool applyWord(TextEditorWidget *widget);
    virtual bool applyLine(TextEditorWidget *widget);
    virtual bool filterSuggestions(TextEditorWidget *widget);

    int currentPosition() const { return m_currentPosition; }
    void setCurrentPosition(int position) { m_currentPosition = position; }

    QTextDocument *replacementDocument() { return &m_replacementDocument; }
    QTextDocument *sourceDocument() { return m_sourceDocument; }

private:
    enum Part {Word, Line};
    bool applyPart(Part part, TextEditor::TextEditorWidget *widget);

    Data m_suggestion;
    QTextDocument m_replacementDocument;
    QTextDocument *m_sourceDocument = nullptr;
    int m_currentPosition = -1;
};

class TEXTEDITOR_EXPORT CyclicSuggestion : public TextSuggestion
{
public:
    CyclicSuggestion(
        const QList<Data> &suggestions, QTextDocument *sourceDocument, int currentCompletion = 0);

    QList<Data> suggestions() const { return m_suggestions; }
    int currentSuggestion() const { return m_currentSuggestion; }

private:
    bool filterSuggestions(TextEditorWidget *widget) override;

    QList<Data> m_suggestions;
    int m_currentSuggestion = 0;
};

class SuggestionHoverHandler final : public BaseHoverHandler
{
public:
    SuggestionHoverHandler() = default;

protected:
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) final;
    void operateTooltip(TextEditor::TextEditorWidget *editorWidget, const QPoint &point) final;

private:
    QTextBlock m_block;
};

} // namespace TextEditor
