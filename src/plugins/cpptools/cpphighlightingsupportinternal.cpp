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
    //Get macro uses
    QList<CheckSymbols::Use> macroUses;
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
