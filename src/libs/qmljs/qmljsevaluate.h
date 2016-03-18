/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "parser/qmljsastvisitor_p.h"
#include "qmljsdocument.h"
#include "qmljsscopechain.h"


namespace QmlJS {

class ValueOwner;
class Context;
class Value;
class ObjectValue;
class FunctionValue;

class QMLJS_EXPORT Evaluate: protected AST::Visitor
{
public:
    Evaluate(const ScopeChain *scopeChain, ReferenceContext *referenceContext = 0);
    ~Evaluate();

    // same as value()
    const Value *operator()(AST::Node *ast);

    // evaluate ast in the given context, resolving references
    const Value *value(AST::Node *ast);

    // evaluate, but stop when encountering a Reference
    const Value *reference(AST::Node *ast);

protected:
    void accept(AST::Node *node);

    const Value *switchResult(const Value *result);

    // Ui
    bool visit(AST::UiProgram *ast) override;
    bool visit(AST::UiHeaderItemList *ast) override;
    bool visit(AST::UiQualifiedPragmaId *ast) override;
    bool visit(AST::UiPragma *ast) override;
    bool visit(AST::UiImport *ast) override;
    bool visit(AST::UiPublicMember *ast) override;
    bool visit(AST::UiSourceElement *ast) override;
    bool visit(AST::UiObjectDefinition *ast) override;
    bool visit(AST::UiObjectInitializer *ast) override;
    bool visit(AST::UiObjectBinding *ast) override;
    bool visit(AST::UiScriptBinding *ast) override;
    bool visit(AST::UiArrayBinding *ast) override;
    bool visit(AST::UiObjectMemberList *ast) override;
    bool visit(AST::UiArrayMemberList *ast) override;
    bool visit(AST::UiQualifiedId *ast) override;

    // QmlJS
    bool visit(AST::ThisExpression *ast) override;
    bool visit(AST::IdentifierExpression *ast) override;
    bool visit(AST::NullExpression *ast) override;
    bool visit(AST::TrueLiteral *ast) override;
    bool visit(AST::FalseLiteral *ast) override;
    bool visit(AST::StringLiteral *ast) override;
    bool visit(AST::NumericLiteral *ast) override;
    bool visit(AST::RegExpLiteral *ast) override;
    bool visit(AST::ArrayLiteral *ast) override;
    bool visit(AST::ObjectLiteral *ast) override;
    bool visit(AST::ElementList *ast) override;
    bool visit(AST::Elision *ast) override;
    bool visit(AST::PropertyAssignmentList *ast) override;
    bool visit(AST::PropertyGetterSetter *ast) override;
    bool visit(AST::PropertyNameAndValue *ast) override;
    bool visit(AST::NestedExpression *ast) override;
    bool visit(AST::IdentifierPropertyName *ast) override;
    bool visit(AST::StringLiteralPropertyName *ast) override;
    bool visit(AST::NumericLiteralPropertyName *ast) override;
    bool visit(AST::ArrayMemberExpression *ast) override;
    bool visit(AST::FieldMemberExpression *ast) override;
    bool visit(AST::NewMemberExpression *ast) override;
    bool visit(AST::NewExpression *ast) override;
    bool visit(AST::CallExpression *ast) override;
    bool visit(AST::ArgumentList *ast) override;
    bool visit(AST::PostIncrementExpression *ast) override;
    bool visit(AST::PostDecrementExpression *ast) override;
    bool visit(AST::DeleteExpression *ast) override;
    bool visit(AST::VoidExpression *ast) override;
    bool visit(AST::TypeOfExpression *ast) override;
    bool visit(AST::PreIncrementExpression *ast) override;
    bool visit(AST::PreDecrementExpression *ast) override;
    bool visit(AST::UnaryPlusExpression *ast) override;
    bool visit(AST::UnaryMinusExpression *ast) override;
    bool visit(AST::TildeExpression *ast) override;
    bool visit(AST::NotExpression *ast) override;
    bool visit(AST::BinaryExpression *ast) override;
    bool visit(AST::ConditionalExpression *ast) override;
    bool visit(AST::Expression *ast) override;
    bool visit(AST::Block *ast) override;
    bool visit(AST::StatementList *ast) override;
    bool visit(AST::VariableStatement *ast) override;
    bool visit(AST::VariableDeclarationList *ast) override;
    bool visit(AST::VariableDeclaration *ast) override;
    bool visit(AST::EmptyStatement *ast) override;
    bool visit(AST::ExpressionStatement *ast) override;
    bool visit(AST::IfStatement *ast) override;
    bool visit(AST::DoWhileStatement *ast) override;
    bool visit(AST::WhileStatement *ast) override;
    bool visit(AST::ForStatement *ast) override;
    bool visit(AST::LocalForStatement *ast) override;
    bool visit(AST::ForEachStatement *ast) override;
    bool visit(AST::LocalForEachStatement *ast) override;
    bool visit(AST::ContinueStatement *ast) override;
    bool visit(AST::BreakStatement *ast) override;
    bool visit(AST::ReturnStatement *ast) override;
    bool visit(AST::WithStatement *ast) override;
    bool visit(AST::SwitchStatement *ast) override;
    bool visit(AST::CaseBlock *ast) override;
    bool visit(AST::CaseClauses *ast) override;
    bool visit(AST::CaseClause *ast) override;
    bool visit(AST::DefaultClause *ast) override;
    bool visit(AST::LabelledStatement *ast) override;
    bool visit(AST::ThrowStatement *ast) override;
    bool visit(AST::TryStatement *ast) override;
    bool visit(AST::Catch *ast) override;
    bool visit(AST::Finally *ast) override;
    bool visit(AST::FunctionDeclaration *ast) override;
    bool visit(AST::FunctionExpression *ast) override;
    bool visit(AST::FormalParameterList *ast) override;
    bool visit(AST::FunctionBody *ast) override;
    bool visit(AST::Program *ast) override;
    bool visit(AST::SourceElements *ast) override;
    bool visit(AST::FunctionSourceElement *ast) override;
    bool visit(AST::StatementSourceElement *ast) override;
    bool visit(AST::DebuggerStatement *ast) override;

private:
    QmlJS::Document::Ptr _doc;
    ValueOwner *_valueOwner;
    ContextPtr _context;
    ReferenceContext *_referenceContext;
    const ScopeChain *_scopeChain;
    const Value *_result;
};

} // namespace Qml
