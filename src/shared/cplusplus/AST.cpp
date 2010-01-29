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
#include "ASTMatcher.h"
#include "MemoryPool.h"

#include <cassert>
#include <algorithm>

using namespace CPlusPlus;

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

bool AST::match(AST *ast, AST *pattern, ASTMatcher *matcher)
{
    if (ast == pattern)
        return true;

    else if (! ast || ! pattern)
        return false;

    return ast->match(pattern, matcher);
}

bool AST::match(AST *pattern, ASTMatcher *matcher)
{
    return match0(pattern, matcher);
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
    else if (attribute_list)
        return attribute_list->lastToken();
    else if (second_lparen_token)
        return second_lparen_token + 1;
    else if (first_lparen_token)
        return first_lparen_token + 1;
    return attribute_token + 1;
}


unsigned AttributeAST::firstToken() const
{
    return identifier_token;
}

unsigned AttributeAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    else if (expression_list)
        return expression_list->lastToken();

    else if (tag_token)
        return tag_token + 1;

    else if (lparen_token)
        return lparen_token + 1;

    return identifier_token + 1;
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

#ifdef ICHECK_BUILD
unsigned QPropertyDeclarationAST::firstToken() const
{
    return property_specifier_token;
}

unsigned QPropertyDeclarationAST::lastToken() const
{
    return rparen_token;
}

unsigned QEnumDeclarationAST::firstToken() const
{
    return enum_specifier_token;
}

unsigned QEnumDeclarationAST::lastToken() const
{
    return rparen_token;
}

unsigned QFlagsDeclarationAST::firstToken() const
{
    return this->flags_specifier_token;
}

unsigned QFlagsDeclarationAST::lastToken() const
{
    return rparen_token;
}

unsigned QDeclareFlagsDeclarationAST::firstToken() const
{
    return declareflags_specifier_token;
}

unsigned QDeclareFlagsDeclarationAST::lastToken() const
{
    return rparen_token;
}
#endif

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


unsigned ArrayInitializerAST::firstToken() const
{
    return lbrace_token;
}

unsigned ArrayInitializerAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    else if (expression_list)
        return expression_list->lastToken();

    return lbrace_token + 1;
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

unsigned QtMemberDeclarationAST::firstToken() const
{
    return q_token;
}

unsigned QtMemberDeclarationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    else if (type_id)
        return type_id->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    return q_token + 1;
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


unsigned BoolLiteralAST::firstToken() const
{
    return literal_token;
}

unsigned BoolLiteralAST::lastToken() const
{
    return literal_token + 1;
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


unsigned CallAST::firstToken() const
{
    return lparen_token;
}

unsigned CallAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    else if (expression_list)
        return expression_list->lastToken();

    return lparen_token + 1;
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
    if (exception_declaration)
        return exception_declaration->lastToken();
    if (lparen_token)
        return lparen_token + 1;

    return catch_token + 1;
}


unsigned ClassSpecifierAST::firstToken() const
{
    return classkey_token;
}

unsigned ClassSpecifierAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    else if (member_specifier_list)
        return member_specifier_list->lastToken();

    else if (lbrace_token)
        return lbrace_token + 1;

    else if (base_clause_list)
        return base_clause_list->lastToken();

    else if (colon_token)
        return colon_token + 1;

    else if (name)
        return name->lastToken();

    else if (attribute_list)
        return attribute_list->lastToken();

    return classkey_token + 1;
}

unsigned CompoundStatementAST::firstToken() const
{
    return lbrace_token;
}

unsigned CompoundStatementAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    else if (statement_list)
        return statement_list->lastToken();

    return lbrace_token + 1;
}


unsigned ConditionAST::firstToken() const
{
    if (type_specifier_list)
        return type_specifier_list->firstToken();

    return declarator->firstToken();
}

