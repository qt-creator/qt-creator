// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "connectioneditorstatements.h"

#include "qmldesigner_global.h"

#include <qmljs/parser/qmljsastvisitor_p.h>

namespace QmlDesigner {

class ConnectionEditorEvaluatorPrivate;

class QMLDESIGNER_EXPORT ConnectionEditorEvaluator : public QmlJS::AST::Visitor
{
public:
    enum Status { UnStarted, UnFinished, Succeeded, Failed };

    ConnectionEditorEvaluator();
    virtual ~ConnectionEditorEvaluator();

    Status status() const;
    ConnectionEditorStatements::Handler resultNode() const;

    static QString getDisplayStringForType(const QString &statement);
    static ConnectionEditorStatements::Handler parseStatement(const QString &statement);

protected:
    bool preVisit(QmlJS::AST::Node *node) override;
    void postVisit(QmlJS::AST::Node *node) override;

    bool visit(QmlJS::AST::Program *program) override;
    bool visit(QmlJS::AST::IfStatement *ifStatement) override;
    bool visit(QmlJS::AST::IdentifierExpression *identifier) override;
    bool visit(QmlJS::AST::BinaryExpression *binaryExpression) override;
    bool visit(QmlJS::AST::FieldMemberExpression *fieldExpression) override;
    bool visit(QmlJS::AST::CallExpression *callExpression) override;
    bool visit(QmlJS::AST::Block *block) override;
    bool visit(QmlJS::AST::ArgumentList *arguments) override;

    void endVisit(QmlJS::AST::Program *program) override;
    void endVisit(QmlJS::AST::FieldMemberExpression *fieldExpression) override;
    void endVisit(QmlJS::AST::CallExpression *callExpression) override;
    void endVisit(QmlJS::AST::IfStatement *ifStatement) override;
    void endVisit(QmlJS::AST::StatementList *statementList) override;

    void throwRecursionDepthError() override;

private:
    std::unique_ptr<ConnectionEditorEvaluatorPrivate> d;
};

} // namespace QmlDesigner
