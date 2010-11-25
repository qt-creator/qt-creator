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
#include "glslengine.h"
#include "glslparser.h"
#include "glslsymbols.h"
#include "glsltypes.h"
#include <QtCore/QDebug>

using namespace GLSL;

Semantic::Semantic(Engine *engine)
    : _engine(engine)
    , _type(0)
    , _scope(0)
{
}

Semantic::~Semantic()
{
}

Scope *Semantic::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

void Semantic::expression(ExpressionAST *ast)
{
    accept(ast);
}

void Semantic::statement(StatementAST *ast)
{
    accept(ast);
}

const Type *Semantic::type(TypeAST *ast)
{
    const Type *t = _engine->undefinedType();
    std::swap(_type, t);
    accept(ast);
    std::swap(_type, t);
    return t;
}

void Semantic::declaration(DeclarationAST *ast)
{
    accept(ast);
}

Scope *Semantic::translationUnit(TranslationUnitAST *ast)
{
    Block *globalScope = _engine->newBlock();
    Scope *previousScope = switchScope(globalScope);
    for (List<DeclarationAST *> *it = ast->declarations; it; it = it->next) {
        DeclarationAST *decl = it->value;
        declaration(decl);
    }
    return switchScope(previousScope);
}

void Semantic::functionIdentifier(FunctionIdentifierAST *ast)
{
    accept(ast);
}

Symbol *Semantic::field(StructTypeAST::Field *ast)
{
    // ast->name
    const Type *ty = type(ast->type);
    QString name;
    if (ast->name)
        name = *ast->name;
    return _engine->newVariable(_scope, name, ty);
}

void Semantic::parameterDeclaration(ParameterDeclarationAST *ast, Function *fun)
{
    const Type *ty = type(ast->type);
    QString name;
    if (ast->name)
        name = *ast->name;
    Argument *arg = _engine->newArgument(fun, name, ty);
    fun->addArgument(arg);
}

bool Semantic::visit(TranslationUnitAST *ast)
{
    Q_UNUSED(ast);
    Q_ASSERT(!"unreachable");
    return false;
}

bool Semantic::visit(FunctionIdentifierAST *ast)
{
    // ast->name
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
    return false;
}

