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

#ifndef CPPPOINTERFORMATTER_H
#define CPPPOINTERFORMATTER_H

#include "cpptools_global.h"

#include <AST.h>
#include <ASTVisitor.h>
#include <Overview.h>
#include <Symbols.h>

#include <cpptools/cpprefactoringchanges.h>
#include <utils/changeset.h>

namespace CppTools {

using namespace CPlusPlus;
using namespace CppTools;
using Utils::ChangeSet;

typedef Utils::ChangeSet::Range Range;

/*!
    \class CppTools::PointerDeclarationFormatter

    \brief Rewrite pointer or reference declarations accordingly to an Overview.

    The following constructs are supported:
    \list
     \o Simple declarations
     \o Parameters and return types of function declarations and definitions
     \o Control flow statements like if, while, for, foreach
    \endlist
*/

class CPPTOOLS_EXPORT PointerDeclarationFormatter: protected ASTVisitor
{
public:
    /*!
        \enum PointerDeclarationFormatter::CursorHandling

        This simplifies the QuickFix implementation.

          \value RespectCursor
                 Consider the cursor position / selection of the CppRefactoringFile
                 for rejecting edit operation candidates for the resulting ChangeSet.
                 If there is a selection, the range of the edit operation candidate
                 should be inside the selection. If there is no selection, the cursor
                 position should be within the range of the edit operation candidate.
          \value IgnoreCursor
                 Cursor position or selection of the CppRefactoringFile will
                _not_ be considered for aborting.
     */
    enum CursorHandling { RespectCursor, IgnoreCursor };

    explicit PointerDeclarationFormatter(const CppRefactoringFilePtr refactoringFile,
                                         const Overview &overview,
                                         CursorHandling cursorHandling = IgnoreCursor);

    /*!
        Returns a ChangeSet for applying the formatting changes.
        The ChangeSet is empty if it was not possible to rewrite anything.
    */
    ChangeSet format(AST *ast)
    {
        if (ast)
            accept(ast);
        return m_changeSet;
    }

protected:
    bool visit(SimpleDeclarationAST *ast);
    bool visit(FunctionDefinitionAST *ast);
    bool visit(ParameterDeclarationAST *ast);
    bool visit(IfStatementAST *ast);
    bool visit(WhileStatementAST *ast);
    bool visit(ForStatementAST *ast);
    bool visit(ForeachStatementAST *ast);

private:
    void processIfWhileForStatement(ExpressionAST *expression, Symbol *symbol);
    void checkAndRewrite(Symbol *symbol, Range range, unsigned charactersToRemove = 0);
    QString rewriteDeclaration(FullySpecifiedType type, const Name *name) const;

    const CppRefactoringFilePtr m_cppRefactoringFile;
    const Overview &m_overview;
    const CursorHandling m_cursorHandling;

    ChangeSet m_changeSet;
};

} // namespace CppTools

#endif // CPPPOINTERFORMATTER_H
