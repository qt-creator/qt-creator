/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljssnippetprovider.h"
#include "qmljshighlighter.h"
#include "qmljseditor.h"
#include "qmljsindenter.h"
#include "qmljsautocompleter.h"
#include "qmljseditorconstants.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/snippets/snippeteditor.h>

#include <QtCore/QLatin1String>

using namespace QmlJSEditor;
using namespace Internal;

QmlJSSnippetProvider::QmlJSSnippetProvider() :
    TextEditor::ISnippetProvider()
{}

QmlJSSnippetProvider::~QmlJSSnippetProvider()
{}

QString QmlJSSnippetProvider::groupId() const
{
    return QLatin1String(Constants::QML_SNIPPETS_GROUP_ID);
}

QString QmlJSSnippetProvider::displayName() const
{
    return tr("QML");
}

void QmlJSSnippetProvider::decorateEditor(TextEditor::SnippetEditor *editor) const
{
    Highlighter *highlighter = new Highlighter;
    const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    highlighter->setFormats(fs.toTextCharFormats(QmlJSTextEditor::highlighterFormatCategories()));
    editor->setSyntaxHighlighter(highlighter);
    editor->setIndenter(new Indenter);
    editor->setAutoCompleter(new AutoCompleter);
}
