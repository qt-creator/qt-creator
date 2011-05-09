#include "findcdbbreakpoint.h"

#include <AST.h>
#include <TranslationUnit.h>

#include <typeinfo>
#include <QDebug>

using namespace CPlusPlus;

FindCdbBreakpoint::FindCdbBreakpoint(TranslationUnit *unit)
    : ASTVisitor(unit)
{
}

unsigned FindCdbBreakpoint::searchFrom(unsigned line)
{
    m_initialLine = line;
    m_breakpointLine = 0;

    accept(translationUnit()->ast());

    return m_breakpointLine;
}

void FindCdbBreakpoint::foundLine(unsigned tokenIndex)
{
    unsigned dummy = 0;
    getTokenStartPosition(tokenIndex, &m_breakpointLine, &dummy);
}

unsigned FindCdbBreakpoint::endLine(unsigned tokenIndex) const
{
    unsigned line = 0, column = 0;
    getTokenEndPosition(tokenIndex, &line, &column);
    return line;
}

unsigned FindCdbBreakpoint::endLine(AST *ast) const
{
    if (ast)
        return endLine(ast->lastToken() - 1);
    else
        return 0;
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
    if (ast->ctor_initializer) {
        foundLine(ast->function_body->firstToken());
        return false;
    }

    if (ast->function_body) {
        if (CompoundStatementAST *stmt = ast->function_body->asCompoundStatement()) {
            accept(stmt);
            if (!m_breakpointLine)
                foundLine(stmt->lastToken() - 1);
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

bool FindCdbBreakpoint::visit(CaseStatementAST *ast)
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
    return m_breakpointLine == 0;
}

bool FindCdbBreakpoint::visit(DoStatementAST *ast)
{
    accept(ast->statement);
    if (m_breakpointLine == 0)
        foundLine(ast->rparen_token);

    return false;
}

bool FindCdbBreakpoint::visit(ExpressionStatementAST *ast)
{
    if (ast->expression)
        foundLine(ast->semicolon_token);
    return false;
}

bool FindCdbBreakpoint::visit(ForeachStatementAST *ast)
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
