/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "snippeteditor.h"

#include <texteditor/basetextdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/normalindenter.h>

#include <QTextDocument>
#include <QFocusEvent>

using namespace TextEditor;

/*!
    \class TextEditor::SnippetEditorWidget
    \brief The SnippetEditorWidget class is a lightweight editor for code snippets
    with basic support for syntax highlighting, indentation, and others.
    \ingroup Snippets
*/

SnippetEditor::SnippetEditor(SnippetEditorWidget *editor)
    : BaseTextEditor(editor)
{
    setContext(Core::Context(Constants::SNIPPET_EDITOR_ID, Constants::C_TEXTEDITOR));
}

Core::Id SnippetEditor::id() const
{
    return Constants::SNIPPET_EDITOR_ID;
}

SnippetEditorWidget::SnippetEditorWidget(QWidget *parent) : BaseTextEditorWidget(parent)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setHighlightCurrentLine(false);
    setLineNumbersVisible(false);
    setParenthesesMatchingEnabled(true);
}

void SnippetEditorWidget::setSyntaxHighlighter(TextEditor::SyntaxHighlighter *highlighter)
{
    baseTextDocument()->setSyntaxHighlighter(highlighter);
}

void SnippetEditorWidget::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::ActiveWindowFocusReason && document()->isModified()) {
        document()->setModified(false);
        emit snippetContentChanged();
    }
    BaseTextEditorWidget::focusOutEvent(event);
}

BaseTextEditor *SnippetEditorWidget::createEditor()
{
    return new SnippetEditor(this);
}
