/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "AST.h"
#include "ASTVisitor.h"
#include "MemoryPool.h"

#include <cassert>
#include <cstddef>
#include <algorithm>

CPLUSPLUS_BEGIN_NAMESPACE

AST::AST()
{ }

AST::~AST()
{ assert(0); }

AccessDeclarationAST *AST::asAccessDeclaration()
{ return dynamic_cast<AccessDeclarationAST *>(this); }

ArrayAccessAST *AST::asArrayAccess()
{ return dynamic_cast<ArrayAccessAST *>(this); }

ArrayDeclaratorAST *AST::asArrayDeclarator()
{ return dynamic_cast<ArrayDeclaratorAST *>(this); }

ArrayInitializerAST *AST::asArrayInitializer()
{ return dynamic_cast<ArrayInitializerAST *>(this); }

AsmDefinitionAST *AST::asAsmDefinition()
{ return dynamic_cast<AsmDefinitionAST *>(this); }

AttributeAST *AST::asAttribute()
{ return dynamic_cast<AttributeAST *>(this); }

AttributeSpecifierAST *AST::asAttributeSpecifier()
{ return dynamic_cast<AttributeSpecifierAST *>(this); }

BaseSpecifierAST *AST::asBaseSpecifier()
{ return dynamic_cast<BaseSpecifierAST *>(this); }

QtMethodAST *AST::asQtMethod()
{ return dynamic_cast<QtMethodAST *>(this); }

BinaryExpressionAST *AST::asBinaryExpression()
{ return dynamic_cast<BinaryExpressionAST *>(this); }

BoolLiteralAST *AST::asBoolLiteral()
{ return dynamic_cast<BoolLiteralAST *>(this); }

BreakStatementAST *AST::asBreakStatement()
{ return dynamic_cast<BreakStatementAST *>(this); }

CallAST *AST::asCall()
{ return dynamic_cast<CallAST *>(this); }

CaseStatementAST *AST::asCaseStatement()
{ return dynamic_cast<CaseStatementAST *>(this); }

CastExpressionAST *AST::asCastExpression()
{ return dynamic_cast<CastExpressionAST *>(this); }

CatchClauseAST *AST::asCatchClause()
{ return dynamic_cast<CatchClauseAST *>(this); }

ClassSpecifierAST *AST::asClassSpecifier()
{ return dynamic_cast<ClassSpecifierAST *>(this); }

CompoundStatementAST *AST::asCompoundStatement()
{ return dynamic_cast<CompoundStatementAST *>(this); }

ConditionAST *AST::asCondition()
{ return dynamic_cast<ConditionAST *>(this); }

ConditionalExpressionAST *AST::asConditionalExpression()
{ return dynamic_cast<ConditionalExpressionAST *>(this); }

ContinueStatementAST *AST::asContinueStatement()
{ return dynamic_cast<ContinueStatementAST *>(this); }

ConversionFunctionIdAST *AST::asConversionFunctionId()
{ return dynamic_cast<ConversionFunctionIdAST *>(this); }

CoreDeclaratorAST *AST::asCoreDeclarator()
{ return dynamic_cast<CoreDeclaratorAST *>(this); }

CppCastExpressionAST *AST::asCppCastExpression()
{ return dynamic_cast<CppCastExpressionAST *>(this); }

CtorInitializerAST *AST::asCtorInitializer()
{ return dynamic_cast<CtorInitializerAST *>(this); }

DeclarationAST *AST::asDeclaration()
{ return dynamic_cast<DeclarationAST *>(this); }

DeclarationStatementAST *AST::asDeclarationStatement()
{ return dynamic_cast<DeclarationStatementAST *>(this); }

DeclaratorAST *AST::asDeclarator()
{ return dynamic_cast<DeclaratorAST *>(this); }

DeclaratorIdAST *AST::asDeclaratorId()
{ return dynamic_cast<DeclaratorIdAST *>(this); }

DeclaratorListAST *AST::asDeclaratorList()
{ return dynamic_cast<DeclaratorListAST *>(this); }

DeleteExpressionAST *AST::asDeleteExpression()
{ return dynamic_cast<DeleteExpressionAST *>(this); }

DestructorNameAST *AST::asDestructorName()
{ return dynamic_cast<DestructorNameAST *>(this); }

DoStatementAST *AST::asDoStatement()
{ return dynamic_cast<DoStatementAST *>(this); }

ElaboratedTypeSpecifierAST *AST::asElaboratedTypeSpecifier()
{ return dynamic_cast<ElaboratedTypeSpecifierAST *>(this); }

EmptyDeclarationAST *AST::asEmptyDeclaration()
{ return dynamic_cast<EmptyDeclarationAST *>(this); }

EnumSpecifierAST *AST::asEnumSpecifier()
{ return dynamic_cast<EnumSpecifierAST *>(this); }

EnumeratorAST *AST::asEnumerator()
{ return dynamic_cast<EnumeratorAST *>(this); }

ExceptionDeclarationAST *AST::asExceptionDeclaration()
{ return dynamic_cast<ExceptionDeclarationAST *>(this); }

ExceptionSpecificationAST *AST::asExceptionSpecification()
{ return dynamic_cast<ExceptionSpecificationAST *>(this); }

ExpressionAST *AST::asExpression()
{ return dynamic_cast<ExpressionAST *>(this); }

ExpressionListAST *AST::asExpressionList()
{ return dynamic_cast<ExpressionListAST *>(this); }

ExpressionOrDeclarationStatementAST *AST::asExpressionOrDeclarationStatement()
{ return dynamic_cast<ExpressionOrDeclarationStatementAST *>(this); }

ExpressionStatementAST *AST::asExpressionStatement()
{ return dynamic_cast<ExpressionStatementAST *>(this); }

ForStatementAST *AST::asForStatement()
{ return dynamic_cast<ForStatementAST *>(this); }

FunctionDeclaratorAST *AST::asFunctionDeclarator()
{ return dynamic_cast<FunctionDeclaratorAST *>(this); }

