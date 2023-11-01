// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "analyzerutils.h"

#include <cppeditor/cppmodelmanager.h>
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
    while (ch.isLetterOrNumber() || ch == '_') {
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
        return nullptr;

    QTextCursor tc = widget->textCursor();
    int line = 0;
    int column = 0;
    const int pos = tc.position();
    widget->convertPosition(pos, &line, &column);

    const CPlusPlus::Snapshot &snapshot = CppEditor::CppModelManager::snapshot();
    CPlusPlus::Document::Ptr doc = snapshot.document(widget->textDocument()->filePath());
    QTC_ASSERT(doc, return nullptr);

    // fetch the expression's code
    CPlusPlus::ExpressionUnderCursor expressionUnderCursor(doc->languageFeatures());
    moveCursorToEndOfName(&tc);
    const QString &expression = expressionUnderCursor(tc);
    CPlusPlus::Scope *scope = doc->scopeAt(line, column);

    CPlusPlus::TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    const QList<CPlusPlus::LookupItem> &lookupItems = typeOfExpression(expression.toUtf8(), scope);
    if (lookupItems.isEmpty())
        return nullptr;

    const CPlusPlus::LookupItem &lookupItem = lookupItems.first(); // ### TODO: select best candidate.
    return lookupItem.declaration();
}
