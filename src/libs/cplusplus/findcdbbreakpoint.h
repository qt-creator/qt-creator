// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/ASTVisitor.h>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT FindCdbBreakpoint: protected ASTVisitor
{
public:
    static const int NO_LINE_FOUND = 0;

public:
    FindCdbBreakpoint(TranslationUnit *unit);

    int operator()(int line) { return searchFrom(line); }

    /**
     * Search for the next breakable line of code.
     *
     * \param line the starting line from where the next breakable code line
     *        should be found
     * \return the next breakable code line (1-based), or \c NO_LINE_FOUND if
     *         no line could be found.
     */
    int searchFrom(int line);

protected:
    void foundLine(unsigned tokenIndex);
    int endLine(unsigned tokenIndex) const;
    int endLine(AST *ast) const;

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
    int m_initialLine;

    int m_breakpointLine;
};

} // namespace CPlusPlus
