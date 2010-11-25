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

void Semantic::expression(Expression *ast)
{
    accept(ast);
}

void Semantic::statement(Statement *ast)
{
    accept(ast);
}

void Semantic::type(Type *ast)
{
    accept(ast);
}

void Semantic::declaration(Declaration *ast)
{
    accept(ast);
}

void Semantic::translationUnit(TranslationUnit *ast)
{
    accept(ast);
}

void Semantic::functionIdentifier(FunctionIdentifier *ast)
{
    accept(ast);
}

void Semantic::field(StructType::Field *ast)
{
    accept(ast);
}

bool Semantic::visit(TranslationUnit *ast)
{
    for (List<Declaration *> *it = ast->declarations; it; it = it->next) {
        Declaration *decl = it->value;
        declaration(decl);
    }
    return false;
}

bool Semantic::visit(FunctionIdentifier *ast)
{
    // ast->name
    type(ast->type);
    return false;
}

bool Semantic::visit(StructType::Field *ast)
{
    // ast->name
    type(ast->type);
    return false;
}


// expressions
bool Semantic::visit(IdentifierExpression *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(LiteralExpression *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(BinaryExpression *ast)
{
    expression(ast->left);
    expression(ast->right);
    return false;
}

bool Semantic::visit(UnaryExpression *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(TernaryExpression *ast)
{
    expression(ast->first);
    expression(ast->second);
    expression(ast->third);
    return false;
}

bool Semantic::visit(AssignmentExpression *ast)
{
    expression(ast->variable);
    expression(ast->value);
    return false;
}

bool Semantic::visit(MemberAccessExpression *ast)
{
    expression(ast->expr);
    // ast->field
    return false;
}

bool Semantic::visit(FunctionCallExpression *ast)
{
    expression(ast->expr);
    functionIdentifier(ast->id);
    for (List<Expression *> *it = ast->arguments; it; it = it->next) {
        Expression *arg = it->value;
        expression(arg);
    }

    return false;
}

bool Semantic::visit(DeclarationExpression *ast)
{
    type(ast->type);
    // ast->name
    expression(ast->initializer);
    return false;
}


// statements
bool Semantic::visit(ExpressionStatement *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(CompoundStatement *ast)
{
    for (List<Statement *> *it = ast->statements; it; it = it->next) {
        Statement *stmt = it->value;
        statement(stmt);
    }
    return false;
}

bool Semantic::visit(IfStatement *ast)
{
    expression(ast->condition);
    statement(ast->thenClause);
    statement(ast->elseClause);
    return false;
}

bool Semantic::visit(WhileStatement *ast)
{
    expression(ast->condition);
    statement(ast->body);
    return false;
}

bool Semantic::visit(DoStatement *ast)
{
    statement(ast->body);
    expression(ast->condition);
    return false;
}

bool Semantic::visit(ForStatement *ast)
{
    statement(ast->init);
    expression(ast->condition);
    expression(ast->increment);
    statement(ast->body);
    return false;
}

bool Semantic::visit(JumpStatement *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(ReturnStatement *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(SwitchStatement *ast)
{
    expression(ast->expr);
    statement(ast->body);
    return false;
}

bool Semantic::visit(CaseLabelStatement *ast)
{
    expression(ast->expr);
    return false;
}

bool Semantic::visit(DeclarationStatement *ast)
{
    for (List<Declaration *> *it = ast->decls; it; it = it->next) {
        Declaration *decl = it->value;
        declaration(decl);
    }
    return false;
}


// types
bool Semantic::visit(BasicType *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(NamedType *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(ArrayType *ast)
{
    type(ast->elementType);
    expression(ast->size);
    return false;
}

bool Semantic::visit(StructType *ast)
{
    // ast->name
    for (List<StructType::Field *> *it = ast->fields; it; it = it->next) {
        StructType::Field *f = it->value;
        field(f);
    }
    return false;
}

bool Semantic::visit(QualifiedType *ast)
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
bool Semantic::visit(PrecisionDeclaration *ast)
{
    type(ast->type);
    return false;
}

bool Semantic::visit(ParameterDeclaration *ast)
{
    type(ast->type);
    return false;
}

bool Semantic::visit(VariableDeclaration *ast)
{
    type(ast->type);
    expression(ast->initializer);
    return false;
}

bool Semantic::visit(TypeDeclaration *ast)
{
    type(ast->type);
    return false;
}

bool Semantic::visit(TypeAndVariableDeclaration *ast)
{
    declaration(ast->typeDecl);
    declaration(ast->varDecl);
    return false;
}

bool Semantic::visit(InvariantDeclaration *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(InitDeclaration *ast)
{
    for (List<Declaration *> *it = ast->decls; it; it = it->next) {
        Declaration *decl = it->value;
        declaration(decl);
    }
    return false;
}

bool Semantic::visit(FunctionDeclaration *ast)
{
    type(ast->returnType);
    for (List<ParameterDeclaration *> *it = ast->params; it; it = it->next) {
        ParameterDeclaration *decl = it->value;
        declaration(decl);
    }
    statement(ast->body);
    return false;
}

