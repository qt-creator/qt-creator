// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textdocumentmanipulator.h"

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

namespace TextEditor {

TextDocumentManipulator::TextDocumentManipulator(TextEditorWidget *textEditorWidget)
    : m_textEditorWidget(textEditorWidget)
{
}

int TextDocumentManipulator::currentPosition() const
{
    return m_textEditorWidget->position();
}

QChar TextDocumentManipulator::characterAt(int position) const
{
    return m_textEditorWidget->characterAt(position);
}

QString TextDocumentManipulator::textAt(int position, int length) const
{
    return m_textEditorWidget->textAt(position, length);
}

QTextCursor TextDocumentManipulator::textCursor() const
{
    return m_textEditorWidget->textCursor();
}

QTextCursor TextDocumentManipulator::textCursorAt(int position) const
{
    return m_textEditorWidget->textCursorAt(position);
}

QTextDocument *TextDocumentManipulator::document() const
{
    return m_textEditorWidget->document();
}

TextEditorWidget *TextDocumentManipulator::editor() const
{
    return m_textEditorWidget;
}

void TextDocumentManipulator::setCursorPosition(int position)
{
    m_textEditorWidget->setCursorPosition(position);
}

void TextDocumentManipulator::addAutoCompleteSkipPosition()
{
    m_textEditorWidget->setAutoCompleteSkipPosition(m_textEditorWidget->textCursor());
}

void TextDocumentManipulator::replace(int position, int length, const QString &text)
{
    m_textEditorWidget->replace(position, length, text);
}

void TextDocumentManipulator::insertCodeSnippet(int position,
                                                const QString &text,
                                                const SnippetParser &parse)
{
    m_textEditorWidget->insertCodeSnippet(position, text, parse);
}

QString TextDocumentManipulator::getLine(int line) const
{
    return m_textEditorWidget->textDocument()->blockText(line - 1);
}

Utils::Text::Position TextDocumentManipulator::cursorPos() const
{
    return m_textEditorWidget->lineColumn();
}

} // namespace TextEditor
