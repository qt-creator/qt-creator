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
    TextSuggestion();
    virtual ~TextSuggestion();
    // Returns true if the suggestion was applied completely, false if it was only partially applied.
    virtual bool apply() = 0;
    // Returns true if the suggestion was applied completely, false if it was only partially applied.
    virtual bool applyWord(TextEditorWidget *widget) = 0;
    virtual bool applyLine(TextEditorWidget *widget) = 0;
    virtual void reset() = 0;

    int currentPosition() const { return m_currentPosition; }
    void setCurrentPosition(int position) { m_currentPosition = position; }

    QTextDocument *replacementDocument() { return &m_replacementDocument; }

private:
    QTextDocument m_replacementDocument;
    int m_currentPosition = -1;
};

class TEXTEDITOR_EXPORT CyclicSuggestion : public TextSuggestion
{
public:
    class TEXTEDITOR_EXPORT Data
    {
    public:
        Utils::Text::Range range;
        Utils::Text::Position position;
        QString text;
    };

    CyclicSuggestion(
        const QList<Data> &suggestions, QTextDocument *sourceDocument, int currentCompletion = 0);

    bool apply() override;
    bool applyWord(TextEditorWidget *widget) override;
    bool applyLine(TextEditorWidget *widget) override;
    void reset() override;

    QList<Data> suggestions() const { return m_suggestions; }
    int currentSuggestion() const { return m_currentSuggestion; }

private:
    enum Part {Word, Line};
    bool applyPart(Part part, TextEditor::TextEditorWidget *widget);

    QList<Data> m_suggestions;
    int m_currentSuggestion = 0;
    QTextDocument *m_sourceDocument = nullptr;
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
