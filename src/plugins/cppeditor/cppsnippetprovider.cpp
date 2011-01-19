/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <QtCore/QCoreApplication>

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
