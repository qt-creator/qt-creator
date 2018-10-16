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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmljsastfwd_p.h"
#include "qmljsglobal_p.h"

QT_QML_BEGIN_NAMESPACE

namespace QmlJS { namespace AST {

class QML_PARSER_EXPORT Visitor
{
public:
    Visitor();
    virtual ~Visitor();

    virtual bool preVisit(Node *) { return true; }
    virtual void postVisit(Node *) {}

    // Ui
    virtual bool visit(UiProgram *) { return true; }
    virtual bool visit(UiHeaderItemList *) { return true; }
    virtual bool visit(UiPragma *) { return true; }
    virtual bool visit(UiImport *) { return true; }
    virtual bool visit(UiPublicMember *) { return true; }
    virtual bool visit(UiSourceElement *) { return true; }
    virtual bool visit(UiObjectDefinition *) { return true; }
    virtual bool visit(UiObjectInitializer *) { return true; }
    virtual bool visit(UiObjectBinding *) { return true; }
    virtual bool visit(UiScriptBinding *) { return true; }
    virtual bool visit(UiArrayBinding *) { return true; }
    virtual bool visit(UiParameterList *) { return true; }
    virtual bool visit(UiObjectMemberList *) { return true; }
    virtual bool visit(UiArrayMemberList *) { return true; }
    virtual bool visit(UiQualifiedId *) { return true; }
    virtual bool visit(UiEnumDeclaration *) { return true; }
    virtual bool visit(UiEnumMemberList *) { return true; }

    virtual void endVisit(UiProgram *) {}
    virtual void endVisit(UiImport *) {}
    virtual void endVisit(UiHeaderItemList *) {}
    virtual void endVisit(UiPragma *) {}
    virtual void endVisit(UiPublicMember *) {}
    virtual void endVisit(UiSourceElement *) {}
    virtual void endVisit(UiObjectDefinition *) {}
    virtual void endVisit(UiObjectInitializer *) {}
    virtual void endVisit(UiObjectBinding *) {}
    virtual void endVisit(UiScriptBinding *) {}
    virtual void endVisit(UiArrayBinding *) {}
    virtual void endVisit(UiParameterList *) {}
    virtual void endVisit(UiObjectMemberList *) {}
    virtual void endVisit(UiArrayMemberList *) {}
    virtual void endVisit(UiQualifiedId *) {}
    virtual void endVisit(UiEnumDeclaration *) {}
    virtual void endVisit(UiEnumMemberList *) { }

    // QmlJS
    virtual bool visit(ThisExpression *) { return true; }
    virtual void endVisit(ThisExpression *) {}

    virtual bool visit(IdentifierExpression *) { return true; }
    virtual void endVisit(IdentifierExpression *) {}

    virtual bool visit(NullExpression *) { return true; }
    virtual void endVisit(NullExpression *) {}

    virtual bool visit(TrueLiteral *) { return true; }
    virtual void endVisit(TrueLiteral *) {}

    virtual bool visit(FalseLiteral *) { return true; }
    virtual void endVisit(FalseLiteral *) {}

    virtual bool visit(SuperLiteral *) { return true; }
    virtual void endVisit(SuperLiteral *) {}

    virtual bool visit(StringLiteral *) { return true; }
    virtual void endVisit(StringLiteral *) {}

    virtual bool visit(TemplateLiteral *) { return true; }
    virtual void endVisit(TemplateLiteral *) {}

    virtual bool visit(NumericLiteral *) { return true; }
    virtual void endVisit(NumericLiteral *) {}

    virtual bool visit(RegExpLiteral *) { return true; }
    virtual void endVisit(RegExpLiteral *) {}

    virtual bool visit(ArrayPattern *) { return true; }
    virtual void endVisit(ArrayPattern *) {}

    virtual bool visit(ObjectPattern *) { return true; }
    virtual void endVisit(ObjectPattern *) {}

    virtual bool visit(PatternElementList *) { return true; }
    virtual void endVisit(PatternElementList *) {}

    virtual bool visit(PatternPropertyList *) { return true; }
    virtual void endVisit(PatternPropertyList *) {}

    virtual bool visit(PatternElement *) { return true; }
    virtual void endVisit(PatternElement *) {}

    virtual bool visit(PatternProperty *) { return true; }
    virtual void endVisit(PatternProperty *) {}

