/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

/**
  \class EditorWidget
  Graphical representation and parent widget for PyEditor::Editor class.
  Represents editor as plain text editor.
  */

#include "pythoneditorwidget.h"
#include "tools/pythonhighlighter.h"
#include "tools/pythonindenter.h"
#include "pythoneditor.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/indenter.h>
#include <texteditor/autocompleter.h>

namespace PythonEditor {

EditorWidget::EditorWidget(QWidget *parent)
    :TextEditor::BaseTextEditorWidget(parent)
{
    m_commentDefinition.multiLineStart.clear();
    m_commentDefinition.multiLineEnd.clear();
    m_commentDefinition.singleLine = QLatin1Char('#');

    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);

    setIndenter(new PythonIndenter());
    new PythonHighlighter(baseTextDocument().data());
}

EditorWidget::~EditorWidget()
{
}

/**
  Comments or uncomments selection using Python commenting syntax
  */
void EditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

/**
  Handles common IDE fonts&colors settings
  (Tools -> Options -> Text editor -> Fonts and colors)
  */
void EditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);

    PythonHighlighter *highlighter =
            qobject_cast<PythonHighlighter *>(baseTextDocument()->syntaxHighlighter());
    if (highlighter)
        highlighter->setFontSettings(fs);
}

TextEditor::BaseTextEditor *EditorWidget::createEditor()
{
    return new PythonEditor(this);
}

} // namespace PythonEditor