bool Semantic::visit(StructTypeAST::Field *ast)
{
    Q_UNUSED(ast);
    Q_ASSERT(!"unreachable");
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
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
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
    switch (ast->token) {
    case Parser::T_VOID:
        _type = _engine->voidType();
        break;

    case Parser::T_BOOL:
        _type = _engine->boolType();
        break;

    case Parser::T_INT:
        _type = _engine->intType();
        break;

    case Parser::T_UINT:
        _type = _engine->uintType();
        break;

    case Parser::T_FLOAT:
        _type = _engine->floatType();
        break;

    case Parser::T_DOUBLE:
        _type = _engine->doubleType();
        break;

    // bvec
    case Parser::T_BVEC2:
        _type = _engine->vectorType(_engine->boolType(), 2);
        break;

    case Parser::T_BVEC3:
        _type = _engine->vectorType(_engine->boolType(), 3);
        break;

    case Parser::T_BVEC4:
        _type = _engine->vectorType(_engine->boolType(), 4);
        break;

    // ivec
    case Parser::T_IVEC2:
        _type = _engine->vectorType(_engine->intType(), 2);
        break;

    case Parser::T_IVEC3:
        _type = _engine->vectorType(_engine->intType(), 3);
        break;

    case Parser::T_IVEC4:
        _type = _engine->vectorType(_engine->intType(), 4);
        break;

    // uvec
    case Parser::T_UVEC2:
        _type = _engine->vectorType(_engine->uintType(), 2);
        break;

    case Parser::T_UVEC3:
        _type = _engine->vectorType(_engine->uintType(), 3);
        break;

    case Parser::T_UVEC4:
        _type = _engine->vectorType(_engine->uintType(), 4);
        break;

    // vec
    case Parser::T_VEC2:
        _type = _engine->vectorType(_engine->floatType(), 2);
        break;

    case Parser::T_VEC3:
        _type = _engine->vectorType(_engine->floatType(), 3);
        break;

    case Parser::T_VEC4:
        _type = _engine->vectorType(_engine->floatType(), 4);
        break;

    // dvec
    case Parser::T_DVEC2:
        _type = _engine->vectorType(_engine->doubleType(), 2);
        break;

    case Parser::T_DVEC3:
        _type = _engine->vectorType(_engine->doubleType(), 3);
        break;

    case Parser::T_DVEC4:
        _type = _engine->vectorType(_engine->doubleType(), 4);
        break;

    // mat2
    case Parser::T_MAT2:
    case Parser::T_MAT2X2:
        _type = _engine->matrixType(_engine->floatType(), 2, 2);
        break;

    case Parser::T_MAT2X3:
        _type = _engine->matrixType(_engine->floatType(), 2, 3);
        break;

    case Parser::T_MAT2X4:
        _type = _engine->matrixType(_engine->floatType(), 2, 4);
        break;

    // mat3
    case Parser::T_MAT3X2:
        _type = _engine->matrixType(_engine->floatType(), 3, 2);
        break;

    case Parser::T_MAT3:
    case Parser::T_MAT3X3:
        _type = _engine->matrixType(_engine->floatType(), 3, 3);
        break;

    case Parser::T_MAT3X4:
        _type = _engine->matrixType(_engine->floatType(), 3, 4);
        break;

    // mat4
    case Parser::T_MAT4X2:
        _type = _engine->matrixType(_engine->floatType(), 4, 2);
        break;

    case Parser::T_MAT4X3:
        _type = _engine->matrixType(_engine->floatType(), 4, 3);
        break;

    case Parser::T_MAT4:
    case Parser::T_MAT4X4:
        _type = _engine->matrixType(_engine->floatType(), 4, 4);
        break;


    // dmat2
    case Parser::T_DMAT2:
    case Parser::T_DMAT2X2:
        _type = _engine->matrixType(_engine->doubleType(), 2, 2);
        break;

    case Parser::T_DMAT2X3:
        _type = _engine->matrixType(_engine->doubleType(), 2, 3);
        break;

    case Parser::T_DMAT2X4:
        _type = _engine->matrixType(_engine->doubleType(), 2, 4);
        break;

    // dmat3
    case Parser::T_DMAT3X2:
        _type = _engine->matrixType(_engine->doubleType(), 3, 2);
        break;

    case Parser::T_DMAT3:
    case Parser::T_DMAT3X3:
        _type = _engine->matrixType(_engine->doubleType(), 3, 3);
        break;

    case Parser::T_DMAT3X4:
        _type = _engine->matrixType(_engine->doubleType(), 3, 4);
        break;

    // dmat4
    case Parser::T_DMAT4X2:
        _type = _engine->matrixType(_engine->doubleType(), 4, 2);
        break;

    case Parser::T_DMAT4X3:
        _type = _engine->matrixType(_engine->doubleType(), 4, 3);
        break;

    case Parser::T_DMAT4:
    case Parser::T_DMAT4X4:
        _type = _engine->matrixType(_engine->doubleType(), 4, 4);
        break;

    default:
        qDebug() << "unknown type:" << GLSLParserTable::spell[ast->token];
    }

    return false;
}

bool Semantic::visit(NamedTypeAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(ArrayTypeAST *ast)
{
    const Type *elementType = type(ast->elementType);
    Q_UNUSED(elementType);
    expression(ast->size);
    return false;
}

bool Semantic::visit(StructTypeAST *ast)
{
    Struct *s = _engine->newStruct(_scope);
    if (ast->name)
        s->setName(*ast->name);
    Scope *previousScope = switchScope(s);
    for (List<StructTypeAST::Field *> *it = ast->fields; it; it = it->next) {
        StructTypeAST::Field *f = it->value;
        if (Symbol *member = field(f))
            s->add(member);
    }
    (void) switchScope(previousScope);
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
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
    return false;
}

bool Semantic::visit(ParameterDeclarationAST *ast)
{
    Q_UNUSED(ast);
    Q_ASSERT(!"unreachable");
    return false;
}

bool Semantic::visit(VariableDeclarationAST *ast)
{
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
    expression(ast->initializer);
    return false;
}

bool Semantic::visit(TypeDeclarationAST *ast)
{
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
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
    Function *fun = _engine->newFunction(_scope);
    if (ast->name)
        fun->setName(*ast->name);

    fun->setReturnType(type(ast->returnType));

    for (List<ParameterDeclarationAST *> *it = ast->params; it; it = it->next) {
        ParameterDeclarationAST *decl = it->value;
        parameterDeclaration(decl, fun);
    }

    if (Scope *enclosingScope = fun->scope())
        enclosingScope->add(fun);

    Scope *previousScope = switchScope(fun);
    statement(ast->body);
    (void) switchScope(previousScope);
    return false;
}

