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

#include "qmljsbind.h"
#include "parser/qmljsast_p.h"

using namespace QmlJS;

Bind::Bind()
{
}

Bind::~Bind()
{
}

void Bind::operator()(Document::Ptr doc)
{
    _doc = doc;
}

void Bind::accept(AST::Node *node)
{
    AST::Node::accept(node, this);
}

bool Bind::visit(AST::UiProgram *)
{
    return true;
}

bool Bind::visit(AST::UiImportList *)
{
    return true;
}

bool Bind::visit(AST::UiImport *)
{
    return true;
}

bool Bind::visit(AST::UiPublicMember *)
{
    return true;
}

bool Bind::visit(AST::UiSourceElement *)
{
    return true;
}

bool Bind::visit(AST::UiObjectDefinition *)
{
    return true;
}

bool Bind::visit(AST::UiObjectInitializer *)
{
    return true;
}

bool Bind::visit(AST::UiObjectBinding *)
{
    return true;
}

bool Bind::visit(AST::UiScriptBinding *)
{
    return true;
}

bool Bind::visit(AST::UiArrayBinding *)
{
    return true;
}

bool Bind::visit(AST::UiObjectMemberList *)
{
    return true;
}

bool Bind::visit(AST::UiArrayMemberList *)
{
    return true;
}

bool Bind::visit(AST::UiQualifiedId *)
{
    return true;
}

bool Bind::visit(AST::UiSignature *)
{
    return true;
}

bool Bind::visit(AST::UiFormalList *)
{
    return true;
}

bool Bind::visit(AST::UiFormal *)
{
    return true;
}

bool Bind::visit(AST::ThisExpression *)
{
    return true;
}

bool Bind::visit(AST::IdentifierExpression *)
{
    return true;
}

bool Bind::visit(AST::NullExpression *)
{
    return true;
}

bool Bind::visit(AST::TrueLiteral *)
{
    return true;
}

bool Bind::visit(AST::FalseLiteral *)
{
    return true;
}

bool Bind::visit(AST::StringLiteral *)
{
    return true;
}

bool Bind::visit(AST::NumericLiteral *)
{
    return true;
}

bool Bind::visit(AST::RegExpLiteral *)
{
    return true;
}

bool Bind::visit(AST::ArrayLiteral *)
{
    return true;
}

bool Bind::visit(AST::ObjectLiteral *)
{
    return true;
}

bool Bind::visit(AST::ElementList *)
{
    return true;
}

bool Bind::visit(AST::Elision *)
{
    return true;
}

bool Bind::visit(AST::PropertyNameAndValueList *)
{
    return true;
}

bool Bind::visit(AST::NestedExpression *)
{
    return true;
}

bool Bind::visit(AST::IdentifierPropertyName *)
{
    return true;
}

bool Bind::visit(AST::StringLiteralPropertyName *)
{
    return true;
}

bool Bind::visit(AST::NumericLiteralPropertyName *)
{
    return true;
}

bool Bind::visit(AST::ArrayMemberExpression *)
{
    return true;
}

bool Bind::visit(AST::FieldMemberExpression *)
{
    return true;
}

bool Bind::visit(AST::NewMemberExpression *)
{
    return true;
}

bool Bind::visit(AST::NewExpression *)
{
    return true;
}

bool Bind::visit(AST::CallExpression *)
{
    return true;
}

bool Bind::visit(AST::ArgumentList *)
{
    return true;
}

bool Bind::visit(AST::PostIncrementExpression *)
{
    return true;
}

bool Bind::visit(AST::PostDecrementExpression *)
{
    return true;
}

bool Bind::visit(AST::DeleteExpression *)
{
    return true;
}

bool Bind::visit(AST::VoidExpression *)
{
    return true;
}

bool Bind::visit(AST::TypeOfExpression *)
{
    return true;
}

bool Bind::visit(AST::PreIncrementExpression *)
{
    return true;
}

bool Bind::visit(AST::PreDecrementExpression *)
{
    return true;
}

bool Bind::visit(AST::UnaryPlusExpression *)
{
    return true;
}

bool Bind::visit(AST::UnaryMinusExpression *)
{
    return true;
}

bool Bind::visit(AST::TildeExpression *)
{
    return true;
}

bool Bind::visit(AST::NotExpression *)
{
    return true;
}

bool Bind::visit(AST::BinaryExpression *)
{
    return true;
}

bool Bind::visit(AST::ConditionalExpression *)
{
    return true;
}

bool Bind::visit(AST::Expression *)
{
    return true;
}

bool Bind::visit(AST::Block *)
{
    return true;
}

bool Bind::visit(AST::StatementList *)
{
    return true;
}

bool Bind::visit(AST::VariableStatement *)
{
    return true;
}

bool Bind::visit(AST::VariableDeclarationList *)
{
    return true;
}

bool Bind::visit(AST::VariableDeclaration *)
{
    return true;
}

bool Bind::visit(AST::EmptyStatement *)
{
    return true;
}

bool Bind::visit(AST::ExpressionStatement *)
{
    return true;
}

bool Bind::visit(AST::IfStatement *)
{
    return true;
}

bool Bind::visit(AST::DoWhileStatement *)
{
    return true;
}

bool Bind::visit(AST::WhileStatement *)
{
    return true;
}

bool Bind::visit(AST::ForStatement *)
{
    return true;
}

bool Bind::visit(AST::LocalForStatement *)
{
    return true;
}

bool Bind::visit(AST::ForEachStatement *)
{
    return true;
}

bool Bind::visit(AST::LocalForEachStatement *)
{
    return true;
}

bool Bind::visit(AST::ContinueStatement *)
{
    return true;
}

bool Bind::visit(AST::BreakStatement *)
{
    return true;
}

bool Bind::visit(AST::ReturnStatement *)
{
    return true;
}

bool Bind::visit(AST::WithStatement *)
{
    return true;
}

bool Bind::visit(AST::SwitchStatement *)
{
    return true;
}

bool Bind::visit(AST::CaseBlock *)
{
    return true;
}

bool Bind::visit(AST::CaseClauses *)
{
    return true;
}

bool Bind::visit(AST::CaseClause *)
{
    return true;
}

bool Bind::visit(AST::DefaultClause *)
{
    return true;
}

bool Bind::visit(AST::LabelledStatement *)
{
    return true;
}

bool Bind::visit(AST::ThrowStatement *)
{
    return true;
}

bool Bind::visit(AST::TryStatement *)
{
    return true;
}

bool Bind::visit(AST::Catch *)
{
    return true;
}

bool Bind::visit(AST::Finally *)
{
    return true;
}

bool Bind::visit(AST::FunctionDeclaration *)
{
    return true;
}

bool Bind::visit(AST::FunctionExpression *)
{
    return true;
}

bool Bind::visit(AST::FormalParameterList *)
{
    return true;
}

bool Bind::visit(AST::FunctionBody *)
{
    return true;
}

bool Bind::visit(AST::Program *)
{
    return true;
}

bool Bind::visit(AST::SourceElements *)
{
    return true;
}

bool Bind::visit(AST::FunctionSourceElement *)
{
    return true;
}

bool Bind::visit(AST::StatementSourceElement *)
{
    return true;
}

bool Bind::visit(AST::DebuggerStatement *)
{
    return true;
}
