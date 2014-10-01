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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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

#include <utils/qtcoverride.h>

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
    bool visit(AST::UiProgram *ast) QTC_OVERRIDE;
    bool visit(AST::UiHeaderItemList *ast) QTC_OVERRIDE;
    bool visit(AST::UiQualifiedPragmaId *ast) QTC_OVERRIDE;
    bool visit(AST::UiPragma *ast) QTC_OVERRIDE;
    bool visit(AST::UiImport *ast) QTC_OVERRIDE;
    bool visit(AST::UiPublicMember *ast) QTC_OVERRIDE;
    bool visit(AST::UiSourceElement *ast) QTC_OVERRIDE;
    bool visit(AST::UiObjectDefinition *ast) QTC_OVERRIDE;
    bool visit(AST::UiObjectInitializer *ast) QTC_OVERRIDE;
    bool visit(AST::UiObjectBinding *ast) QTC_OVERRIDE;
    bool visit(AST::UiScriptBinding *ast) QTC_OVERRIDE;
    bool visit(AST::UiArrayBinding *ast) QTC_OVERRIDE;
    bool visit(AST::UiObjectMemberList *ast) QTC_OVERRIDE;
    bool visit(AST::UiArrayMemberList *ast) QTC_OVERRIDE;
    bool visit(AST::UiQualifiedId *ast) QTC_OVERRIDE;

    // QmlJS
    bool visit(AST::ThisExpression *ast) QTC_OVERRIDE;
    bool visit(AST::IdentifierExpression *ast) QTC_OVERRIDE;
    bool visit(AST::NullExpression *ast) QTC_OVERRIDE;
    bool visit(AST::TrueLiteral *ast) QTC_OVERRIDE;
    bool visit(AST::FalseLiteral *ast) QTC_OVERRIDE;
    bool visit(AST::StringLiteral *ast) QTC_OVERRIDE;
    bool visit(AST::NumericLiteral *ast) QTC_OVERRIDE;
    bool visit(AST::RegExpLiteral *ast) QTC_OVERRIDE;
    bool visit(AST::ArrayLiteral *ast) QTC_OVERRIDE;
    bool visit(AST::ObjectLiteral *ast) QTC_OVERRIDE;
    bool visit(AST::ElementList *ast) QTC_OVERRIDE;
    bool visit(AST::Elision *ast) QTC_OVERRIDE;
    bool visit(AST::PropertyAssignmentList *ast) QTC_OVERRIDE;
    bool visit(AST::PropertyGetterSetter *ast) QTC_OVERRIDE;
    bool visit(AST::PropertyNameAndValue *ast) QTC_OVERRIDE;
    bool visit(AST::NestedExpression *ast) QTC_OVERRIDE;
    bool visit(AST::IdentifierPropertyName *ast) QTC_OVERRIDE;
    bool visit(AST::StringLiteralPropertyName *ast) QTC_OVERRIDE;
    bool visit(AST::NumericLiteralPropertyName *ast) QTC_OVERRIDE;
    bool visit(AST::ArrayMemberExpression *ast) QTC_OVERRIDE;
    bool visit(AST::FieldMemberExpression *ast) QTC_OVERRIDE;
    bool visit(AST::NewMemberExpression *ast) QTC_OVERRIDE;
    bool visit(AST::NewExpression *ast) QTC_OVERRIDE;
    bool visit(AST::CallExpression *ast) QTC_OVERRIDE;
    bool visit(AST::ArgumentList *ast) QTC_OVERRIDE;
    bool visit(AST::PostIncrementExpression *ast) QTC_OVERRIDE;
    bool visit(AST::PostDecrementExpression *ast) QTC_OVERRIDE;
    bool visit(AST::DeleteExpression *ast) QTC_OVERRIDE;
    bool visit(AST::VoidExpression *ast) QTC_OVERRIDE;
    bool visit(AST::TypeOfExpression *ast) QTC_OVERRIDE;
    bool visit(AST::PreIncrementExpression *ast) QTC_OVERRIDE;
    bool visit(AST::PreDecrementExpression *ast) QTC_OVERRIDE;
    bool visit(AST::UnaryPlusExpression *ast) QTC_OVERRIDE;
    bool visit(AST::UnaryMinusExpression *ast) QTC_OVERRIDE;
    bool visit(AST::TildeExpression *ast) QTC_OVERRIDE;
    bool visit(AST::NotExpression *ast) QTC_OVERRIDE;
    bool visit(AST::BinaryExpression *ast) QTC_OVERRIDE;
    bool visit(AST::ConditionalExpression *ast) QTC_OVERRIDE;
    bool visit(AST::Expression *ast) QTC_OVERRIDE;
    bool visit(AST::Block *ast) QTC_OVERRIDE;
    bool visit(AST::StatementList *ast) QTC_OVERRIDE;
    bool visit(AST::VariableStatement *ast) QTC_OVERRIDE;
    bool visit(AST::VariableDeclarationList *ast) QTC_OVERRIDE;
    bool visit(AST::VariableDeclaration *ast) QTC_OVERRIDE;
    bool visit(AST::EmptyStatement *ast) QTC_OVERRIDE;
    bool visit(AST::ExpressionStatement *ast) QTC_OVERRIDE;
    bool visit(AST::IfStatement *ast) QTC_OVERRIDE;
    bool visit(AST::DoWhileStatement *ast) QTC_OVERRIDE;
    bool visit(AST::WhileStatement *ast) QTC_OVERRIDE;
    bool visit(AST::ForStatement *ast) QTC_OVERRIDE;
    bool visit(AST::LocalForStatement *ast) QTC_OVERRIDE;
    bool visit(AST::ForEachStatement *ast) QTC_OVERRIDE;
    bool visit(AST::LocalForEachStatement *ast) QTC_OVERRIDE;
    bool visit(AST::ContinueStatement *ast) QTC_OVERRIDE;
    bool visit(AST::BreakStatement *ast) QTC_OVERRIDE;
    bool visit(AST::ReturnStatement *ast) QTC_OVERRIDE;
    bool visit(AST::WithStatement *ast) QTC_OVERRIDE;
    bool visit(AST::SwitchStatement *ast) QTC_OVERRIDE;
    bool visit(AST::CaseBlock *ast) QTC_OVERRIDE;
    bool visit(AST::CaseClauses *ast) QTC_OVERRIDE;
    bool visit(AST::CaseClause *ast) QTC_OVERRIDE;
    bool visit(AST::DefaultClause *ast) QTC_OVERRIDE;
    bool visit(AST::LabelledStatement *ast) QTC_OVERRIDE;
    bool visit(AST::ThrowStatement *ast) QTC_OVERRIDE;
    bool visit(AST::TryStatement *ast) QTC_OVERRIDE;
    bool visit(AST::Catch *ast) QTC_OVERRIDE;
    bool visit(AST::Finally *ast) QTC_OVERRIDE;
    bool visit(AST::FunctionDeclaration *ast) QTC_OVERRIDE;
    bool visit(AST::FunctionExpression *ast) QTC_OVERRIDE;
    bool visit(AST::FormalParameterList *ast) QTC_OVERRIDE;
    bool visit(AST::FunctionBody *ast) QTC_OVERRIDE;
    bool visit(AST::Program *ast) QTC_OVERRIDE;
    bool visit(AST::SourceElements *ast) QTC_OVERRIDE;
    bool visit(AST::FunctionSourceElement *ast) QTC_OVERRIDE;
    bool visit(AST::StatementSourceElement *ast) QTC_OVERRIDE;
    bool visit(AST::DebuggerStatement *ast) QTC_OVERRIDE;

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
