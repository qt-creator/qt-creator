/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLCHECK_H
#define QMLCHECK_H

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
    virtual ~Evaluate();

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
    virtual bool visit(AST::UiProgram *ast);
    virtual bool visit(AST::UiImportList *ast);
    virtual bool visit(AST::UiImport *ast);
    virtual bool visit(AST::UiPublicMember *ast);
    virtual bool visit(AST::UiSourceElement *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiObjectInitializer *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiScriptBinding *ast);
    virtual bool visit(AST::UiArrayBinding *ast);
    virtual bool visit(AST::UiObjectMemberList *ast);
    virtual bool visit(AST::UiArrayMemberList *ast);
    virtual bool visit(AST::UiQualifiedId *ast);

    // QmlJS
    virtual bool visit(AST::ThisExpression *ast);
    virtual bool visit(AST::IdentifierExpression *ast);
    virtual bool visit(AST::NullExpression *ast);
    virtual bool visit(AST::TrueLiteral *ast);
    virtual bool visit(AST::FalseLiteral *ast);
    virtual bool visit(AST::StringLiteral *ast);
    virtual bool visit(AST::NumericLiteral *ast);
    virtual bool visit(AST::RegExpLiteral *ast);
    virtual bool visit(AST::ArrayLiteral *ast);
    virtual bool visit(AST::ObjectLiteral *ast);
    virtual bool visit(AST::ElementList *ast);
    virtual bool visit(AST::Elision *ast);
    virtual bool visit(AST::PropertyNameAndValueList *ast);
    virtual bool visit(AST::NestedExpression *ast);
    virtual bool visit(AST::IdentifierPropertyName *ast);
    virtual bool visit(AST::StringLiteralPropertyName *ast);
    virtual bool visit(AST::NumericLiteralPropertyName *ast);
    virtual bool visit(AST::ArrayMemberExpression *ast);
    virtual bool visit(AST::FieldMemberExpression *ast);
    virtual bool visit(AST::NewMemberExpression *ast);
    virtual bool visit(AST::NewExpression *ast);
    virtual bool visit(AST::CallExpression *ast);
    virtual bool visit(AST::ArgumentList *ast);
    virtual bool visit(AST::PostIncrementExpression *ast);
    virtual bool visit(AST::PostDecrementExpression *ast);
    virtual bool visit(AST::DeleteExpression *ast);
    virtual bool visit(AST::VoidExpression *ast);
    virtual bool visit(AST::TypeOfExpression *ast);
    virtual bool visit(AST::PreIncrementExpression *ast);
    virtual bool visit(AST::PreDecrementExpression *ast);
    virtual bool visit(AST::UnaryPlusExpression *ast);
    virtual bool visit(AST::UnaryMinusExpression *ast);
    virtual bool visit(AST::TildeExpression *ast);
    virtual bool visit(AST::NotExpression *ast);
    virtual bool visit(AST::BinaryExpression *ast);
    virtual bool visit(AST::ConditionalExpression *ast);
    virtual bool visit(AST::Expression *ast);
    virtual bool visit(AST::Block *ast);
    virtual bool visit(AST::StatementList *ast);
    virtual bool visit(AST::VariableStatement *ast);
    virtual bool visit(AST::VariableDeclarationList *ast);
    virtual bool visit(AST::VariableDeclaration *ast);
    virtual bool visit(AST::EmptyStatement *ast);
    virtual bool visit(AST::ExpressionStatement *ast);
    virtual bool visit(AST::IfStatement *ast);
    virtual bool visit(AST::DoWhileStatement *ast);
    virtual bool visit(AST::WhileStatement *ast);
    virtual bool visit(AST::ForStatement *ast);
    virtual bool visit(AST::LocalForStatement *ast);
    virtual bool visit(AST::ForEachStatement *ast);
    virtual bool visit(AST::LocalForEachStatement *ast);
    virtual bool visit(AST::ContinueStatement *ast);
    virtual bool visit(AST::BreakStatement *ast);
    virtual bool visit(AST::ReturnStatement *ast);
    virtual bool visit(AST::WithStatement *ast);
    virtual bool visit(AST::SwitchStatement *ast);
    virtual bool visit(AST::CaseBlock *ast);
    virtual bool visit(AST::CaseClauses *ast);
    virtual bool visit(AST::CaseClause *ast);
    virtual bool visit(AST::DefaultClause *ast);
    virtual bool visit(AST::LabelledStatement *ast);
    virtual bool visit(AST::ThrowStatement *ast);
    virtual bool visit(AST::TryStatement *ast);
    virtual bool visit(AST::Catch *ast);
    virtual bool visit(AST::Finally *ast);
    virtual bool visit(AST::FunctionDeclaration *ast);
    virtual bool visit(AST::FunctionExpression *ast);
    virtual bool visit(AST::FormalParameterList *ast);
    virtual bool visit(AST::FunctionBody *ast);
    virtual bool visit(AST::Program *ast);
    virtual bool visit(AST::SourceElements *ast);
    virtual bool visit(AST::FunctionSourceElement *ast);
    virtual bool visit(AST::StatementSourceElement *ast);
    virtual bool visit(AST::DebuggerStatement *ast);

private:
    QmlJS::Document::Ptr _doc;
    ValueOwner *_valueOwner;
    ContextPtr _context;
    ReferenceContext *_referenceContext;
    const ScopeChain *_scopeChain;
    const Value *_result;
};

} // namespace Qml

#endif // QMLCheck_H
