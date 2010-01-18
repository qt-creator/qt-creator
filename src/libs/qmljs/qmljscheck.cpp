/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljscheck.h"
#include "parser/qmljsast_p.h"

using namespace QmlJS;

Check::Check()
{
}

Check::~Check()
{
}

void Check::operator()(Document::Ptr doc)
{
    _doc = doc;
}

void Check::accept(AST::Node *node)
{
    AST::Node::accept(node, this);
}

bool Check::visit(AST::UiProgram *)
{
    return true;
}

bool Check::visit(AST::UiImportList *)
{
    return true;
}

bool Check::visit(AST::UiImport *)
{
    return true;
}

bool Check::visit(AST::UiPublicMember *)
{
    return true;
}

bool Check::visit(AST::UiSourceElement *)
{
    return true;
}

bool Check::visit(AST::UiObjectDefinition *)
{
    return true;
}

bool Check::visit(AST::UiObjectInitializer *)
{
    return true;
}

bool Check::visit(AST::UiObjectBinding *)
{
    return true;
}

bool Check::visit(AST::UiScriptBinding *)
{
    return true;
}

bool Check::visit(AST::UiArrayBinding *)
{
    return true;
}

bool Check::visit(AST::UiObjectMemberList *)
{
    return true;
}

bool Check::visit(AST::UiArrayMemberList *)
{
    return true;
}

bool Check::visit(AST::UiQualifiedId *)
{
    return true;
}

bool Check::visit(AST::UiSignature *)
{
    return true;
}

bool Check::visit(AST::UiFormalList *)
{
    return true;
}

bool Check::visit(AST::UiFormal *)
{
    return true;
}

bool Check::visit(AST::ThisExpression *)
{
    return true;
}

bool Check::visit(AST::IdentifierExpression *)
{
    return true;
}

bool Check::visit(AST::NullExpression *)
{
    return true;
}

bool Check::visit(AST::TrueLiteral *)
{
    return true;
}

bool Check::visit(AST::FalseLiteral *)
{
    return true;
}

bool Check::visit(AST::StringLiteral *)
{
    return true;
}

bool Check::visit(AST::NumericLiteral *)
{
    return true;
}

bool Check::visit(AST::RegExpLiteral *)
{
    return true;
}

bool Check::visit(AST::ArrayLiteral *)
{
    return true;
}

bool Check::visit(AST::ObjectLiteral *)
{
    return true;
}

bool Check::visit(AST::ElementList *)
{
    return true;
}

bool Check::visit(AST::Elision *)
{
    return true;
}

bool Check::visit(AST::PropertyNameAndValueList *)
{
    return true;
}

bool Check::visit(AST::NestedExpression *)
{
    return true;
}

bool Check::visit(AST::IdentifierPropertyName *)
{
    return true;
}

bool Check::visit(AST::StringLiteralPropertyName *)
{
    return true;
}

bool Check::visit(AST::NumericLiteralPropertyName *)
{
    return true;
}

bool Check::visit(AST::ArrayMemberExpression *)
{
    return true;
}

bool Check::visit(AST::FieldMemberExpression *)
{
    return true;
}

bool Check::visit(AST::NewMemberExpression *)
{
    return true;
}

bool Check::visit(AST::NewExpression *)
{
    return true;
}

bool Check::visit(AST::CallExpression *)
{
    return true;
}

bool Check::visit(AST::ArgumentList *)
{
    return true;
}

bool Check::visit(AST::PostIncrementExpression *)
{
    return true;
}

bool Check::visit(AST::PostDecrementExpression *)
{
    return true;
}

bool Check::visit(AST::DeleteExpression *)
{
    return true;
}

bool Check::visit(AST::VoidExpression *)
{
    return true;
}

bool Check::visit(AST::TypeOfExpression *)
{
    return true;
}

bool Check::visit(AST::PreIncrementExpression *)
{
    return true;
}

bool Check::visit(AST::PreDecrementExpression *)
{
    return true;
}

bool Check::visit(AST::UnaryPlusExpression *)
{
    return true;
}

bool Check::visit(AST::UnaryMinusExpression *)
{
    return true;
}

bool Check::visit(AST::TildeExpression *)
{
    return true;
}

bool Check::visit(AST::NotExpression *)
{
    return true;
}

bool Check::visit(AST::BinaryExpression *)
{
    return true;
}

bool Check::visit(AST::ConditionalExpression *)
{
    return true;
}

bool Check::visit(AST::Expression *)
{
    return true;
}

bool Check::visit(AST::Block *)
{
    return true;
}

bool Check::visit(AST::StatementList *)
{
    return true;
}

bool Check::visit(AST::VariableStatement *)
{
    return true;
}

bool Check::visit(AST::VariableDeclarationList *)
{
    return true;
}

bool Check::visit(AST::VariableDeclaration *)
{
    return true;
}

bool Check::visit(AST::EmptyStatement *)
{
    return true;
}

bool Check::visit(AST::ExpressionStatement *)
{
    return true;
}

bool Check::visit(AST::IfStatement *)
{
    return true;
}

bool Check::visit(AST::DoWhileStatement *)
{
    return true;
}

bool Check::visit(AST::WhileStatement *)
{
    return true;
}

bool Check::visit(AST::ForStatement *)
{
    return true;
}

bool Check::visit(AST::LocalForStatement *)
{
    return true;
}

bool Check::visit(AST::ForEachStatement *)
{
    return true;
}

bool Check::visit(AST::LocalForEachStatement *)
{
    return true;
}

bool Check::visit(AST::ContinueStatement *)
{
    return true;
}

bool Check::visit(AST::BreakStatement *)
{
    return true;
}

bool Check::visit(AST::ReturnStatement *)
{
    return true;
}

bool Check::visit(AST::WithStatement *)
{
    return true;
}

bool Check::visit(AST::SwitchStatement *)
{
    return true;
}

bool Check::visit(AST::CaseBlock *)
{
    return true;
}

bool Check::visit(AST::CaseClauses *)
{
    return true;
}

bool Check::visit(AST::CaseClause *)
{
    return true;
}

bool Check::visit(AST::DefaultClause *)
{
    return true;
}

bool Check::visit(AST::LabelledStatement *)
{
    return true;
}

bool Check::visit(AST::ThrowStatement *)
{
    return true;
}

bool Check::visit(AST::TryStatement *)
{
    return true;
}

bool Check::visit(AST::Catch *)
{
    return true;
}

bool Check::visit(AST::Finally *)
{
    return true;
}

bool Check::visit(AST::FunctionDeclaration *)
{
    return true;
}

bool Check::visit(AST::FunctionExpression *)
{
    return true;
}

bool Check::visit(AST::FormalParameterList *)
{
    return true;
}

bool Check::visit(AST::FunctionBody *)
{
    return true;
}

bool Check::visit(AST::Program *)
{
    return true;
}

bool Check::visit(AST::SourceElements *)
{
    return true;
}

bool Check::visit(AST::FunctionSourceElement *)
{
    return true;
}

bool Check::visit(AST::StatementSourceElement *)
{
    return true;
}

bool Check::visit(AST::DebuggerStatement *)
{
    return true;
}
