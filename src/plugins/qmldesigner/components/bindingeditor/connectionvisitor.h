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
