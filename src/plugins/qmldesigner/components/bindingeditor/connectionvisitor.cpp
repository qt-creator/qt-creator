/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "connectionvisitor.h"

namespace QmlDesigner {

ConnectionVisitor::ConnectionVisitor()
{
}

bool ConnectionVisitor::visit(QmlJS::AST::TemplateLiteral *ast)
{
    m_expression.append(
        qMakePair(QmlJS::AST::Node::Kind::Kind_StringLiteral, ast->value.toString()));
    QmlJS::AST::Node::accept(ast->expression, this);
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::StringLiteral *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_StringLiteral,
                                  ast->value.toString()));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::NumericLiteral *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_NumericLiteral,
                                  QString::number(ast->value)));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::TrueLiteral *ast)
{
    Q_UNUSED(ast)
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_TrueLiteral, QString("true")));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::FalseLiteral *ast)
{
    Q_UNUSED(ast)
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_FalseLiteral, QString("false")));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::BinaryExpression *ast)
{
    Q_UNUSED(ast)
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_BinaryExpression,
                                  QString()));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::CallExpression *ast)
{
    Q_UNUSED(ast)
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_CallExpression,
                                  QString()));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::ArgumentList *ast)
{
    Q_UNUSED(ast)
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_ArgumentList,
                                  QString()));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::FunctionExpression *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_FunctionExpression,
                                  ast->name.toString()));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::FieldMemberExpression *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_FieldMemberExpression,
                                  ast->name.toString()));
    return true;
}

bool ConnectionVisitor::visit(QmlJS::AST::IdentifierExpression *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_IdentifierExpression,
                                  ast->name.toString()));
    return true;
}

void ConnectionVisitor::throwRecursionDepthError()
{
    qWarning("Warning: Hit maximum recursion depth while visiting AST in ConnectionVisitor");
}

} // QmlDesigner namespace
