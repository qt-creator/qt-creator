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

#ifndef FINDCDBBREAKPOINT_H
#define FINDCDBBREAKPOINT_H

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

#endif // FINDCDBBREAKPOINT_H
