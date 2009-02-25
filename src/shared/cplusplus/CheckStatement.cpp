/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "CheckStatement.h"
#include "Semantic.h"
#include "AST.h"
#include "TranslationUnit.h"
#include "Scope.h"
#include "CoreTypes.h"
#include "Control.h"
#include "Symbols.h"

CPLUSPLUS_BEGIN_NAMESPACE

CheckStatement::CheckStatement(Semantic *semantic)
    : SemanticCheck(semantic),
      _statement(0),
      _scope(0)
{ }

CheckStatement::~CheckStatement()
{ }

void CheckStatement::check(StatementAST *statement, Scope *scope)
{
    Scope *previousScope = switchScope(scope);
    StatementAST *previousStatement = switchStatement(statement);
    accept(statement);
    (void) switchStatement(previousStatement);
    (void) switchScope(previousScope);
}

StatementAST *CheckStatement::switchStatement(StatementAST *statement)
{
    StatementAST *previousStatement = _statement;
    _statement = statement;
    return previousStatement;
}

Scope *CheckStatement::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

bool CheckStatement::visit(CaseStatementAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    semantic()->check(ast->statement, _scope);
    return false;
}

bool CheckStatement::visit(CompoundStatementAST *ast)
{
    Block *block = control()->newBlock(ast->lbrace_token);
    _scope->enterSymbol(block);
    Scope *previousScope = switchScope(block->members());
    for (StatementAST *it = ast->statements; it; it = it->next) {
        semantic()->check(it, _scope);
    }
    (void) switchScope(previousScope);
    return false;
}

bool CheckStatement::visit(DeclarationStatementAST *ast)
{
    semantic()->check(ast->declaration, _scope);
    return false;
}

bool CheckStatement::visit(DoStatementAST *ast)
{
    semantic()->check(ast->statement, _scope);
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckStatement::visit(ExpressionOrDeclarationStatementAST *ast)
{
//    translationUnit()->warning(ast->firstToken(),
//                               "ambiguous expression or declaration statement");
    if (ast->declaration)
        semantic()->check(ast->declaration, _scope);
    else
        semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckStatement::visit(ExpressionStatementAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckStatement::visit(ForStatementAST *ast)
{
    Block *block = control()->newBlock(ast->for_token);
    _scope->enterSymbol(block);
    Scope *previousScope = switchScope(block->members());
    semantic()->check(ast->initializer, _scope);
    FullySpecifiedType condTy = semantic()->check(ast->condition, _scope);
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    semantic()->check(ast->statement, _scope);
    (void) switchScope(previousScope);
    return false;
}

bool CheckStatement::visit(IfStatementAST *ast)
{
    Block *block = control()->newBlock(ast->if_token);
    _scope->enterSymbol(block);
    Scope *previousScope = switchScope(block->members());
    FullySpecifiedType exprTy = semantic()->check(ast->condition, _scope);
    semantic()->check(ast->statement, _scope);
    semantic()->check(ast->else_statement, _scope);
    (void) switchScope(previousScope);
    return false;
}

bool CheckStatement::visit(LabeledStatementAST *ast)
{
    semantic()->check(ast->statement, _scope);
    return false;
}

bool CheckStatement::visit(BreakStatementAST *)
{
    return false;
}

bool CheckStatement::visit(ContinueStatementAST *)
{
    return false;
}

bool CheckStatement::visit(GotoStatementAST *)
{
    return false;
}

bool CheckStatement::visit(ReturnStatementAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckStatement::visit(SwitchStatementAST *ast)
{
    Block *block = control()->newBlock(ast->switch_token);
    _scope->enterSymbol(block);
    Scope *previousScope = switchScope(block->members());
    FullySpecifiedType condTy = semantic()->check(ast->condition, _scope);
    semantic()->check(ast->statement, _scope);
    (void) switchScope(previousScope);
    return false;
}

bool CheckStatement::visit(TryBlockStatementAST *ast)
{
    semantic()->check(ast->statement, _scope);
    for (CatchClauseAST *c = ast->catch_clause_seq; c; c = c->next) {
        semantic()->check(c, _scope);
    }
    return false;
}

bool CheckStatement::visit(CatchClauseAST *ast)
{
    Block *block = control()->newBlock(ast->catch_token);
    _scope->enterSymbol(block);
    Scope *previousScope = switchScope(block->members());
    semantic()->check(ast->exception_declaration, _scope);
    semantic()->check(ast->statement, _scope);
    (void) switchScope(previousScope);
    return false;
}

bool CheckStatement::visit(WhileStatementAST *ast)
{
    Block *block = control()->newBlock(ast->while_token);
    _scope->enterSymbol(block);
    Scope *previousScope = switchScope(block->members());
    FullySpecifiedType condTy = semantic()->check(ast->condition, _scope);
    semantic()->check(ast->statement, _scope);
    (void) switchScope(previousScope);
    return false;
}

CPLUSPLUS_END_NAMESPACE