unsigned ConditionAST::lastToken() const
{
    if (declarator)
        return declarator->lastToken();

    else if (type_specifier_list)
        return type_specifier_list->lastToken();

    // ### assert?
    return 0;
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


unsigned ConversionFunctionIdAST::firstToken() const
{
    return operator_token;
}

unsigned ConversionFunctionIdAST::lastToken() const
{
    if (ptr_operator_list)
        return ptr_operator_list->lastToken();

    else if (type_specifier_list)
        return type_specifier_list->lastToken();

    return operator_token + 1;
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


unsigned CtorInitializerAST::firstToken() const
{
    return colon_token;
}

unsigned CtorInitializerAST::lastToken() const
{
    if (member_initializer_list)
        return member_initializer_list->lastToken();
    return colon_token + 1;
}

unsigned DeclaratorAST::firstToken() const
{
    if (attribute_list)
        return attribute_list->firstToken();
    if (ptr_operator_list)
        return ptr_operator_list->firstToken();
    else if (core_declarator)
        return core_declarator->firstToken();
    else if (postfix_declarator_list)
        return postfix_declarator_list->firstToken();
    else if (attribute_list)
        return attribute_list->firstToken();
    else if (initializer)
        return initializer->firstToken();
    // ### assert?
    return 0;
}

unsigned DeclaratorAST::lastToken() const
{
    if (initializer)
        return initializer->lastToken();

    else if (post_attribute_list)
        return post_attribute_list->lastToken();

    else if (postfix_declarator_list)
        return postfix_declarator_list->lastToken();

    else if (core_declarator)
        return core_declarator->lastToken();

    else if (ptr_operator_list)
        return ptr_operator_list->lastToken();

    else if (attribute_list)
        return attribute_list->lastToken();

    // ### assert?
    return 0;
}


unsigned DeclarationStatementAST::firstToken() const
{
    return declaration->firstToken();
}

unsigned DeclarationStatementAST::lastToken() const
{
    return declaration->lastToken();
}


unsigned DeclaratorIdAST::firstToken() const
{
    return name->firstToken();
}

unsigned DeclaratorIdAST::lastToken() const
{
    return name->lastToken();
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


unsigned EmptyDeclarationAST::firstToken() const
{
    return semicolon_token;
}

unsigned EmptyDeclarationAST::lastToken() const
{
    return semicolon_token + 1;
}


unsigned EnumSpecifierAST::firstToken() const
{
    return enum_token;
}

unsigned EnumSpecifierAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    if (enumerator_list)
        return enumerator_list->lastToken();

    if (lbrace_token)
        return lbrace_token + 1;
    if (name)
        return name->lastToken();

    return enum_token + 1;
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


unsigned ExceptionDeclarationAST::firstToken() const
{
    if (type_specifier_list)
        return type_specifier_list->firstToken();
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

    else if (type_specifier_list)
        return type_specifier_list->lastToken();

    return 0;
}


unsigned ExceptionSpecificationAST::firstToken() const
{
    return throw_token;
}

unsigned ExceptionSpecificationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    else if (type_id_list)
        return type_id_list->lastToken();

    else if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;

    else if (lparen_token)
        return lparen_token + 1;

    return throw_token + 1;
}

unsigned ExpressionOrDeclarationStatementAST::firstToken() const
{
    return declaration->firstToken();
}

unsigned ExpressionOrDeclarationStatementAST::lastToken() const
{
    return declaration->lastToken();
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

unsigned ForeachStatementAST::firstToken() const
{
    return foreach_token;
}

unsigned ForeachStatementAST::lastToken() const
{
    if (statement)
        return statement->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (expression)
        return expression->lastToken();
    else if (comma_token)
        return comma_token + 1;

    return foreach_token + 1;
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


unsigned FunctionDeclaratorAST::firstToken() const
{
    return lparen_token;
}

unsigned FunctionDeclaratorAST::lastToken() const
{
    if (exception_specification)
        return exception_specification->lastToken();

    else if (cv_qualifier_list)
        return cv_qualifier_list->lastToken();

    else if (rparen_token)
        return rparen_token + 1;

    else if (parameters)
        return parameters->lastToken();

    return lparen_token + 1;
}


unsigned FunctionDefinitionAST::firstToken() const
{
    if (decl_specifier_list)
        return decl_specifier_list->firstToken();
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

    else if (declarator)
        return declarator->lastToken();

    else if (decl_specifier_list)
        return decl_specifier_list->lastToken();

    // ### assert
    return 0;
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


unsigned LinkageBodyAST::firstToken() const
{
    return lbrace_token;
}

unsigned LinkageBodyAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    else if (declaration_list)
        return declaration_list->lastToken();

    return lbrace_token + 1;
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


unsigned MemInitializerAST::firstToken() const
{
    return name->firstToken();
}

unsigned MemInitializerAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    else if (expression_list)
        return expression_list->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    return name->lastToken();
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


unsigned NamedTypeSpecifierAST::firstToken() const
{
    return name->firstToken();
}

unsigned NamedTypeSpecifierAST::lastToken() const
{
    return name->lastToken();
}


unsigned NamespaceAST::firstToken() const
{
    return namespace_token;
}

unsigned NamespaceAST::lastToken() const
{
    if (linkage_body)
        return linkage_body->lastToken();

    else if (attribute_list)
        return attribute_list->lastToken();

    else if (identifier_token)
        return identifier_token + 1;

    return namespace_token + 1;
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


unsigned NewPlacementAST::firstToken() const
{
    return lparen_token;
}

unsigned NewPlacementAST::lastToken() const
{
    return rparen_token + 1;
}


unsigned NewArrayDeclaratorAST::firstToken() const
{
    return lbracket_token;
}

unsigned NewArrayDeclaratorAST::lastToken() const
{
    return rbracket_token + 1;
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


unsigned NewTypeIdAST::firstToken() const
{
    return type_specifier_list->firstToken();
}

unsigned NewTypeIdAST::lastToken() const
{
    if (new_array_declarator_list)
        return new_array_declarator_list->lastToken();

    else if (ptr_operator_list)
        return ptr_operator_list->lastToken();

    else if (type_specifier_list)
        return type_specifier_list->lastToken();

    // ### assert?
    return 0;
}


unsigned NumericLiteralAST::firstToken() const
{
    return literal_token;
}

unsigned NumericLiteralAST::lastToken() const
{
    return literal_token + 1;
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


unsigned ParameterDeclarationAST::firstToken() const
{
    return type_specifier_list->firstToken();
}

unsigned ParameterDeclarationAST::lastToken() const
{
    if (expression)
        return expression->lastToken();

    else if (equal_token)
        return equal_token + 1;

    else if (declarator)
        return declarator->lastToken();

    else if (type_specifier_list)
        return type_specifier_list->lastToken();

    // ### assert?
    return 0;
}


unsigned ParameterDeclarationClauseAST::firstToken() const
{
    if (parameter_declaration_list)
        return parameter_declaration_list->firstToken();
    return dot_dot_dot_token;
}

unsigned ParameterDeclarationClauseAST::lastToken() const
{
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    return parameter_declaration_list->lastToken();
}


unsigned PointerAST::firstToken() const
{
    return star_token;
}

unsigned PointerAST::lastToken() const
{
    if (cv_qualifier_list)
        return cv_qualifier_list->lastToken();

    return star_token + 1;
}


unsigned PointerToMemberAST::firstToken() const
{
    if (global_scope_token)
        return global_scope_token;
    else if (nested_name_specifier_list)
        return nested_name_specifier_list->firstToken();
    return star_token;
}

unsigned PointerToMemberAST::lastToken() const
{
    if (cv_qualifier_list)
        return cv_qualifier_list->lastToken();

    else if (star_token)
        return star_token + 1;

    else if (nested_name_specifier_list)
        return nested_name_specifier_list->lastToken();

    else if (global_scope_token)
        return global_scope_token + 1;

    // ### assert(0);
    return 0;
}


unsigned PostIncrDecrAST::firstToken() const
{
    return incr_decr_token;
}

unsigned PostIncrDecrAST::lastToken() const
{
    return incr_decr_token + 1;
}


unsigned PostfixExpressionAST::firstToken() const
{
    return base_expression->firstToken();
}

unsigned PostfixExpressionAST::lastToken() const
{
    if (postfix_expression_list)
        return postfix_expression_list->lastToken();
    return base_expression->lastToken();
}


unsigned QualifiedNameAST::firstToken() const
{
    if (global_scope_token)
        return global_scope_token;
    else if (nested_name_specifier_list)
        return nested_name_specifier_list->firstToken();
    return unqualified_name->firstToken();
}

unsigned QualifiedNameAST::lastToken() const
{
    if (unqualified_name)
        return unqualified_name->lastToken();

    if (nested_name_specifier_list)
        return nested_name_specifier_list->lastToken();

    if (global_scope_token)
        return global_scope_token + 1;

    return 0;
}


unsigned ReferenceAST::firstToken() const
{
    return amp_token;
}

unsigned ReferenceAST::lastToken() const
{
    return amp_token + 1;
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


unsigned SimpleDeclarationAST::firstToken() const
{
    if (decl_specifier_list)
        return decl_specifier_list->firstToken();
    else if (declarator_list)
        return declarator_list->firstToken();
    return semicolon_token;
}

unsigned SimpleDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;

    else if (declarator_list)
        return declarator_list->lastToken();

    else if (decl_specifier_list)
        return decl_specifier_list->lastToken();

    // ### assert(0);
    return 0;
}

unsigned SimpleNameAST::firstToken() const
{
    return identifier_token;
}

unsigned SimpleNameAST::lastToken() const
{
    return identifier_token + 1;
}


unsigned SimpleSpecifierAST::firstToken() const
{
    return specifier_token;
}

unsigned SimpleSpecifierAST::lastToken() const
{
    return specifier_token + 1;
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

    else if (template_parameter_list)
        return template_parameter_list->lastToken();

    else if (less_token)
        return less_token + 1;

    else if (template_token)
        return template_token + 1;

    else if (export_token)
        return export_token + 1;

    // ### assert(0);
    return 0;
}


unsigned TemplateIdAST::firstToken() const
{
    return identifier_token;
}

unsigned TemplateIdAST::lastToken() const
{
    if (greater_token)
        return greater_token + 1;

    else if (template_argument_list)
        return template_argument_list->lastToken();

    else if (less_token)
        return less_token + 1;

    return identifier_token + 1;
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

    else if (template_parameter_list)
        return template_parameter_list->lastToken();

    else if (less_token)
        return less_token + 1;

    return template_token + 1;
}


unsigned ThisExpressionAST::firstToken() const
{
    return this_token;
}

unsigned ThisExpressionAST::lastToken() const
{
    return this_token + 1;
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

unsigned TranslationUnitAST::firstToken() const
{
    if(declaration_list)
        return declaration_list->firstToken();
    return 0;
}

unsigned TranslationUnitAST::lastToken() const
{
    if (declaration_list)
        return declaration_list->lastToken();

    // ### assert(0);
    return 0;
}

unsigned TryBlockStatementAST::firstToken() const
{
    return try_token;
}

unsigned TryBlockStatementAST::lastToken() const
{
    if (catch_clause_list)
        return catch_clause_list->lastToken();

    else if (statement)
        return statement->lastToken();

    return try_token + 1;
}


unsigned TypeConstructorCallAST::firstToken() const
{
    return type_specifier_list->firstToken();
}

unsigned TypeConstructorCallAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    else if (expression_list)
        return expression_list->lastToken();

    else if (lparen_token)
        return lparen_token + 1;

    else if (type_specifier_list)
        return type_specifier_list->lastToken();

    // ### assert(0);
    return 0;
}


unsigned TypeIdAST::firstToken() const
{
    return type_specifier_list->firstToken();
}

unsigned TypeIdAST::lastToken() const
{
    if (declarator)
        return declarator->lastToken();

    else if (type_specifier_list)
        return type_specifier_list->lastToken();

    // ### assert(0);
    return 0;
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


unsigned TypenameCallExpressionAST::firstToken() const
{
    return typename_token;
}

unsigned TypenameCallExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    else if (expression_list)
        return expression_list->lastToken();

    else if (lparen_token)
        return lparen_token + 1;

    else if (name)
        return name->lastToken();

    return typename_token + 1;
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
unsigned ObjCClassForwardDeclarationAST::firstToken() const
{
    if (attribute_list)
        return attribute_list->firstToken();

    return class_token;
}

unsigned ObjCClassForwardDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;

    else if (identifier_list)
        return identifier_list->lastToken();

    return class_token + 1;
}

unsigned ObjCProtocolForwardDeclarationAST::firstToken() const
{
    if (attribute_list)
        return attribute_list->firstToken();

    return protocol_token;
}

unsigned ObjCProtocolForwardDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;

    else if (identifier_list)
        return identifier_list->lastToken();

    return protocol_token + 1;
}

unsigned ObjCClassDeclarationAST::firstToken() const
{
    if (attribute_list)
        return attribute_list->firstToken();

    if (interface_token)
        return interface_token;
    else
        return implementation_token;
}

unsigned ObjCClassDeclarationAST::lastToken() const
{
    if (end_token)                   return end_token + 1;
    if (member_declaration_list)         return member_declaration_list->lastToken();
    if (inst_vars_decl)              return inst_vars_decl->lastToken();
    if (protocol_refs)
        return protocol_refs->lastToken();
    if (superclass)
        return superclass->lastToken();
    if (colon_token)                 return colon_token + 1;
    if (rparen_token)
        return rparen_token + 1;
    if (category_name)
        return category_name->lastToken();
    if (lparen_token)
        return lparen_token + 1;
    if (class_name)                  return class_name->lastToken();

    if (interface_token)
        return interface_token + 1;
    else
        return implementation_token + 1;
}

unsigned ObjCProtocolDeclarationAST::firstToken() const
{
    if (attribute_list)
        return attribute_list->firstToken();
    return protocol_token;
}

unsigned ObjCProtocolDeclarationAST::lastToken() const
{
    if (end_token)
        return end_token + 1;

    else if (member_declaration_list)
        return member_declaration_list->lastToken();

    else if (protocol_refs)
        return protocol_refs->lastToken();

    else if (name)
        return name->lastToken();

    else if (attribute_list)
        return attribute_list->lastToken();

    return protocol_token + 1;
}

unsigned ObjCProtocolRefsAST::firstToken() const
{
    return less_token;
}

unsigned ObjCProtocolRefsAST::lastToken() const
{
    if (greater_token)
        return greater_token + 1;

    else if (identifier_list)
        return identifier_list->lastToken();

    return less_token + 1;
}

unsigned ObjCMessageExpressionAST::firstToken() const
{
    return lbracket_token;
}

unsigned ObjCMessageExpressionAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;

    if (receiver_expression)
        return receiver_expression->lastToken();

    if (selector)
        return selector->lastToken();

    if (argument_list)
        return argument_list->lastToken();

    return lbracket_token + 1;
}

unsigned ObjCMessageArgumentAST::firstToken() const
{
    if (parameter_value_expression)
        return parameter_value_expression->firstToken();

    // ### assert?
    return 0;
}

unsigned ObjCMessageArgumentAST::lastToken() const
{
    if (parameter_value_expression)
        return parameter_value_expression->lastToken();

    // ### assert?
    return 0;
}

unsigned ObjCProtocolExpressionAST::firstToken() const
{
    return protocol_token;
}

unsigned ObjCProtocolExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    if (identifier_token)
        return identifier_token + 1;

    if (lparen_token)
        return lparen_token + 1;

    return protocol_token + 1;
}

unsigned ObjCTypeNameAST::firstToken() const
{
    return lparen_token;
}

unsigned ObjCTypeNameAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;

    if (type_id)
        return type_id->lastToken();

    if (type_qualifier)
        return type_qualifier + 1;

    return lparen_token + 1;
}

unsigned ObjCEncodeExpressionAST::firstToken() const
{
    return encode_token;
}

unsigned ObjCEncodeExpressionAST::lastToken() const
{
    if (type_name)
        return type_name->lastToken();

    return encode_token + 1;
}

unsigned ObjCSelectorWithoutArgumentsAST::firstToken() const
{
    return name_token;
}

unsigned ObjCSelectorWithoutArgumentsAST::lastToken() const
{
    return name_token + 1;
}

unsigned ObjCSelectorArgumentAST::firstToken() const
{
    return name_token;
}

unsigned ObjCSelectorArgumentAST::lastToken() const
{
    if (colon_token)
        return colon_token + 1;
    else
        return name_token + 1;
}

unsigned ObjCSelectorWithArgumentsAST::firstToken() const
{
    return selector_argument_list->firstToken();
}

unsigned ObjCSelectorWithArgumentsAST::lastToken() const
{
    return selector_argument_list->lastToken();
}

unsigned ObjCSelectorExpressionAST::firstToken() const
{
    return selector_token;
}

unsigned ObjCSelectorExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (selector)
        return selector->lastToken();
    if (lparen_token)
        return rparen_token + 1;
    return selector_token + 1;
}

unsigned ObjCInstanceVariablesDeclarationAST::firstToken() const
{
    return lbrace_token;
}

unsigned ObjCInstanceVariablesDeclarationAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;

    if (instance_variable_list)
        return instance_variable_list->lastToken();

    return lbrace_token + 1;
}

unsigned ObjCVisibilityDeclarationAST::firstToken() const
{
    return visibility_token;
}

unsigned ObjCVisibilityDeclarationAST::lastToken() const
{
    return visibility_token + 1;
}

unsigned ObjCPropertyAttributeAST::firstToken() const
{
    return attribute_identifier_token;
}

unsigned ObjCPropertyAttributeAST::lastToken() const
{
    if (method_selector)
        return method_selector->lastToken();
    if (equals_token)
        return equals_token + 1;

    return attribute_identifier_token + 1;
}

unsigned ObjCPropertyDeclarationAST::firstToken() const
{
    if (attribute_list)
        return attribute_list->firstToken();

    return property_token;
}

unsigned ObjCPropertyDeclarationAST::lastToken() const
{
    if (simple_declaration)
        return simple_declaration->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (property_attribute_list)
        return property_attribute_list->lastToken();
    else if (lparen_token)
        return lparen_token + 1;

    return property_token + 1;
}

unsigned ObjCMessageArgumentDeclarationAST::firstToken() const
{
    if (type_name)
        return type_name->firstToken();
    else
        return param_name_token;
}

unsigned ObjCMessageArgumentDeclarationAST::lastToken() const
{
    if (param_name_token)
        return param_name_token + 1;
    else if (type_name)
        return type_name->lastToken();

    // ### assert?
    return 0;
}

unsigned ObjCMethodPrototypeAST::firstToken() const
{
    return method_type_token;
}

unsigned ObjCMethodPrototypeAST::lastToken() const
{
    if (attribute_list)
        return attribute_list->lastToken();
    else if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    else if (argument_list)
        return argument_list->lastToken();
    else if (type_name)
        return type_name->lastToken();
    return method_type_token + 1;
}

unsigned ObjCMethodDeclarationAST::firstToken() const
{
    return method_prototype->firstToken();
}

unsigned ObjCMethodDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (function_body)
        return function_body->lastToken();
    return method_prototype->lastToken();
}

unsigned ObjCSynthesizedPropertyAST::firstToken() const
{
    if (property_identifier)
        return property_identifier;
    else if (equals_token)
        return equals_token;
    else
        return property_alias_identifier;
}

unsigned ObjCSynthesizedPropertyAST::lastToken() const
{
    if (property_alias_identifier)
        return property_alias_identifier + 1;
    else if (equals_token)
        return equals_token + 1;
    else
        return property_identifier + 1;
}

unsigned ObjCSynthesizedPropertiesDeclarationAST::firstToken() const
{
    return synthesized_token;
}

unsigned ObjCSynthesizedPropertiesDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    else if (property_identifier_list)
        return property_identifier_list->lastToken();
    else
        return synthesized_token + 1;
}

