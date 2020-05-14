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

#include "cppassert.h"

#include <algorithm>


/*
   All firstToken/lastToken functions below which have a doxygen comment with
   \generated in it, will be re-generated when the tool "cplusplus-update-frontend" is run.

   For functions which are hand-coded, or which should not be changed, make sure that
   the comment is gone.
 */


using namespace CPlusPlus;

AST::AST()
{ }

AST::~AST()
{ CPP_CHECK(0); }

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

int GnuAttributeSpecifierAST::firstToken() const
{
    return attribute_token;
}

int MsvcDeclspecSpecifierAST::firstToken() const
{
    return attribute_token;
}

int StdAttributeSpecifierAST::firstToken() const
{
    return first_lbracket_token;
}

int BaseSpecifierAST::firstToken() const
{
    if (virtual_token && access_specifier_token)
        return std::min(virtual_token, access_specifier_token);
    if (virtual_token)
        return virtual_token;
    if (access_specifier_token)
        return access_specifier_token;
    if (name)
        return name->firstToken();
    // assert?
    return 0;
}

int BaseSpecifierAST::lastToken() const
{
    if (ellipsis_token)
        return ellipsis_token;
    else if (name)
        return name->lastToken();
    else if (virtual_token && access_specifier_token)
        return std::max(virtual_token, access_specifier_token) + 1;
    else if (virtual_token)
        return virtual_token + 1;
    else if (access_specifier_token)
        return access_specifier_token + 1;
    // assert?
    return 0;
}

/** \generated */
int AccessDeclarationAST::firstToken() const
{
    if (access_specifier_token)
        return access_specifier_token;
    if (slots_token)
        return slots_token;
    if (colon_token)
        return colon_token;
    return 0;
}

/** \generated */
int AccessDeclarationAST::lastToken() const
{
    if (colon_token)
        return colon_token + 1;
    if (slots_token)
        return slots_token + 1;
    if (access_specifier_token)
        return access_specifier_token + 1;
    return 1;
}

/** \generated */
int ArrayAccessAST::firstToken() const
{
    if (base_expression)
        if (int candidate = base_expression->firstToken())
            return candidate;
    if (lbracket_token)
        return lbracket_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rbracket_token)
        return rbracket_token;
    return 0;
}

/** \generated */
int ArrayAccessAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lbracket_token)
        return lbracket_token + 1;
    if (base_expression)
        if (int candidate = base_expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ArrayDeclaratorAST::firstToken() const
{
    if (lbracket_token)
        return lbracket_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rbracket_token)
        return rbracket_token;
    return 0;
}

/** \generated */
int ArrayDeclaratorAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lbracket_token)
        return lbracket_token + 1;
    return 1;
}

/** \generated */
int ArrayInitializerAST::firstToken() const
{
    if (lbrace_token)
        return lbrace_token;
    if (expression_list)
        if (int candidate = expression_list->firstToken())
            return candidate;
    if (rbrace_token)
        return rbrace_token;
    return 0;
}

/** \generated */
int ArrayInitializerAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;
    if (expression_list)
        if (int candidate = expression_list->lastToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token + 1;
    return 1;
}

/** \generated */
int AsmDefinitionAST::firstToken() const
{
    if (asm_token)
        return asm_token;
    if (volatile_token)
        return volatile_token;
    if (lparen_token)
        return lparen_token;
    if (rparen_token)
        return rparen_token;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int AsmDefinitionAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (rparen_token)
        return rparen_token + 1;
    if (lparen_token)
        return lparen_token + 1;
    if (volatile_token)
        return volatile_token + 1;
    if (asm_token)
        return asm_token + 1;
    return 1;
}

/** \generated */
int GnuAttributeAST::firstToken() const
{
    if (identifier_token)
        return identifier_token;
    if (lparen_token)
        return lparen_token;
    if (tag_token)
        return tag_token;
    if (expression_list)
        if (int candidate = expression_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int GnuAttributeAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression_list)
        if (int candidate = expression_list->lastToken())
            return candidate;
    if (tag_token)
        return tag_token + 1;
    if (lparen_token)
        return lparen_token + 1;
    if (identifier_token)
        return identifier_token + 1;
    return 1;
}