FunctionDefinitionAST *AST::asFunctionDefinition()
{ return dynamic_cast<FunctionDefinitionAST *>(this); }

GotoStatementAST *AST::asGotoStatement()
{ return dynamic_cast<GotoStatementAST *>(this); }

IfStatementAST *AST::asIfStatement()
{ return dynamic_cast<IfStatementAST *>(this); }

LabeledStatementAST *AST::asLabeledStatement()
{ return dynamic_cast<LabeledStatementAST *>(this); }

LinkageBodyAST *AST::asLinkageBody()
{ return dynamic_cast<LinkageBodyAST *>(this); }

LinkageSpecificationAST *AST::asLinkageSpecification()
{ return dynamic_cast<LinkageSpecificationAST *>(this); }

MemInitializerAST *AST::asMemInitializer()
{ return dynamic_cast<MemInitializerAST *>(this); }

MemberAccessAST *AST::asMemberAccess()
{ return dynamic_cast<MemberAccessAST *>(this); }

NameAST *AST::asName()
{ return dynamic_cast<NameAST *>(this); }

NamedTypeSpecifierAST *AST::asNamedTypeSpecifier()
{ return dynamic_cast<NamedTypeSpecifierAST *>(this); }

NamespaceAST *AST::asNamespace()
{ return dynamic_cast<NamespaceAST *>(this); }

NamespaceAliasDefinitionAST *AST::asNamespaceAliasDefinition()
{ return dynamic_cast<NamespaceAliasDefinitionAST *>(this); }

NestedDeclaratorAST *AST::asNestedDeclarator()
{ return dynamic_cast<NestedDeclaratorAST *>(this); }

NestedExpressionAST *AST::asNestedExpression()
{ return dynamic_cast<NestedExpressionAST *>(this); }

NestedNameSpecifierAST *AST::asNestedNameSpecifier()
{ return dynamic_cast<NestedNameSpecifierAST *>(this); }

NewDeclaratorAST *AST::asNewDeclarator()
{ return dynamic_cast<NewDeclaratorAST *>(this); }

NewExpressionAST *AST::asNewExpression()
{ return dynamic_cast<NewExpressionAST *>(this); }

NewInitializerAST *AST::asNewInitializer()
{ return dynamic_cast<NewInitializerAST *>(this); }

NewTypeIdAST *AST::asNewTypeId()
{ return dynamic_cast<NewTypeIdAST *>(this); }

NumericLiteralAST *AST::asNumericLiteral()
{ return dynamic_cast<NumericLiteralAST *>(this); }

OperatorAST *AST::asOperator()
{ return dynamic_cast<OperatorAST *>(this); }

OperatorFunctionIdAST *AST::asOperatorFunctionId()
{ return dynamic_cast<OperatorFunctionIdAST *>(this); }

ParameterDeclarationAST *AST::asParameterDeclaration()
{ return dynamic_cast<ParameterDeclarationAST *>(this); }

ParameterDeclarationClauseAST *AST::asParameterDeclarationClause()
{ return dynamic_cast<ParameterDeclarationClauseAST *>(this); }

PointerAST *AST::asPointer()
{ return dynamic_cast<PointerAST *>(this); }

PointerToMemberAST *AST::asPointerToMember()
{ return dynamic_cast<PointerToMemberAST *>(this); }

PostIncrDecrAST *AST::asPostIncrDecr()
{ return dynamic_cast<PostIncrDecrAST *>(this); }

PostfixAST *AST::asPostfix()
{ return dynamic_cast<PostfixAST *>(this); }

PostfixDeclaratorAST *AST::asPostfixDeclarator()
{ return dynamic_cast<PostfixDeclaratorAST *>(this); }

PostfixExpressionAST *AST::asPostfixExpression()
{ return dynamic_cast<PostfixExpressionAST *>(this); }

PtrOperatorAST *AST::asPtrOperator()
{ return dynamic_cast<PtrOperatorAST *>(this); }

QualifiedNameAST *AST::asQualifiedName()
{ return dynamic_cast<QualifiedNameAST *>(this); }

ReferenceAST *AST::asReference()
{ return dynamic_cast<ReferenceAST *>(this); }

ReturnStatementAST *AST::asReturnStatement()
{ return dynamic_cast<ReturnStatementAST *>(this); }

SimpleDeclarationAST *AST::asSimpleDeclaration()
{ return dynamic_cast<SimpleDeclarationAST *>(this); }

SimpleNameAST *AST::asSimpleName()
{ return dynamic_cast<SimpleNameAST *>(this); }

SimpleSpecifierAST *AST::asSimpleSpecifier()
{ return dynamic_cast<SimpleSpecifierAST *>(this); }

SizeofExpressionAST *AST::asSizeofExpression()
{ return dynamic_cast<SizeofExpressionAST *>(this); }

SpecifierAST *AST::asSpecifier()
{ return dynamic_cast<SpecifierAST *>(this); }

StatementAST *AST::asStatement()
{ return dynamic_cast<StatementAST *>(this); }

StringLiteralAST *AST::asStringLiteral()
{ return dynamic_cast<StringLiteralAST *>(this); }

SwitchStatementAST *AST::asSwitchStatement()
{ return dynamic_cast<SwitchStatementAST *>(this); }

TemplateArgumentListAST *AST::asTemplateArgumentList()
{ return dynamic_cast<TemplateArgumentListAST *>(this); }

TemplateDeclarationAST *AST::asTemplateDeclaration()
{ return dynamic_cast<TemplateDeclarationAST *>(this); }

TemplateIdAST *AST::asTemplateId()
{ return dynamic_cast<TemplateIdAST *>(this); }

TemplateTypeParameterAST *AST::asTemplateTypeParameter()
{ return dynamic_cast<TemplateTypeParameterAST *>(this); }

ThisExpressionAST *AST::asThisExpression()
{ return dynamic_cast<ThisExpressionAST *>(this); }

ThrowExpressionAST *AST::asThrowExpression()
{ return dynamic_cast<ThrowExpressionAST *>(this); }

TranslationUnitAST *AST::asTranslationUnit()
{ return dynamic_cast<TranslationUnitAST *>(this); }

