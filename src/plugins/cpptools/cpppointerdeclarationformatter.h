/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPPOINTERDECLARATIONFORMATTER_H
#define CPPPOINTERDECLARATIONFORMATTER_H

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

#endif // CPPPOINTERDECLARATIONFORMATTER_H
