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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
}

void AccessDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
        for (ExpressionListAST *expr = expression_list; expr; expr = expr->next)
            accept(expr->expression, visitor);
    }
    visitor->endVisit(this);
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
        // ### accept the asm operand list.
    }
    visitor->endVisit(this);
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
    else if (volatile_token)
        return volatile_token + 1;
    return asm_token + 1;
}

void BaseSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
    visitor->endVisit(this);
}

unsigned BaseSpecifierAST::firstToken() const
{
    if (virtual_token && access_specifier_token)
        return std::min(virtual_token, access_specifier_token);
    return name->firstToken();
}

unsigned BaseSpecifierAST::lastToken() const
{
    if (name)
        return name->lastToken();
    else if (virtual_token && access_specifier_token)
        return std::min(virtual_token, access_specifier_token) + 1;
    else if (virtual_token)
        return virtual_token + 1;
    else if (access_specifier_token)
        return access_specifier_token + 1;
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
    visitor->endVisit(this);
}

void BinaryExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left_expression, visitor);
        accept(right_expression, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
}

unsigned BoolLiteralAST::firstToken() const
{
    return literal_token;
}

unsigned BoolLiteralAST::lastToken() const
{
    return literal_token + 1;
}

void CompoundLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(type_id, visitor);
        accept(initializer, visitor);
    }
    visitor->endVisit(this);
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

void BreakStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
        accept(expression, visitor);
        accept(statement, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
}

unsigned DeclaratorListAST::firstToken() const
{
    return declarator->firstToken();
}

unsigned DeclaratorListAST::lastToken() const
{
    for (const DeclaratorListAST *it = this; it; it = it->next) {
        if (! it->next)
            return it->declarator->lastToken();
    }
    return 0;
}

void DeleteExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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

void ExpressionListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (const ExpressionListAST *it = this; it; it = it->next) {
            accept(it->expression, visitor);
        }
    }
    visitor->endVisit(this);
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

void ExpressionOrDeclarationStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declaration, visitor);
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
        accept(parameters, visitor);
        for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next)
            accept(it, visitor);
        accept(exception_specification, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
}

unsigned LinkageSpecificationAST::firstToken() const
{
    return extern_token;
}

unsigned LinkageSpecificationAST::lastToken() const
{
    if (declaration)
        return declaration->lastToken();
    else if (extern_type_token)
        return extern_type_token + 1;
    return extern_token + 1;
}

void MemInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    else if (namespace_name_token)
        return namespace_name_token + 1;
    return namespace_token + 1;
}

void NestedDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarator, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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

void NewPlacementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExpressionListAST *it = expression_list; it; it = it->next) {
            accept(it->expression, visitor);
        }
    }
    visitor->endVisit(this);
}

unsigned NewPlacementAST::firstToken() const
{
    return lparen_token;
}

unsigned NewPlacementAST::lastToken() const
{
    return rparen_token + 1;
}

void NewArrayDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

unsigned NewArrayDeclaratorAST::firstToken() const
{
    return lbracket_token;
}

unsigned NewArrayDeclaratorAST::lastToken() const
{
    return rbracket_token + 1;
}

void NewExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(new_placement, visitor);
        accept(type_id, visitor);
        accept(new_type_id, visitor);
        accept(new_initializer, visitor);
    }
    visitor->endVisit(this);
}

unsigned NewExpressionAST::firstToken() const
{
    if (scope_token)
        return scope_token;
    return new_token;
}

unsigned NewExpressionAST::lastToken() const
{
    // ### FIXME
    if (new_token)
        return new_token + 1;
    else if (scope_token)
        return scope_token + 1;
    // ### assert?
    return 0;
}

void NewInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void NewTypeIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);

        for (PtrOperatorAST *it = ptr_operators; it; it = it->next)
            accept(it, visitor);

        for (NewArrayDeclaratorAST *it = new_array_declarators; it; it = it->next)
            accept(it, visitor);

    }
    visitor->endVisit(this);
}

unsigned NewTypeIdAST::firstToken() const
{
    return type_specifier->firstToken();
}

unsigned NewTypeIdAST::lastToken() const
{
    for (NewArrayDeclaratorAST *it = new_array_declarators; it; it = it->next) {
        if (! it->next)
            return it->lastToken();
    }

    for (PtrOperatorAST *it = ptr_operators; it; it = it->next) {
        if (it->next)
            return it->lastToken();
    }

    if (type_specifier)
        return type_specifier->lastToken();

    // ### assert?
    return 0;
}

void NumericLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
}

unsigned NumericLiteralAST::firstToken() const
{
    return literal_token;
}

unsigned NumericLiteralAST::lastToken() const
{
    return literal_token + 1;
}

void OperatorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
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

void OperatorFunctionIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(op, visitor);
    }
    visitor->endVisit(this);
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

void ParameterDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        accept(declarator, visitor);
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void ParameterDeclarationClauseAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclarationAST *param = parameter_declarations; param;
                param = param->next)
            accept(param, visitor);
    }
    visitor->endVisit(this);
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

void PointerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = cv_qualifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
    }
    visitor->endVisit(this);
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

void PointerToMemberAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(nested_name_specifier, visitor);
        for (SpecifierAST *spec = cv_qualifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
    }
    visitor->endVisit(this);
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

void PostIncrDecrAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
}

unsigned PostIncrDecrAST::firstToken() const
{
    return incr_decr_token;
}

unsigned PostIncrDecrAST::lastToken() const
{
    return incr_decr_token + 1;
}

void PostfixExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base_expression, visitor);
        for (PostfixAST *fx = postfix_expressions; fx; fx = fx->next)
            accept(fx, visitor);
    }
    visitor->endVisit(this);
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

void QualifiedNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(nested_name_specifier, visitor);
        accept(unqualified_name, visitor);
    }
    visitor->endVisit(this);
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

void ReferenceAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
}

unsigned ReferenceAST::firstToken() const
{
    return amp_token;
}

unsigned ReferenceAST::lastToken() const
{
    return amp_token + 1;
}

void ReturnStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void SimpleDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = decl_specifier_seq; spec;
                 spec = spec->next)
            accept(spec, visitor);
        accept(declarators, visitor);
    }
    visitor->endVisit(this);
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

void SimpleNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
}

unsigned SimpleSpecifierAST::firstToken() const
{
    return specifier_token;
}

unsigned SimpleSpecifierAST::lastToken() const
{
    return specifier_token + 1;
}

void TypeofSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void SizeofExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void StringLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

unsigned StringLiteralAST::firstToken() const
{
    return literal_token;
}

unsigned StringLiteralAST::lastToken() const
{
    if (next)
        return next->lastToken();
    return literal_token + 1;
}

void SwitchStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(statement, visitor);
    }
    visitor->endVisit(this);
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

void TemplateArgumentListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(template_argument, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
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

void TemplateDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclarationAST *param = template_parameters; param;
                 param = param->next)
            accept(param, visitor);
        accept(declaration, visitor);
    }
    visitor->endVisit(this);
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

void TemplateIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (TemplateArgumentListAST *it = template_arguments; it; it = it->next) {
            accept(it, visitor);
        }
    }
    visitor->endVisit(this);
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

void TemplateTypeParameterAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
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

void ThisExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
}

unsigned ThisExpressionAST::firstToken() const
{
    return this_token;
}

unsigned ThisExpressionAST::lastToken() const
{
    return this_token + 1;
}

void ThrowExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void TranslationUnitAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (DeclarationAST *decl = declarations; decl;
                 decl = decl->next)
            accept(decl, visitor);
    }
    visitor->endVisit(this);
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

void TryBlockStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(catch_clause_seq, visitor);
    }
    visitor->endVisit(this);
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

void TypeConstructorCallAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *spec = type_specifier; spec; spec = spec->next)
            accept(spec, visitor);
        for (ExpressionListAST *expr = expression_list;expr;
                 expr = expr->next)
            accept(expr->expression, visitor);
    }
    visitor->endVisit(this);
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
    visitor->endVisit(this);
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

void TypeidExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void TypenameCallExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        for (ExpressionListAST *expr = expression_list;expr;
                 expr = expr->next)
            accept(expr->expression, visitor);
    }
    visitor->endVisit(this);
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

void TypenameTypeParameterAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        accept(type_id, visitor);
    }
    visitor->endVisit(this);
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

void UnaryExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
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

void UsingAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
    visitor->endVisit(this);
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

void UsingDirectiveAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
    }
    visitor->endVisit(this);
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

void WhileStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        accept(condition, visitor);
        accept(statement, visitor);
    }
    visitor->endVisit(this);
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

void IdentifierListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
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

void ObjCClassDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (SpecifierAST *it = attributes; it; it = it->next) {
            accept(it, visitor);
        }
    }
    visitor->endVisit(this);
}


CPLUSPLUS_END_NAMESPACE
