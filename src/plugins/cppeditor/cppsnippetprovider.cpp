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

#include "cppsnippetprovider.h"
#include "cpphighlighter.h"
#include "cppeditor.h"
#include "cppautocompleter.h"
#include "cppeditorconstants.h"

#include <cpptools/cppqtstyleindenter.h>

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/snippets/snippeteditor.h>

#include <QLatin1String>
#include <QCoreApplication>

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
    return QCoreApplication::translate("CppEditor::Internal::CppSnippetProvider", "C++");
}

void CppSnippetProvider::decorateEditor(TextEditor::SnippetEditorWidget *editor) const
{
    CppHighlighter *highlighter = new CppHighlighter;
    const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    const QVector<QTextCharFormat> &formats =
        fs.toTextCharFormats(CPPEditorWidget::highlighterFormatCategories());
    highlighter->setFormats(formats.constBegin(), formats.constEnd());
    editor->setSyntaxHighlighter(highlighter);
    editor->setIndenter(new CppTools::CppQtStyleIndenter);
    editor->setAutoCompleter(new CppAutoCompleter);
}
