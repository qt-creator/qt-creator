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

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/ASTVisitor.h>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT FindCdbBreakpoint: protected ASTVisitor
{
public:
    static const unsigned NO_LINE_FOUND = 0;

public:
    FindCdbBreakpoint(TranslationUnit *unit);

    unsigned operator()(unsigned line)
    { return searchFrom(line); }

    /**
     * Search for the next breakable line of code.
     *
     * \param line the starting line from where the next breakable code line
     *        should be found
     * \return the next breakable code line (1-based), or \c NO_LINE_FOUND if
     *         no line could be found.
     */
    unsigned searchFrom(unsigned line);

protected:
    void foundLine(unsigned tokenIndex);
    unsigned endLine(unsigned tokenIndex) const;
    unsigned endLine(AST *ast) const;

protected:
    using ASTVisitor::visit;

    bool preVisit(AST *ast);

    bool visit(FunctionDefinitionAST *ast);

    // Statements:
    bool visit(QtMemberDeclarationAST *ast);
    bool visit(CompoundStatementAST *ast);
    bool visit(DeclarationStatementAST *ast);
    bool visit(DoStatementAST *ast);
    bool visit(ExpressionStatementAST *ast);
    bool visit(ForeachStatementAST *ast);
    bool visit(RangeBasedForStatementAST *ast);
    bool visit(ForStatementAST *ast);
    bool visit(IfStatementAST *ast);
    bool visit(LabeledStatementAST *ast);
    bool visit(BreakStatementAST *ast);
    bool visit(ContinueStatementAST *ast);
    bool visit(GotoStatementAST *ast);
    bool visit(ReturnStatementAST *ast);
    bool visit(SwitchStatementAST *ast);
    bool visit(TryBlockStatementAST *ast);
    bool visit(CatchClauseAST *ast);
    bool visit(WhileStatementAST *ast);
    bool visit(ObjCFastEnumerationAST *ast);
    bool visit(ObjCSynchronizedStatementAST *ast);

private:
    unsigned m_initialLine;

    unsigned m_breakpointLine;
};

} // namespace CPlusPlus
