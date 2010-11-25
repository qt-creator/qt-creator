/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "glslsemantic.h"

using namespace GLSL;

Semantic::Semantic()
{
}

Semantic::~Semantic()
{
}

void Semantic::expression(ExpressionAST *ast)
{
    accept(ast);
}

void Semantic::statement(StatementAST *ast)
{
    accept(ast);
}

void Semantic::type(TypeAST *ast)
{
    accept(ast);
}

void Semantic::declaration(DeclarationAST *ast)
{
    accept(ast);
}

void Semantic::translationUnit(TranslationUnitAST *ast)
{
    accept(ast);
}

void Semantic::functionIdentifier(FunctionIdentifierAST *ast)
{
    accept(ast);
}

void Semantic::field(StructTypeAST::Field *ast)
{
    accept(ast);
}

bool Semantic::visit(TranslationUnitAST *ast)
{
    for (List<DeclarationAST *> *it = ast->declarations; it; it = it->next) {
        DeclarationAST *decl = it->value;
        declaration(decl);
    }
    return false;
}

bool Semantic::visit(FunctionIdentifierAST *ast)
{
    // ast->name
    type(ast->type);
    return false;
}

bool Semantic::visit(StructTypeAST::Field *ast)
{
    // ast->name
    type(ast->type);
    return false;
}


// expressions
bool Semantic::visit(IdentifierExpressionAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(LiteralExpressionAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(BinaryExpressionAST *ast)
{
    expression(ast->left);
    expression(ast->right);
    return false;
}

bool Semantic::visit(UnaryExpressionAST *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(TernaryExpressionAST *ast)
{
    expression(ast->first);
    expression(ast->second);
    expression(ast->third);
    return false;
}

bool Semantic::visit(AssignmentExpressionAST *ast)
{
    expression(ast->variable);
    expression(ast->value);
    return false;
}

bool Semantic::visit(MemberAccessExpressionAST *ast)
{
    expression(ast->expr);
    // ast->field
    return false;
}

bool Semantic::visit(FunctionCallExpressionAST *ast)
{
    expression(ast->expr);
    functionIdentifier(ast->id);
    for (List<ExpressionAST *> *it = ast->arguments; it; it = it->next) {
        ExpressionAST *arg = it->value;
        expression(arg);
    }

    return false;
}

bool Semantic::visit(DeclarationExpressionAST *ast)
{
    type(ast->type);
    // ast->name
    expression(ast->initializer);
    return false;
}


// statements
bool Semantic::visit(ExpressionStatementAST *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(CompoundStatementAST *ast)
{
    for (List<StatementAST *> *it = ast->statements; it; it = it->next) {
        StatementAST *stmt = it->value;
        statement(stmt);
    }
    return false;
}

bool Semantic::visit(IfStatementAST *ast)
{
    expression(ast->condition);
    statement(ast->thenClause);
    statement(ast->elseClause);
    return false;
}

bool Semantic::visit(WhileStatementAST *ast)
{
    expression(ast->condition);
    statement(ast->body);
    return false;
}

bool Semantic::visit(DoStatementAST *ast)
{
    statement(ast->body);
    expression(ast->condition);
    return false;
}

bool Semantic::visit(ForStatementAST *ast)
{
    statement(ast->init);
    expression(ast->condition);
    expression(ast->increment);
    statement(ast->body);
    return false;
}

bool Semantic::visit(JumpStatementAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(ReturnStatementAST *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(SwitchStatementAST *ast)
{
    expression(ast->expr);
    statement(ast->body);
    return false;
}

bool Semantic::visit(CaseLabelStatementAST *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(DeclarationStatementAST *ast)
{
    for (List<DeclarationAST *> *it = ast->decls; it; it = it->next) {
        DeclarationAST *decl = it->value;
        declaration(decl);
    }
    return false;
}


// types
bool Semantic::visit(BasicTypeAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(NamedTypeAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(ArrayTypeAST *ast)
{
    type(ast->elementType);
    expression(ast->size);
    return false;
}

bool Semantic::visit(StructTypeAST *ast)
{
    // ast->name
    for (List<StructTypeAST::Field *> *it = ast->fields; it; it = it->next) {
        StructTypeAST::Field *f = it->value;
        field(f);
    }
    return false;
}

bool Semantic::visit(QualifiedTypeAST *ast)
{
    accept(ast->type);
    for (List<LayoutQualifier *> *it = ast->layout_list; it; it = it->next) {
        LayoutQualifier *q = it->value;
        // q->name;
        // q->number;
        Q_UNUSED(q);
    }
    return false;
}


// declarations
bool Semantic::visit(PrecisionDeclarationAST *ast)
{
    type(ast->type);
    return false;
}

bool Semantic::visit(ParameterDeclarationAST *ast)
{
    type(ast->type);
    return false;
}

bool Semantic::visit(VariableDeclarationAST *ast)
{
    type(ast->type);
    expression(ast->initializer);
    return false;
}

bool Semantic::visit(TypeDeclarationAST *ast)
{
    type(ast->type);
    return false;
}

bool Semantic::visit(TypeAndVariableDeclarationAST *ast)
{
    declaration(ast->typeDecl);
    declaration(ast->varDecl);
    return false;
}

bool Semantic::visit(InvariantDeclarationAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(InitDeclarationAST *ast)
{
    for (List<DeclarationAST *> *it = ast->decls; it; it = it->next) {
        DeclarationAST *decl = it->value;
        declaration(decl);
    }
    return false;
}

bool Semantic::visit(FunctionDeclarationAST *ast)
{
    type(ast->returnType);
    for (List<ParameterDeclarationAST *> *it = ast->params; it; it = it->next) {
        ParameterDeclarationAST *decl = it->value;
        declaration(decl);
    }
    statement(ast->body);
    return false;
}

