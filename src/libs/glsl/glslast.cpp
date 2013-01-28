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

void TranslationUnitAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarations, visitor);
    }
    visitor->endVisit(this);
}

void IdentifierExpressionAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void LiteralExpressionAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void BinaryExpressionAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left, visitor);
        accept(right, visitor);
    }
    visitor->endVisit(this);
}

void UnaryExpressionAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void TernaryExpressionAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(first, visitor);
        accept(second, visitor);
        accept(third, visitor);
    }
    visitor->endVisit(this);
}

void AssignmentExpressionAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(variable, visitor);
        accept(value, visitor);
    }
    visitor->endVisit(this);
}

void MemberAccessExpressionAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void FunctionCallExpressionAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expr, visitor);
        accept(id, visitor);
        accept(arguments, visitor);
    }
    visitor->endVisit(this);
}

void FunctionIdentifierAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(type, visitor);
    visitor->endVisit(this);
}

void DeclarationExpressionAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(type, visitor);
        accept(initializer, visitor);
    }
    visitor->endVisit(this);
}

void ExpressionStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void CompoundStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(statements, visitor);
    visitor->endVisit(this);
}

void IfStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(thenClause, visitor);
        accept(elseClause, visitor);
    }
    visitor->endVisit(this);
}

void WhileStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

void DoStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(body, visitor);
        accept(condition, visitor);
    }
    visitor->endVisit(this);
}

void ForStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(init, visitor);
        accept(condition, visitor);
        accept(increment, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

void JumpStatementAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void ReturnStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void SwitchStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expr, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}

void CaseLabelStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(expr, visitor);
    visitor->endVisit(this);
}

void DeclarationStatementAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(decl, visitor);
    visitor->endVisit(this);
}

BasicTypeAST::BasicTypeAST(int _token, const char *_name)
    : TypeAST(Kind_BasicType), token(_token), name(_name)
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

void BasicTypeAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

TypeAST::Precision BasicTypeAST::precision() const
{
    return prec;
}

bool BasicTypeAST::setPrecision(Precision precision)
{
    if (prec == PrecNotValid)
        return false;
    prec = precision;
    return true;
}

void NamedTypeAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

TypeAST::Precision NamedTypeAST::precision() const
{
    // Named types are typically structs, which cannot have their precision set.
    return PrecNotValid;
}

bool NamedTypeAST::setPrecision(Precision)
{
    return false;
}

void ArrayTypeAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(elementType, visitor);
        accept(size, visitor);
    }
    visitor->endVisit(this);
}

TypeAST::Precision ArrayTypeAST::precision() const
{
    return elementType ? elementType->precision() : PrecNotValid;
}

bool ArrayTypeAST::setPrecision(Precision precision)
{
    if (elementType)
        return elementType->setPrecision(precision);
    else
        return false;
}

void StructTypeAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(fields, visitor);
    visitor->endVisit(this);
}

TypeAST::Precision StructTypeAST::precision() const
{
    return PrecNotValid;
}

bool StructTypeAST::setPrecision(Precision)
{
    // Structs cannot have a precision set.
    return false;
}

void StructTypeAST::Field::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(type, visitor);
    visitor->endVisit(this);
}

void StructTypeAST::Field::setInnerType(TypeAST *innerType)
{
    if (!innerType)
        return;
    TypeAST **parent = &type;
    TypeAST *inner = type;
    while (inner != 0) {
        ArrayTypeAST *array = inner->asArrayType();
        if (!array)
            break;
        parent = &(array->elementType);
        inner = array->elementType;
    }
    *parent = innerType;
}

List<StructTypeAST::Field *> *StructTypeAST::fixInnerTypes(TypeAST *innerType, List<Field *> *fields)
{
    if (!fields)
        return fields;
    List<Field *> *head = fields->next;
    List<Field *> *current = head;
    do {
        current->value->setInnerType(innerType);
        current = current->next;
    } while (current && current != head);
    return fields;
}

void LayoutQualifierAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
}

void QualifiedTypeAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(type, visitor);
    visitor->endVisit(this);
}

void PrecisionDeclarationAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(type, visitor);
    visitor->endVisit(this);
}

void ParameterDeclarationAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(type, visitor);
    visitor->endVisit(this);
}

void VariableDeclarationAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(type, visitor);
        accept(initializer, visitor);
    }
    visitor->endVisit(this);
}

TypeAST *VariableDeclarationAST::declarationType(List<DeclarationAST *> *decls)
{
    VariableDeclarationAST *var = decls->value->asVariableDeclaration();
    return var ? var->type : 0;
}

void TypeDeclarationAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(type, visitor);
    visitor->endVisit(this);
}

void TypeAndVariableDeclarationAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(typeDecl, visitor);
        accept(varDecl, visitor);
    }
    visitor->endVisit(this);
}

void InvariantDeclarationAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void InitDeclarationAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(decls, visitor);
    visitor->endVisit(this);
}

void FunctionDeclarationAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(returnType, visitor);
        accept(params, visitor);
        accept(body, visitor);
    }
    visitor->endVisit(this);
}
