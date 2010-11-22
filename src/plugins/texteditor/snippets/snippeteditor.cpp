/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "snippeteditor.h"

#include <texteditor/basetextdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/normalindenter.h>

#include <QtGui/QTextDocument>
#include <QtGui/QFocusEvent>

using namespace TextEditor;

SnippetEditorEditable::SnippetEditorEditable(SnippetEditor *editor) :
    BaseTextEditorEditable(editor),
    m_context(Constants::SNIPPET_EDITOR_ID, Constants::C_TEXTEDITOR)
{}

SnippetEditorEditable::~SnippetEditorEditable()
{}

QString SnippetEditorEditable::id() const
{
    return Constants::SNIPPET_EDITOR_ID;
}

SnippetEditor::SnippetEditor(QWidget *parent) : BaseTextEditor(parent)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setHighlightCurrentLine(false);
    setLineNumbersVisible(false);
    setParenthesesMatchingEnabled(true);
}

SnippetEditor::~SnippetEditor()
{}

void SnippetEditor::setSyntaxHighlighter(TextEditor::SyntaxHighlighter *highlighter)
{
    baseTextDocument()->setSyntaxHighlighter(highlighter);
}

void SnippetEditor::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::ActiveWindowFocusReason && document()->isModified())
        emit snippetContentChanged();
}

BaseTextEditorEditable *SnippetEditor::createEditableInterface()
{
    return new SnippetEditorEditable(this);
}
