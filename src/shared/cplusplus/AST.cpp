/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/
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

CompoundLiteralAST *AST::asCompoundLiteral()
{ return dynamic_cast<CompoundLiteralAST *>(this); }

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

AttributeSpecifierAST *AttributeSpecifierAST::clone(MemoryPool *pool) const
{
    AttributeSpecifierAST *ast = new (pool) AttributeSpecifierAST;
    ast->attribute_token = attribute_token;
    ast->first_lparen_token = first_lparen_token;
    ast->second_lparen_token = second_lparen_token;
    if (attributes)
        ast->attributes = attributes->clone(pool);
    ast->first_rparen_token = first_rparen_token;
    ast->second_rparen_token = second_rparen_token;
    return ast;
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

AttributeAST *AttributeAST::clone(MemoryPool *pool) const
{
    AttributeAST *ast = new (pool) AttributeAST;
    ast->identifier_token = identifier_token;
    ast->lparen_token = lparen_token;
    ast->tag_token = tag_token;
    if (expression_list)
        ast->expression_list = expression_list->clone(pool);
    ast->rparen_token = rparen_token;
    if (next)
        ast->next = next->clone(pool);
    return ast;
}

void AttributeAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it->expression, visitor);
    }
}

AccessDeclarationAST *AccessDeclarationAST::clone(MemoryPool *pool) const
{
    AccessDeclarationAST *ast = new (pool) AccessDeclarationAST;
    ast->access_specifier_token = access_specifier_token;
    ast->slots_token = slots_token;
    ast->colon_token = colon_token;
    return ast;
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

ArrayAccessAST *ArrayAccessAST::clone(MemoryPool *pool) const
{
    ArrayAccessAST *ast = new (pool) ArrayAccessAST;
    ast->lbracket_token = lbracket_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rbracket_token = rbracket_token;
    return ast;
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

ArrayDeclaratorAST *ArrayDeclaratorAST::clone(MemoryPool *pool) const
{
    ArrayDeclaratorAST *ast = new (pool) ArrayDeclaratorAST;
    ast->lbracket_token = lbracket_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rbracket_token = rbracket_token;
    return ast;
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

ArrayInitializerAST *ArrayInitializerAST::clone(MemoryPool *pool) const
{
    ArrayInitializerAST *ast = new (pool) ArrayInitializerAST;
    ast->lbrace_token = lbrace_token;
    if (expression_list)
        ast->expression_list = expression_list->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
}

void ArrayInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExpressionListAST *expr = expression_list; expr; expr = expr->next)
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

AsmDefinitionAST *AsmDefinitionAST::clone(MemoryPool *pool) const
{
    AsmDefinitionAST *ast = new (pool) AsmDefinitionAST;
    ast->asm_token = asm_token;
    if (cv_qualifier_seq)
        ast->cv_qualifier_seq = cv_qualifier_seq->clone(pool);
    ast->lparen_token = lparen_token;
    ast->rparen_token = rparen_token;
    ast->semicolon_token = semicolon_token;
    return ast;
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

BaseSpecifierAST *BaseSpecifierAST::clone(MemoryPool *pool) const
{
    BaseSpecifierAST *ast = new (pool) BaseSpecifierAST;
    ast->token_virtual = token_virtual;
    ast->token_access_specifier = token_access_specifier;
    if (name)
        ast->name = name->clone(pool);
    if (next)
        ast->next = next->clone(pool);
    return ast;
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

QtMethodAST *QtMethodAST::clone(MemoryPool *pool) const
{
    QtMethodAST *ast = new (pool) QtMethodAST;
    ast->method_token = method_token;
    ast->lparen_token = lparen_token;
    if (declarator)
        ast->declarator = declarator->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

void QtMethodAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarator, visitor);
    }
}

BinaryExpressionAST *BinaryExpressionAST::clone(MemoryPool *pool) const
{
    BinaryExpressionAST *ast = new (pool) BinaryExpressionAST;
    if (left_expression)
        ast->left_expression = left_expression->clone(pool);
    ast->binary_op_token = binary_op_token;
    if (right_expression)
        ast->right_expression = right_expression->clone(pool);
    return ast;
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

BoolLiteralAST *BoolLiteralAST::clone(MemoryPool *pool) const
{
    BoolLiteralAST *ast = new (pool) BoolLiteralAST;
    ast->token = token;
    return ast;
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

CompoundLiteralAST *CompoundLiteralAST::clone(MemoryPool *pool) const
{
    CompoundLiteralAST *ast = new (pool) CompoundLiteralAST;
    ast->lparen_token = lparen_token;
    if (type_id)
        ast->type_id = type_id->clone(pool);
    ast->rparen_token = rparen_token;
    if (initializer)
        ast->initializer = initializer->clone(pool);
    return ast;
}

void CompoundLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(type_id, visitor);
        accept(initializer, visitor);
    }
}

unsigned CompoundLiteralAST::firstToken() const
{
    return lparen_token;
}

unsigned CompoundLiteralAST::lastToken() const
{
    if (initializer)
        return initializer->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (type_id)
        return type_id->lastToken();
    return lparen_token + 1;
}

BreakStatementAST *BreakStatementAST::clone(MemoryPool *pool) const
{
    BreakStatementAST *ast = new (pool) BreakStatementAST;
    ast->break_token = break_token;
    ast->semicolon_token = semicolon_token;
    return ast;
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

CallAST *CallAST::clone(MemoryPool *pool) const
{
    CallAST *ast = new (pool) CallAST;
    ast->lparen_token = lparen_token;
    if (expression_list)
        ast->expression_list = expression_list;
    ast->rparen_token = rparen_token;
    return ast;
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

CaseStatementAST *CaseStatementAST::clone(MemoryPool *pool) const
{
    CaseStatementAST *ast = new (pool) CaseStatementAST;
    ast->case_token = case_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->colon_token = colon_token;
    if (statement)
        ast->statement = statement->clone(pool);
    return ast;
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

CastExpressionAST *CastExpressionAST::clone(MemoryPool *pool) const
{
    CastExpressionAST *ast = new (pool) CastExpressionAST;
    ast->lparen_token = lparen_token;
    if (type_id)
        ast->type_id = type_id->clone(pool);
    ast->rparen_token = rparen_token;
    if (expression)
        ast->expression = expression->clone(pool);
    return ast;
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

CatchClauseAST *CatchClauseAST::clone(MemoryPool *pool) const
{
    CatchClauseAST *ast = new (pool) CatchClauseAST;
    ast->catch_token = catch_token;
    ast->lparen_token = lparen_token;
    if (exception_declaration)
        ast->exception_declaration = exception_declaration->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement)
        ast->statement = statement->clone(pool);
    if (next)
        ast->next = next->clone(pool);
    return ast;
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

ClassSpecifierAST *ClassSpecifierAST::clone(MemoryPool *pool) const
{
    ClassSpecifierAST *ast = new (pool) ClassSpecifierAST;
    ast->classkey_token = classkey_token;
    if (attributes)
        ast->attributes = attributes->clone(pool);
    if (name)
        ast->name = name->clone(pool);
    ast->colon_token = colon_token;
    if (base_clause)
        ast->base_clause = base_clause->clone(pool);
    ast->lbrace_token = lbrace_token;
    if (member_specifiers)
        ast->member_specifiers = member_specifiers->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
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

CompoundStatementAST *CompoundStatementAST::clone(MemoryPool *pool) const
{
    CompoundStatementAST *ast = new (pool) CompoundStatementAST;
    ast->lbrace_token = lbrace_token;
    if (statements)
        ast->statements = statements->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
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

ConditionAST *ConditionAST::clone(MemoryPool *pool) const
{
    ConditionAST *ast = new (pool) ConditionAST;
    if (type_specifier)
        ast->type_specifier = type_specifier->clone(pool);
    if (declarator)
        ast->declarator = declarator->clone(pool);
    return ast;
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

ConditionalExpressionAST *ConditionalExpressionAST::clone(MemoryPool *pool) const
{
    ConditionalExpressionAST *ast = new (pool) ConditionalExpressionAST;
    if (condition)
        ast->condition = condition->clone(pool);
    ast->question_token = question_token;
    if (left_expression)
        ast->left_expression = left_expression->clone(pool);
    ast->colon_token = colon_token;
    if (right_expression)
        ast->right_expression = right_expression->clone(pool);
    return ast;
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

ContinueStatementAST *ContinueStatementAST::clone(MemoryPool *pool) const
{
    ContinueStatementAST *ast = new (pool) ContinueStatementAST;
    ast->continue_token = continue_token;
    ast->semicolon_token = semicolon_token;
    return ast;
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

ConversionFunctionIdAST *ConversionFunctionIdAST::clone(MemoryPool *pool) const
{
    ConversionFunctionIdAST *ast = new (pool) ConversionFunctionIdAST;
    ast->operator_token = operator_token;
    if (type_specifier)
        ast->type_specifier = type_specifier->clone(pool);
    if (ptr_operators)
        ast->ptr_operators = ptr_operators->clone(pool);
    return ast;
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

CppCastExpressionAST *CppCastExpressionAST::clone(MemoryPool *pool) const
{
    CppCastExpressionAST *ast = new (pool) CppCastExpressionAST;
    ast->cast_token = cast_token;
    ast->less_token = less_token;
    if (type_id)
        ast->type_id = type_id->clone(pool);
    ast->greater_token = greater_token;
    ast->lparen_token = lparen_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
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

CtorInitializerAST *CtorInitializerAST::clone(MemoryPool *pool) const
{
    CtorInitializerAST *ast = new (pool) CtorInitializerAST;
    ast->colon_token = colon_token;
    if (member_initializers)
        ast->member_initializers = member_initializers->clone(pool);
    return ast;
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

DeclaratorAST *DeclaratorAST::clone(MemoryPool *pool) const
{
    DeclaratorAST *ast = new (pool) DeclaratorAST;
    if (ptr_operators)
        ast->ptr_operators = ptr_operators->clone(pool);
    if (core_declarator)
        ast->core_declarator = core_declarator->clone(pool);
    if (postfix_declarators)
        ast->postfix_declarators = postfix_declarators->clone(pool);
    if (attributes)
        ast->attributes = attributes->clone(pool);
    if (initializer)
        ast->initializer = initializer->clone(pool);
    return ast;
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

DeclarationStatementAST *DeclarationStatementAST::clone(MemoryPool *pool) const
{
    DeclarationStatementAST *ast = new (pool) DeclarationStatementAST;
    if (declaration)
        ast->declaration = declaration->clone(pool);
    return ast;
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

DeclaratorIdAST *DeclaratorIdAST::clone(MemoryPool *pool) const
{
    DeclaratorIdAST *ast = new (pool) DeclaratorIdAST;
    if (name)
        ast->name = name->clone(pool);
    return ast;
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

DeclaratorListAST *DeclaratorListAST::clone(MemoryPool *pool) const
{
    DeclaratorListAST *ast = new (pool) DeclaratorListAST;
    if (declarator)
        ast->declarator = declarator->clone(pool);
    if (next)
        ast->next = next->clone(pool);
    return ast;
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

DeleteExpressionAST *DeleteExpressionAST::clone(MemoryPool *pool) const
{
    DeleteExpressionAST *ast = new (pool) DeleteExpressionAST;
    ast->scope_token = scope_token;
    ast->delete_token = delete_token;
    ast->lbracket_token = lbracket_token;
    ast->rbracket_token = rbracket_token;
    if (expression)
        ast->expression = expression->clone(pool);
    return ast;
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

DestructorNameAST *DestructorNameAST::clone(MemoryPool *pool) const
{
    DestructorNameAST *ast = new (pool) DestructorNameAST;
    ast->tilde_token = tilde_token;
    ast->identifier_token = identifier_token;
    return ast;
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

DoStatementAST *DoStatementAST::clone(MemoryPool *pool) const
{
    DoStatementAST *ast = new (pool) DoStatementAST;
    ast->do_token = do_token;
    if (statement)
        ast->statement = statement->clone(pool);
    ast->while_token = while_token;
    ast->lparen_token = lparen_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    ast->semicolon_token = semicolon_token;
    return ast;
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

ElaboratedTypeSpecifierAST *ElaboratedTypeSpecifierAST::clone(MemoryPool *pool) const
{
    ElaboratedTypeSpecifierAST *ast = new (pool) ElaboratedTypeSpecifierAST;
    ast->classkey_token = classkey_token;
    if (name)
        ast->name = name->clone(pool);
    return ast;
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

EmptyDeclarationAST *EmptyDeclarationAST::clone(MemoryPool *pool) const
{
    EmptyDeclarationAST *ast = new (pool) EmptyDeclarationAST;
    ast->semicolon_token = semicolon_token;
    return ast;
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

EnumSpecifierAST *EnumSpecifierAST::clone(MemoryPool *pool) const
{
    EnumSpecifierAST *ast = new (pool) EnumSpecifierAST;
    ast->enum_token = enum_token;
    if (name)
        ast->name = name->clone(pool);
    ast->lbrace_token = lbrace_token;
    if (enumerators)
        ast->enumerators = enumerators->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
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

EnumeratorAST *EnumeratorAST::clone(MemoryPool *pool) const
{
    EnumeratorAST *ast = new (pool) EnumeratorAST;
    ast->identifier_token = identifier_token;
    ast->equal_token = equal_token;
    if (expression)
        ast->expression = expression->clone(pool);
    if (next)
        ast->next = next->clone(pool);
    return ast;
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

ExceptionDeclarationAST *ExceptionDeclarationAST::clone(MemoryPool *pool) const
{
    ExceptionDeclarationAST *ast = new (pool) ExceptionDeclarationAST;
    if (type_specifier)
        ast->type_specifier = type_specifier->clone(pool);
    if (declarator)
        ast->declarator = declarator->clone(pool);
    ast->dot_dot_dot_token = dot_dot_dot_token;
    return ast;
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

ExceptionSpecificationAST *ExceptionSpecificationAST::clone(MemoryPool *pool) const
{
    ExceptionSpecificationAST *ast = new (pool) ExceptionSpecificationAST;
    ast->throw_token = throw_token;
    ast->lparen_token = lparen_token;
    ast->dot_dot_dot_token = dot_dot_dot_token;
    if (type_ids)
        ast->type_ids = type_ids->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
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

ExpressionListAST *ExpressionListAST::clone(MemoryPool *pool) const
{
    ExpressionListAST *ast = new (pool) ExpressionListAST;
    if (expression)
        ast->expression = expression->clone(pool);
    if (next)
        ast->next = next->clone(pool);
    return ast;
}

void ExpressionListAST::accept0(ASTVisitor *visitor)
{
    for (const ExpressionListAST *it = this; it; it = it->next) {
        accept(it->expression, visitor);
    }
}

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

ExpressionOrDeclarationStatementAST *ExpressionOrDeclarationStatementAST::clone(MemoryPool *pool) const
{
    ExpressionOrDeclarationStatementAST *ast = new (pool) ExpressionOrDeclarationStatementAST;
    if (expression)
        ast->expression = expression->clone(pool);
    if (declaration)
        ast->declaration = declaration->clone(pool);
    return ast;
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

ExpressionStatementAST *ExpressionStatementAST::clone(MemoryPool *pool) const
{
    ExpressionStatementAST *ast = new (pool) ExpressionStatementAST;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
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

ForStatementAST *ForStatementAST::clone(MemoryPool *pool) const
{
    ForStatementAST *ast = new (pool) ForStatementAST;
    ast->for_token = for_token;
    ast->lparen_token = lparen_token;
    if (initializer)
        ast->initializer = initializer->clone(pool);
    if (condition)
        ast->condition = condition->clone(pool);
    ast->semicolon_token = semicolon_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement)
        ast->statement = statement->clone(pool);
    return ast;
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

FunctionDeclaratorAST *FunctionDeclaratorAST::clone(MemoryPool *pool) const
{
    FunctionDeclaratorAST *ast = new (pool) FunctionDeclaratorAST;
    ast->lparen_token = lparen_token;
    if (parameters)
        ast->parameters = parameters->clone(pool);
    ast->rparen_token = rparen_token;
    if (cv_qualifier_seq)
        ast->cv_qualifier_seq = cv_qualifier_seq->clone(pool);
    if (exception_specification)
        ast->exception_specification = exception_specification->clone(pool);
    return ast;
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

FunctionDefinitionAST *FunctionDefinitionAST::clone(MemoryPool *pool) const
{
    FunctionDefinitionAST *ast = new (pool) FunctionDefinitionAST;
    if (decl_specifier_seq)
        ast->decl_specifier_seq = decl_specifier_seq->clone(pool);
    if (declarator)
        ast->declarator = declarator->clone(pool);
    if (ctor_initializer)
        ast->ctor_initializer = ctor_initializer->clone(pool);
    if (function_body)
        ast->function_body = function_body->clone(pool);
    return ast;
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

GotoStatementAST *GotoStatementAST::clone(MemoryPool *pool) const
{
    GotoStatementAST *ast = new (pool) GotoStatementAST;
    ast->goto_token = goto_token;
    ast->identifier_token = identifier_token;
    ast->semicolon_token = semicolon_token;
    return ast;
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

IfStatementAST *IfStatementAST::clone(MemoryPool *pool) const
{
    IfStatementAST *ast = new (pool) IfStatementAST;
    ast->if_token = if_token;
    ast->lparen_token = lparen_token;
    if (condition)
        ast->condition = condition->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement)
        ast->statement = statement->clone(pool);
    ast->else_token = else_token;
    if (else_statement)
        ast->else_statement = else_statement->clone(pool);
    return ast;
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

LabeledStatementAST *LabeledStatementAST::clone(MemoryPool *pool) const
{
    LabeledStatementAST *ast = new (pool) LabeledStatementAST;
    ast->label_token = label_token;
    ast->colon_token = colon_token;
    if (statement)
        ast->statement = statement->clone(pool);
    return ast;
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

LinkageBodyAST *LinkageBodyAST::clone(MemoryPool *pool) const
{
    LinkageBodyAST *ast = new (pool) LinkageBodyAST;
    ast->lbrace_token = lbrace_token;
    if (declarations)
        ast->declarations = declarations->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
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

LinkageSpecificationAST *LinkageSpecificationAST::clone(MemoryPool *pool) const
{
    LinkageSpecificationAST *ast = new (pool) LinkageSpecificationAST;
    ast->extern_token = extern_token;
    ast->extern_type = extern_type;
    if (declaration)
        ast->declaration = declaration->clone(pool);
    return ast;
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

MemInitializerAST *MemInitializerAST::clone(MemoryPool *pool) const
{
    MemInitializerAST *ast = new (pool) MemInitializerAST;
    if (name)
        ast->name = name->clone(pool);
    ast->lparen_token = lparen_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    if (next)
        ast->next = next->clone(pool);
    return ast;
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

MemberAccessAST *MemberAccessAST::clone(MemoryPool *pool) const
{
    MemberAccessAST *ast = new (pool) MemberAccessAST;
    ast->access_token = access_token;
    ast->template_token = template_token;
    if (member_name)
        ast->member_name = member_name->clone(pool);
    return ast;
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

NamedTypeSpecifierAST *NamedTypeSpecifierAST::clone(MemoryPool *pool) const
{
    NamedTypeSpecifierAST *ast = new (pool) NamedTypeSpecifierAST;
    if (name)
        ast->name = name->clone(pool);
    return ast;
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

NamespaceAST *NamespaceAST::clone(MemoryPool *pool) const
{
    NamespaceAST *ast = new (pool) NamespaceAST;
    ast->namespace_token = namespace_token;
    ast->identifier_token = identifier_token;
    if (attributes)
        ast->attributes = attributes->clone(pool);
    if (linkage_body)
        ast->linkage_body = linkage_body->clone(pool);
    return ast;
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

NamespaceAliasDefinitionAST *NamespaceAliasDefinitionAST::clone(MemoryPool *pool) const
{
    NamespaceAliasDefinitionAST *ast = new (pool) NamespaceAliasDefinitionAST;
    ast->namespace_token = namespace_token;
    ast->namespace_name = namespace_name;
    ast->equal_token = equal_token;
    if (name)
        ast->name = name->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
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

NestedDeclaratorAST *NestedDeclaratorAST::clone(MemoryPool *pool) const
{
    NestedDeclaratorAST *ast = new (pool) NestedDeclaratorAST;
    ast->lparen_token = lparen_token;
    if (declarator)
        ast->declarator = declarator->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
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

NestedExpressionAST *NestedExpressionAST::clone(MemoryPool *pool) const
{
    NestedExpressionAST *ast = new (pool) NestedExpressionAST;
    ast->lparen_token = lparen_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
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

NestedNameSpecifierAST *NestedNameSpecifierAST::clone(MemoryPool *pool) const
{
    NestedNameSpecifierAST *ast = new (pool) NestedNameSpecifierAST;
    if (class_or_namespace_name)
        ast->class_or_namespace_name = class_or_namespace_name->clone(pool);
    ast->scope_token = scope_token;
    if (next)
        ast->next = next->clone(pool);
    return ast;
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

NewDeclaratorAST *NewDeclaratorAST::clone(MemoryPool *pool) const
{
    NewDeclaratorAST *ast = new (pool) NewDeclaratorAST;
    if (ptr_operators)
        ast->ptr_operators = ptr_operators->clone(pool);
    if (declarator)
        ast->declarator = declarator->clone(pool);
    return ast;
}

void NewDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (PtrOperatorAST *ptr_op = ptr_operators; ptr_op;
                 ptr_op = static_cast<PtrOperatorAST *>(ptr_op->next)) {
            accept(ptr_op, visitor);
        }

        accept(declarator, visitor);
    }
}

unsigned NewDeclaratorAST::firstToken() const
{
    return ptr_operators->firstToken();
}

unsigned NewDeclaratorAST::lastToken() const
{
    if (declarator)
        return declarator->lastToken();

    for (PtrOperatorAST *it = ptr_operators; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return 0;
}

NewExpressionAST *NewExpressionAST::clone(MemoryPool *pool) const
{
    NewExpressionAST *ast = new (pool) NewExpressionAST;
    ast->scope_token = scope_token;
    ast->new_token = new_token;
    if (expression)
        ast->expression = expression->clone(pool);
    if (type_id)
        ast->type_id = type_id->clone(pool);
    if (new_type_id)
        ast->new_type_id = new_type_id->clone(pool);
    if (new_initializer)
        ast->new_initializer = new_initializer->clone(pool);
    return ast;
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
    if (new_initializer)
        return new_initializer->lastToken();
    else if (new_type_id)
        return new_type_id->lastToken();
    else if (type_id)
        return type_id->lastToken();
    else if (expression)
        return expression->lastToken();
    else if (new_token)
        return new_token + 1;
    else if (scope_token)
        return scope_token + 1;
    // ### assert?
    return 0;
}

NewInitializerAST *NewInitializerAST::clone(MemoryPool *pool) const
{
    NewInitializerAST *ast = new (pool) NewInitializerAST;
    ast->lparen_token = lparen_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
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
    if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    return lparen_token + 1;
}

TypeIdAST *TypeIdAST::clone(MemoryPool *pool) const
{
    TypeIdAST *ast = new (pool) TypeIdAST;
    if (type_specifier)
        ast->type_specifier = type_specifier->clone(pool);
    if (declarator)
        ast->declarator = declarator->clone(pool);
    return ast;
}

NewTypeIdAST *NewTypeIdAST::clone(MemoryPool *pool) const
{
    NewTypeIdAST *ast = new (pool) NewTypeIdAST;
    if (type_specifier)
        ast->type_specifier = type_specifier->clone(pool);
    if (new_initializer)
        ast->new_initializer = new_initializer->clone(pool);
    if (new_declarator)
        ast->new_declarator = new_declarator->clone(pool);
    return ast;
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
    if (new_declarator)
        return new_declarator->lastToken();
    else if (new_initializer)
        return new_initializer->lastToken();
    for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    // ### assert?
    return 0;
}

NumericLiteralAST *NumericLiteralAST::clone(MemoryPool *pool) const
{
    NumericLiteralAST *ast = new (pool) NumericLiteralAST;
    ast->token = token;
    return ast;
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
    return token + 1;
}

OperatorAST *OperatorAST::clone(MemoryPool *pool) const
{
    OperatorAST *ast = new (pool) OperatorAST;
    ast->op_token = op_token;
    ast->open_token = open_token;
    ast->close_token = close_token;
    return ast;
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
    if (close_token)
        return close_token + 1;
    else if (open_token)
        return open_token + 1;
    return op_token + 1;
}

OperatorFunctionIdAST *OperatorFunctionIdAST::clone(MemoryPool *pool) const
{
    OperatorFunctionIdAST *ast = new (pool) OperatorFunctionIdAST;
    ast->operator_token = operator_token;
    if (op)
        ast->op = op->clone(pool);
    return ast;
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
    if (op)
        return op->lastToken();
    return operator_token + 1;
}

ParameterDeclarationAST *ParameterDeclarationAST::clone(MemoryPool *pool) const
{
    ParameterDeclarationAST *ast = new (pool) ParameterDeclarationAST;
    if (type_specifier)
        ast->type_specifier = type_specifier->clone(pool);
    if (declarator)
        ast->declarator = declarator->clone(pool);
    ast->equal_token = equal_token;
    if (expression)
        ast->expression = expression->clone(pool);
    return ast;
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
    // ### assert?
    return 0;
}

ParameterDeclarationClauseAST *ParameterDeclarationClauseAST::clone(MemoryPool *pool) const
{
    ParameterDeclarationClauseAST *ast = new (pool) ParameterDeclarationClauseAST;
    if (parameter_declarations)
        ast->parameter_declarations = parameter_declarations;
    ast->dot_dot_dot_token = dot_dot_dot_token;
    return ast;
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
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    return parameter_declarations->lastToken();
}

PointerAST *PointerAST::clone(MemoryPool *pool) const
{
    PointerAST *ast = new (pool) PointerAST;
    ast->star_token = star_token;
    if (cv_qualifier_seq)
        ast->cv_qualifier_seq = cv_qualifier_seq->clone(pool);
    return ast;
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
    for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return star_token + 1;
}

PointerToMemberAST *PointerToMemberAST::clone(MemoryPool *pool) const
{
    PointerToMemberAST *ast = new (pool) PointerToMemberAST;
    ast->global_scope_token = global_scope_token;
    if (nested_name_specifier)
        ast->nested_name_specifier = nested_name_specifier->clone(pool);
    ast->star_token = star_token;
    if (cv_qualifier_seq)
        ast->cv_qualifier_seq = cv_qualifier_seq->clone(pool);
    return ast;
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
    for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (star_token)
        return star_token + 1;

    for (NestedNameSpecifierAST *it = nested_name_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (global_scope_token)
        return global_scope_token + 1;

    return 0;
}

PostIncrDecrAST *PostIncrDecrAST::clone(MemoryPool *pool) const
{
    PostIncrDecrAST *ast = new (pool) PostIncrDecrAST;
    ast->incr_decr_token = incr_decr_token;
    return ast;
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
    return incr_decr_token + 1;
}

PostfixExpressionAST *PostfixExpressionAST::clone(MemoryPool *pool) const
{
    PostfixExpressionAST *ast = new (pool) PostfixExpressionAST;
    if (base_expression)
        ast->base_expression = base_expression->clone(pool);
    if (postfix_expressions)
        ast->postfix_expressions = postfix_expressions->clone(pool);
    return ast;
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
    for (PostfixAST *it = postfix_expressions; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return base_expression->lastToken();
}

QualifiedNameAST *QualifiedNameAST::clone(MemoryPool *pool) const
{
    QualifiedNameAST *ast = new (pool) QualifiedNameAST;
    ast->global_scope_token = global_scope_token;
    if (nested_name_specifier)
        ast->nested_name_specifier = nested_name_specifier->clone(pool);
    if (unqualified_name)
        ast->unqualified_name = unqualified_name->clone(pool);
    return ast;
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
    if (unqualified_name)
        return unqualified_name->lastToken();

    for (NestedNameSpecifierAST *it = nested_name_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (global_scope_token)
        return global_scope_token + 1;

    return 0;
}

ReferenceAST *ReferenceAST::clone(MemoryPool *pool) const
{
    ReferenceAST *ast = new (pool) ReferenceAST;
    ast->amp_token = amp_token;
    return ast;
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
    return amp_token + 1;
}

ReturnStatementAST *ReturnStatementAST::clone(MemoryPool *pool) const
{
    ReturnStatementAST *ast = new (pool) ReturnStatementAST;
    ast->return_token = return_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
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
    if (semicolon_token)
        return semicolon_token + 1;
    else if (expression)
        return expression->lastToken();
    return return_token + 1;
}

SimpleDeclarationAST *SimpleDeclarationAST::clone(MemoryPool *pool) const
{
    SimpleDeclarationAST *ast = new (pool) SimpleDeclarationAST;
    if (decl_specifier_seq)
        ast->decl_specifier_seq = decl_specifier_seq->clone(pool);
    if (declarators)
        ast->declarators = declarators->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
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
    return semicolon_token;
}

unsigned SimpleDeclarationAST::lastToken() const
{
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

SimpleNameAST *SimpleNameAST::clone(MemoryPool *pool) const
{
    SimpleNameAST *ast = new (pool) SimpleNameAST;
    ast->identifier_token = identifier_token;
    return ast;
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
    return identifier_token + 1;
}

void SimpleSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

SimpleSpecifierAST *SimpleSpecifierAST::clone(MemoryPool *pool) const
{
    SimpleSpecifierAST *ast = new (pool) SimpleSpecifierAST;
    ast->specifier_token = specifier_token;
    if (next)
        ast->next = next->clone(pool);
    return ast;
}

unsigned SimpleSpecifierAST::firstToken() const
{
    return specifier_token;
}

unsigned SimpleSpecifierAST::lastToken() const
{
    return specifier_token + 1;
}

TypeofSpecifierAST *TypeofSpecifierAST::clone(MemoryPool *pool) const
{
    TypeofSpecifierAST *ast = new (pool) TypeofSpecifierAST;
    ast->typeof_token = typeof_token;
    if (expression)
        ast->expression = expression->clone(pool);
    if (next)
        ast->next = next->clone(pool);
    return ast;
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
    if (expression)
        return expression->lastToken();
    return typeof_token + 1;
}

SizeofExpressionAST *SizeofExpressionAST::clone(MemoryPool *pool) const
{
    SizeofExpressionAST *ast = new (pool) SizeofExpressionAST;
    ast->sizeof_token = sizeof_token;
    if (expression)
        ast->expression = expression->clone(pool);
    return ast;
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
    if (expression)
        return expression->lastToken();
    return sizeof_token + 1;
}

StringLiteralAST *StringLiteralAST::clone(MemoryPool *pool) const
{
    StringLiteralAST *ast = new (pool) StringLiteralAST;
    ast->token = token;
    if (next)
        ast->next = next->clone(pool);
    return ast;
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
    if (next)
        return next->lastToken();
    return token + 1;
}

SwitchStatementAST *SwitchStatementAST::clone(MemoryPool *pool) const
{
    SwitchStatementAST *ast = new (pool) SwitchStatementAST;
    ast->switch_token = switch_token;
    ast->lparen_token = lparen_token;
    if (condition)
        ast->condition = condition->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement)
        ast->statement = statement->clone(pool);
    return ast;
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
    if (statement)
        return statement->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (condition)
        return condition->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    return switch_token + 1;
}

TemplateArgumentListAST *TemplateArgumentListAST::clone(MemoryPool *pool) const
{
    TemplateArgumentListAST *ast = new (pool) TemplateArgumentListAST;
    if (template_argument)
        ast->template_argument = template_argument->clone(pool);
    if (next)
        ast->next = next->clone(pool);
    return ast;
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
    for (const TemplateArgumentListAST *it = this; it; it = it->next) {
        if (! it->next && it->template_argument)
            return it->template_argument->lastToken();
    }
    return 0;
}

TemplateDeclarationAST *TemplateDeclarationAST::clone(MemoryPool *pool) const
{
    TemplateDeclarationAST *ast = new (pool) TemplateDeclarationAST;
    ast->export_token = export_token;
    ast->template_token = template_token;
    ast->less_token = less_token;
    if (template_parameters)
        ast->template_parameters = template_parameters->clone(pool);
    ast->greater_token = greater_token;
    if (declaration)
        ast->declaration = declaration->clone(pool);
    return ast;
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
    if (declaration)
        return declaration->lastToken();
    else if (greater_token)
        return greater_token + 1;

    for (DeclarationAST *it = template_parameters; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (less_token)
        return less_token + 1;
    else if (template_token)
        return template_token + 1;
    else if (export_token)
        return export_token + 1;

    return 0;
}

TemplateIdAST *TemplateIdAST::clone(MemoryPool *pool) const
{
    TemplateIdAST *ast = new (pool) TemplateIdAST;
    ast->identifier_token = identifier_token;
    ast->less_token = less_token;
    if (template_arguments)
        ast->template_arguments = template_arguments->clone(pool);
    ast->greater_token = greater_token;
    return ast;
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
    if (greater_token)
        return greater_token + 1;

    for (TemplateArgumentListAST *it = template_arguments; it; it = it->next) {
        if (! it->next && it->template_argument)
            return it->template_argument->lastToken();
    }

    if (less_token)
        return less_token + 1;

    return identifier_token + 1;
}

TemplateTypeParameterAST *TemplateTypeParameterAST::clone(MemoryPool *pool) const
{
    TemplateTypeParameterAST *ast = new (pool) TemplateTypeParameterAST;
    ast->template_token = template_token;
    ast->less_token = less_token;
    if (template_parameters)
        ast->template_parameters = template_parameters->clone(pool);
    ast->greater_token = greater_token;
    ast->class_token = class_token;
    if (name)
        ast->name = name->clone(pool);
    ast->equal_token = equal_token;
    if (type_id)
        ast->type_id = type_id->clone(pool);
    return ast;
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
    if (type_id)
        return type_id->lastToken();
    else if (equal_token)
        return equal_token + 1;
    else if (name)
        return name->lastToken();
    else if (class_token)
        return class_token + 1;
    else if (greater_token)
        return greater_token + 1;

    for (DeclarationAST *it = template_parameters; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (less_token)
        return less_token + 1;

    return template_token + 1;
}

ThisExpressionAST *ThisExpressionAST::clone(MemoryPool *pool) const
{
    ThisExpressionAST *ast = new (pool) ThisExpressionAST;
    ast->this_token = this_token;
    return ast;
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
    return this_token + 1;
}

ThrowExpressionAST *ThrowExpressionAST::clone(MemoryPool *pool) const
{
    ThrowExpressionAST *ast = new (pool) ThrowExpressionAST;
    ast->throw_token = throw_token;
    if (expression)
        ast->expression = expression->clone(pool);
    return ast;
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
    if (expression)
        return expression->lastToken();
    return throw_token + 1;
}

TranslationUnitAST *TranslationUnitAST::clone(MemoryPool *pool) const
{
    TranslationUnitAST *ast = new (pool) TranslationUnitAST;
    if (declarations)
        ast->declarations = declarations->clone(pool);
    return ast;
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
    for (DeclarationAST *it = declarations; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }
    return 0;
}

TryBlockStatementAST *TryBlockStatementAST::clone(MemoryPool *pool) const
{
    TryBlockStatementAST *ast = new (pool) TryBlockStatementAST;
    ast->try_token = try_token;
    if (statement)
        ast->statement = statement->clone(pool);
    if (catch_clause_seq)
        ast->catch_clause_seq = catch_clause_seq->clone(pool);
    return ast;
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
    for (CatchClauseAST *it = catch_clause_seq; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (statement)
        return statement->lastToken();

    return try_token + 1;
}

TypeConstructorCallAST *TypeConstructorCallAST::clone(MemoryPool *pool) const
{
    TypeConstructorCallAST *ast = new (pool) TypeConstructorCallAST;
    if (type_specifier)
        ast->type_specifier = type_specifier->clone(pool);
    ast->lparen_token = lparen_token;
    if (expression_list)
        ast->expression_list = expression_list;
    ast->rparen_token = rparen_token;
    return ast;
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
    if (rparen_token)
        return rparen_token + 1;

    for (ExpressionListAST *it = expression_list; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (lparen_token)
        return lparen_token + 1;


    for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return 0;
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
    if (declarator)
        return declarator->lastToken();

    for (SpecifierAST *it = type_specifier; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return 0;
}

TypeidExpressionAST *TypeidExpressionAST::clone(MemoryPool *pool) const
{
    TypeidExpressionAST *ast = new (pool) TypeidExpressionAST;
    ast->typeid_token = typeid_token;
    ast->lparen_token = lparen_token;
    if (expression)
        ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
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
    if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    else if (lparen_token)
        return lparen_token + 1;

    return typeid_token + 1;
}

TypenameCallExpressionAST *TypenameCallExpressionAST::clone(MemoryPool *pool) const
{
    TypenameCallExpressionAST *ast = new (pool) TypenameCallExpressionAST;
    ast->typename_token = typename_token;
    if (name)
        ast->name = name->clone(pool);
    ast->lparen_token = lparen_token;
    if (expression_list)
        ast->expression_list = expression_list;
    ast->rparen_token = rparen_token;
    return ast;
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
    if (rparen_token)
        return rparen_token + 1;

    for (ExpressionListAST *it = expression_list; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    if (lparen_token)
        return lparen_token + 1;
    else if (name)
        return name->lastToken();

    return typename_token + 1;
}

TypenameTypeParameterAST *TypenameTypeParameterAST::clone(MemoryPool *pool) const
{
    TypenameTypeParameterAST *ast = new (pool) TypenameTypeParameterAST;
    ast->classkey_token = classkey_token;
    if (name)
        ast->name = name->clone(pool);
    ast->equal_token = equal_token;
    if (type_id)
        ast->type_id = type_id->clone(pool);
    return ast;
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
    if (type_id)
        return type_id->lastToken();
    else if (equal_token)
        return equal_token + 1;
    else if (name)
        return name->lastToken();
    return classkey_token + 1;
}

UnaryExpressionAST *UnaryExpressionAST::clone(MemoryPool *pool) const
{
    UnaryExpressionAST *ast = new (pool) UnaryExpressionAST;
    ast->unary_op_token = unary_op_token;
    if (expression)
        ast->expression = expression->clone(pool);
    return ast;
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
    if (expression)
        return expression->lastToken();
    return unary_op_token + 1;
}

UsingAST *UsingAST::clone(MemoryPool *pool) const
{
    UsingAST *ast = new (pool) UsingAST;
    ast->using_token = using_token;
    ast->typename_token = typename_token;
    if (name)
        ast->name = name->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
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
    if (semicolon_token)
        return semicolon_token + 1;
    else if (name)
        return name->lastToken();
    else if (typename_token)
        return typename_token + 1;
    return using_token + 1;
}

UsingDirectiveAST *UsingDirectiveAST::clone(MemoryPool *pool) const
{
    UsingDirectiveAST *ast = new (pool) UsingDirectiveAST;
    ast->using_token = using_token;
    ast->namespace_token = namespace_token;
    if (name)
        ast->name = name->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
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
    if (semicolon_token)
        return semicolon_token + 1;
    else if (name)
        return name->lastToken();
    else if (namespace_token)
        return namespace_token + 1;
    return using_token + 1;
}

WhileStatementAST *WhileStatementAST::clone(MemoryPool *pool) const
{
    WhileStatementAST *ast = new (pool) WhileStatementAST;
    ast->while_token = while_token;
    ast->lparen_token = lparen_token;
    if (condition)
        ast->condition = condition->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement)
        ast->statement = statement->clone(pool);
    return ast;
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
    if (statement)
        return statement->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (condition)
        return condition->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    return while_token + 1;
}

// ObjC++
unsigned IdentifierListAST::firstToken() const
{
    return identifier_token;
}

unsigned IdentifierListAST::lastToken() const
{
    for (const IdentifierListAST *it = this; it; it = it->next) {
        if (! it->next && it->identifier_token) {
            return it->identifier_token + 1;
        }
    }
    // ### assert?
    return 0;
}

IdentifierListAST *IdentifierListAST::clone(MemoryPool *pool) const
{
    IdentifierListAST *ast = new (pool) IdentifierListAST;
    ast->identifier_token = identifier_token;
    if (next)
        ast->next = next->clone(pool);
    return ast;
}

void IdentifierListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}

unsigned ObjCClassDeclarationAST::firstToken() const
{
    if (attributes)
        return attributes->firstToken();
    return class_token;
}

unsigned ObjCClassDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;

    for (IdentifierListAST *it = identifier_list; it; it = it->next) {
        if (! it->next && it->identifier_token)
            return it->identifier_token + 1;
    }

    for (SpecifierAST *it = attributes; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    return class_token + 1;
}

ObjCClassDeclarationAST *ObjCClassDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCClassDeclarationAST *ast = new (pool) ObjCClassDeclarationAST;
    if (attributes)
        ast->attributes = attributes->clone(pool);
    ast->class_token = class_token;
    if (identifier_list)
        ast->identifier_list = identifier_list->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

void ObjCClassDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *it = attributes; it; it = it->next) {
            accept(it, visitor);
        }
    }
}


CPLUSPLUS_END_NAMESPACE