TryBlockStatementAST *AST::asTryBlockStatement()
{ return dynamic_cast<TryBlockStatementAST *>(this); }

TypeConstructorCallAST *AST::asTypeConstructorCall()
{ return dynamic_cast<TypeConstructorCallAST *>(this); }

TypeIdAST *AST::asTypeId()
{ return dynamic_cast<TypeIdAST *>(this); }

TypeidExpressionAST *AST::asTypeidExpression()
{ return dynamic_cast<TypeidExpressionAST *>(this); }

TypenameCallExpressionAST *AST::asTypenameCallExpression()
{ return dynamic_cast<TypenameCallExpressionAST *>(this); }

TypenameTypeParameterAST *AST::asTypenameTypeParameter()
{ return dynamic_cast<TypenameTypeParameterAST *>(this); }

TypeofSpecifierAST *AST::asTypeofSpecifier()
{ return dynamic_cast<TypeofSpecifierAST *>(this); }

UnaryExpressionAST *AST::asUnaryExpression()
{ return dynamic_cast<UnaryExpressionAST *>(this); }

UsingAST *AST::asUsing()
{ return dynamic_cast<UsingAST *>(this); }

UsingDirectiveAST *AST::asUsingDirective()
{ return dynamic_cast<UsingDirectiveAST *>(this); }

WhileStatementAST *AST::asWhileStatement()
{ return dynamic_cast<WhileStatementAST *>(this); }

void AST::accept(ASTVisitor *visitor)
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

unsigned AttributeSpecifierAST::firstToken() const
{
    return attribute_token;
}

unsigned AttributeSpecifierAST::lastToken() const
{
    if (second_rparen_token)
        return second_rparen_token + 1;
    else if (first_rparen_token)
        return first_rparen_token + 1;
    else if (attributes)
        return attributes->lastToken();
    else if (second_lparen_token)
        return second_lparen_token + 1;
    else if (first_lparen_token)
        return first_lparen_token + 1;
    return attribute_token + 1;
}

void AttributeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (AttributeAST *attr = attributes; attr; attr = attr->next)
            accept(attr, visitor);
    }
}

unsigned AttributeAST::firstToken() const
{
    return identifier_token;
}

unsigned AttributeAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    for (ExpressionListAST *it = expression_list;
            it->expression && it->next; it = it->next) {
        if (! it->next && it->expression) {
            return it->expression->lastToken();
        }
    }

    if (tag_token)
        return tag_token + 1;

    if (lparen_token)
        return lparen_token + 1;

    return identifier_token + 1;
}

void AttributeAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it->expression, visitor);
    }
}

void AccessDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned AccessDeclarationAST::firstToken() const
{
    return access_specifier_token;
}

unsigned AccessDeclarationAST::lastToken() const
{
    if (colon_token)
        return colon_token + 1;
    else if (slots_token)
        return slots_token + 1;
    return access_specifier_token + 1;
}

void ArrayAccessAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned ArrayAccessAST::firstToken() const
{
    return lbracket_token;
}

unsigned ArrayAccessAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    else if (expression)
        return expression->lastToken();
    return lbracket_token + 1;
}

void ArrayDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(this->expression, visitor);
    }
}

unsigned ArrayDeclaratorAST::firstToken() const
{
    return lbracket_token;
}

unsigned ArrayDeclaratorAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    else if (expression)
        return expression->lastToken();
    return lbracket_token + 1;
}

void ArrayInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExpressionListAST *expr = expression_list;expr;
                 expr = expr->next)
            accept(expr->expression, visitor);
    }
}

unsigned ArrayInitializerAST::firstToken() const
{
    return lbrace_token;
}

unsigned ArrayInitializerAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    for (ExpressionListAST *it = expression_list; it; it = it->next) {
        if (! it->next && it->expression)
            return it->expression->lastToken();
    }

    return lbrace_token + 1;
}

void AsmDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = cv_qualifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
    }
}

unsigned AsmDefinitionAST::firstToken() const
{
    return asm_token;
}

unsigned AsmDefinitionAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    else if (rparen_token)
        return rparen_token + 1;
    else if (lparen_token)
        return lparen_token + 1;
    for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return asm_token + 1;
}

void BaseSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
}

unsigned BaseSpecifierAST::firstToken() const
{
    if (token_virtual && token_access_specifier)
        return std::min(token_virtual, token_access_specifier);
    return name->firstToken();
}

unsigned BaseSpecifierAST::lastToken() const
{
    if (name)
        return name->lastToken();
    else if (token_virtual && token_access_specifier)
        return std::min(token_virtual, token_access_specifier) + 1;
    else if (token_virtual)
        return token_virtual + 1;
    else if (token_access_specifier)
        return token_access_specifier + 1;
    // assert?
    return 0;
}

unsigned QtMethodAST::firstToken() const
{ return method_token; }

unsigned QtMethodAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    else if (declarator)
        return declarator->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    return method_token + 1;
}

void QtMethodAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarator, visitor);
    }
}

void BinaryExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left_expression, visitor);
        accept(right_expression, visitor);
    }
}

unsigned BinaryExpressionAST::firstToken() const
{
    return left_expression->firstToken();
}

unsigned BinaryExpressionAST::lastToken() const
{
    if (right_expression)
        return right_expression->lastToken();
    else if (binary_op_token)
        return binary_op_token + 1;
    return left_expression->lastToken();
}

void BoolLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned BoolLiteralAST::firstToken() const
{
    return token;
}

unsigned BoolLiteralAST::lastToken() const
{
    return token + 1;
}

void BreakStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned BreakStatementAST::firstToken() const
{
    return break_token;
}

unsigned BreakStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    return break_token + 1;
}

void CallAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExpressionListAST *expr = expression_list;
                 expr; expr = expr->next)
            accept(expr->expression, visitor);
    }
}

unsigned CallAST::firstToken() const
{
    return lparen_token;
}

