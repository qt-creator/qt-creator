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

#include "analyzerutils.h"

#include "analyzerconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/itexteditor.h>
#include <utils/qtcassert.h>

#include <cplusplus/ExpressionUnderCursor.h>
#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/LookupItem.h>
#include <cplusplus/TypeOfExpression.h>

// shared/cplusplus includes
#include <Scope.h>
#include <Symbol.h>

#include <QTextDocumentFragment>
#include <QTextCursor>
#include <QWidget>

using namespace Analyzer;
using namespace Core;

static void moveCursorToEndOfName(QTextCursor *tc)
{
    QTextDocument *doc = tc->document();
    if (!doc)
        return;

    QChar ch = doc->characterAt(tc->position());
    while (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
        tc->movePosition(QTextCursor::NextCharacter);
        ch = doc->characterAt(tc->position());
    }
}

// TODO: Can this be improved? This code is ripped from CppEditor, especially CppElementEvaluater
// We cannot depend on this since CppEditor plugin code is internal and requires building the implementation files ourselves
CPlusPlus::Symbol *AnalyzerUtils::findSymbolUnderCursor()
{
    IEditor *editor = EditorManager::currentEditor();
    if (!editor)
        return 0;
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
    if (!textEditor)
        return 0;
    TextEditor::BaseTextEditorWidget *editorWidget = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
    if (!editorWidget)
        return 0;

    QPlainTextEdit *ptEdit = qobject_cast<QPlainTextEdit *>(editor->widget());
    if (!ptEdit)
        return 0;

    QTextCursor tc;
    tc = ptEdit->textCursor();
    int line = 0;
    int column = 0;
    const int pos = tc.position();
    editorWidget->convertPosition(pos, &line, &column);

    const CPlusPlus::Snapshot &snapshot = CPlusPlus::CppModelManagerInterface::instance()->snapshot();
    CPlusPlus::Document::Ptr doc = snapshot.document(editor->document()->fileName());
    QTC_ASSERT(doc, return 0);

    // fetch the expression's code
    CPlusPlus::ExpressionUnderCursor expressionUnderCursor;
    moveCursorToEndOfName(&tc);
    const QString &expression = expressionUnderCursor(tc);
    CPlusPlus::Scope *scope = doc->scopeAt(line, column);

    CPlusPlus::TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    const QList<CPlusPlus::LookupItem> &lookupItems = typeOfExpression(expression.toUtf8(), scope);
    if (lookupItems.isEmpty())
        return 0;

    const CPlusPlus::LookupItem &lookupItem = lookupItems.first(); // ### TODO: select best candidate.
    return lookupItem.declaration();
}
