// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include "parser/qmljsastvisitor_p.h"
#include "qmljsdocument.h"

namespace QmlJS {

class QMLJS_EXPORT ScopeAstPath: protected AST::Visitor
{
public:
    ScopeAstPath(Document::Ptr doc);

    QList<AST::Node *> operator()(quint32 offset);

protected:
    void accept(AST::Node *node);

    using Visitor::visit;

    bool preVisit(AST::Node *node) override;
    bool visit(AST::TemplateLiteral *node) override;
    bool visit(AST::UiPublicMember *node) override;
    bool visit(AST::UiScriptBinding *node) override;
    bool visit(AST::UiObjectDefinition *node) override;
    bool visit(AST::UiObjectBinding *node) override;
    bool visit(AST::FunctionDeclaration *node) override;
    bool visit(AST::FunctionExpression *node) override;

    void throwRecursionDepthError() override;
private:
    bool containsOffset(SourceLocation start, SourceLocation end);

    QList<AST::Node *> _result;
    Document::Ptr _doc;
    quint32 _offset = 0;
};

} // namespace QmlJS
