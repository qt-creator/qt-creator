// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsscopeastpath.h"

#include "parser/qmljsast_p.h"
#include <QDebug>

using namespace QmlJS;
using namespace AST;

ScopeAstPath::ScopeAstPath(Document::Ptr doc)
    : _doc(doc)
{
}

QList<Node *> ScopeAstPath::operator()(quint32 offset)
{
    _result.clear();
    _offset = offset;
    if (_doc)
        accept(_doc->ast());
    return _result;
}

void ScopeAstPath::accept(Node *node)
{
    Node::accept(node, this);
}

bool ScopeAstPath::preVisit(Node *node)
{
    if (Statement *stmt = node->statementCast())
        return containsOffset(stmt->firstSourceLocation(), stmt->lastSourceLocation());
    else if (ExpressionNode *exp = node->expressionCast())
        return containsOffset(exp->firstSourceLocation(), exp->lastSourceLocation());
    else if (UiObjectMember *ui = node->uiObjectMemberCast())
        return containsOffset(ui->firstSourceLocation(), ui->lastSourceLocation());
    return true;
}

bool ScopeAstPath::visit(AST::TemplateLiteral *node)
{
    Node::accept(node->expression, this);
    return true;
}

bool ScopeAstPath::visit(UiPublicMember *node)
{
    if (node && node->statement && node->statement->kind == node->Kind_Block
            && containsOffset(node->statement->firstSourceLocation(),
                              node->statement->lastSourceLocation())) {
        _result.append(node);
        accept(node->statement);
        return false;
    }
    return true;
}

bool ScopeAstPath::visit(UiScriptBinding *node)
{
    if (node && node->statement && node->statement->kind == node->Kind_Block
            && containsOffset(node->statement->firstSourceLocation(),
                              node->statement->lastSourceLocation()))
    {
        _result.append(node);
        accept(node->statement);
        return false;
    }
    return true;
}

bool ScopeAstPath::visit(UiObjectDefinition *node)
{
    _result.append(node);
    accept(node->initializer);
    return false;
}

bool ScopeAstPath::visit(UiObjectBinding *node)
{
    _result.append(node);
    accept(node->initializer);
    return false;
}

bool ScopeAstPath::visit(FunctionDeclaration *node)
{
    return visit(static_cast<FunctionExpression *>(node));
}

bool ScopeAstPath::visit(FunctionExpression *node)
{
    accept(node->formals);
    _result.append(node);
    accept(node->body);
    return false;
}

void ScopeAstPath::throwRecursionDepthError()
{
    qWarning("ScopeAstPath hit the maximum recursion limit while visiting the AST.");
}

bool ScopeAstPath::containsOffset(SourceLocation start, SourceLocation end)
{
    return _offset >= start.begin() && _offset <= end.end();
}