unsigned CallAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    for (ExpressionListAST *it = expression_list; it; it = it->next) {
        if (! it->next && it->expression)
            return it->expression->lastToken();
    }
    return lparen_token + 1;
}

void CaseStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned CaseStatementAST::firstToken() const
{
    return case_token;
}

unsigned CaseStatementAST::lastToken() const
{
    if (statement)
        return statement->lastToken();
    else if (colon_token)
        return colon_token + 1;
    else if (expression)
        return expression->lastToken();
    return case_token + 1;
}

void CastExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned CastExpressionAST::firstToken() const
{
    return lparen_token;
}

unsigned CastExpressionAST::lastToken() const
{
    if (expression)
        return expression->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (type_id)
        return type_id->lastToken();
    return lparen_token + 1;
}

void CatchClauseAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(exception_declaration, visitor);
        accept(statement, visitor);
    }
}

unsigned CatchClauseAST::firstToken() const
{
    return catch_token;
}

unsigned CatchClauseAST::lastToken() const
{
    if (statement)
        return statement->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    for (DeclarationAST *it = exception_declaration; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    if (lparen_token)
        return lparen_token + 1;

    return catch_token + 1;
}

void ClassSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = attributes; spec; spec = spec->next)
            accept(spec, visitor);
        accept(name, visitor);
        for (BaseSpecifierAST *spec = base_clause; spec; spec = spec->next)
            accept(spec, visitor);
        for (DeclarationAST *decl = member_specifiers; decl; decl = decl->next)
            accept(decl, visitor);
    }
}

unsigned ClassSpecifierAST::firstToken() const
{
    return classkey_token;
}

unsigned ClassSpecifierAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    for (DeclarationAST *it = member_specifiers; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (lbrace_token)
        return lbrace_token + 1;

    for (BaseSpecifierAST *it = base_clause; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (colon_token)
        return colon_token + 1;

    if (name)
        return name->lastToken();

    for (SpecifierAST *it = attributes; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return classkey_token + 1;
}

void CompoundStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (StatementAST *stmt = statements; stmt; stmt = stmt->next)
            accept(stmt, visitor);
    }
}

unsigned CompoundStatementAST::firstToken() const
{
    return lbrace_token;
}

unsigned CompoundStatementAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    for (StatementAST *it = statements; it ; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return lbrace_token + 1;
}

void ConditionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        accept(declarator, visitor);
    }
}

unsigned ConditionAST::firstToken() const
{
    if (type_specifier)
        return type_specifier->firstToken();

    return declarator->firstToken();
}

unsigned ConditionAST::lastToken() const
{
    if (declarator)
        return declarator->lastToken();

    for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    // ### assert?
    return 0;
}

void ConditionalExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(left_expression, visitor);
        accept(right_expression, visitor);
    }
}

unsigned ConditionalExpressionAST::firstToken() const
{
    return condition->firstToken();
}

unsigned ConditionalExpressionAST::lastToken() const
{
    if (right_expression)
        return right_expression->lastToken();
    else if (colon_token)
        return colon_token + 1;
    else if (left_expression)
        return left_expression->lastToken();
    else if (question_token)
        return question_token + 1;
    else if (condition)
        return condition->lastToken();
    // ### assert?
    return 0;
}

void ContinueStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned ContinueStatementAST::firstToken() const
{
    return continue_token;
}

unsigned ContinueStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    return continue_token + 1;
}

void ConversionFunctionIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        for (PtrOperatorAST *ptr_op = ptr_operators; ptr_op;
                 ptr_op = static_cast<PtrOperatorAST *>(ptr_op->next))
            accept(ptr_op, visitor);
    }
}

unsigned ConversionFunctionIdAST::firstToken() const
{
    return operator_token;
}

unsigned ConversionFunctionIdAST::lastToken() const
{
    for (PtrOperatorAST *it = ptr_operators; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return operator_token + 1;
}

void CppCastExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(type_id, visitor);
        accept(expression, visitor);
    }
}

unsigned CppCastExpressionAST::firstToken() const
{
    return cast_token;
}

unsigned CppCastExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    else if (greater_token)
        return greater_token + 1;
    else if (type_id)
        return type_id->lastToken();
    else if (less_token)
        return less_token + 1;
    return cast_token + 1;
}

void CtorInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (MemInitializerAST *mem_init = member_initializers;
                 mem_init; mem_init = mem_init->next)
            accept(mem_init, visitor);
    }
}

unsigned CtorInitializerAST::firstToken() const
{
    return colon_token;
}

unsigned CtorInitializerAST::lastToken() const
{
    for (MemInitializerAST *it = member_initializers; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return colon_token + 1;
}

void DeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (PtrOperatorAST *ptr_op = ptr_operators; ptr_op; ptr_op = ptr_op->next) {
            accept(ptr_op, visitor);
        }
        accept(core_declarator, visitor);
        for (PostfixDeclaratorAST *fx = postfix_declarators; fx; fx = fx->next) {
            accept(fx, visitor);
        }
        accept(attributes, visitor);
        accept(initializer, visitor);
    }
}

unsigned DeclaratorAST::firstToken() const
{
    if (ptr_operators)
        return ptr_operators->firstToken();
    else if (core_declarator)
        return core_declarator->firstToken();
    else if (postfix_declarators)
        return postfix_declarators->firstToken();
    else if (attributes)
        return attributes->firstToken();
    else if (initializer)
        return initializer->firstToken();
    // ### assert?
    return 0;
}

unsigned DeclaratorAST::lastToken() const
{
    if (initializer)
        return initializer->lastToken();

    for (SpecifierAST *it = attributes; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    for (PostfixDeclaratorAST *it = postfix_declarators; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (core_declarator)
        return core_declarator->lastToken();

    for (PtrOperatorAST *it = ptr_operators; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    // ### assert?
    return 0;
}

void DeclarationStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declaration, visitor);
    }
}

unsigned DeclarationStatementAST::firstToken() const
{
    return declaration->firstToken();
}

unsigned DeclarationStatementAST::lastToken() const
{
    return declaration->lastToken();
}

void DeclaratorIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
}

