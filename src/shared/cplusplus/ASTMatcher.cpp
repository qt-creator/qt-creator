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

#include "AST.h"
#include "ASTMatcher.h"
#include "Control.h"
#include "TranslationUnit.h"
#include "Literals.h"

using namespace CPlusPlus;

ASTMatcher::ASTMatcher(TranslationUnit *translationUnit, TranslationUnit *patternTranslationUnit)
    : _translationUnit(translationUnit), _patternTranslationUnit(patternTranslationUnit)
{ }

ASTMatcher::~ASTMatcher()
{ }

TranslationUnit *ASTMatcher::translationUnit() const
{ return _translationUnit; }

TranslationUnit *ASTMatcher::patternTranslationUnit() const
{ return _patternTranslationUnit; }

bool ASTMatcher::matchToken(unsigned tokenIndex, unsigned patternTokenIndex) const
{
    const Token &token = _translationUnit->tokenAt(tokenIndex);
    const Token &otherToken = _patternTranslationUnit->tokenAt(patternTokenIndex);
    if (token.f.kind != otherToken.f.kind)
        return false;
    else if (token.is(T_IDENTIFIER)) {
        if (! token.identifier->isEqualTo(otherToken.identifier))
            return false;
    } else if (token.isLiteral()) {
        if (! token.literal->isEqualTo(otherToken.literal))
            return false;
    }
    return true;
}

bool ASTMatcher::match(SimpleSpecifierAST *node, SimpleSpecifierAST *pattern)
{
    if (! matchToken(node->specifier_token, pattern->specifier_token))
        return false;
    return true;
}