    virtual bool visit(Elision *) { return true; }
    virtual void endVisit(Elision *) {}

    virtual bool visit(NestedExpression *) { return true; }
    virtual void endVisit(NestedExpression *) {}

    virtual bool visit(IdentifierPropertyName *) { return true; }
    virtual void endVisit(IdentifierPropertyName *) {}

    virtual bool visit(StringLiteralPropertyName *) { return true; }
    virtual void endVisit(StringLiteralPropertyName *) {}

    virtual bool visit(NumericLiteralPropertyName *) { return true; }
    virtual void endVisit(NumericLiteralPropertyName *) {}

    virtual bool visit(ComputedPropertyName *) { return true; }
    virtual void endVisit(ComputedPropertyName *) {}

    virtual bool visit(ArrayMemberExpression *) { return true; }
    virtual void endVisit(ArrayMemberExpression *) {}

    virtual bool visit(FieldMemberExpression *) { return true; }
    virtual void endVisit(FieldMemberExpression *) {}

    virtual bool visit(TaggedTemplate *) { return true; }
    virtual void endVisit(TaggedTemplate *) {}

    virtual bool visit(NewMemberExpression *) { return true; }
    virtual void endVisit(NewMemberExpression *) {}

    virtual bool visit(NewExpression *) { return true; }
    virtual void endVisit(NewExpression *) {}

    virtual bool visit(CallExpression *) { return true; }
    virtual void endVisit(CallExpression *) {}

    virtual bool visit(ArgumentList *) { return true; }
    virtual void endVisit(ArgumentList *) {}

    virtual bool visit(PostIncrementExpression *) { return true; }
    virtual void endVisit(PostIncrementExpression *) {}

    virtual bool visit(PostDecrementExpression *) { return true; }
    virtual void endVisit(PostDecrementExpression *) {}

    virtual bool visit(DeleteExpression *) { return true; }
    virtual void endVisit(DeleteExpression *) {}

    virtual bool visit(VoidExpression *) { return true; }
    virtual void endVisit(VoidExpression *) {}

    virtual bool visit(TypeOfExpression *) { return true; }
    virtual void endVisit(TypeOfExpression *) {}

    virtual bool visit(PreIncrementExpression *) { return true; }
    virtual void endVisit(PreIncrementExpression *) {}

    virtual bool visit(PreDecrementExpression *) { return true; }
    virtual void endVisit(PreDecrementExpression *) {}

    virtual bool visit(UnaryPlusExpression *) { return true; }
    virtual void endVisit(UnaryPlusExpression *) {}

    virtual bool visit(UnaryMinusExpression *) { return true; }
    virtual void endVisit(UnaryMinusExpression *) {}

    virtual bool visit(TildeExpression *) { return true; }
    virtual void endVisit(TildeExpression *) {}

    virtual bool visit(NotExpression *) { return true; }
    virtual void endVisit(NotExpression *) {}

    virtual bool visit(BinaryExpression *) { return true; }
    virtual void endVisit(BinaryExpression *) {}

    virtual bool visit(ConditionalExpression *) { return true; }
    virtual void endVisit(ConditionalExpression *) {}

    virtual bool visit(Expression *) { return true; }
    virtual void endVisit(Expression *) {}

    virtual bool visit(Block *) { return true; }
    virtual void endVisit(Block *) {}

    virtual bool visit(StatementList *) { return true; }
    virtual void endVisit(StatementList *) {}

    virtual bool visit(VariableStatement *) { return true; }
    virtual void endVisit(VariableStatement *) {}

    virtual bool visit(VariableDeclarationList *) { return true; }
    virtual void endVisit(VariableDeclarationList *) {}

    virtual bool visit(EmptyStatement *) { return true; }
    virtual void endVisit(EmptyStatement *) {}

    virtual bool visit(ExpressionStatement *) { return true; }
    virtual void endVisit(ExpressionStatement *) {}

    virtual bool visit(IfStatement *) { return true; }
    virtual void endVisit(IfStatement *) {}

    virtual bool visit(DoWhileStatement *) { return true; }
    virtual void endVisit(DoWhileStatement *) {}

    virtual bool visit(WhileStatement *) { return true; }
    virtual void endVisit(WhileStatement *) {}

