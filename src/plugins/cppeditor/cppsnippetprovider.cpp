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

#include "cppsnippetprovider.h"
#include "cpphighlighter.h"
#include "cppeditor.h"
#include "cppqtstyleindenter.h"
#include "cppautocompleter.h"
#include "cppeditorconstants.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/snippets/snippeteditor.h>

#include <QtCore/QLatin1String>

using namespace CppEditor;
using namespace Internal;

CppSnippetProvider::CppSnippetProvider() :
    TextEditor::ISnippetProvider()
{}

CppSnippetProvider::~CppSnippetProvider()
{}

QString CppSnippetProvider::groupId() const
{
    return QLatin1String(Constants::CPP_SNIPPETS_GROUP_ID);
}

QString CppSnippetProvider::displayName() const
{
    return tr("C++");
}

void CppSnippetProvider::decorateEditor(TextEditor::SnippetEditor *editor) const
{
    CppHighlighter *highlighter = new CppHighlighter;
    const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    const QVector<QTextCharFormat> &formats =
        fs.toTextCharFormats(CPPEditor::highlighterFormatCategories());
    highlighter->setFormats(formats.constBegin(), formats.constEnd());
    editor->setSyntaxHighlighter(highlighter);
    editor->setIndenter(new CppQtStyleIndenter);
    editor->setAutoCompleter(new CppAutoCompleter);
}
