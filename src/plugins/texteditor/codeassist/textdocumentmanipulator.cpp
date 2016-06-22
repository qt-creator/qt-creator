/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

int TextDocumentManipulator::positionAt(TextPositionOperation textPositionOperation) const
{
    return m_textEditorWidget->position(textPositionOperation);
}

QChar TextDocumentManipulator::characterAt(int position) const
{
    return m_textEditorWidget->characterAt(position);
}

QString TextDocumentManipulator::textAt(int position, int length) const
{
    return m_textEditorWidget->textAt(position, length);
}

QTextCursor TextDocumentManipulator::textCursorAt(int position) const
{
    auto cursor = m_textEditorWidget->textCursor();
    cursor.setPosition(position);

    return cursor;
}

void TextDocumentManipulator::setCursorPosition(int position)
{
    m_textEditorWidget->setCursorPosition(position);
}

void TextDocumentManipulator::setAutoCompleteSkipPosition(int position)
{
    QTextCursor cursor = m_textEditorWidget->textCursor();
    cursor.setPosition(position);
    m_textEditorWidget->setAutoCompleteSkipPosition(cursor);
}

bool TextDocumentManipulator::replace(int position, int length, const QString &text)
{
    bool textWillBeReplaced = textIsDifferentAt(position, length, text);

    if (textWillBeReplaced)
        replaceWithoutCheck(position, length, text);

    return textWillBeReplaced;
}

void TextDocumentManipulator::insertCodeSnippet(int position, const QString &text)
{
    auto cursor = m_textEditorWidget->textCursor();
    cursor.setPosition(position, QTextCursor::KeepAnchor);
    m_textEditorWidget->insertCodeSnippet(cursor, text);
}

void TextDocumentManipulator::paste()
{
    m_textEditorWidget->paste();
}

void TextDocumentManipulator::encourageApply()
{
    m_textEditorWidget->encourageApply();
}

namespace {

bool hasOnlyBlanksBeforeCursorInLine(QTextCursor textCursor)
{
    textCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);

    const auto textBeforeCursor = textCursor.selectedText();

    const auto nonSpace = std::find_if(textBeforeCursor.cbegin(),
                                       textBeforeCursor.cend(),
                                       [] (const QChar &signBeforeCursor) {
        return !signBeforeCursor.isSpace();
    });

    return nonSpace == textBeforeCursor.cend();
}

}

void TextDocumentManipulator::autoIndent(int position, int length)
{
    auto cursor = m_textEditorWidget->textCursor();
    cursor.setPosition(position);
    if (hasOnlyBlanksBeforeCursorInLine(cursor)) {
        cursor.setPosition(position + length, QTextCursor::KeepAnchor);

        m_textEditorWidget->textDocument()->autoIndent(cursor);
    }
}

bool TextDocumentManipulator::textIsDifferentAt(int position, int length, const QString &text) const
{
    const auto textToBeReplaced = m_textEditorWidget->textAt(position, length);

    return text != textToBeReplaced;
}

void TextDocumentManipulator::replaceWithoutCheck(int position, int length, const QString &text)
{
    auto cursor = m_textEditorWidget->textCursor();
    cursor.setPosition(position);
    cursor.setPosition(position + length, QTextCursor::KeepAnchor);
    cursor.insertText(text);
}

} // namespace TextEditor
