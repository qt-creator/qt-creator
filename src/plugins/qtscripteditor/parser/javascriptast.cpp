/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "javascriptast_p.h"
#include "javascriptastvisitor_p.h"

QT_BEGIN_NAMESPACE

namespace JavaScript { namespace AST {

ExpressionNode *Node::expressionCast()
{
    return 0;
}

BinaryExpression *Node::binaryExpressionCast()
{
    return 0;
}

Statement *Node::statementCast()
{
    return 0;
}

ExpressionNode *ExpressionNode::expressionCast()
{
    return this;
}

BinaryExpression *BinaryExpression::binaryExpressionCast()
{
    return this;
}

Statement *Statement::statementCast()
{
    return this;
}

void ThisExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void IdentifierExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NullExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void TrueLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void FalseLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void StringLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NumericLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void RegExpLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ArrayLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(elements, visitor);
        acceptChild(elision, visitor);
    }

    visitor->endVisit(this);
}

void ObjectLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(properties, visitor);
    }

    visitor->endVisit(this);
}

void ElementList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        ElementList *it = this;
        do {
            acceptChild(it->elision, visitor);
            acceptChild(it->expression, visitor);
            it = it->next;
        } while (it);
    }

    visitor->endVisit(this);
}

void Elision::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        // ###
    }

    visitor->endVisit(this);
}

void PropertyNameAndValueList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        PropertyNameAndValueList *it = this;
        do {
            acceptChild(it->name, visitor);
            acceptChild(it->value, visitor);
            it = it->next;
        } while (it);
    }

    visitor->endVisit(this);
}

void IdentifierPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void StringLiteralPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NumericLiteralPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ArrayMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(base, visitor);
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void FieldMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(base, visitor);
    }

    visitor->endVisit(this);
}

void NewMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(base, visitor);
        acceptChild(arguments, visitor);
    }

    visitor->endVisit(this);
}

void NewExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void CallExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(base, visitor);
        acceptChild(arguments, visitor);
    }

    visitor->endVisit(this);
}

void ArgumentList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        ArgumentList *it = this;
        do {
            acceptChild(it->expression, visitor);
            it = it->next;
        } while (it);
    }

    visitor->endVisit(this);
}

void PostIncrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(base, visitor);
    }

    visitor->endVisit(this);
}

void PostDecrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(base, visitor);
    }

    visitor->endVisit(this);
}

void DeleteExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void VoidExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void TypeOfExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void PreIncrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void PreDecrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void UnaryPlusExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void UnaryMinusExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void TildeExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void NotExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void BinaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(left, visitor);
        acceptChild(right, visitor);
    }

    visitor->endVisit(this);
}

void ConditionalExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
        acceptChild(ok, visitor);
        acceptChild(ko, visitor);
    }

    visitor->endVisit(this);
}

void Expression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(left, visitor);
        acceptChild(right, visitor);
    }

    visitor->endVisit(this);
}

void Block::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statements, visitor);
    }

    visitor->endVisit(this);
}

void StatementList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        StatementList *it = this;
        do {
            acceptChild(it->statement, visitor);
            it = it->next;
        } while (it);
    }

    visitor->endVisit(this);
}

void VariableStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(declarations, visitor);
    }

    visitor->endVisit(this);
}

void VariableDeclarationList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        VariableDeclarationList *it = this;
        do {
            acceptChild(it->declaration, visitor);
            it = it->next;
        } while (it);
    }

    visitor->endVisit(this);
}

void VariableDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void EmptyStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ExpressionStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void IfStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
        acceptChild(ok, visitor);
        acceptChild(ko, visitor);
    }

    visitor->endVisit(this);
}

void DoWhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statement, visitor);
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void WhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void ForStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(initialiser, visitor);
        acceptChild(condition, visitor);
        acceptChild(expression, visitor);
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void LocalForStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(declarations, visitor);
        acceptChild(condition, visitor);
        acceptChild(expression, visitor);
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void ForEachStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(initialiser, visitor);
        acceptChild(expression, visitor);
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void LocalForEachStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(declaration, visitor);
        acceptChild(expression, visitor);
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void ContinueStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void BreakStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ReturnStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void WithStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void SwitchStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
        acceptChild(block, visitor);
    }

    visitor->endVisit(this);
}

void CaseBlock::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(clauses, visitor);
        acceptChild(defaultClause, visitor);
        acceptChild(moreClauses, visitor);
    }

    visitor->endVisit(this);
}

void CaseClauses::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        CaseClauses *it = this;
        do {
            acceptChild(it->clause, visitor);
            it = it->next;
        } while (it);
    }

    visitor->endVisit(this);
}

void CaseClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
        acceptChild(statements, visitor);
    }

    visitor->endVisit(this);
}

void DefaultClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statements, visitor);
    }

    visitor->endVisit(this);
}

void LabelledStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void ThrowStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(expression, visitor);
    }

    visitor->endVisit(this);
}

void TryStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statement, visitor);
        acceptChild(catchExpression, visitor);
        acceptChild(finallyExpression, visitor);
    }

    visitor->endVisit(this);
}

void Catch::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void Finally::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void FunctionDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(formals, visitor);
        acceptChild(body, visitor);
    }

    visitor->endVisit(this);
}

void FunctionExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(formals, visitor);
        acceptChild(body, visitor);
    }

    visitor->endVisit(this);
}

void FormalParameterList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        // ###
    }

    visitor->endVisit(this);
}

void FunctionBody::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(elements, visitor);
    }

    visitor->endVisit(this);
}

void Program::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(elements, visitor);
    }

    visitor->endVisit(this);
}

void SourceElements::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        SourceElements *it = this;
        do {
            acceptChild(it->element, visitor);
            it = it->next;
        } while (it);
    }

    visitor->endVisit(this);
}

void FunctionSourceElement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(declaration, visitor);
    }

    visitor->endVisit(this);
}

void StatementSourceElement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        acceptChild(statement, visitor);
    }

    visitor->endVisit(this);
}

void DebuggerStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

} } // namespace JavaScript::AST

QT_END_NAMESPACE