unsigned DeclaratorIdAST::firstToken() const
{
    return name->firstToken();
}

unsigned DeclaratorIdAST::lastToken() const
{
    return name->lastToken();
}

void DeclaratorListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclaratorListAST *it = this; it; it = it->next)
            accept(it->declarator, visitor);
    }
}

unsigned DeclaratorListAST::firstToken() const
{
    return declarator->firstToken();
}

unsigned DeclaratorListAST::lastToken() const
{
    for (const DeclaratorListAST *it = this; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return 0;
}

void DeleteExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned DeleteExpressionAST::firstToken() const
{
    if (scope_token)
        return scope_token;
    return delete_token;
}

unsigned DeleteExpressionAST::lastToken() const
{
    if (expression)
        return expression->lastToken();
    else if (rbracket_token)
        return rbracket_token + 1;
    else if (lbracket_token)
        return lbracket_token + 1;
    else if (delete_token)
        return delete_token + 1;
    return scope_token + 1;
}

void DestructorNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned DestructorNameAST::firstToken() const
{
    return tilde_token;
}

unsigned DestructorNameAST::lastToken() const
{
    if (identifier_token)
        return identifier_token + 1;
    return tilde_token + 1;
}

void DoStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(expression, visitor);
    }
}

unsigned DoStatementAST::firstToken() const
{
    return do_token;
}

unsigned DoStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    else if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    else if (while_token)
        return while_token + 1;
    else if (statement)
        return statement->lastToken();
    return do_token + 1;
}

void ElaboratedTypeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
}

unsigned ElaboratedTypeSpecifierAST::firstToken() const
{
    return classkey_token;
}

unsigned ElaboratedTypeSpecifierAST::lastToken() const
{
    if (name)
        return name->lastToken();
    return classkey_token + 1;
}

void EmptyDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned EmptyDeclarationAST::firstToken() const
{
    return semicolon_token;
}

unsigned EmptyDeclarationAST::lastToken() const
{
    return semicolon_token + 1;
}

void EnumSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        for (EnumeratorAST *enumerator = enumerators; enumerator;
                 enumerator = enumerator->next)
            accept(enumerator, visitor);
    }
}

unsigned EnumSpecifierAST::firstToken() const
{
    return enum_token;
}

unsigned EnumSpecifierAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    for (EnumeratorAST *it = enumerators; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (lbrace_token)
        return lbrace_token + 1;
    if (name)
        return name->lastToken();

    return enum_token + 1;
}

void EnumeratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned EnumeratorAST::firstToken() const
{
    return identifier_token;
}

unsigned EnumeratorAST::lastToken() const
{
    if (expression)
        return expression->lastToken();
    else if (equal_token)
        return equal_token + 1;
    return identifier_token + 1;
}

void ExceptionDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        accept(declarator, visitor);
    }
}

unsigned ExceptionDeclarationAST::firstToken() const
{
    if (type_specifier)
        return type_specifier->firstToken();
    if (declarator)
        return declarator->firstToken();
    return dot_dot_dot_token;
}

unsigned ExceptionDeclarationAST::lastToken() const
{
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    else if (declarator)
        return declarator->lastToken();
    for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return 0;
}

void ExceptionSpecificationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExpressionListAST *type_id = type_ids; type_id;
                 type_id = type_id->next)
            accept(type_id->expression, visitor);
    }
}

unsigned ExceptionSpecificationAST::firstToken() const
{
    return throw_token;
}

unsigned ExceptionSpecificationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    for (ExpressionListAST *it = type_ids; it; it = it->next) {
        if (! it->next && it->expression)
            return it->expression->lastToken();
    }

    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    else if (lparen_token)
        return lparen_token + 1;

    return throw_token + 1;
}

void ExpressionListAST::accept0(ASTVisitor *)
{ assert(0); }

unsigned ExpressionListAST::firstToken() const
{
    return expression->firstToken();
}

unsigned ExpressionListAST::lastToken() const
{
    for (const ExpressionListAST *it = this; it; it = it->next) {
        if (! it->next)
            return it->expression->lastToken();
    }
    return 0;
}

void ExpressionOrDeclarationStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declaration, visitor);
        accept(expression, visitor);
    }
}

unsigned ExpressionOrDeclarationStatementAST::firstToken() const
{
    return declaration->firstToken();
}

unsigned ExpressionOrDeclarationStatementAST::lastToken() const
{
    return declaration->lastToken();
}

void ExpressionStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned ExpressionStatementAST::firstToken() const
{
    if (expression)
        return expression->firstToken();
    return semicolon_token;
}

unsigned ExpressionStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    else if (expression)
        return expression->lastToken();
    // ### assert?
    return 0;
}

void ForStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(initializer, visitor);
        accept(condition, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
    }
}

unsigned ForStatementAST::firstToken() const
{
    return for_token;
}

unsigned ForStatementAST::lastToken() const
{
    if (statement)
        return statement->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    else if (semicolon_token)
        return semicolon_token + 1;
    else if (condition)
        return condition->lastToken();
    else if (initializer)
        return initializer->lastToken();
    else if (lparen_token)
        return lparen_token + 1;

    return for_token + 1;
}

void FunctionDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned FunctionDeclaratorAST::firstToken() const
{
    return lparen_token;
}

unsigned FunctionDeclaratorAST::lastToken() const
{
    if (exception_specification)
        return exception_specification->lastToken();

    for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (rparen_token)
        return rparen_token + 1;
    else if (parameters)
        return parameters->lastToken();

    return lparen_token + 1;
}

void FunctionDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = decl_specifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
        accept(declarator, visitor);
        accept(ctor_initializer, visitor);
        accept(function_body, visitor);
    }
}

unsigned FunctionDefinitionAST::firstToken() const
{
    if (decl_specifier_seq)
        return decl_specifier_seq->firstToken();
    else if (declarator)
        return declarator->firstToken();
    else if (ctor_initializer)
        return ctor_initializer->firstToken();
    return function_body->firstToken();
}

