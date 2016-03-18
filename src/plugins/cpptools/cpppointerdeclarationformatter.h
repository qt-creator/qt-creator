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

#pragma once

#include "cpptools_global.h"
#include "cpprefactoringchanges.h"

#include <cplusplus/ASTVisitor.h>

#include <utils/changeset.h>

namespace CPlusPlus { class Overview; }

namespace CppTools {

using namespace CPlusPlus;
using namespace CppTools;

/*!
    \class CppTools::PointerDeclarationFormatter

    \brief The PointerDeclarationFormatter class rewrites pointer or reference
    declarations to an Overview.

    The following constructs are supported:
    \list
     \li Simple declarations
     \li Parameters and return types of function declarations and definitions
     \li Control flow statements like if, while, for, foreach
    \endlist
*/

class CPPTOOLS_EXPORT PointerDeclarationFormatter: protected ASTVisitor
{
public:
    /*!
        \enum PointerDeclarationFormatter::CursorHandling

        This enum type simplifies the QuickFix implementation.

          \value RespectCursor
                 Consider the cursor position or selection of the CppRefactoringFile
                 for rejecting edit operation candidates for the resulting ChangeSet.
                 If there is a selection, the range of the edit operation candidate
                 should be inside the selection. If there is no selection, the cursor
                 position should be within the range of the edit operation candidate.
          \value IgnoreCursor
                 Cursor position or selection of the CppRefactoringFile will
                _not_ be considered for aborting.
     */
    enum CursorHandling { RespectCursor, IgnoreCursor };

    explicit PointerDeclarationFormatter(const CppRefactoringFilePtr &refactoringFile,
                                         Overview &overview,
                                         CursorHandling cursorHandling = IgnoreCursor);

    /*!
        Returns a ChangeSet for applying the formatting changes.
        The ChangeSet is empty if it was not possible to rewrite anything.
    */
    Utils::ChangeSet format(AST *ast)
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
    class TokenRange {
    public:
        TokenRange() : start(0), end(0) {}
        TokenRange(unsigned start, unsigned end) : start(start), end(end) {}
        unsigned start;
        unsigned end;
    };

    void processIfWhileForStatement(ExpressionAST *expression, Symbol *symbol);
    void checkAndRewrite(DeclaratorAST *declarator, Symbol *symbol, TokenRange range,
                         unsigned charactersToRemove = 0);
    void printCandidate(AST *ast);

    const CppRefactoringFilePtr m_cppRefactoringFile;
    Overview &m_overview;
    const CursorHandling m_cursorHandling;

    Utils::ChangeSet m_changeSet;
};

} // namespace CppTools