/** \generated */
int BinaryExpressionAST::firstToken() const
{
    if (left_expression)
        if (int candidate = left_expression->firstToken())
            return candidate;
    if (binary_op_token)
        return binary_op_token;
    if (right_expression)
        if (int candidate = right_expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int BinaryExpressionAST::lastToken() const
{
    if (right_expression)
        if (int candidate = right_expression->lastToken())
            return candidate;
    if (binary_op_token)
        return binary_op_token + 1;
    if (left_expression)
        if (int candidate = left_expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int BoolLiteralAST::firstToken() const
{
    if (literal_token)
        return literal_token;
    return 0;
}

/** \generated */
int BoolLiteralAST::lastToken() const
{
    if (literal_token)
        return literal_token + 1;
    return 1;
}

/** \generated */
int BracedInitializerAST::firstToken() const
{
    if (lbrace_token)
        return lbrace_token;
    if (expression_list)
        if (int candidate = expression_list->firstToken())
            return candidate;
    if (comma_token)
        return comma_token;
    if (rbrace_token)
        return rbrace_token;
    return 0;
}

/** \generated */
int BracedInitializerAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;
    if (comma_token)
        return comma_token + 1;
    if (expression_list)
        if (int candidate = expression_list->lastToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token + 1;
    return 1;
}

/** \generated */
int BreakStatementAST::firstToken() const
{
    if (break_token)
        return break_token;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int BreakStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (break_token)
        return break_token + 1;
    return 1;
}

/** \generated */
int CallAST::firstToken() const
{
    if (base_expression)
        if (int candidate = base_expression->firstToken())
            return candidate;
    if (lparen_token)
        return lparen_token;
    if (expression_list)
        if (int candidate = expression_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int CallAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression_list)
        if (int candidate = expression_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (base_expression)
        if (int candidate = base_expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int CaptureAST::firstToken() const
{
    if (amper_token)
        return amper_token;
    if (identifier)
        if (int candidate = identifier->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int CaptureAST::lastToken() const
{
    if (identifier)
        if (int candidate = identifier->lastToken())
            return candidate;
    if (amper_token)
        return amper_token + 1;
    return 1;
}

/** \generated */
int CaseStatementAST::firstToken() const
{
    if (case_token)
        return case_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (colon_token)
        return colon_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int CaseStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (case_token)
        return case_token + 1;
    return 1;
}

/** \generated */
int CastExpressionAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int CastExpressionAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int CatchClauseAST::firstToken() const
{
    if (catch_token)
        return catch_token;
    if (lparen_token)
        return lparen_token;
    if (exception_declaration)
        if (int candidate = exception_declaration->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int CatchClauseAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (exception_declaration)
        if (int candidate = exception_declaration->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (catch_token)
        return catch_token + 1;
    return 1;
}

/** \generated */
int ClassSpecifierAST::firstToken() const
{
    if (classkey_token)
        return classkey_token;
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (final_token)
        return final_token;
    if (colon_token)
        return colon_token;
    if (base_clause_list)
        if (int candidate = base_clause_list->firstToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    if (lbrace_token)
        return lbrace_token;
    if (member_specifier_list)
        if (int candidate = member_specifier_list->firstToken())
            return candidate;
    if (rbrace_token)
        return rbrace_token;
    return 0;
}

/** \generated */
int ClassSpecifierAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;
    if (member_specifier_list)
        if (int candidate = member_specifier_list->lastToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token + 1;
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (base_clause_list)
        if (int candidate = base_clause_list->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    if (final_token)
        return final_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (classkey_token)
        return classkey_token + 1;
    return 1;
}

/** \generated */
int CompoundExpressionAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int CompoundExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int CompoundLiteralAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (initializer)
        if (int candidate = initializer->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int CompoundLiteralAST::lastToken() const
{
    if (initializer)
        if (int candidate = initializer->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int CompoundStatementAST::firstToken() const
{
    if (lbrace_token)
        return lbrace_token;
    if (statement_list)
        if (int candidate = statement_list->firstToken())
            return candidate;
    if (rbrace_token)
        return rbrace_token;
    return 0;
}

/** \generated */
int CompoundStatementAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;
    if (statement_list)
        if (int candidate = statement_list->lastToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token + 1;
    return 1;
}

/** \generated */
int ConditionAST::firstToken() const
{
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ConditionAST::lastToken() const
{
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ConditionalExpressionAST::firstToken() const
{
    if (condition)
        if (int candidate = condition->firstToken())
            return candidate;
    if (question_token)
        return question_token;
    if (left_expression)
        if (int candidate = left_expression->firstToken())
            return candidate;
    if (colon_token)
        return colon_token;
    if (right_expression)
        if (int candidate = right_expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ConditionalExpressionAST::lastToken() const
{
    if (right_expression)
        if (int candidate = right_expression->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    if (left_expression)
        if (int candidate = left_expression->lastToken())
            return candidate;
    if (question_token)
        return question_token + 1;
    if (condition)
        if (int candidate = condition->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ContinueStatementAST::firstToken() const
{
    if (continue_token)
        return continue_token;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ContinueStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (continue_token)
        return continue_token + 1;
    return 1;
}

/** \generated */
int ConversionFunctionIdAST::firstToken() const
{
    if (operator_token)
        return operator_token;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (ptr_operator_list)
        if (int candidate = ptr_operator_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ConversionFunctionIdAST::lastToken() const
{
    if (ptr_operator_list)
        if (int candidate = ptr_operator_list->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    if (operator_token)
        return operator_token + 1;
    return 1;
}

/** \generated */
int CppCastExpressionAST::firstToken() const
{
    if (cast_token)
        return cast_token;
    if (less_token)
        return less_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    if (greater_token)
        return greater_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int CppCastExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (greater_token)
        return greater_token + 1;
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (less_token)
        return less_token + 1;
    if (cast_token)
        return cast_token + 1;
    return 1;
}

/** \generated */
int CtorInitializerAST::firstToken() const
{
    if (colon_token)
        return colon_token;
    if (member_initializer_list)
        if (int candidate = member_initializer_list->firstToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    return 0;
}

/** \generated */
int CtorInitializerAST::lastToken() const
{
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (member_initializer_list)
        if (int candidate = member_initializer_list->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    return 1;
}

/** \generated */
int DeclarationStatementAST::firstToken() const
{
    if (declaration)
        if (int candidate = declaration->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int DeclarationStatementAST::lastToken() const
{
    if (declaration)
        if (int candidate = declaration->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int DeclaratorAST::firstToken() const
{
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (ptr_operator_list)
        if (int candidate = ptr_operator_list->firstToken())
            return candidate;
    if (core_declarator)
        if (int candidate = core_declarator->firstToken())
            return candidate;
    if (postfix_declarator_list)
        if (int candidate = postfix_declarator_list->firstToken())
            return candidate;
    if (post_attribute_list)
        if (int candidate = post_attribute_list->firstToken())
            return candidate;
    if (equal_token)
        return equal_token;
    if (initializer)
        if (int candidate = initializer->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int DeclaratorAST::lastToken() const
{
    if (initializer)
        if (int candidate = initializer->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (post_attribute_list)
        if (int candidate = post_attribute_list->lastToken())
            return candidate;
    if (postfix_declarator_list)
        if (int candidate = postfix_declarator_list->lastToken())
            return candidate;
    if (core_declarator)
        if (int candidate = core_declarator->lastToken())
            return candidate;
    if (ptr_operator_list)
        if (int candidate = ptr_operator_list->lastToken())
            return candidate;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int DeclaratorIdAST::firstToken() const
{
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int DeclaratorIdAST::lastToken() const
{
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    return 1;
}

/** \generated */
int DeleteExpressionAST::firstToken() const
{
    if (scope_token)
        return scope_token;
    if (delete_token)
        return delete_token;
    if (lbracket_token)
        return lbracket_token;
    if (rbracket_token)
        return rbracket_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int DeleteExpressionAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (rbracket_token)
        return rbracket_token + 1;
    if (lbracket_token)
        return lbracket_token + 1;
    if (delete_token)
        return delete_token + 1;
    if (scope_token)
        return scope_token + 1;
    return 1;
}

/** \generated */
int DestructorNameAST::firstToken() const
{
    if (tilde_token)
        return tilde_token;
    if (unqualified_name)
        if (int candidate = unqualified_name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int DestructorNameAST::lastToken() const
{
    if (unqualified_name)
        if (int candidate = unqualified_name->lastToken())
            return candidate;
    if (tilde_token)
        return tilde_token + 1;
    return 1;
}

/** \generated */
int DoStatementAST::firstToken() const
{
    if (do_token)
        return do_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    if (while_token)
        return while_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int DoStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (while_token)
        return while_token + 1;
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (do_token)
        return do_token + 1;
    return 1;
}

/** \generated */
int ElaboratedTypeSpecifierAST::firstToken() const
{
    if (classkey_token)
        return classkey_token;
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ElaboratedTypeSpecifierAST::lastToken() const
{
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (classkey_token)
        return classkey_token + 1;
    return 1;
}

/** \generated */
int EmptyDeclarationAST::firstToken() const
{
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int EmptyDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    return 1;
}

/** \generated */
int EnumSpecifierAST::firstToken() const
{
    if (enum_token)
        return enum_token;
    if (key_token)
        return key_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (colon_token)
        return colon_token;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token;
    if (enumerator_list)
        if (int candidate = enumerator_list->firstToken())
            return candidate;
    if (stray_comma_token)
        return stray_comma_token;
    if (rbrace_token)
        return rbrace_token;
    return 0;
}

/** \generated */
int EnumSpecifierAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;
    if (stray_comma_token)
        return stray_comma_token + 1;
    if (enumerator_list)
        if (int candidate = enumerator_list->lastToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token + 1;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (key_token)
        return key_token + 1;
    if (enum_token)
        return enum_token + 1;
    return 1;
}

/** \generated */
int EnumeratorAST::firstToken() const
{
    if (identifier_token)
        return identifier_token;
    if (equal_token)
        return equal_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int EnumeratorAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (identifier_token)
        return identifier_token + 1;
    return 1;
}

/** \generated */
int ExceptionDeclarationAST::firstToken() const
{
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    return 0;
}

/** \generated */
int ExceptionDeclarationAST::lastToken() const
{
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int DynamicExceptionSpecificationAST::firstToken() const
{
    if (throw_token)
        return throw_token;
    if (lparen_token)
        return lparen_token;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    if (type_id_list)
        if (int candidate = type_id_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int DynamicExceptionSpecificationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (type_id_list)
        if (int candidate = type_id_list->lastToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (lparen_token)
        return lparen_token + 1;
    if (throw_token)
        return throw_token + 1;
    return 1;
}

/** \generated */
int ExpressionOrDeclarationStatementAST::firstToken() const
{
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (declaration)
        if (int candidate = declaration->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ExpressionOrDeclarationStatementAST::lastToken() const
{
    if (declaration)
        if (int candidate = declaration->lastToken())
            return candidate;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ExpressionStatementAST::firstToken() const
{
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ExpressionStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ForStatementAST::firstToken() const
{
    if (for_token)
        return for_token;
    if (lparen_token)
        return lparen_token;
    if (initializer)
        if (int candidate = initializer->firstToken())
            return candidate;
    if (condition)
        if (int candidate = condition->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ForStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token + 1;
    if (condition)
        if (int candidate = condition->lastToken())
            return candidate;
    if (initializer)
        if (int candidate = initializer->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (for_token)
        return for_token + 1;
    return 1;
}

/** \generated */
int ForeachStatementAST::firstToken() const
{
    if (foreach_token)
        return foreach_token;
    if (lparen_token)
        return lparen_token;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (initializer)
        if (int candidate = initializer->firstToken())
            return candidate;
    if (comma_token)
        return comma_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ForeachStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (comma_token)
        return comma_token + 1;
    if (initializer)
        if (int candidate = initializer->lastToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (foreach_token)
        return foreach_token + 1;
    return 1;
}

/** \generated */
int FunctionDeclaratorAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (parameter_declaration_clause)
        if (int candidate = parameter_declaration_clause->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (cv_qualifier_list)
        if (int candidate = cv_qualifier_list->firstToken())
            return candidate;
    if (ref_qualifier_token)
        return ref_qualifier_token;
    if (exception_specification)
        if (int candidate = exception_specification->firstToken())
            return candidate;
    if (trailing_return_type)
        if (int candidate = trailing_return_type->firstToken())
            return candidate;
    if (as_cpp_initializer)
        if (int candidate = as_cpp_initializer->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int FunctionDeclaratorAST::lastToken() const
{
    if (as_cpp_initializer)
        if (int candidate = as_cpp_initializer->lastToken())
            return candidate;
    if (trailing_return_type)
        if (int candidate = trailing_return_type->lastToken())
            return candidate;
    if (exception_specification)
        if (int candidate = exception_specification->lastToken())
            return candidate;
    if (ref_qualifier_token)
        return ref_qualifier_token + 1;
    if (cv_qualifier_list)
        if (int candidate = cv_qualifier_list->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (parameter_declaration_clause)
        if (int candidate = parameter_declaration_clause->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int FunctionDefinitionAST::firstToken() const
{
    if (qt_invokable_token)
        return qt_invokable_token;
    if (decl_specifier_list)
        if (int candidate = decl_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (ctor_initializer)
        if (int candidate = ctor_initializer->firstToken())
            return candidate;
    if (function_body)
        if (int candidate = function_body->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int FunctionDefinitionAST::lastToken() const
{
    if (function_body)
        if (int candidate = function_body->lastToken())
            return candidate;
    if (ctor_initializer)
        if (int candidate = ctor_initializer->lastToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (decl_specifier_list)
        if (int candidate = decl_specifier_list->lastToken())
            return candidate;
    if (qt_invokable_token)
        return qt_invokable_token + 1;
    return 1;
}

/** \generated */
int GotoStatementAST::firstToken() const
{
    if (goto_token)
        return goto_token;
    if (identifier_token)
        return identifier_token;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int GotoStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (identifier_token)
        return identifier_token + 1;
    if (goto_token)
        return goto_token + 1;
    return 1;
}

/** \generated */
int IdExpressionAST::firstToken() const
{
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int IdExpressionAST::lastToken() const
{
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int IfStatementAST::firstToken() const
{
    if (if_token)
        return if_token;
    if (constexpr_token)
        return constexpr_token;
    if (lparen_token)
        return lparen_token;
    if (condition)
        if (int candidate = condition->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    if (else_token)
        return else_token;
    if (else_statement)
        if (int candidate = else_statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int IfStatementAST::lastToken() const
{
    if (else_statement)
        if (int candidate = else_statement->lastToken())
            return candidate;
    if (else_token)
        return else_token + 1;
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (condition)
        if (int candidate = condition->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (constexpr_token)
        return constexpr_token + 1;
    if (if_token)
        return if_token + 1;
    return 1;
}

/** \generated */
int LabeledStatementAST::firstToken() const
{
    if (label_token)
        return label_token;
    if (colon_token)
        return colon_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int LabeledStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    if (label_token)
        return label_token + 1;
    return 1;
}

/** \generated */
int LambdaCaptureAST::firstToken() const
{
    if (default_capture_token)
        return default_capture_token;
    if (capture_list)
        if (int candidate = capture_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int LambdaCaptureAST::lastToken() const
{
    if (capture_list)
        if (int candidate = capture_list->lastToken())
            return candidate;
    if (default_capture_token)
        return default_capture_token + 1;
    return 1;
}

/** \generated */
int LambdaDeclaratorAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (parameter_declaration_clause)
        if (int candidate = parameter_declaration_clause->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (attributes)
        if (int candidate = attributes->firstToken())
            return candidate;
    if (mutable_token)
        return mutable_token;
    if (exception_specification)
        if (int candidate = exception_specification->firstToken())
            return candidate;
    if (trailing_return_type)
        if (int candidate = trailing_return_type->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int LambdaDeclaratorAST::lastToken() const
{
    if (trailing_return_type)
        if (int candidate = trailing_return_type->lastToken())
            return candidate;
    if (exception_specification)
        if (int candidate = exception_specification->lastToken())
            return candidate;
    if (mutable_token)
        return mutable_token + 1;
    if (attributes)
        if (int candidate = attributes->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (parameter_declaration_clause)
        if (int candidate = parameter_declaration_clause->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int LambdaExpressionAST::firstToken() const
{
    if (lambda_introducer)
        if (int candidate = lambda_introducer->firstToken())
            return candidate;
    if (lambda_declarator)
        if (int candidate = lambda_declarator->firstToken())
            return candidate;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int LambdaExpressionAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (lambda_declarator)
        if (int candidate = lambda_declarator->lastToken())
            return candidate;
    if (lambda_introducer)
        if (int candidate = lambda_introducer->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int LambdaIntroducerAST::firstToken() const
{
    if (lbracket_token)
        return lbracket_token;
    if (lambda_capture)
        if (int candidate = lambda_capture->firstToken())
            return candidate;
    if (rbracket_token)
        return rbracket_token;
    return 0;
}

/** \generated */
int LambdaIntroducerAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    if (lambda_capture)
        if (int candidate = lambda_capture->lastToken())
            return candidate;
    if (lbracket_token)
        return lbracket_token + 1;
    return 1;
}

/** \generated */
int LinkageBodyAST::firstToken() const
{
    if (lbrace_token)
        return lbrace_token;
    if (declaration_list)
        if (int candidate = declaration_list->firstToken())
            return candidate;
    if (rbrace_token)
        return rbrace_token;
    return 0;
}

/** \generated */
int LinkageBodyAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;
    if (declaration_list)
        if (int candidate = declaration_list->lastToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token + 1;
    return 1;
}

/** \generated */
int LinkageSpecificationAST::firstToken() const
{
    if (extern_token)
        return extern_token;
    if (extern_type_token)
        return extern_type_token;
    if (declaration)
        if (int candidate = declaration->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int LinkageSpecificationAST::lastToken() const
{
    if (declaration)
        if (int candidate = declaration->lastToken())
            return candidate;
    if (extern_type_token)
        return extern_type_token + 1;
    if (extern_token)
        return extern_token + 1;
    return 1;
}

/** \generated */
int MemInitializerAST::firstToken() const
{
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int MemInitializerAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int MemberAccessAST::firstToken() const
{
    if (base_expression)
        if (int candidate = base_expression->firstToken())
            return candidate;
    if (access_token)
        return access_token;
    if (template_token)
        return template_token;
    if (member_name)
        if (int candidate = member_name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int MemberAccessAST::lastToken() const
{
    if (member_name)
        if (int candidate = member_name->lastToken())
            return candidate;
    if (template_token)
        return template_token + 1;
    if (access_token)
        return access_token + 1;
    if (base_expression)
        if (int candidate = base_expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int NamedTypeSpecifierAST::firstToken() const
{
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int NamedTypeSpecifierAST::lastToken() const
{
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int NamespaceAST::firstToken() const
{
    if (inline_token)
        return inline_token;
    if (namespace_token)
        return namespace_token;
    if (identifier_token)
        return identifier_token;
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (linkage_body)
        if (int candidate = linkage_body->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int NamespaceAST::lastToken() const
{
    if (linkage_body)
        if (int candidate = linkage_body->lastToken())
            return candidate;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (identifier_token)
        return identifier_token + 1;
    if (namespace_token)
        return namespace_token + 1;
    if (inline_token)
        return inline_token + 1;
    return 1;
}

/** \generated */
int NamespaceAliasDefinitionAST::firstToken() const
{
    if (namespace_token)
        return namespace_token;
    if (namespace_name_token)
        return namespace_name_token;
    if (equal_token)
        return equal_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int NamespaceAliasDefinitionAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (namespace_name_token)
        return namespace_name_token + 1;
    if (namespace_token)
        return namespace_token + 1;
    return 1;
}

/** \generated */
int NestedDeclaratorAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int NestedDeclaratorAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int NestedExpressionAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int NestedExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int NestedNameSpecifierAST::firstToken() const
{
    if (class_or_namespace_name)
        if (int candidate = class_or_namespace_name->firstToken())
            return candidate;
    if (scope_token)
        return scope_token;
    return 0;
}

/** \generated */
int NestedNameSpecifierAST::lastToken() const
{
    if (scope_token)
        return scope_token + 1;
    if (class_or_namespace_name)
        if (int candidate = class_or_namespace_name->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int NewArrayDeclaratorAST::firstToken() const
{
    if (lbracket_token)
        return lbracket_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rbracket_token)
        return rbracket_token;
    return 0;
}

/** \generated */
int NewArrayDeclaratorAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lbracket_token)
        return lbracket_token + 1;
    return 1;
}

/** \generated */
int NewExpressionAST::firstToken() const
{
    if (scope_token)
        return scope_token;
    if (new_token)
        return new_token;
    if (new_placement)
        if (int candidate = new_placement->firstToken())
            return candidate;
    if (lparen_token)
        return lparen_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (new_type_id)
        if (int candidate = new_type_id->firstToken())
            return candidate;
    if (new_initializer)
        if (int candidate = new_initializer->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int NewExpressionAST::lastToken() const
{
    if (new_initializer)
        if (int candidate = new_initializer->lastToken())
            return candidate;
    if (new_type_id)
        if (int candidate = new_type_id->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (new_placement)
        if (int candidate = new_placement->lastToken())
            return candidate;
    if (new_token)
        return new_token + 1;
    if (scope_token)
        return scope_token + 1;
    return 1;
}

/** \generated */
int ExpressionListParenAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (expression_list)
        if (int candidate = expression_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int ExpressionListParenAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression_list)
        if (int candidate = expression_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int NewTypeIdAST::firstToken() const
{
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (ptr_operator_list)
        if (int candidate = ptr_operator_list->firstToken())
            return candidate;
    if (new_array_declarator_list)
        if (int candidate = new_array_declarator_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int NewTypeIdAST::lastToken() const
{
    if (new_array_declarator_list)
        if (int candidate = new_array_declarator_list->lastToken())
            return candidate;
    if (ptr_operator_list)
        if (int candidate = ptr_operator_list->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int NumericLiteralAST::firstToken() const
{
    if (literal_token)
        return literal_token;
    return 0;
}

/** \generated */
int NumericLiteralAST::lastToken() const
{
    if (literal_token)
        return literal_token + 1;
    return 1;
}

/** \generated */
int ObjCClassDeclarationAST::firstToken() const
{
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (interface_token)
        return interface_token;
    if (implementation_token)
        return implementation_token;
    if (class_name)
        if (int candidate = class_name->firstToken())
            return candidate;
    if (lparen_token)
        return lparen_token;
    if (category_name)
        if (int candidate = category_name->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (colon_token)
        return colon_token;
    if (superclass)
        if (int candidate = superclass->firstToken())
            return candidate;
    if (protocol_refs)
        if (int candidate = protocol_refs->firstToken())
            return candidate;
    if (inst_vars_decl)
        if (int candidate = inst_vars_decl->firstToken())
            return candidate;
    if (member_declaration_list)
        if (int candidate = member_declaration_list->firstToken())
            return candidate;
    if (end_token)
        return end_token;
    return 0;
}

/** \generated */
int ObjCClassDeclarationAST::lastToken() const
{
    if (end_token)
        return end_token + 1;
    if (member_declaration_list)
        if (int candidate = member_declaration_list->lastToken())
            return candidate;
    if (inst_vars_decl)
        if (int candidate = inst_vars_decl->lastToken())
            return candidate;
    if (protocol_refs)
        if (int candidate = protocol_refs->lastToken())
            return candidate;
    if (superclass)
        if (int candidate = superclass->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    if (rparen_token)
        return rparen_token + 1;
    if (category_name)
        if (int candidate = category_name->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (class_name)
        if (int candidate = class_name->lastToken())
            return candidate;
    if (implementation_token)
        return implementation_token + 1;
    if (interface_token)
        return interface_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCClassForwardDeclarationAST::firstToken() const
{
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (class_token)
        return class_token;
    if (identifier_list)
        if (int candidate = identifier_list->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ObjCClassForwardDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (identifier_list)
        if (int candidate = identifier_list->lastToken())
            return candidate;
    if (class_token)
        return class_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCDynamicPropertiesDeclarationAST::firstToken() const
{
    if (dynamic_token)
        return dynamic_token;
    if (property_identifier_list)
        if (int candidate = property_identifier_list->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ObjCDynamicPropertiesDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (property_identifier_list)
        if (int candidate = property_identifier_list->lastToken())
            return candidate;
    if (dynamic_token)
        return dynamic_token + 1;
    return 1;
}

/** \generated */
int ObjCEncodeExpressionAST::firstToken() const
{
    if (encode_token)
        return encode_token;
    if (type_name)
        if (int candidate = type_name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCEncodeExpressionAST::lastToken() const
{
    if (type_name)
        if (int candidate = type_name->lastToken())
            return candidate;
    if (encode_token)
        return encode_token + 1;
    return 1;
}

/** \generated */
int ObjCFastEnumerationAST::firstToken() const
{
    if (for_token)
        return for_token;
    if (lparen_token)
        return lparen_token;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (initializer)
        if (int candidate = initializer->firstToken())
            return candidate;
    if (in_token)
        return in_token;
    if (fast_enumeratable_expression)
        if (int candidate = fast_enumeratable_expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCFastEnumerationAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (fast_enumeratable_expression)
        if (int candidate = fast_enumeratable_expression->lastToken())
            return candidate;
    if (in_token)
        return in_token + 1;
    if (initializer)
        if (int candidate = initializer->lastToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (for_token)
        return for_token + 1;
    return 1;
}

/** \generated */
int ObjCInstanceVariablesDeclarationAST::firstToken() const
{
    if (lbrace_token)
        return lbrace_token;
    if (instance_variable_list)
        if (int candidate = instance_variable_list->firstToken())
            return candidate;
    if (rbrace_token)
        return rbrace_token;
    return 0;
}

/** \generated */
int ObjCInstanceVariablesDeclarationAST::lastToken() const
{
    if (rbrace_token)
        return rbrace_token + 1;
    if (instance_variable_list)
        if (int candidate = instance_variable_list->lastToken())
            return candidate;
    if (lbrace_token)
        return lbrace_token + 1;
    return 1;
}

/** \generated */
int ObjCMessageArgumentAST::firstToken() const
{
    if (parameter_value_expression)
        if (int candidate = parameter_value_expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCMessageArgumentAST::lastToken() const
{
    if (parameter_value_expression)
        if (int candidate = parameter_value_expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCMessageArgumentDeclarationAST::firstToken() const
{
    if (type_name)
        if (int candidate = type_name->firstToken())
            return candidate;
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (param_name)
        if (int candidate = param_name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCMessageArgumentDeclarationAST::lastToken() const
{
    if (param_name)
        if (int candidate = param_name->lastToken())
            return candidate;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (type_name)
        if (int candidate = type_name->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCMessageExpressionAST::firstToken() const
{
    if (lbracket_token)
        return lbracket_token;
    if (receiver_expression)
        if (int candidate = receiver_expression->firstToken())
            return candidate;
    if (selector)
        if (int candidate = selector->firstToken())
            return candidate;
    if (argument_list)
        if (int candidate = argument_list->firstToken())
            return candidate;
    if (rbracket_token)
        return rbracket_token;
    return 0;
}

/** \generated */
int ObjCMessageExpressionAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    if (argument_list)
        if (int candidate = argument_list->lastToken())
            return candidate;
    if (selector)
        if (int candidate = selector->lastToken())
            return candidate;
    if (receiver_expression)
        if (int candidate = receiver_expression->lastToken())
            return candidate;
    if (lbracket_token)
        return lbracket_token + 1;
    return 1;
}

/** \generated */
int ObjCMethodDeclarationAST::firstToken() const
{
    if (method_prototype)
        if (int candidate = method_prototype->firstToken())
            return candidate;
    if (function_body)
        if (int candidate = function_body->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ObjCMethodDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (function_body)
        if (int candidate = function_body->lastToken())
            return candidate;
    if (method_prototype)
        if (int candidate = method_prototype->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCMethodPrototypeAST::firstToken() const
{
    if (method_type_token)
        return method_type_token;
    if (type_name)
        if (int candidate = type_name->firstToken())
            return candidate;
    if (selector)
        if (int candidate = selector->firstToken())
            return candidate;
    if (argument_list)
        if (int candidate = argument_list->firstToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCMethodPrototypeAST::lastToken() const
{
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (argument_list)
        if (int candidate = argument_list->lastToken())
            return candidate;
    if (selector)
        if (int candidate = selector->lastToken())
            return candidate;
    if (type_name)
        if (int candidate = type_name->lastToken())
            return candidate;
    if (method_type_token)
        return method_type_token + 1;
    return 1;
}

/** \generated */
int ObjCPropertyAttributeAST::firstToken() const
{
    if (attribute_identifier_token)
        return attribute_identifier_token;
    if (equals_token)
        return equals_token;
    if (method_selector)
        if (int candidate = method_selector->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCPropertyAttributeAST::lastToken() const
{
    if (method_selector)
        if (int candidate = method_selector->lastToken())
            return candidate;
    if (equals_token)
        return equals_token + 1;
    if (attribute_identifier_token)
        return attribute_identifier_token + 1;
    return 1;
}

/** \generated */
int ObjCPropertyDeclarationAST::firstToken() const
{
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (property_token)
        return property_token;
    if (lparen_token)
        return lparen_token;
    if (property_attribute_list)
        if (int candidate = property_attribute_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (simple_declaration)
        if (int candidate = simple_declaration->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCPropertyDeclarationAST::lastToken() const
{
    if (simple_declaration)
        if (int candidate = simple_declaration->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (property_attribute_list)
        if (int candidate = property_attribute_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (property_token)
        return property_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCProtocolDeclarationAST::firstToken() const
{
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (protocol_token)
        return protocol_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (protocol_refs)
        if (int candidate = protocol_refs->firstToken())
            return candidate;
    if (member_declaration_list)
        if (int candidate = member_declaration_list->firstToken())
            return candidate;
    if (end_token)
        return end_token;
    return 0;
}

/** \generated */
int ObjCProtocolDeclarationAST::lastToken() const
{
    if (end_token)
        return end_token + 1;
    if (member_declaration_list)
        if (int candidate = member_declaration_list->lastToken())
            return candidate;
    if (protocol_refs)
        if (int candidate = protocol_refs->lastToken())
            return candidate;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (protocol_token)
        return protocol_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCProtocolExpressionAST::firstToken() const
{
    if (protocol_token)
        return protocol_token;
    if (lparen_token)
        return lparen_token;
    if (identifier_token)
        return identifier_token;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int ObjCProtocolExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (identifier_token)
        return identifier_token + 1;
    if (lparen_token)
        return lparen_token + 1;
    if (protocol_token)
        return protocol_token + 1;
    return 1;
}

/** \generated */
int ObjCProtocolForwardDeclarationAST::firstToken() const
{
    if (attribute_list)
        if (int candidate = attribute_list->firstToken())
            return candidate;
    if (protocol_token)
        return protocol_token;
    if (identifier_list)
        if (int candidate = identifier_list->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ObjCProtocolForwardDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (identifier_list)
        if (int candidate = identifier_list->lastToken())
            return candidate;
    if (protocol_token)
        return protocol_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCProtocolRefsAST::firstToken() const
{
    if (less_token)
        return less_token;
    if (identifier_list)
        if (int candidate = identifier_list->firstToken())
            return candidate;
    if (greater_token)
        return greater_token;
    return 0;
}

/** \generated */
int ObjCProtocolRefsAST::lastToken() const
{
    if (greater_token)
        return greater_token + 1;
    if (identifier_list)
        if (int candidate = identifier_list->lastToken())
            return candidate;
    if (less_token)
        return less_token + 1;
    return 1;
}

/** \generated */
int ObjCSelectorAST::firstToken() const
{
    if (selector_argument_list)
        if (int candidate = selector_argument_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCSelectorAST::lastToken() const
{
    if (selector_argument_list)
        if (int candidate = selector_argument_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ObjCSelectorArgumentAST::firstToken() const
{
    if (name_token)
        return name_token;
    if (colon_token)
        return colon_token;
    return 0;
}

/** \generated */
int ObjCSelectorArgumentAST::lastToken() const
{
    if (colon_token)
        return colon_token + 1;
    if (name_token)
        return name_token + 1;
    return 1;
}

/** \generated */
int ObjCSelectorExpressionAST::firstToken() const
{
    if (selector_token)
        return selector_token;
    if (lparen_token)
        return lparen_token;
    if (selector)
        if (int candidate = selector->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int ObjCSelectorExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (selector)
        if (int candidate = selector->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (selector_token)
        return selector_token + 1;
    return 1;
}

/** \generated */
int ObjCSynchronizedStatementAST::firstToken() const
{
    if (synchronized_token)
        return synchronized_token;
    if (lparen_token)
        return lparen_token;
    if (synchronized_object)
        if (int candidate = synchronized_object->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ObjCSynchronizedStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (synchronized_object)
        if (int candidate = synchronized_object->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (synchronized_token)
        return synchronized_token + 1;
    return 1;
}

/** \generated */
int ObjCSynthesizedPropertiesDeclarationAST::firstToken() const
{
    if (synthesized_token)
        return synthesized_token;
    if (property_identifier_list)
        if (int candidate = property_identifier_list->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ObjCSynthesizedPropertiesDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (property_identifier_list)
        if (int candidate = property_identifier_list->lastToken())
            return candidate;
    if (synthesized_token)
        return synthesized_token + 1;
    return 1;
}

/** \generated */
int ObjCSynthesizedPropertyAST::firstToken() const
{
    if (property_identifier_token)
        return property_identifier_token;
    if (equals_token)
        return equals_token;
    if (alias_identifier_token)
        return alias_identifier_token;
    return 0;
}

/** \generated */
int ObjCSynthesizedPropertyAST::lastToken() const
{
    if (alias_identifier_token)
        return alias_identifier_token + 1;
    if (equals_token)
        return equals_token + 1;
    if (property_identifier_token)
        return property_identifier_token + 1;
    return 1;
}

/** \generated */
int ObjCTypeNameAST::firstToken() const
{
    if (lparen_token)
        return lparen_token;
    if (type_qualifier_token)
        return type_qualifier_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int ObjCTypeNameAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (type_qualifier_token)
        return type_qualifier_token + 1;
    if (lparen_token)
        return lparen_token + 1;
    return 1;
}

/** \generated */
int ObjCVisibilityDeclarationAST::firstToken() const
{
    if (visibility_token)
        return visibility_token;
    return 0;
}

/** \generated */
int ObjCVisibilityDeclarationAST::lastToken() const
{
    if (visibility_token)
        return visibility_token + 1;
    return 1;
}

/** \generated */
int OperatorAST::firstToken() const
{
    if (op_token)
        return op_token;
    if (open_token)
        return open_token;
    if (close_token)
        return close_token;
    return 0;
}

/** \generated */
int OperatorAST::lastToken() const
{
    if (close_token)
        return close_token + 1;
    if (open_token)
        return open_token + 1;
    if (op_token)
        return op_token + 1;
    return 1;
}

/** \generated */
int OperatorFunctionIdAST::firstToken() const
{
    if (operator_token)
        return operator_token;
    if (op)
        if (int candidate = op->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int OperatorFunctionIdAST::lastToken() const
{
    if (op)
        if (int candidate = op->lastToken())
            return candidate;
    if (operator_token)
        return operator_token + 1;
    return 1;
}

/** \generated */
int ParameterDeclarationAST::firstToken() const
{
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (equal_token)
        return equal_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ParameterDeclarationAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int ParameterDeclarationClauseAST::firstToken() const
{
    if (parameter_declaration_list)
        if (int candidate = parameter_declaration_list->firstToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    return 0;
}

/** \generated */
int ParameterDeclarationClauseAST::lastToken() const
{
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (parameter_declaration_list)
        if (int candidate = parameter_declaration_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int PointerAST::firstToken() const
{
    if (star_token)
        return star_token;
    if (cv_qualifier_list)
        if (int candidate = cv_qualifier_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int PointerAST::lastToken() const
{
    if (cv_qualifier_list)
        if (int candidate = cv_qualifier_list->lastToken())
            return candidate;
    if (star_token)
        return star_token + 1;
    return 1;
}

/** \generated */
int PointerToMemberAST::firstToken() const
{
    if (global_scope_token)
        return global_scope_token;
    if (nested_name_specifier_list)
        if (int candidate = nested_name_specifier_list->firstToken())
            return candidate;
    if (star_token)
        return star_token;
    if (cv_qualifier_list)
        if (int candidate = cv_qualifier_list->firstToken())
            return candidate;
    if (ref_qualifier_token)
        return ref_qualifier_token;
    return 0;
}

/** \generated */
int PointerToMemberAST::lastToken() const
{
    if (ref_qualifier_token)
        return ref_qualifier_token + 1;
    if (cv_qualifier_list)
        if (int candidate = cv_qualifier_list->lastToken())
            return candidate;
    if (star_token)
        return star_token + 1;
    if (nested_name_specifier_list)
        if (int candidate = nested_name_specifier_list->lastToken())
            return candidate;
    if (global_scope_token)
        return global_scope_token + 1;
    return 1;
}

/** \generated */
int PostIncrDecrAST::firstToken() const
{
    if (base_expression)
        if (int candidate = base_expression->firstToken())
            return candidate;
    if (incr_decr_token)
        return incr_decr_token;
    return 0;
}

/** \generated */
int PostIncrDecrAST::lastToken() const
{
    if (incr_decr_token)
        return incr_decr_token + 1;
    if (base_expression)
        if (int candidate = base_expression->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int QtEnumDeclarationAST::firstToken() const
{
    if (enum_specifier_token)
        return enum_specifier_token;
    if (lparen_token)
        return lparen_token;
    if (enumerator_list)
        if (int candidate = enumerator_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int QtEnumDeclarationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (enumerator_list)
        if (int candidate = enumerator_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (enum_specifier_token)
        return enum_specifier_token + 1;
    return 1;
}

/** \generated */
int QtFlagsDeclarationAST::firstToken() const
{
    if (flags_specifier_token)
        return flags_specifier_token;
    if (lparen_token)
        return lparen_token;
    if (flag_enums_list)
        if (int candidate = flag_enums_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int QtFlagsDeclarationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (flag_enums_list)
        if (int candidate = flag_enums_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (flags_specifier_token)
        return flags_specifier_token + 1;
    return 1;
}

/** \generated */
int QtInterfaceNameAST::firstToken() const
{
    if (interface_name)
        if (int candidate = interface_name->firstToken())
            return candidate;
    if (constraint_list)
        if (int candidate = constraint_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int QtInterfaceNameAST::lastToken() const
{
    if (constraint_list)
        if (int candidate = constraint_list->lastToken())
            return candidate;
    if (interface_name)
        if (int candidate = interface_name->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int QtInterfacesDeclarationAST::firstToken() const
{
    if (interfaces_token)
        return interfaces_token;
    if (lparen_token)
        return lparen_token;
    if (interface_name_list)
        if (int candidate = interface_name_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int QtInterfacesDeclarationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (interface_name_list)
        if (int candidate = interface_name_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (interfaces_token)
        return interfaces_token + 1;
    return 1;
}

/** \generated */
int QtMemberDeclarationAST::firstToken() const
{
    if (q_token)
        return q_token;
    if (lparen_token)
        return lparen_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int QtMemberDeclarationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (q_token)
        return q_token + 1;
    return 1;
}

/** \generated */
int QtMethodAST::firstToken() const
{
    if (method_token)
        return method_token;
    if (lparen_token)
        return lparen_token;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int QtMethodAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (method_token)
        return method_token + 1;
    return 1;
}

/** \generated */
int QtObjectTagAST::firstToken() const
{
    if (q_object_token)
        return q_object_token;
    return 0;
}

/** \generated */
int QtObjectTagAST::lastToken() const
{
    if (q_object_token)
        return q_object_token + 1;
    return 1;
}

/** \generated */
int QtPrivateSlotAST::firstToken() const
{
    if (q_private_slot_token)
        return q_private_slot_token;
    if (lparen_token)
        return lparen_token;
    if (dptr_token)
        return dptr_token;
    if (dptr_lparen_token)
        return dptr_lparen_token;
    if (dptr_rparen_token)
        return dptr_rparen_token;
    if (comma_token)
        return comma_token;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int QtPrivateSlotAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    if (comma_token)
        return comma_token + 1;
    if (dptr_rparen_token)
        return dptr_rparen_token + 1;
    if (dptr_lparen_token)
        return dptr_lparen_token + 1;
    if (dptr_token)
        return dptr_token + 1;
    if (lparen_token)
        return lparen_token + 1;
    if (q_private_slot_token)
        return q_private_slot_token + 1;
    return 1;
}

/** \generated */
int QtPropertyDeclarationAST::firstToken() const
{
    if (property_specifier_token)
        return property_specifier_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (comma_token)
        return comma_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    if (property_name)
        if (int candidate = property_name->firstToken())
            return candidate;
    if (property_declaration_item_list)
        if (int candidate = property_declaration_item_list->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int QtPropertyDeclarationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (property_declaration_item_list)
        if (int candidate = property_declaration_item_list->lastToken())
            return candidate;
    if (property_name)
        if (int candidate = property_name->lastToken())
            return candidate;
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (comma_token)
        return comma_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (property_specifier_token)
        return property_specifier_token + 1;
    return 1;
}

/** \generated */
int QtPropertyDeclarationItemAST::firstToken() const
{
    if (item_name_token)
        return item_name_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int QtPropertyDeclarationItemAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (item_name_token)
        return item_name_token + 1;
    return 1;
}

/** \generated */
int QualifiedNameAST::firstToken() const
{
    if (global_scope_token)
        return global_scope_token;
    if (nested_name_specifier_list)
        if (int candidate = nested_name_specifier_list->firstToken())
            return candidate;
    if (unqualified_name)
        if (int candidate = unqualified_name->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int QualifiedNameAST::lastToken() const
{
    if (unqualified_name)
        if (int candidate = unqualified_name->lastToken())
            return candidate;
    if (nested_name_specifier_list)
        if (int candidate = nested_name_specifier_list->lastToken())
            return candidate;
    if (global_scope_token)
        return global_scope_token + 1;
    return 1;
}

/** \generated */
int ReferenceAST::firstToken() const
{
    if (reference_token)
        return reference_token;
    return 0;
}

/** \generated */
int ReferenceAST::lastToken() const
{
    if (reference_token)
        return reference_token + 1;
    return 1;
}

/** \generated */
int ReturnStatementAST::firstToken() const
{
    if (return_token)
        return return_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int ReturnStatementAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (return_token)
        return return_token + 1;
    return 1;
}

/** \generated */
int SimpleDeclarationAST::firstToken() const
{
    if (qt_invokable_token)
        return qt_invokable_token;
    if (decl_specifier_list)
        if (int candidate = decl_specifier_list->firstToken())
            return candidate;
    if (declarator_list)
        if (int candidate = declarator_list->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int SimpleDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (declarator_list)
        if (int candidate = declarator_list->lastToken())
            return candidate;
    if (decl_specifier_list)
        if (int candidate = decl_specifier_list->lastToken())
            return candidate;
    if (qt_invokable_token)
        return qt_invokable_token + 1;
    return 1;
}

/** \generated */
int SimpleNameAST::firstToken() const
{
    if (identifier_token)
        return identifier_token;
    return 0;
}

/** \generated */
int SimpleNameAST::lastToken() const
{
    if (identifier_token)
        return identifier_token + 1;
    return 1;
}

/** \generated */
int SimpleSpecifierAST::firstToken() const
{
    if (specifier_token)
        return specifier_token;
    return 0;
}

/** \generated */
int SimpleSpecifierAST::lastToken() const
{
    if (specifier_token)
        return specifier_token + 1;
    return 1;
}

/** \generated */
int SizeofExpressionAST::firstToken() const
{
    if (sizeof_token)
        return sizeof_token;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int SizeofExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (sizeof_token)
        return sizeof_token + 1;
    return 1;
}

/** \generated */
int StringLiteralAST::firstToken() const
{
    if (literal_token)
        return literal_token;
    if (next)
        if (int candidate = next->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int StringLiteralAST::lastToken() const
{
    if (next)
        if (int candidate = next->lastToken())
            return candidate;
    if (literal_token)
        return literal_token + 1;
    return 1;
}

/** \generated */
int SwitchStatementAST::firstToken() const
{
    if (switch_token)
        return switch_token;
    if (lparen_token)
        return lparen_token;
    if (condition)
        if (int candidate = condition->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int SwitchStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (condition)
        if (int candidate = condition->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (switch_token)
        return switch_token + 1;
    return 1;
}

/** \generated */
int TemplateDeclarationAST::firstToken() const
{
    if (export_token)
        return export_token;
    if (template_token)
        return template_token;
    if (less_token)
        return less_token;
    if (template_parameter_list)
        if (int candidate = template_parameter_list->firstToken())
            return candidate;
    if (greater_token)
        return greater_token;
    if (declaration)
        if (int candidate = declaration->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TemplateDeclarationAST::lastToken() const
{
    if (declaration)
        if (int candidate = declaration->lastToken())
            return candidate;
    if (greater_token)
        return greater_token + 1;
    if (template_parameter_list)
        if (int candidate = template_parameter_list->lastToken())
            return candidate;
    if (less_token)
        return less_token + 1;
    if (template_token)
        return template_token + 1;
    if (export_token)
        return export_token + 1;
    return 1;
}

/** \generated */
int TemplateIdAST::firstToken() const
{
    if (template_token)
        return template_token;
    if (identifier_token)
        return identifier_token;
    if (less_token)
        return less_token;
    if (template_argument_list)
        if (int candidate = template_argument_list->firstToken())
            return candidate;
    if (greater_token)
        return greater_token;
    return 0;
}

/** \generated */
int TemplateIdAST::lastToken() const
{
    if (greater_token)
        return greater_token + 1;
    if (template_argument_list)
        if (int candidate = template_argument_list->lastToken())
            return candidate;
    if (less_token)
        return less_token + 1;
    if (identifier_token)
        return identifier_token + 1;
    if (template_token)
        return template_token + 1;
    return 1;
}

/** \generated */
int TemplateTypeParameterAST::firstToken() const
{
    if (template_token)
        return template_token;
    if (less_token)
        return less_token;
    if (template_parameter_list)
        if (int candidate = template_parameter_list->firstToken())
            return candidate;
    if (greater_token)
        return greater_token;
    if (class_token)
        return class_token;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (equal_token)
        return equal_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TemplateTypeParameterAST::lastToken() const
{
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (class_token)
        return class_token + 1;
    if (greater_token)
        return greater_token + 1;
    if (template_parameter_list)
        if (int candidate = template_parameter_list->lastToken())
            return candidate;
    if (less_token)
        return less_token + 1;
    if (template_token)
        return template_token + 1;
    return 1;
}

/** \generated */
int ThisExpressionAST::firstToken() const
{
    if (this_token)
        return this_token;
    return 0;
}

/** \generated */
int ThisExpressionAST::lastToken() const
{
    if (this_token)
        return this_token + 1;
    return 1;
}

/** \generated */
int ThrowExpressionAST::firstToken() const
{
    if (throw_token)
        return throw_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int ThrowExpressionAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (throw_token)
        return throw_token + 1;
    return 1;
}

/** \generated */
int TrailingReturnTypeAST::firstToken() const
{
    if (arrow_token)
        return arrow_token;
    if (attributes)
        if (int candidate = attributes->firstToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TrailingReturnTypeAST::lastToken() const
{
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    if (attributes)
        if (int candidate = attributes->lastToken())
            return candidate;
    if (arrow_token)
        return arrow_token + 1;
    return 1;
}

/** \generated */
int TranslationUnitAST::firstToken() const
{
    if (declaration_list)
        if (int candidate = declaration_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TranslationUnitAST::lastToken() const
{
    if (declaration_list)
        if (int candidate = declaration_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int TryBlockStatementAST::firstToken() const
{
    if (try_token)
        return try_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    if (catch_clause_list)
        if (int candidate = catch_clause_list->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TryBlockStatementAST::lastToken() const
{
    if (catch_clause_list)
        if (int candidate = catch_clause_list->lastToken())
            return candidate;
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (try_token)
        return try_token + 1;
    return 1;
}

/** \generated */
int TypeConstructorCallAST::firstToken() const
{
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TypeConstructorCallAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int TypeIdAST::firstToken() const
{
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TypeIdAST::lastToken() const
{
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int TypeidExpressionAST::firstToken() const
{
    if (typeid_token)
        return typeid_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int TypeidExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (typeid_token)
        return typeid_token + 1;
    return 1;
}

/** \generated */
int TypenameCallExpressionAST::firstToken() const
{
    if (typename_token)
        return typename_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TypenameCallExpressionAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (typename_token)
        return typename_token + 1;
    return 1;
}

/** \generated */
int TypenameTypeParameterAST::firstToken() const
{
    if (classkey_token)
        return classkey_token;
    if (dot_dot_dot_token)
        return dot_dot_dot_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (equal_token)
        return equal_token;
    if (type_id)
        if (int candidate = type_id->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int TypenameTypeParameterAST::lastToken() const
{
    if (type_id)
        if (int candidate = type_id->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (dot_dot_dot_token)
        return dot_dot_dot_token + 1;
    if (classkey_token)
        return classkey_token + 1;
    return 1;
}

/** \generated */
int TypeofSpecifierAST::firstToken() const
{
    if (typeof_token)
        return typeof_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int TypeofSpecifierAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (typeof_token)
        return typeof_token + 1;
    return 1;
}

/** \generated */
int UnaryExpressionAST::firstToken() const
{
    if (unary_op_token)
        return unary_op_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int UnaryExpressionAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (unary_op_token)
        return unary_op_token + 1;
    return 1;
}

/** \generated */
int UsingAST::firstToken() const
{
    if (using_token)
        return using_token;
    if (typename_token)
        return typename_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int UsingAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (typename_token)
        return typename_token + 1;
    if (using_token)
        return using_token + 1;
    return 1;
}

/** \generated */
int UsingDirectiveAST::firstToken() const
{
    if (using_token)
        return using_token;
    if (namespace_token)
        return namespace_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int UsingDirectiveAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (namespace_token)
        return namespace_token + 1;
    if (using_token)
        return using_token + 1;
    return 1;
}

/** \generated */
int WhileStatementAST::firstToken() const
{
    if (while_token)
        return while_token;
    if (lparen_token)
        return lparen_token;
    if (condition)
        if (int candidate = condition->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int WhileStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (condition)
        if (int candidate = condition->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (while_token)
        return while_token + 1;
    return 1;
}

/** \generated */
int GnuAttributeSpecifierAST::lastToken() const
{
    if (second_rparen_token)
        return second_rparen_token + 1;
    if (first_rparen_token)
        return first_rparen_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (second_lparen_token)
        return second_lparen_token + 1;
    if (first_lparen_token)
        return first_lparen_token + 1;
    if (attribute_token)
        return attribute_token + 1;
    return 1;
}

int MsvcDeclspecSpecifierAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (attribute_token)
        return attribute_token + 1;
    return 1;
}

int StdAttributeSpecifierAST::lastToken() const
{
    if (second_rbracket_token)
        return second_rbracket_token + 1;
    if (first_rbracket_token)
        return first_rbracket_token + 1;
    if (attribute_list)
        if (int candidate = attribute_list->lastToken())
            return candidate;
    if (second_lbracket_token)
        return second_lbracket_token + 1;
    if (first_lbracket_token)
        return first_lbracket_token + 1;
    return 1;
}

/** \generated */
int PointerLiteralAST::firstToken() const
{
    if (literal_token)
        return literal_token;
    return 0;
}

/** \generated */
int PointerLiteralAST::lastToken() const
{
    if (literal_token)
        return literal_token + 1;
    return 1;
}

/** \generated */
int NoExceptSpecificationAST::firstToken() const
{
    if (noexcept_token)
        return noexcept_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int NoExceptSpecificationAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (noexcept_token)
        return noexcept_token + 1;
    return 1;
}

/** \generated */
int StaticAssertDeclarationAST::firstToken() const
{
    if (static_assert_token)
        return static_assert_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (comma_token)
        return comma_token;
    if (string_literal)
        if (int candidate = string_literal->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int StaticAssertDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (rparen_token)
        return rparen_token + 1;
    if (string_literal)
        if (int candidate = string_literal->lastToken())
            return candidate;
    if (comma_token)
        return comma_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (static_assert_token)
        return static_assert_token + 1;
    return 1;
}

/** \generated */
int DecltypeSpecifierAST::firstToken() const
{
    if (decltype_token)
        return decltype_token;
    if (lparen_token)
        return lparen_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int DecltypeSpecifierAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (decltype_token)
        return decltype_token + 1;
    return 1;
}

/** \generated */
int RangeBasedForStatementAST::firstToken() const
{
    if (for_token)
        return for_token;
    if (lparen_token)
        return lparen_token;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->firstToken())
            return candidate;
    if (declarator)
        if (int candidate = declarator->firstToken())
            return candidate;
    if (colon_token)
        return colon_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    if (statement)
        if (int candidate = statement->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int RangeBasedForStatementAST::lastToken() const
{
    if (statement)
        if (int candidate = statement->lastToken())
            return candidate;
    if (rparen_token)
        return rparen_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (colon_token)
        return colon_token + 1;
    if (declarator)
        if (int candidate = declarator->lastToken())
            return candidate;
    if (type_specifier_list)
        if (int candidate = type_specifier_list->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (for_token)
        return for_token + 1;
    return 1;
}

/** \generated */
int AlignofExpressionAST::firstToken() const
{
    if (alignof_token)
        return alignof_token;
    if (lparen_token)
        return lparen_token;
    if (typeId)
        if (int candidate = typeId->firstToken())
            return candidate;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int AlignofExpressionAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (typeId)
        if (int candidate = typeId->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (alignof_token)
        return alignof_token + 1;
    return 1;
}

/** \generated */
int AliasDeclarationAST::firstToken() const
{
    if (using_token)
        return using_token;
    if (name)
        if (int candidate = name->firstToken())
            return candidate;
    if (equal_token)
        return equal_token;
    if (typeId)
        if (int candidate = typeId->firstToken())
            return candidate;
    if (semicolon_token)
        return semicolon_token;
    return 0;
}

/** \generated */
int AliasDeclarationAST::lastToken() const
{
    if (semicolon_token)
        return semicolon_token + 1;
    if (typeId)
        if (int candidate = typeId->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (name)
        if (int candidate = name->lastToken())
            return candidate;
    if (using_token)
        return using_token + 1;
    return 1;
}

/** \generated */
int DesignatedInitializerAST::firstToken() const
{
    if (designator_list)
        if (int candidate = designator_list->firstToken())
            return candidate;
    if (equal_token)
        return equal_token;
    if (initializer)
        if (int candidate = initializer->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int DesignatedInitializerAST::lastToken() const
{
    if (initializer)
        if (int candidate = initializer->lastToken())
            return candidate;
    if (equal_token)
        return equal_token + 1;
    if (designator_list)
        if (int candidate = designator_list->lastToken())
            return candidate;
    return 1;
}

/** \generated */
int BracketDesignatorAST::firstToken() const
{
    if (lbracket_token)
        return lbracket_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    if (rbracket_token)
        return rbracket_token;
    return 0;
}

/** \generated */
int BracketDesignatorAST::lastToken() const
{
    if (rbracket_token)
        return rbracket_token + 1;
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (lbracket_token)
        return lbracket_token + 1;
    return 1;
}

/** \generated */
int DotDesignatorAST::firstToken() const
{
    if (dot_token)
        return dot_token;
    if (identifier_token)
        return identifier_token;
    return 0;
}

/** \generated */
int DotDesignatorAST::lastToken() const
{
    if (identifier_token)
        return identifier_token + 1;
    if (dot_token)
        return dot_token + 1;
    return 1;
}

/** \generated */
int AlignmentSpecifierAST::firstToken() const
{
    if (align_token)
        return align_token;
    if (lparen_token)
        return lparen_token;
    if (typeIdExprOrAlignmentExpr)
        if (int candidate = typeIdExprOrAlignmentExpr->firstToken())
            return candidate;
    if (ellipses_token)
        return ellipses_token;
    if (rparen_token)
        return rparen_token;
    return 0;
}

/** \generated */
int AlignmentSpecifierAST::lastToken() const
{
    if (rparen_token)
        return rparen_token + 1;
    if (ellipses_token)
        return ellipses_token + 1;
    if (typeIdExprOrAlignmentExpr)
        if (int candidate = typeIdExprOrAlignmentExpr->lastToken())
            return candidate;
    if (lparen_token)
        return lparen_token + 1;
    if (align_token)
        return align_token + 1;
    return 1;
}

/** \generated */
int NoExceptOperatorExpressionAST::firstToken() const
{
    if (noexcept_token)
        return noexcept_token;
    if (expression)
        if (int candidate = expression->firstToken())
            return candidate;
    return 0;
}

/** \generated */
int NoExceptOperatorExpressionAST::lastToken() const
{
    if (expression)
        if (int candidate = expression->lastToken())
            return candidate;
    if (noexcept_token)
        return noexcept_token + 1;
    return 1;
}