unsigned ObjCDynamicPropertiesDeclarationAST::firstToken() const
{
    return dynamic_token;
}

unsigned ObjCDynamicPropertiesDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    else if (property_identifier_list)
        return property_identifier_list->lastToken();
    else
        return dynamic_token + 1;
}

unsigned ObjCFastEnumerationAST::firstToken() const
{
    return for_token;
}

unsigned ObjCFastEnumerationAST::lastToken() const
{
    if (statement)
        return statement->lastToken();
    else if (rparen_token)
        return rparen_token + 1;
    else if (fast_enumeratable_expression)
        return fast_enumeratable_expression->lastToken();
    else if (in_token)
        return in_token + 1;
    else if (initializer)
        return initializer->lastToken();
    else if (declarator)
        return declarator->lastToken();
    else if (type_specifier_list)
        return type_specifier_list->lastToken();
    else if (lparen_token)
        return lparen_token + 1;
    else
        return for_token + 1;
}

unsigned ObjCSynchronizedStatementAST::firstToken() const
{
    return synchronized_token;
}

unsigned ObjCSynchronizedStatementAST::lastToken() const
{
    if (statement) return statement->lastToken();
    if (rparen_token) return rparen_token + 1;
    if (synchronized_object) return synchronized_object->lastToken();
    if (lparen_token) return lparen_token + 1;
    return synchronized_token + 1;
}


