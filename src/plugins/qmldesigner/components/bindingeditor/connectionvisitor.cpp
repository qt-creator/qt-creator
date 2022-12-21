// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

bool ConnectionVisitor::visit([[maybe_unused]] QmlJS::AST::TrueLiteral *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_TrueLiteral, QString("true")));
    return true;
}

bool ConnectionVisitor::visit([[maybe_unused]] QmlJS::AST::FalseLiteral *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_FalseLiteral, QString("false")));
    return true;
}

bool ConnectionVisitor::visit([[maybe_unused]] QmlJS::AST::BinaryExpression *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_BinaryExpression,
                                  QString()));
    return true;
}

bool ConnectionVisitor::visit([[maybe_unused]] QmlJS::AST::CallExpression *ast)
{
    m_expression.append(qMakePair(QmlJS::AST::Node::Kind::Kind_CallExpression,
                                  QString()));
    return true;
}

bool ConnectionVisitor::visit([[maybe_unused]] QmlJS::AST::ArgumentList *ast)
{
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
