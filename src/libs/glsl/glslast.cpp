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

void TranslationUnit::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarations, visitor);
    }
    visitor->endVisit(this);
}

void IdentifierExpression::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void LiteralExpression::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void BinaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left, visitor);
        accept(right, visitor);
    }
    visitor->endVisit(this);
}

void UnaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
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

void AssignmentExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(variable, visitor);
        accept(value, visitor);
    }
    visitor->endVisit(this);
}

void MemberAccessExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void FunctionCallExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expr, visitor);
        accept(arguments, visitor);
    }
    visitor->endVisit(this);
}

void ExpressionStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void CompoundStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(statements, visitor);
    visitor->endVisit(this);
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

void WhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

void DoStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(body, visitor);
        accept(condition, visitor);
    }
    visitor->endVisit(this);
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

void JumpStatement::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void ReturnStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void SwitchStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expr, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

void CaseLabelStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
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

void StructType::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(fields, visitor);
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
    *parent = innerType;
}

void StructType::fixInnerTypes(Type *innerType, List<Field *> *fields)
{
    if (!fields)
        return;
    List<Field *> *head = fields->next;
    List<Field *> *current = head;
    do {
        current->value->setInnerType(innerType);
        current = current->next;
    } while (current && current != head);
}