unsigned FunctionDefinitionAST::lastToken() const
{
    if (function_body)
        return function_body->lastToken();
    else if (ctor_initializer)
        return ctor_initializer->lastToken();
    if (declarator)
        return declarator->lastToken();

    for (SpecifierAST *it = decl_specifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    // ### assert
    return 0;
}

void GotoStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned GotoStatementAST::firstToken() const
{
    return goto_token;
}

unsigned GotoStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    else if (identifier_token)
        return identifier_token + 1;
    else if (goto_token)
        return goto_token + 1;
    return 0;
}

void IfStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(statement, visitor);
        accept(else_statement, visitor);
    }
}

unsigned IfStatementAST::firstToken() const
{
    return if_token;
}

unsigned IfStatementAST::lastToken() const
{
    if (else_statement)
        return else_statement->lastToken();
    else if (else_token)
        return else_token + 1;
    else if (statement)
        return statement->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (condition)
        return condition->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    return if_token + 1;
}

void LabeledStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
    }
}

unsigned LabeledStatementAST::firstToken() const
{
    return label_token;
}

unsigned LabeledStatementAST::lastToken() const
{
    if (statement)
        return statement->lastToken();
    else if (colon_token)
        return colon_token + 1;
    return label_token + 1;
}

void LinkageBodyAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclarationAST *decl = declarations; decl;
                 decl = decl->next)
            accept(decl, visitor);
    }
}

unsigned LinkageBodyAST::firstToken() const
{
    return lbrace_token;
}

unsigned LinkageBodyAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    for (DeclarationAST *it = declarations; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return lbrace_token + 1;
}

void LinkageSpecificationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declaration, visitor);
    }
}

unsigned LinkageSpecificationAST::firstToken() const
{
    return extern_token;
}

unsigned LinkageSpecificationAST::lastToken() const
{
    if (declaration)
        return declaration->lastToken();
    else if (extern_type)
        return extern_type + 1;
    return extern_token + 1;
}

void MemInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned MemInitializerAST::firstToken() const
{
    return name->firstToken();
}

unsigned MemInitializerAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    return name->lastToken();
}

void MemberAccessAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(member_name, visitor);
    }
}

unsigned MemberAccessAST::firstToken() const
{
    return access_token;
}

unsigned MemberAccessAST::lastToken() const
{
    if (member_name)
        return member_name->lastToken();
    else if (template_token)
        return template_token + 1;
    return access_token + 1;
}

void NamedTypeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
}

unsigned NamedTypeSpecifierAST::firstToken() const
{
    return name->firstToken();
}

unsigned NamedTypeSpecifierAST::lastToken() const
{
    return name->lastToken();
}

void NamespaceAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *attr = attributes; attr; attr = attr->next) {
            accept(attr, visitor);
        }
        accept(linkage_body, visitor);
    }
}

unsigned NamespaceAST::firstToken() const
{
    return namespace_token;
}

unsigned NamespaceAST::lastToken() const
{
    if (linkage_body)
        return linkage_body->lastToken();

    for (SpecifierAST *it = attributes; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (identifier_token)
        return identifier_token + 1;

    return namespace_token + 1;
}

void NamespaceAliasDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
}

unsigned NamespaceAliasDefinitionAST::firstToken() const
{
    return namespace_token;
}

unsigned NamespaceAliasDefinitionAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    else if (name)
        return name->lastToken();
    else if (equal_token)
        return equal_token + 1;
    else if (namespace_name)
        return namespace_name + 1;
    return namespace_token + 1;
}

void NestedDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarator, visitor);
    }
}

unsigned NestedDeclaratorAST::firstToken() const
{
    return lparen_token;
}

unsigned NestedDeclaratorAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    else if (declarator)
        return declarator->lastToken();
    return lparen_token + 1;
}

void NestedExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned NestedExpressionAST::firstToken() const
{
    return lparen_token;
}

unsigned NestedExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    return lparen_token + 1;
}

void NestedNameSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(class_or_namespace_name, visitor);
        accept(next, visitor); // ### I'm not 100% sure about this.
    }
}

unsigned NestedNameSpecifierAST::firstToken() const
{
    return class_or_namespace_name->firstToken();
}

unsigned NestedNameSpecifierAST::lastToken() const
{
    if (scope_token)
        return scope_token + 1;
    return class_or_namespace_name->lastToken();
}

void NewDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (PtrOperatorAST *ptr_op = ptr_operators; ptr_op;
                 ptr_op = static_cast<PtrOperatorAST *>(ptr_op->next))
            accept(ptr_op, visitor);
        // ### TODO accept the brackets
    }
}

unsigned NewDeclaratorAST::firstToken() const
{
    return ptr_operators->firstToken();
}

unsigned NewDeclaratorAST::lastToken() const
{
    assert(0 && "review me");
    if (declarator)
        return declarator->lastToken();
    assert(0); // ### implement me
    return 0;
}

void NewExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(type_id, visitor);
        accept(new_type_id, visitor);
        accept(new_initializer, visitor);
    }
}

unsigned NewExpressionAST::firstToken() const
{
    if (scope_token)
        return scope_token;
    return new_token;
}

unsigned NewExpressionAST::lastToken() const
{
    assert(0 && "review me");
    if (new_initializer)
        return new_initializer->lastToken();
    else if (new_type_id)
        return new_type_id->lastToken();
    else if (type_id)
        return type_id->lastToken();
    else if (expression)
        return expression->lastToken();
    return new_token + 1;
}

void NewInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned NewInitializerAST::firstToken() const
{
    return lparen_token;
}

unsigned NewInitializerAST::lastToken() const
{
    assert(0 && "review me");
    return rparen_token + 1;
}

void NewTypeIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        accept(new_initializer, visitor);
        accept(new_declarator, visitor);
    }
}

unsigned NewTypeIdAST::firstToken() const
{
    return type_specifier->firstToken();
}

unsigned NewTypeIdAST::lastToken() const
{
    assert(0 && "review me");
    if (new_declarator)
        return new_declarator->lastToken();
    else if (new_initializer)
        return new_initializer->lastToken();
    for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return 0;
}

void NumericLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned NumericLiteralAST::firstToken() const
{
    return token;
}

unsigned NumericLiteralAST::lastToken() const
{
    assert(0 && "review me");
    return token + 1;
}

void OperatorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned OperatorAST::firstToken() const
{
    return op_token;
}

unsigned OperatorAST::lastToken() const
{
    assert(0 && "review me");
    if (close_token)
        return close_token + 1;
    return op_token + 1;
}

void OperatorFunctionIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(op, visitor);
    }
}

unsigned OperatorFunctionIdAST::firstToken() const
{
    return operator_token;
}

unsigned OperatorFunctionIdAST::lastToken() const
{
    assert(0 && "review me");
    if (op)
        return op->lastToken();
    return operator_token + 1;
}

void ParameterDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        accept(declarator, visitor);
        accept(expression, visitor);
    }
}

unsigned ParameterDeclarationAST::firstToken() const
{
    return type_specifier->firstToken();
}

unsigned ParameterDeclarationAST::lastToken() const
{
    assert(0 && "review me");
    if (expression)
        return expression->lastToken();
    else if (equal_token)
        return equal_token + 1;
    else if (declarator)
        return declarator->lastToken();
        for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return 0;
}

void ParameterDeclarationClauseAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclarationAST *param = parameter_declarations; param;
                param = param->next)
            accept(param, visitor);
    }
}

unsigned ParameterDeclarationClauseAST::firstToken() const
{
    if (parameter_declarations)
        return parameter_declarations->firstToken();
    return dot_dot_dot_token;
}

unsigned ParameterDeclarationClauseAST::lastToken() const
{
    assert(0 && "review me");
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    return parameter_declarations->lastToken();
}

void PointerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = cv_qualifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
    }
}

unsigned PointerAST::firstToken() const
{
    return star_token;
}

unsigned PointerAST::lastToken() const
{
    assert(0 && "review me");
    for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return star_token + 1;
}

void PointerToMemberAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(nested_name_specifier, visitor);
        for (SpecifierAST *spec = cv_qualifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
    }
}

unsigned PointerToMemberAST::firstToken() const
{
    if (global_scope_token)
        return global_scope_token;
    else if (nested_name_specifier)
        return nested_name_specifier->firstToken();
    return star_token;
}

unsigned PointerToMemberAST::lastToken() const
{
    assert(0 && "review me");
    for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return star_token + 1;
}

void PostIncrDecrAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned PostIncrDecrAST::firstToken() const
{
    return incr_decr_token;
}

unsigned PostIncrDecrAST::lastToken() const
{
    assert(0 && "review me");
    return incr_decr_token + 1;
}

void PostfixExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base_expression, visitor);
        for (PostfixAST *fx = postfix_expressions; fx; fx = fx->next)
            accept(fx, visitor);
    }
}

unsigned PostfixExpressionAST::firstToken() const
{
    return base_expression->firstToken();
}

unsigned PostfixExpressionAST::lastToken() const
{
    assert(0 && "review me");
    for (PostfixAST *it = postfix_expressions; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return base_expression->lastToken();
}

void QualifiedNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(nested_name_specifier, visitor);
        accept(unqualified_name, visitor);
    }
}

unsigned QualifiedNameAST::firstToken() const
{
    if (global_scope_token)
        return global_scope_token;
    else if (nested_name_specifier)
        return nested_name_specifier->firstToken();
    return unqualified_name->firstToken();
}

unsigned QualifiedNameAST::lastToken() const
{
    assert(0 && "review me");
    if (unqualified_name)
        return unqualified_name->lastToken();
    else if (nested_name_specifier)
        return nested_name_specifier->lastToken();
    return global_scope_token + 1;
}

void ReferenceAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned ReferenceAST::firstToken() const
{
    return amp_token;
}

unsigned ReferenceAST::lastToken() const
{
    assert(0 && "review me");
    return amp_token + 1;
}

void ReturnStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned ReturnStatementAST::firstToken() const
{
    return return_token;
}

unsigned ReturnStatementAST::lastToken() const
{
    assert(0 && "review me");
    return semicolon_token + 1;
}

void SimpleDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = decl_specifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
        accept(declarators, visitor);
    }
}

unsigned SimpleDeclarationAST::firstToken() const
{
    if (decl_specifier_seq)
        return decl_specifier_seq->firstToken();
    else if (declarators)
        return declarators->firstToken();
    else
        return semicolon_token;
}

unsigned SimpleDeclarationAST::lastToken() const
{
    assert(0 && "review me");
    if (semicolon_token)
        return semicolon_token + 1;
    for (DeclaratorListAST *it = declarators; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    for (SpecifierAST *it = decl_specifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return 0;
}

void SimpleNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned SimpleNameAST::firstToken() const
{
    return identifier_token;
}

unsigned SimpleNameAST::lastToken() const
{
    assert(0 && "review me");
    return identifier_token + 1;
}

void SimpleSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned SimpleSpecifierAST::firstToken() const
{
    return specifier_token;
}

unsigned SimpleSpecifierAST::lastToken() const
{
    assert(0 && "review me");
    return specifier_token + 1;
}

void TypeofSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned TypeofSpecifierAST::firstToken() const
{
    return typeof_token;
}

unsigned TypeofSpecifierAST::lastToken() const
{
    assert(0 && "review me");
    if (expression)
        return expression->lastToken();
    return typeof_token + 1;
}

void SizeofExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned SizeofExpressionAST::firstToken() const
{
    return sizeof_token;
}

unsigned SizeofExpressionAST::lastToken() const
{
    assert(0 && "review me");
    if (expression)
        return expression->lastToken();
    return sizeof_token + 1;
}

void StringLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(next, visitor);
    }
}

unsigned StringLiteralAST::firstToken() const
{
    return token;
}

unsigned StringLiteralAST::lastToken() const
{
    assert(0 && "review me");
    if (next)
        return next->lastToken();
    return token + 1;
}

void SwitchStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(statement, visitor);
    }
}

