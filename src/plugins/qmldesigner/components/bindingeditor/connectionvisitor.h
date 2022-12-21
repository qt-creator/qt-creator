// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CONNECTIONVISITOR_H
#define CONNECTIONVISITOR_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner {

class ConnectionVisitor : public QmlJS::AST::Visitor
{
public:
    explicit ConnectionVisitor();

    bool visit(QmlJS::AST::TemplateLiteral *ast) override;
    bool visit(QmlJS::AST::StringLiteral *ast) override;
    bool visit(QmlJS::AST::NumericLiteral *ast) override;
    bool visit(QmlJS::AST::TrueLiteral *ast) override;
    bool visit(QmlJS::AST::FalseLiteral *ast) override;

    bool visit(QmlJS::AST::BinaryExpression *ast) override;
    bool visit(QmlJS::AST::CallExpression *ast) override;

    bool visit(QmlJS::AST::ArgumentList *ast) override;
    bool visit(QmlJS::AST::FunctionExpression *ast) override; // unused

    bool visit(QmlJS::AST::FieldMemberExpression *ast) override;
    bool visit(QmlJS::AST::IdentifierExpression *ast) override;

    void throwRecursionDepthError() override;

    const QList<QPair<QmlJS::AST::Node::Kind, QString>> &expression() const {
        return m_expression;
    }

private:
    QList<QPair<QmlJS::AST::Node::Kind, QString>> m_expression;
};

}

#endif //CONNECTIONVISITOR_H
