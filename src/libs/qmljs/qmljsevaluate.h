/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLJSEVALUATE_H
#define QMLJSEVALUATE_H

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
    bool visit(AST::UiProgram *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiHeaderItemList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiQualifiedPragmaId *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiPragma *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiImport *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiPublicMember *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiSourceElement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectDefinition *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectInitializer *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiScriptBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiArrayBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectMemberList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiArrayMemberList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiQualifiedId *ast) Q_DECL_OVERRIDE;

    // QmlJS
    bool visit(AST::ThisExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::IdentifierExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NullExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::TrueLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FalseLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::StringLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NumericLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::RegExpLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ArrayLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ObjectLiteral *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ElementList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Elision *ast) Q_DECL_OVERRIDE;
    bool visit(AST::PropertyAssignmentList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::PropertyGetterSetter *ast) Q_DECL_OVERRIDE;
    bool visit(AST::PropertyNameAndValue *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NestedExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::IdentifierPropertyName *ast) Q_DECL_OVERRIDE;
    bool visit(AST::StringLiteralPropertyName *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NumericLiteralPropertyName *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ArrayMemberExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FieldMemberExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NewMemberExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NewExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::CallExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ArgumentList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::PostIncrementExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::PostDecrementExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::DeleteExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::VoidExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::TypeOfExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::PreIncrementExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::PreDecrementExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UnaryPlusExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UnaryMinusExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::TildeExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NotExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::BinaryExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ConditionalExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Expression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Block *ast) Q_DECL_OVERRIDE;
    bool visit(AST::StatementList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::VariableStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::VariableDeclarationList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::VariableDeclaration *ast) Q_DECL_OVERRIDE;
    bool visit(AST::EmptyStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ExpressionStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::IfStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::DoWhileStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::WhileStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ForStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::LocalForStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ForEachStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::LocalForEachStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ContinueStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::BreakStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ReturnStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::WithStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::SwitchStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::CaseBlock *ast) Q_DECL_OVERRIDE;
    bool visit(AST::CaseClauses *ast) Q_DECL_OVERRIDE;
    bool visit(AST::CaseClause *ast) Q_DECL_OVERRIDE;
    bool visit(AST::DefaultClause *ast) Q_DECL_OVERRIDE;
    bool visit(AST::LabelledStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ThrowStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::TryStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Catch *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Finally *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FunctionDeclaration *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FunctionExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FormalParameterList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FunctionBody *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Program *ast) Q_DECL_OVERRIDE;
    bool visit(AST::SourceElements *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FunctionSourceElement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::StatementSourceElement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::DebuggerStatement *ast) Q_DECL_OVERRIDE;

private:
    QmlJS::Document::Ptr _doc;
    ValueOwner *_valueOwner;
    ContextPtr _context;
    ReferenceContext *_referenceContext;
    const ScopeChain *_scopeChain;
    const Value *_result;
};

} // namespace Qml

#endif // QMLJSEVALUATE_H
