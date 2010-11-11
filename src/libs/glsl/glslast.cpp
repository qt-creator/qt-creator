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
#include "glslast.h"
#include "glslastvisitor.h"
#include "glslparsertable_p.h"

using namespace GLSL;

AST::~AST()
{
}

void AST::accept(Visitor *visitor)
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

void AST::accept(AST *ast, Visitor *visitor)
{
    if (ast)
        ast->accept(visitor);
}

Statement *AST::makeCompound(Statement *left, Statement *right)
{
    if (!left)
        return right;
    else if (!right)
        return left;
    CompoundStatement *compound = left->asCompoundStatement();
    if (compound) {
        compound->statements.push_back(right);
    } else {
        compound = new CompoundStatement();
        compound->statements.push_back(left);
        compound->statements.push_back(right);
    }
    return compound;
}

void TranslationUnit::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarations, visitor);
    }
    visitor->endVisit(this);
}

IdentifierExpression::~IdentifierExpression()
{
}

void IdentifierExpression::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

LiteralExpression::~LiteralExpression()
{
}

void LiteralExpression::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

BinaryExpression::~BinaryExpression()
{
    delete left;
    delete right;
}

void BinaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left, visitor);
        accept(right, visitor);
    }
    visitor->endVisit(this);
}

UnaryExpression::~UnaryExpression()
{
    delete expr;
}

void UnaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

TernaryExpression::~TernaryExpression()
{
    delete first;
    delete second;
    delete third;
}

void TernaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(first, visitor);
        accept(second, visitor);
        accept(third, visitor);
    }
    visitor->endVisit(this);
}

AssignmentExpression::~AssignmentExpression()
{
    delete variable;
    delete value;
}

void AssignmentExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(variable, visitor);
        accept(value, visitor);
    }
    visitor->endVisit(this);
}

MemberAccessExpression::~MemberAccessExpression()
{
    delete expr;
}

void MemberAccessExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

FunctionCallExpression::~FunctionCallExpression()
{
    delete expr;
    for (std::vector<Expression *>::iterator it = arguments.begin(); it != arguments.end(); ++it)
        delete *it;
}

void FunctionCallExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expr, visitor);
        for (std::vector<Expression *>::iterator it = arguments.begin(); it != arguments.end(); ++it)
            accept(*it, visitor);
    }
    visitor->endVisit(this);
}

ExpressionStatement::~ExpressionStatement()
{
    delete expr;
}

void ExpressionStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

CompoundStatement::~CompoundStatement()
{
    for (std::vector<Statement *>::iterator it = statements.begin(); it != statements.end(); ++it)
        delete *it;
}

void CompoundStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (std::vector<Statement *>::iterator it = statements.begin(); it != statements.end(); ++it)
            accept(*it, visitor);
    }
    visitor->endVisit(this);
}

IfStatement::~IfStatement()
{
    delete condition;
    delete thenClause;
    delete elseClause;
}

void IfStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(thenClause, visitor);
        accept(elseClause, visitor);
    }
    visitor->endVisit(this);
}

WhileStatement::~WhileStatement()
{
    delete condition;
    delete body;
}

void WhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

DoStatement::~DoStatement()
{
    delete body;
    delete condition;
}

void DoStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(body, visitor);
        accept(condition, visitor);
    }
    visitor->endVisit(this);
}

ForStatement::~ForStatement()
{
    delete init;
    delete condition;
    delete increment;
    delete body;
}

void ForStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(init, visitor);
        accept(condition, visitor);
        accept(increment, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

JumpStatement::~JumpStatement()
{
}

void JumpStatement::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

ReturnStatement::~ReturnStatement()
{
    delete expr;
}

void ReturnStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

SwitchStatement::~SwitchStatement()
{
    delete expr;
    delete body;
}

void SwitchStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expr, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

CaseLabelStatement::~CaseLabelStatement()
{
    delete expr;
}

void CaseLabelStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

Type *Type::clone(Type *type)
{
    if (!type)
        return 0;
    Type *c = type->clone();
    c->lineno = type->lineno;
    return c;
}

BasicType::BasicType(int _token)
    : Type(Kind_BasicType), token(_token)
{
    switch (token) {
    case GLSLParserTable::T_VOID:
    case GLSLParserTable::T_BOOL:
    case GLSLParserTable::T_BVEC2:
    case GLSLParserTable::T_BVEC3:
    case GLSLParserTable::T_BVEC4:
        prec = PrecNotValid;
        break;
    default:
        prec = PrecUnspecified;
        break;
    }
}

BasicType::~BasicType()
{
}

void BasicType::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

Type::Precision BasicType::precision() const
{
    return prec;
}

bool BasicType::setPrecision(Precision precision)
{
    if (prec == PrecNotValid)
        return false;
    prec = precision;
    return true;
}

Type *BasicType::clone() const
{
    return new BasicType(token, prec);
}

NamedType::~NamedType()
{
}

void NamedType::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

Type::Precision NamedType::precision() const
{
    // Named types are typically structs, which cannot have their precision set.
    return PrecNotValid;
}

bool NamedType::setPrecision(Precision)
{
    return false;
}

Type *NamedType::clone() const
{
    return new NamedType(name);
}

ArrayType::~ArrayType()
{
    delete elementType;
    delete size;
}

void ArrayType::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(elementType, visitor);
        accept(size, visitor);
    }
    visitor->endVisit(this);
}

Type::Precision ArrayType::precision() const
{
    return elementType ? elementType->precision() : PrecNotValid;
}

bool ArrayType::setPrecision(Precision precision)
{
    if (elementType)
        return elementType->setPrecision(precision);
    else
        return false;
}

Type *ArrayType::clone() const
{
    if (kind == Kind_ArrayType)
        return new ArrayType(Type::clone(elementType), size);
    else
        return new ArrayType(Type::clone(elementType));
}

StructType::~StructType()
{
    for (std::vector<Field *>::iterator it = fields.begin(); it != fields.end(); ++it)
        delete *it;
}

void StructType::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (std::vector<Field *>::iterator it = fields.begin(); it != fields.end(); ++it)
            accept(*it, visitor);
    }
    visitor->endVisit(this);
}

Type::Precision StructType::precision() const
{
    return PrecNotValid;
}

bool StructType::setPrecision(Precision)
{
    // Structs cannot have a precision set.
    return false;
}

Type *StructType::clone() const
{
    StructType *stype;
    if (kind == Kind_AnonymousStructType)
        stype = new StructType();
    else
        stype = new StructType(name);
    for (std::vector<Field *>::const_iterator it = fields.begin(); it != fields.end(); ++it) {
        stype->fields.push_back
            (new Field((*it)->name, Type::clone((*it)->type)));
    }
    return stype;
}

StructType::Field::~Field()
{
    delete type;
}

void StructType::Field::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(type, visitor);
    visitor->endVisit(this);
}

void StructType::Field::setInnerType(Type *innerType)
{
    if (!innerType)
        return;
    Type **parent = &type;
    Type *inner = type;
    while (inner != 0) {
        ArrayType *array = inner->asArrayType();
        if (!array)
            break;
        parent = &(array->elementType);
        inner = array->elementType;
    }
    *parent = Type::clone(innerType);
}

void StructType::fixInnerTypes(Type *innerType, std::vector<Field *> &fields)
{
    for (size_t index = 0; index < fields.size(); ++index)
        fields[index]->setInnerType(innerType);
    delete innerType;   // No longer needed - cloned into all fields.
}

void StructType::addFields(const std::vector<Field *> &list)
{
    for (size_t index = 0; index < list.size(); ++index)
        fields.push_back(list[index]);
}