    virtual bool visit(ForStatement *) { return true; }
    virtual void endVisit(ForStatement *) {}

    virtual bool visit(ForEachStatement *) { return true; }
    virtual void endVisit(ForEachStatement *) {}

    virtual bool visit(ContinueStatement *) { return true; }
    virtual void endVisit(ContinueStatement *) {}

    virtual bool visit(BreakStatement *) { return true; }
    virtual void endVisit(BreakStatement *) {}

    virtual bool visit(ReturnStatement *) { return true; }
    virtual void endVisit(ReturnStatement *) {}

    virtual bool visit(YieldExpression *) { return true; }
    virtual void endVisit(YieldExpression *) {}

    virtual bool visit(WithStatement *) { return true; }
    virtual void endVisit(WithStatement *) {}

    virtual bool visit(SwitchStatement *) { return true; }
    virtual void endVisit(SwitchStatement *) {}

    virtual bool visit(CaseBlock *) { return true; }
    virtual void endVisit(CaseBlock *) {}

    virtual bool visit(CaseClauses *) { return true; }
    virtual void endVisit(CaseClauses *) {}

    virtual bool visit(CaseClause *) { return true; }
    virtual void endVisit(CaseClause *) {}

    virtual bool visit(DefaultClause *) { return true; }
    virtual void endVisit(DefaultClause *) {}

    virtual bool visit(LabelledStatement *) { return true; }
    virtual void endVisit(LabelledStatement *) {}

    virtual bool visit(ThrowStatement *) { return true; }
    virtual void endVisit(ThrowStatement *) {}

    virtual bool visit(TryStatement *) { return true; }
    virtual void endVisit(TryStatement *) {}

    virtual bool visit(Catch *) { return true; }
    virtual void endVisit(Catch *) {}

    virtual bool visit(Finally *) { return true; }
    virtual void endVisit(Finally *) {}

    virtual bool visit(FunctionDeclaration *) { return true; }
    virtual void endVisit(FunctionDeclaration *) {}

    virtual bool visit(FunctionExpression *) { return true; }
    virtual void endVisit(FunctionExpression *) {}

    virtual bool visit(FormalParameterList *) { return true; }
    virtual void endVisit(FormalParameterList *) {}

    virtual bool visit(ClassExpression *) { return true; }
    virtual void endVisit(ClassExpression *) {}

    virtual bool visit(ClassDeclaration *) { return true; }
    virtual void endVisit(ClassDeclaration *) {}

    virtual bool visit(ClassElementList *) { return true; }
    virtual void endVisit(ClassElementList *) {}

    virtual bool visit(Program *) { return true; }
    virtual void endVisit(Program *) {}

    virtual bool visit(NameSpaceImport *) { return true; }
    virtual void endVisit(NameSpaceImport *) {}

    virtual bool visit(ImportSpecifier *) { return true; }
    virtual void endVisit(ImportSpecifier *) {}

    virtual bool visit(ImportsList *) { return true; }
    virtual void endVisit(ImportsList *) {}

    virtual bool visit(NamedImports *) { return true; }
    virtual void endVisit(NamedImports *) {}

    virtual bool visit(FromClause *) { return true; }
    virtual void endVisit(FromClause *) {}

    virtual bool visit(ImportClause *) { return true; }
    virtual void endVisit(ImportClause *) {}

    virtual bool visit(ImportDeclaration *) { return true; }
    virtual void endVisit(ImportDeclaration *) {}

    virtual bool visit(ExportSpecifier *) { return true; }
    virtual void endVisit(ExportSpecifier *) {}

    virtual bool visit(ExportsList *) { return true; }
    virtual void endVisit(ExportsList *) {}

    virtual bool visit(ExportClause *) { return true; }
    virtual void endVisit(ExportClause *) {}

    virtual bool visit(ExportDeclaration *) { return true; }
    virtual void endVisit(ExportDeclaration *) {}

    virtual bool visit(ModuleItem *) { return true; }
    virtual void endVisit(ModuleItem *) {}

    virtual bool visit(ESModule *) { return true; }
    virtual void endVisit(ESModule *) {}

    virtual bool visit(DebuggerStatement *) { return true; }
    virtual void endVisit(DebuggerStatement *) {}
};

} } // namespace AST

QT_QML_END_NAMESPACE

