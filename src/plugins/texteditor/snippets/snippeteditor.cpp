// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snippeteditor.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <utils/qtcassert.h>

#include <QFocusEvent>

namespace TextEditor {

/*!
    \class TextEditor::SnippetEditorWidget
    \brief The SnippetEditorWidget class is a lightweight editor for code snippets
    with basic support for syntax highlighting, indentation, and others.
    \ingroup Snippets
*/


SnippetEditorWidget::SnippetEditorWidget(QWidget *parent)
    : TextEditorWidget(parent)
{
    setupFallBackEditor(TextEditor::Constants::SNIPPET_EDITOR_ID);
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setHighlightCurrentLine(false);
    setLineNumbersVisible(false);
    setParenthesesMatchingEnabled(true);
}

void SnippetEditorWidget::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::ActiveWindowFocusReason && document()->isModified()) {
        document()->setModified(false);
        emit snippetContentChanged();
    }
    TextEditorWidget::focusOutEvent(event);
}

void SnippetEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QPlainTextEdit::contextMenuEvent(e);
}

} // namespace
