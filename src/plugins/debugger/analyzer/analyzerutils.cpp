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

#include "analyzerutils.h"

#include <cpptools/cppmodelmanager.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <utils/qtcassert.h>

#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/TypeOfExpression.h>

#include <QTextCursor>

using namespace Core;
using namespace ProjectExplorer;

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
    TextEditor::TextEditorWidget *widget = TextEditor::TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return 0;

    QTextCursor tc = widget->textCursor();
    int line = 0;
    int column = 0;
    const int pos = tc.position();
    widget->convertPosition(pos, &line, &column);

    const CPlusPlus::Snapshot &snapshot = CppTools::CppModelManager::instance()->snapshot();
    CPlusPlus::Document::Ptr doc = snapshot.document(widget->textDocument()->filePath());
    QTC_ASSERT(doc, return 0);

    // fetch the expression's code
    CPlusPlus::ExpressionUnderCursor expressionUnderCursor(doc->languageFeatures());
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
