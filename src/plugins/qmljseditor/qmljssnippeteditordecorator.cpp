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

#include "qmljssnippeteditordecorator.h"
#include "qmljshighlighter.h"
#include "qmljseditor.h"
#include "qmljsindenter.h"
#include "qmljsautocompleter.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/snippets/snippeteditor.h>

using namespace QmlJSEditor;
using namespace Internal;

QmlJSSnippetEditorDecorator::QmlJSSnippetEditorDecorator() :
    TextEditor::ISnippetEditorDecorator()
{}

QmlJSSnippetEditorDecorator::~QmlJSSnippetEditorDecorator()
{}

bool QmlJSSnippetEditorDecorator::supports(TextEditor::Snippet::Group group) const
{
    if (group == TextEditor::Snippet::Qml)
        return true;
    return false;
}

void QmlJSSnippetEditorDecorator::apply(TextEditor::SnippetEditor *editor) const
{
    Highlighter *highlighter = new Highlighter;
    const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    highlighter->setFormats(fs.toTextCharFormats(QmlJSTextEditor::highlighterFormatCategories()));
    editor->installSyntaxHighlighter(highlighter);
    editor->setIndenter(new Indenter);
    editor->setAutoCompleter(new AutoCompleter);
}
