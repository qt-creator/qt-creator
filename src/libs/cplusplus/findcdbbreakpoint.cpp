// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findcdbbreakpoint.h"

#include <cplusplus/AST.h>
#include <cplusplus/TranslationUnit.h>

#include <QDebug>

#include <typeinfo>

using namespace CPlusPlus;

FindCdbBreakpoint::FindCdbBreakpoint(TranslationUnit *unit)
    : ASTVisitor(unit)
{
}

int FindCdbBreakpoint::searchFrom(int line)
{
    m_initialLine = line;
    m_breakpointLine = NO_LINE_FOUND;

    accept(translationUnit()->ast());

    return m_breakpointLine;
}

void FindCdbBreakpoint::foundLine(unsigned tokenIndex)
{
    int dummy = 0;
    getTokenStartPosition(tokenIndex, &m_breakpointLine, &dummy);
}

int FindCdbBreakpoint::endLine(unsigned tokenIndex) const
{
    int line = 0, column = 0;
    getTokenEndPosition(tokenIndex, &line, &column);
    return line;
}

int FindCdbBreakpoint::endLine(AST *ast) const
{
    if (ast)
        return endLine(ast->lastToken() - 1);
    else
        return NO_LINE_FOUND;
}

bool FindCdbBreakpoint::preVisit(AST *ast)
{
    if (m_breakpointLine)
        return false;

    if (endLine(ast) < m_initialLine)
        return false;

    return true;
}

bool FindCdbBreakpoint::visit(FunctionDefinitionAST *ast)
{
    if (ast->function_body) {
        if (CompoundStatementAST *stmt = ast->function_body->asCompoundStatement()) {
            accept(stmt);
            if (!m_breakpointLine)
                foundLine(ast->function_body->firstToken());
            return false;
        }
    }

    return true;
}

bool FindCdbBreakpoint::visit(QtMemberDeclarationAST *ast)
{
    foundLine(ast->lastToken() - 1);
    return false;
}

bool FindCdbBreakpoint::visit(CompoundStatementAST *ast)
{
    accept(ast->statement_list);
    return false;
}

bool FindCdbBreakpoint::visit(DeclarationStatementAST *ast)
{
    foundLine(ast->lastToken() - 1);
    return m_breakpointLine == NO_LINE_FOUND;
}

bool FindCdbBreakpoint::visit(DoStatementAST *ast)
{
    accept(ast->statement);
    if (m_breakpointLine == NO_LINE_FOUND)
        foundLine(ast->rparen_token);

    return false;
}

bool FindCdbBreakpoint::visit(ExpressionStatementAST *ast)
{
    if (ast->expression) {
        accept(ast->expression);
        if (m_breakpointLine == NO_LINE_FOUND)
            foundLine(ast->semicolon_token);
    }
    return false;
}

bool FindCdbBreakpoint::visit(ForeachStatementAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    return false;
}

bool FindCdbBreakpoint::visit(RangeBasedForStatementAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    return false;
}

bool FindCdbBreakpoint::visit(ForStatementAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    return false;
}

bool FindCdbBreakpoint::visit(IfStatementAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    accept(ast->else_statement);
    return false;
}

bool FindCdbBreakpoint::visit(LabeledStatementAST *ast)
{
    foundLine(ast->lastToken() - 1);
     return false;
}

bool FindCdbBreakpoint::visit(BreakStatementAST *ast)
{
    foundLine(ast->lastToken() - 1);
    return false;
}

bool FindCdbBreakpoint::visit(ContinueStatementAST *ast)
{
    foundLine(ast->lastToken() - 1);
    return false;
}

bool FindCdbBreakpoint::visit(GotoStatementAST *ast)
{
    foundLine(ast->lastToken() - 1);
    return false;
}

bool FindCdbBreakpoint::visit(ReturnStatementAST *ast)
{
    foundLine(ast->lastToken() - 1);
    return false;
}

bool FindCdbBreakpoint::visit(SwitchStatementAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    return false;
}

bool FindCdbBreakpoint::visit(TryBlockStatementAST *ast)
{
    accept(ast->statement);
    accept(ast->catch_clause_list);
    return false;
}

bool FindCdbBreakpoint::visit(CatchClauseAST *ast)
{
    accept(ast->statement);
    return false;
}

bool FindCdbBreakpoint::visit(WhileStatementAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    return false;
}

bool FindCdbBreakpoint::visit(ObjCFastEnumerationAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    return false;
}

bool FindCdbBreakpoint::visit(ObjCSynchronizedStatementAST *ast)
{
    if (m_initialLine <= endLine(ast->rparen_token))
        foundLine(ast->rparen_token);

    accept(ast->statement);
    return false;
}