unsigned SwitchStatementAST::firstToken() const
{
    return switch_token;
}

unsigned SwitchStatementAST::lastToken() const
{
    assert(0 && "review me");
    if (statement)
        return statement->lastToken();
    return rparen_token + 1;
}

void TemplateArgumentListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(template_argument, visitor);
        accept(next, visitor);
    }
}

unsigned TemplateArgumentListAST::firstToken() const
{
    return template_argument->firstToken();
}

unsigned TemplateArgumentListAST::lastToken() const
{
    assert(0 && "review me");
    for (const TemplateArgumentListAST *it = this; it; it = it->next) {
        if (! it->next)
            return it->template_argument->lastToken();
    }
    return 0;
}

void TemplateDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclarationAST *param = template_parameters; param;
                 param = param->next)
            accept(param, visitor);
        accept(declaration, visitor);
    }
}

unsigned TemplateDeclarationAST::firstToken() const
{
    if (export_token)
        return export_token;
    return template_token;
}

unsigned TemplateDeclarationAST::lastToken() const
{
    assert(0 && "review me");
    if (declaration)
        return declaration->lastToken();
    return greater_token + 1;
}

void TemplateIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (TemplateArgumentListAST *it = template_arguments; it; it = it->next) {
            accept(it, visitor);
        }
    }
}

unsigned TemplateIdAST::firstToken() const
{
    return identifier_token;
}

unsigned TemplateIdAST::lastToken() const
{
    assert(0 && "review me");
    return greater_token + 1;
}

void TemplateTypeParameterAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned TemplateTypeParameterAST::firstToken() const
{
    return template_token;
}

unsigned TemplateTypeParameterAST::lastToken() const
{
    assert(0 && "review me");
    if (type_id)
        return type_id->lastToken();
    else if (equal_token)
        return equal_token + 1;
    else if (name)
        return name->lastToken();
    else if (class_token)
        return class_token + 1;
    return greater_token + 1;
}

void ThisExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned ThisExpressionAST::firstToken() const
{
    return this_token;
}

unsigned ThisExpressionAST::lastToken() const
{
    assert(0 && "review me");
    return this_token + 1;
}

void ThrowExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned ThrowExpressionAST::firstToken() const
{
    return throw_token;
}

unsigned ThrowExpressionAST::lastToken() const
{
    assert(0 && "review me");
    if (expression)
        return expression->lastToken();
    return throw_token + 1;
}

void TranslationUnitAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclarationAST *decl = declarations; decl;
                 decl = decl->next)
            accept(decl, visitor);
    }
}

unsigned TranslationUnitAST::firstToken() const
{
    return declarations->firstToken();
}

unsigned TranslationUnitAST::lastToken() const
{
    assert(0 && "review me");
    for (DeclarationAST *it = declarations; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return 0;
}

void TryBlockStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(catch_clause_seq, visitor);
    }
}

unsigned TryBlockStatementAST::firstToken() const
{
    return try_token;
}

unsigned TryBlockStatementAST::lastToken() const
{
    assert(0 && "review me");
    for (CatchClauseAST *it = catch_clause_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    if (statement)
        return statement->lastToken();
    return try_token + 1;
}

void TypeConstructorCallAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        for (ExpressionListAST *expr = expression_list;expr;
                 expr = expr->next)
            accept(expr->expression, visitor);
    }
}

unsigned TypeConstructorCallAST::firstToken() const
{
    return type_specifier->firstToken();
}

unsigned TypeConstructorCallAST::lastToken() const
{
    assert(0 && "review me");
    return rparen_token + 1;
}

void TypeIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        accept(declarator, visitor);
    }
}

unsigned TypeIdAST::firstToken() const
{
    return type_specifier->firstToken();
}

unsigned TypeIdAST::lastToken() const
{
    assert(0 && "review me");
    if (declarator)
        return declarator->lastToken();
    return type_specifier->lastToken();
}

void TypeidExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned TypeidExpressionAST::firstToken() const
{
    return typeid_token;
}

unsigned TypeidExpressionAST::lastToken() const
{
    assert(0 && "review me");
    return rparen_token + 1;
}

void TypenameCallExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        for (ExpressionListAST *expr = expression_list;expr;
                 expr = expr->next)
            accept(expr->expression, visitor);
    }
}

unsigned TypenameCallExpressionAST::firstToken() const
{
    return typename_token;
}

unsigned TypenameCallExpressionAST::lastToken() const
{
    assert(0 && "review me");
    return rparen_token + 1;
}

void TypenameTypeParameterAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        accept(type_id, visitor);
    }
}

unsigned TypenameTypeParameterAST::firstToken() const
{
    return classkey_token;
}

unsigned TypenameTypeParameterAST::lastToken() const
{
    assert(0 && "review me");
    if (type_id)
        return type_id->lastToken();
    else if (equal_token)
        return equal_token + 1;
    else if (name)
        return name->lastToken();
    return classkey_token + 1;
}

void UnaryExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
}

unsigned UnaryExpressionAST::firstToken() const
{
    return unary_op_token;
}

unsigned UnaryExpressionAST::lastToken() const
{
    assert(0 && "review me");
    if (expression)
        return expression->lastToken();
    return unary_op_token + 1;
}

void UsingAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
}

unsigned UsingAST::firstToken() const
{
    return using_token;
}

unsigned UsingAST::lastToken() const
{
    assert(0 && "review me");
    return semicolon_token + 1;
}

void UsingDirectiveAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
}

unsigned UsingDirectiveAST::firstToken() const
{
    return using_token;
}

unsigned UsingDirectiveAST::lastToken() const
{
    assert(0 && "review me");
    return semicolon_token + 1;
}

void WhileStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(statement, visitor);
    }
}

unsigned WhileStatementAST::firstToken() const
{
    return while_token;
}

unsigned WhileStatementAST::lastToken() const
{
    assert(0 && "review me");
    if (statement)
        return statement->lastToken();
    return rparen_token + 1;
}

CPLUSPLUS_END_NAMESPACE
