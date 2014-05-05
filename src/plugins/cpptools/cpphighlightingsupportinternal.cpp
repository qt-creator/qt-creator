/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "cpphighlightingsupportinternal.h"

#include "cppchecksymbols.h"
#include "cpptoolsreuse.h"

#include <texteditor/basetextdocument.h>
#include <texteditor/convenience.h>

#include <cplusplus/SimpleLexer.h>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

CppHighlightingSupportInternal::CppHighlightingSupportInternal(
        TextEditor::BaseTextDocument *baseTextDocument)
    : CppHighlightingSupport(baseTextDocument)
{
}

CppHighlightingSupportInternal::~CppHighlightingSupportInternal()
{
}

QFuture<TextEditor::HighlightingResult> CppHighlightingSupportInternal::highlightingFuture(
        const Document::Ptr &doc,
        const Snapshot &snapshot) const
{
    typedef TextEditor::HighlightingResult Result;
    QList<Result> macroUses;

    QTextDocument *textDocument = baseTextDocument()->document();
    using TextEditor::Convenience::convertPosition;

    // Get macro definitions
    foreach (const CPlusPlus::Macro& macro, doc->definedMacros()) {
        int line, column;
        convertPosition(textDocument, macro.utf16CharOffset(), &line, &column);

        ++column; //Highlighting starts at (column-1) --> compensate here
        Result use(line, column, macro.nameToQString().size(), MacroUse);
        macroUses.append(use);
    }

    // Get macro uses
    foreach (const Document::MacroUse &macro, doc->macroUses()) {
        const QString name = macro.macro().nameToQString();

        //Filter out QtKeywords
        if (isQtKeyword(QStringRef(&name)))
            continue;

        // Filter out C++ keywords
        // FIXME: Check default values or get from document.
        LanguageFeatures features;
        features.cxx11Enabled = true;
        features.c99Enabled = true;

        SimpleLexer tokenize;
        tokenize.setLanguageFeatures(features);

        const QList<Token> tokens = tokenize(name);
        if (tokens.length() && (tokens.at(0).isKeyword() || tokens.at(0).isObjCAtKeyword()))
            continue;

        int line, column;
        convertPosition(textDocument, macro.utf16charsBegin(), &line, &column);
        ++column; //Highlighting starts at (column-1) --> compensate here
        Result use(line, column, name.size(), MacroUse);
        macroUses.append(use);
    }

    LookupContext context(doc, snapshot);
    return CheckSymbols::go(doc, context, macroUses);
}
