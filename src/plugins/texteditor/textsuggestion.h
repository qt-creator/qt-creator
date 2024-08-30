// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

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
    virtual int position() = 0;

    int currentPosition() const { return m_currentPosition; }
    void setCurrentPosition(int position) { m_currentPosition = position; }

    QTextDocument *document() { return &m_replacementDocument; }

private:
    QTextDocument m_replacementDocument;
    int m_currentPosition = -1;
};

} // namespace TextEditor
