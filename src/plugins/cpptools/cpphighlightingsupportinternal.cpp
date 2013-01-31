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

#include "cppchecksymbols.h"
#include "cpphighlightingsupportinternal.h"

#include <cplusplus/LookupContext.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/Token.h>
#include <cpptools/cpptoolsreuse.h>
#include <texteditor/itexteditor.h>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

CppHighlightingSupportInternal::CppHighlightingSupportInternal(TextEditor::ITextEditor *editor)
    : CppHighlightingSupport(editor)
{
}

CppHighlightingSupportInternal::~CppHighlightingSupportInternal()
{
}

QFuture<CppHighlightingSupport::Use> CppHighlightingSupportInternal::highlightingFuture(
        const Document::Ptr &doc,
        const Snapshot &snapshot) const
{
    QList<CheckSymbols::Use> macroUses;

    //Get macro definitions
    foreach (const CPlusPlus::Macro& macro, doc->definedMacros()) {
        int line, column;
        editor()->convertPosition(macro.offset(), &line, &column);
        ++column; //Highlighting starts at (column-1) --> compensate here
        CheckSymbols::Use use(line, column, macro.name().size(), SemanticInfo::MacroUse);
        macroUses.append(use);
    }

    //Get macro uses
    foreach (Document::MacroUse macro, doc->macroUses()) {
        const QString name = QString::fromUtf8(macro.macro().name());

        //Filter out QtKeywords
        if (isQtKeyword(QStringRef(&name)))
            continue;

        //Filter out C++ keywords
        SimpleLexer tokenize;
        tokenize.setQtMocRunEnabled(false);
        tokenize.setObjCEnabled(false);
        tokenize.setCxx0xEnabled(true);
        const QList<Token> tokens = tokenize(name);
        if (tokens.length() && (tokens.at(0).isKeyword() || tokens.at(0).isObjCAtKeyword()))
            continue;

        int line, column;
        editor()->convertPosition(macro.begin(), &line, &column);
        ++column; //Highlighting starts at (column-1) --> compensate here
        CheckSymbols::Use use(line, column, name.size(), SemanticInfo::MacroUse);
        macroUses.append(use);
    }

    LookupContext context(doc, snapshot);
    return CheckSymbols::go(doc, context, macroUses);
}

CppHighlightingSupportInternalFactory::~CppHighlightingSupportInternalFactory()
{
}

CppHighlightingSupport *CppHighlightingSupportInternalFactory::highlightingSupport(TextEditor::ITextEditor *editor)
{
    return new CppHighlightingSupportInternal(editor);
}