bool ASTMatcher::match(AttributeSpecifierAST *node, AttributeSpecifierAST *pattern)
{
    if (! matchToken(node->attribute_token, pattern->attribute_token))
        return false;
    if (! matchToken(node->first_lparen_token, pattern->first_lparen_token))
        return false;
    if (! matchToken(node->second_lparen_token, pattern->second_lparen_token))
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! matchToken(node->first_rparen_token, pattern->first_rparen_token))
        return false;
    if (! matchToken(node->second_rparen_token, pattern->second_rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(AttributeAST *node, AttributeAST *pattern)
{
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! matchToken(node->tag_token, pattern->tag_token))
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(TypeofSpecifierAST *node, TypeofSpecifierAST *pattern)
{
    if (! matchToken(node->typeof_token, pattern->typeof_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(DeclaratorAST *node, DeclaratorAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! AST::match(node->ptr_operator_list, pattern->ptr_operator_list, this))
        return false;
    if (! AST::match(node->core_declarator, pattern->core_declarator, this))
        return false;
    if (! AST::match(node->postfix_declarator_list, pattern->postfix_declarator_list, this))
        return false;
    if (! AST::match(node->post_attribute_list, pattern->post_attribute_list, this))
        return false;
    if (! matchToken(node->equals_token, pattern->equals_token))
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    return true;
}

bool ASTMatcher::match(SimpleDeclarationAST *node, SimpleDeclarationAST *pattern)
{
    if (! matchToken(node->qt_invokable_token, pattern->qt_invokable_token))
        return false;
    if (! AST::match(node->decl_specifier_list, pattern->decl_specifier_list, this))
        return false;
    if (! AST::match(node->declarator_list, pattern->declarator_list, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(EmptyDeclarationAST *node, EmptyDeclarationAST *pattern)
{
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(AccessDeclarationAST *node, AccessDeclarationAST *pattern)
{
    if (! matchToken(node->access_specifier_token, pattern->access_specifier_token))
        return false;
    if (! matchToken(node->slots_token, pattern->slots_token))
        return false;
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    return true;
}

bool ASTMatcher::match(AsmDefinitionAST *node, AsmDefinitionAST *pattern)
{
    if (! matchToken(node->asm_token, pattern->asm_token))
        return false;
    if (! matchToken(node->volatile_token, pattern->volatile_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(BaseSpecifierAST *node, BaseSpecifierAST *pattern)
{
    if (! matchToken(node->virtual_token, pattern->virtual_token))
        return false;
    if (! matchToken(node->access_specifier_token, pattern->access_specifier_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    return true;
}

bool ASTMatcher::match(CompoundLiteralAST *node, CompoundLiteralAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    return true;
}

bool ASTMatcher::match(QtMethodAST *node, QtMethodAST *pattern)
{
    if (! matchToken(node->method_token, pattern->method_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(BinaryExpressionAST *node, BinaryExpressionAST *pattern)
{
    if (! AST::match(node->left_expression, pattern->left_expression, this))
        return false;
    if (! matchToken(node->binary_op_token, pattern->binary_op_token))
        return false;
    if (! AST::match(node->right_expression, pattern->right_expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(CastExpressionAST *node, CastExpressionAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(ClassSpecifierAST *node, ClassSpecifierAST *pattern)
{
    if (! matchToken(node->classkey_token, pattern->classkey_token))
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    if (! AST::match(node->base_clause_list, pattern->base_clause_list, this))
        return false;
    if (! matchToken(node->lbrace_token, pattern->lbrace_token))
        return false;
    if (! AST::match(node->member_specifier_list, pattern->member_specifier_list, this))
        return false;
    if (! matchToken(node->rbrace_token, pattern->rbrace_token))
        return false;
    return true;
}

bool ASTMatcher::match(CaseStatementAST *node, CaseStatementAST *pattern)
{
    if (! matchToken(node->case_token, pattern->case_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(CompoundStatementAST *node, CompoundStatementAST *pattern)
{
    if (! matchToken(node->lbrace_token, pattern->lbrace_token))
        return false;
    if (! AST::match(node->statement_list, pattern->statement_list, this))
        return false;
    if (! matchToken(node->rbrace_token, pattern->rbrace_token))
        return false;
    return true;
}

bool ASTMatcher::match(ConditionAST *node, ConditionAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    return true;
}

bool ASTMatcher::match(ConditionalExpressionAST *node, ConditionalExpressionAST *pattern)
{
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (! matchToken(node->question_token, pattern->question_token))
        return false;
    if (! AST::match(node->left_expression, pattern->left_expression, this))
        return false;
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    if (! AST::match(node->right_expression, pattern->right_expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(CppCastExpressionAST *node, CppCastExpressionAST *pattern)
{
    if (! matchToken(node->cast_token, pattern->cast_token))
        return false;
    if (! matchToken(node->less_token, pattern->less_token))
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (! matchToken(node->greater_token, pattern->greater_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(CtorInitializerAST *node, CtorInitializerAST *pattern)
{
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    if (! AST::match(node->member_initializer_list, pattern->member_initializer_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(DeclarationStatementAST *node, DeclarationStatementAST *pattern)
{
    if (! AST::match(node->declaration, pattern->declaration, this))
        return false;
    return true;
}

bool ASTMatcher::match(DeclaratorIdAST *node, DeclaratorIdAST *pattern)
{
    if (! AST::match(node->name, pattern->name, this))
        return false;
    return true;
}

bool ASTMatcher::match(NestedDeclaratorAST *node, NestedDeclaratorAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(FunctionDeclaratorAST *node, FunctionDeclaratorAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->parameters, pattern->parameters, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->cv_qualifier_list, pattern->cv_qualifier_list, this))
        return false;
    if (! AST::match(node->exception_specification, pattern->exception_specification, this))
        return false;
    if (! AST::match(node->as_cpp_initializer, pattern->as_cpp_initializer, this))
        return false;
    return true;
}

bool ASTMatcher::match(ArrayDeclaratorAST *node, ArrayDeclaratorAST *pattern)
{
    if (! matchToken(node->lbracket_token, pattern->lbracket_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rbracket_token, pattern->rbracket_token))
        return false;
    return true;
}

bool ASTMatcher::match(DeleteExpressionAST *node, DeleteExpressionAST *pattern)
{
    if (! matchToken(node->scope_token, pattern->scope_token))
        return false;
    if (! matchToken(node->delete_token, pattern->delete_token))
        return false;
    if (! matchToken(node->lbracket_token, pattern->lbracket_token))
        return false;
    if (! matchToken(node->rbracket_token, pattern->rbracket_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(DoStatementAST *node, DoStatementAST *pattern)
{
    if (! matchToken(node->do_token, pattern->do_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    if (! matchToken(node->while_token, pattern->while_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(NamedTypeSpecifierAST *node, NamedTypeSpecifierAST *pattern)
{
    if (! AST::match(node->name, pattern->name, this))
        return false;
    return true;
}

bool ASTMatcher::match(ElaboratedTypeSpecifierAST *node, ElaboratedTypeSpecifierAST *pattern)
{
    if (! matchToken(node->classkey_token, pattern->classkey_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    return true;
}

bool ASTMatcher::match(EnumSpecifierAST *node, EnumSpecifierAST *pattern)
{
    if (! matchToken(node->enum_token, pattern->enum_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->lbrace_token, pattern->lbrace_token))
        return false;
    if (! AST::match(node->enumerator_list, pattern->enumerator_list, this))
        return false;
    if (! matchToken(node->rbrace_token, pattern->rbrace_token))
        return false;
    return true;
}

bool ASTMatcher::match(EnumeratorAST *node, EnumeratorAST *pattern)
{
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    if (! matchToken(node->equal_token, pattern->equal_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(ExceptionDeclarationAST *node, ExceptionDeclarationAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! matchToken(node->dot_dot_dot_token, pattern->dot_dot_dot_token))
        return false;
    return true;
}

bool ASTMatcher::match(ExceptionSpecificationAST *node, ExceptionSpecificationAST *pattern)
{
    if (! matchToken(node->throw_token, pattern->throw_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! matchToken(node->dot_dot_dot_token, pattern->dot_dot_dot_token))
        return false;
    if (! AST::match(node->type_id_list, pattern->type_id_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(ExpressionOrDeclarationStatementAST *node, ExpressionOrDeclarationStatementAST *pattern)
{
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! AST::match(node->declaration, pattern->declaration, this))
        return false;
    return true;
}

bool ASTMatcher::match(ExpressionStatementAST *node, ExpressionStatementAST *pattern)
{
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(FunctionDefinitionAST *node, FunctionDefinitionAST *pattern)
{
    if (! matchToken(node->qt_invokable_token, pattern->qt_invokable_token))
        return false;
    if (! AST::match(node->decl_specifier_list, pattern->decl_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! AST::match(node->ctor_initializer, pattern->ctor_initializer, this))
        return false;
    if (! AST::match(node->function_body, pattern->function_body, this))
        return false;
    return true;
}

bool ASTMatcher::match(ForeachStatementAST *node, ForeachStatementAST *pattern)
{
    if (! matchToken(node->foreach_token, pattern->foreach_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    if (! matchToken(node->comma_token, pattern->comma_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ForStatementAST *node, ForStatementAST *pattern)
{
    if (! matchToken(node->for_token, pattern->for_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(IfStatementAST *node, IfStatementAST *pattern)
{
    if (! matchToken(node->if_token, pattern->if_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    if (! matchToken(node->else_token, pattern->else_token))
        return false;
    if (! AST::match(node->else_statement, pattern->else_statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ArrayInitializerAST *node, ArrayInitializerAST *pattern)
{
    if (! matchToken(node->lbrace_token, pattern->lbrace_token))
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (! matchToken(node->rbrace_token, pattern->rbrace_token))
        return false;
    return true;
}

bool ASTMatcher::match(LabeledStatementAST *node, LabeledStatementAST *pattern)
{
    if (! matchToken(node->label_token, pattern->label_token))
        return false;
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(LinkageBodyAST *node, LinkageBodyAST *pattern)
{
    if (! matchToken(node->lbrace_token, pattern->lbrace_token))
        return false;
    if (! AST::match(node->declaration_list, pattern->declaration_list, this))
        return false;
    if (! matchToken(node->rbrace_token, pattern->rbrace_token))
        return false;
    return true;
}

bool ASTMatcher::match(LinkageSpecificationAST *node, LinkageSpecificationAST *pattern)
{
    if (! matchToken(node->extern_token, pattern->extern_token))
        return false;
    if (! matchToken(node->extern_type_token, pattern->extern_type_token))
        return false;
    if (! AST::match(node->declaration, pattern->declaration, this))
        return false;
    return true;
}

bool ASTMatcher::match(MemInitializerAST *node, MemInitializerAST *pattern)
{
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(NestedNameSpecifierAST *node, NestedNameSpecifierAST *pattern)
{
    if (! AST::match(node->class_or_namespace_name, pattern->class_or_namespace_name, this))
        return false;
    if (! matchToken(node->scope_token, pattern->scope_token))
        return false;
    return true;
}

bool ASTMatcher::match(QualifiedNameAST *node, QualifiedNameAST *pattern)
{
    if (! matchToken(node->global_scope_token, pattern->global_scope_token))
        return false;
    if (! AST::match(node->nested_name_specifier_list, pattern->nested_name_specifier_list, this))
        return false;
    if (! AST::match(node->unqualified_name, pattern->unqualified_name, this))
        return false;
    return true;
}

bool ASTMatcher::match(OperatorFunctionIdAST *node, OperatorFunctionIdAST *pattern)
{
    if (! matchToken(node->operator_token, pattern->operator_token))
        return false;
    if (! AST::match(node->op, pattern->op, this))
        return false;
    return true;
}

bool ASTMatcher::match(ConversionFunctionIdAST *node, ConversionFunctionIdAST *pattern)
{
    if (! matchToken(node->operator_token, pattern->operator_token))
        return false;
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->ptr_operator_list, pattern->ptr_operator_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(SimpleNameAST *node, SimpleNameAST *pattern)
{
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    return true;
}

bool ASTMatcher::match(DestructorNameAST *node, DestructorNameAST *pattern)
{
    if (! matchToken(node->tilde_token, pattern->tilde_token))
        return false;
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    return true;
}

bool ASTMatcher::match(TemplateIdAST *node, TemplateIdAST *pattern)
{
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    if (! matchToken(node->less_token, pattern->less_token))
        return false;
    if (! AST::match(node->template_argument_list, pattern->template_argument_list, this))
        return false;
    if (! matchToken(node->greater_token, pattern->greater_token))
        return false;
    return true;
}

bool ASTMatcher::match(NamespaceAST *node, NamespaceAST *pattern)
{
    if (! matchToken(node->namespace_token, pattern->namespace_token))
        return false;
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! AST::match(node->linkage_body, pattern->linkage_body, this))
        return false;
    return true;
}

bool ASTMatcher::match(NamespaceAliasDefinitionAST *node, NamespaceAliasDefinitionAST *pattern)
{
    if (! matchToken(node->namespace_token, pattern->namespace_token))
        return false;
    if (! matchToken(node->namespace_name_token, pattern->namespace_name_token))
        return false;
    if (! matchToken(node->equal_token, pattern->equal_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(NewPlacementAST *node, NewPlacementAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(NewArrayDeclaratorAST *node, NewArrayDeclaratorAST *pattern)
{
    if (! matchToken(node->lbracket_token, pattern->lbracket_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rbracket_token, pattern->rbracket_token))
        return false;
    return true;
}

bool ASTMatcher::match(NewExpressionAST *node, NewExpressionAST *pattern)
{
    if (! matchToken(node->scope_token, pattern->scope_token))
        return false;
    if (! matchToken(node->new_token, pattern->new_token))
        return false;
    if (! AST::match(node->new_placement, pattern->new_placement, this))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->new_type_id, pattern->new_type_id, this))
        return false;
    if (! AST::match(node->new_initializer, pattern->new_initializer, this))
        return false;
    return true;
}

bool ASTMatcher::match(NewInitializerAST *node, NewInitializerAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(NewTypeIdAST *node, NewTypeIdAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->ptr_operator_list, pattern->ptr_operator_list, this))
        return false;
    if (! AST::match(node->new_array_declarator_list, pattern->new_array_declarator_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(OperatorAST *node, OperatorAST *pattern)
{
    if (! matchToken(node->op_token, pattern->op_token))
        return false;
    if (! matchToken(node->open_token, pattern->open_token))
        return false;
    if (! matchToken(node->close_token, pattern->close_token))
        return false;
    return true;
}

bool ASTMatcher::match(ParameterDeclarationAST *node, ParameterDeclarationAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! matchToken(node->equal_token, pattern->equal_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(ParameterDeclarationClauseAST *node, ParameterDeclarationClauseAST *pattern)
{
    if (! AST::match(node->parameter_declaration_list, pattern->parameter_declaration_list, this))
        return false;
    if (! matchToken(node->dot_dot_dot_token, pattern->dot_dot_dot_token))
        return false;
    return true;
}

bool ASTMatcher::match(CallAST *node, CallAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(ArrayAccessAST *node, ArrayAccessAST *pattern)
{
    if (! matchToken(node->lbracket_token, pattern->lbracket_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rbracket_token, pattern->rbracket_token))
        return false;
    return true;
}

bool ASTMatcher::match(PostIncrDecrAST *node, PostIncrDecrAST *pattern)
{
    if (! matchToken(node->incr_decr_token, pattern->incr_decr_token))
        return false;
    return true;
}

bool ASTMatcher::match(MemberAccessAST *node, MemberAccessAST *pattern)
{
    if (! matchToken(node->access_token, pattern->access_token))
        return false;
    if (! matchToken(node->template_token, pattern->template_token))
        return false;
    if (! AST::match(node->member_name, pattern->member_name, this))
        return false;
    return true;
}

bool ASTMatcher::match(TypeidExpressionAST *node, TypeidExpressionAST *pattern)
{
    if (! matchToken(node->typeid_token, pattern->typeid_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(TypenameCallExpressionAST *node, TypenameCallExpressionAST *pattern)
{
    if (! matchToken(node->typename_token, pattern->typename_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(TypeConstructorCallAST *node, TypeConstructorCallAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(PostfixExpressionAST *node, PostfixExpressionAST *pattern)
{
    if (! AST::match(node->base_expression, pattern->base_expression, this))
        return false;
    if (! AST::match(node->postfix_expression_list, pattern->postfix_expression_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(PointerToMemberAST *node, PointerToMemberAST *pattern)
{
    if (! matchToken(node->global_scope_token, pattern->global_scope_token))
        return false;
    if (! AST::match(node->nested_name_specifier_list, pattern->nested_name_specifier_list, this))
        return false;
    if (! matchToken(node->star_token, pattern->star_token))
        return false;
    if (! AST::match(node->cv_qualifier_list, pattern->cv_qualifier_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(PointerAST *node, PointerAST *pattern)
{
    if (! matchToken(node->star_token, pattern->star_token))
        return false;
    if (! AST::match(node->cv_qualifier_list, pattern->cv_qualifier_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(ReferenceAST *node, ReferenceAST *pattern)
{
    if (! matchToken(node->amp_token, pattern->amp_token))
        return false;
    return true;
}

bool ASTMatcher::match(BreakStatementAST *node, BreakStatementAST *pattern)
{
    if (! matchToken(node->break_token, pattern->break_token))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ContinueStatementAST *node, ContinueStatementAST *pattern)
{
    if (! matchToken(node->continue_token, pattern->continue_token))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(GotoStatementAST *node, GotoStatementAST *pattern)
{
    if (! matchToken(node->goto_token, pattern->goto_token))
        return false;
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ReturnStatementAST *node, ReturnStatementAST *pattern)
{
    if (! matchToken(node->return_token, pattern->return_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(SizeofExpressionAST *node, SizeofExpressionAST *pattern)
{
    if (! matchToken(node->sizeof_token, pattern->sizeof_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(NumericLiteralAST *node, NumericLiteralAST *pattern)
{
    if (! matchToken(node->literal_token, pattern->literal_token))
        return false;
    return true;
}

bool ASTMatcher::match(BoolLiteralAST *node, BoolLiteralAST *pattern)
{
    if (! matchToken(node->literal_token, pattern->literal_token))
        return false;
    return true;
}

bool ASTMatcher::match(ThisExpressionAST *node, ThisExpressionAST *pattern)
{
    if (! matchToken(node->this_token, pattern->this_token))
        return false;
    return true;
}

bool ASTMatcher::match(NestedExpressionAST *node, NestedExpressionAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(StringLiteralAST *node, StringLiteralAST *pattern)
{
    if (! matchToken(node->literal_token, pattern->literal_token))
        return false;
    if (! AST::match(node->next, pattern->next, this))
        return false;
    return true;
}

bool ASTMatcher::match(SwitchStatementAST *node, SwitchStatementAST *pattern)
{
    if (! matchToken(node->switch_token, pattern->switch_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(TemplateDeclarationAST *node, TemplateDeclarationAST *pattern)
{
    if (! matchToken(node->export_token, pattern->export_token))
        return false;
    if (! matchToken(node->template_token, pattern->template_token))
        return false;
    if (! matchToken(node->less_token, pattern->less_token))
        return false;
    if (! AST::match(node->template_parameter_list, pattern->template_parameter_list, this))
        return false;
    if (! matchToken(node->greater_token, pattern->greater_token))
        return false;
    if (! AST::match(node->declaration, pattern->declaration, this))
        return false;
    return true;
}

bool ASTMatcher::match(ThrowExpressionAST *node, ThrowExpressionAST *pattern)
{
    if (! matchToken(node->throw_token, pattern->throw_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(TranslationUnitAST *node, TranslationUnitAST *pattern)
{
    if (! AST::match(node->declaration_list, pattern->declaration_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(TryBlockStatementAST *node, TryBlockStatementAST *pattern)
{
    if (! matchToken(node->try_token, pattern->try_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    if (! AST::match(node->catch_clause_list, pattern->catch_clause_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(CatchClauseAST *node, CatchClauseAST *pattern)
{
    if (! matchToken(node->catch_token, pattern->catch_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->exception_declaration, pattern->exception_declaration, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(TypeIdAST *node, TypeIdAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    return true;
}

bool ASTMatcher::match(TypenameTypeParameterAST *node, TypenameTypeParameterAST *pattern)
{
    if (! matchToken(node->classkey_token, pattern->classkey_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->equal_token, pattern->equal_token))
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    return true;
}

bool ASTMatcher::match(TemplateTypeParameterAST *node, TemplateTypeParameterAST *pattern)
{
    if (! matchToken(node->template_token, pattern->template_token))
        return false;
    if (! matchToken(node->less_token, pattern->less_token))
        return false;
    if (! AST::match(node->template_parameter_list, pattern->template_parameter_list, this))
        return false;
    if (! matchToken(node->greater_token, pattern->greater_token))
        return false;
    if (! matchToken(node->class_token, pattern->class_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->equal_token, pattern->equal_token))
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    return true;
}

bool ASTMatcher::match(UnaryExpressionAST *node, UnaryExpressionAST *pattern)
{
    if (! matchToken(node->unary_op_token, pattern->unary_op_token))
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(UsingAST *node, UsingAST *pattern)
{
    if (! matchToken(node->using_token, pattern->using_token))
        return false;
    if (! matchToken(node->typename_token, pattern->typename_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(UsingDirectiveAST *node, UsingDirectiveAST *pattern)
{
    if (! matchToken(node->using_token, pattern->using_token))
        return false;
    if (! matchToken(node->namespace_token, pattern->namespace_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(WhileStatementAST *node, WhileStatementAST *pattern)
{
    if (! matchToken(node->while_token, pattern->while_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCClassForwardDeclarationAST *node, ObjCClassForwardDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! matchToken(node->class_token, pattern->class_token))
        return false;
    if (! AST::match(node->identifier_list, pattern->identifier_list, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCClassDeclarationAST *node, ObjCClassDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! matchToken(node->interface_token, pattern->interface_token))
        return false;
    if (! matchToken(node->implementation_token, pattern->implementation_token))
        return false;
    if (! AST::match(node->class_name, pattern->class_name, this))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->category_name, pattern->category_name, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    if (! AST::match(node->superclass, pattern->superclass, this))
        return false;
    if (! AST::match(node->protocol_refs, pattern->protocol_refs, this))
        return false;
    if (! AST::match(node->inst_vars_decl, pattern->inst_vars_decl, this))
        return false;
    if (! AST::match(node->member_declaration_list, pattern->member_declaration_list, this))
        return false;
    if (! matchToken(node->end_token, pattern->end_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolForwardDeclarationAST *node, ObjCProtocolForwardDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! matchToken(node->protocol_token, pattern->protocol_token))
        return false;
    if (! AST::match(node->identifier_list, pattern->identifier_list, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolDeclarationAST *node, ObjCProtocolDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! matchToken(node->protocol_token, pattern->protocol_token))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! AST::match(node->protocol_refs, pattern->protocol_refs, this))
        return false;
    if (! AST::match(node->member_declaration_list, pattern->member_declaration_list, this))
        return false;
    if (! matchToken(node->end_token, pattern->end_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolRefsAST *node, ObjCProtocolRefsAST *pattern)
{
    if (! matchToken(node->less_token, pattern->less_token))
        return false;
    if (! AST::match(node->identifier_list, pattern->identifier_list, this))
        return false;
    if (! matchToken(node->greater_token, pattern->greater_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCMessageArgumentAST *node, ObjCMessageArgumentAST *pattern)
{
    if (! AST::match(node->parameter_value_expression, pattern->parameter_value_expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCMessageExpressionAST *node, ObjCMessageExpressionAST *pattern)
{
    if (! matchToken(node->lbracket_token, pattern->lbracket_token))
        return false;
    if (! AST::match(node->receiver_expression, pattern->receiver_expression, this))
        return false;
    if (! AST::match(node->selector, pattern->selector, this))
        return false;
    if (! AST::match(node->argument_list, pattern->argument_list, this))
        return false;
    if (! matchToken(node->rbracket_token, pattern->rbracket_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolExpressionAST *node, ObjCProtocolExpressionAST *pattern)
{
    if (! matchToken(node->protocol_token, pattern->protocol_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! matchToken(node->identifier_token, pattern->identifier_token))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCTypeNameAST *node, ObjCTypeNameAST *pattern)
{
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCEncodeExpressionAST *node, ObjCEncodeExpressionAST *pattern)
{
    if (! matchToken(node->encode_token, pattern->encode_token))
        return false;
    if (! AST::match(node->type_name, pattern->type_name, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSelectorWithoutArgumentsAST *node, ObjCSelectorWithoutArgumentsAST *pattern)
{
    if (! matchToken(node->name_token, pattern->name_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSelectorArgumentAST *node, ObjCSelectorArgumentAST *pattern)
{
    if (! matchToken(node->name_token, pattern->name_token))
        return false;
    if (! matchToken(node->colon_token, pattern->colon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSelectorWithArgumentsAST *node, ObjCSelectorWithArgumentsAST *pattern)
{
    if (! AST::match(node->selector_argument_list, pattern->selector_argument_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSelectorExpressionAST *node, ObjCSelectorExpressionAST *pattern)
{
    if (! matchToken(node->selector_token, pattern->selector_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->selector, pattern->selector, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCInstanceVariablesDeclarationAST *node, ObjCInstanceVariablesDeclarationAST *pattern)
{
    if (! matchToken(node->lbrace_token, pattern->lbrace_token))
        return false;
    if (! AST::match(node->instance_variable_list, pattern->instance_variable_list, this))
        return false;
    if (! matchToken(node->rbrace_token, pattern->rbrace_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCVisibilityDeclarationAST *node, ObjCVisibilityDeclarationAST *pattern)
{
    if (! matchToken(node->visibility_token, pattern->visibility_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCPropertyAttributeAST *node, ObjCPropertyAttributeAST *pattern)
{
    if (! matchToken(node->attribute_identifier_token, pattern->attribute_identifier_token))
        return false;
    if (! matchToken(node->equals_token, pattern->equals_token))
        return false;
    if (! AST::match(node->method_selector, pattern->method_selector, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCPropertyDeclarationAST *node, ObjCPropertyDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! matchToken(node->property_token, pattern->property_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->property_attribute_list, pattern->property_attribute_list, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->simple_declaration, pattern->simple_declaration, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCMessageArgumentDeclarationAST *node, ObjCMessageArgumentDeclarationAST *pattern)
{
    if (! AST::match(node->type_name, pattern->type_name, this))
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! matchToken(node->param_name_token, pattern->param_name_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCMethodPrototypeAST *node, ObjCMethodPrototypeAST *pattern)
{
    if (! matchToken(node->method_type_token, pattern->method_type_token))
        return false;
    if (! AST::match(node->type_name, pattern->type_name, this))
        return false;
    if (! AST::match(node->selector, pattern->selector, this))
        return false;
    if (! AST::match(node->argument_list, pattern->argument_list, this))
        return false;
    if (! matchToken(node->dot_dot_dot_token, pattern->dot_dot_dot_token))
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCMethodDeclarationAST *node, ObjCMethodDeclarationAST *pattern)
{
    if (! AST::match(node->method_prototype, pattern->method_prototype, this))
        return false;
    if (! AST::match(node->function_body, pattern->function_body, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSynthesizedPropertyAST *node, ObjCSynthesizedPropertyAST *pattern)
{
    if (! matchToken(node->equals_token, pattern->equals_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSynthesizedPropertiesDeclarationAST *node, ObjCSynthesizedPropertiesDeclarationAST *pattern)
{
    if (! matchToken(node->synthesized_token, pattern->synthesized_token))
        return false;
    if (! AST::match(node->property_identifier_list, pattern->property_identifier_list, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCDynamicPropertiesDeclarationAST *node, ObjCDynamicPropertiesDeclarationAST *pattern)
{
    if (! matchToken(node->dynamic_token, pattern->dynamic_token))
        return false;
    if (! AST::match(node->property_identifier_list, pattern->property_identifier_list, this))
        return false;
    if (! matchToken(node->semicolon_token, pattern->semicolon_token))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCFastEnumerationAST *node, ObjCFastEnumerationAST *pattern)
{
    if (! matchToken(node->for_token, pattern->for_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    if (! matchToken(node->in_token, pattern->in_token))
        return false;
    if (! AST::match(node->fast_enumeratable_expression, pattern->fast_enumeratable_expression, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->body_statement, pattern->body_statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSynchronizedStatementAST *node, ObjCSynchronizedStatementAST *pattern)
{
    if (! matchToken(node->synchronized_token, pattern->synchronized_token))
        return false;
    if (! matchToken(node->lparen_token, pattern->lparen_token))
        return false;
    if (! AST::match(node->synchronized_object, pattern->synchronized_object, this))
        return false;
    if (! matchToken(node->rparen_token, pattern->rparen_token))
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

