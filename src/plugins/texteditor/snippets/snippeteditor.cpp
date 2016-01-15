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

} // namespace
