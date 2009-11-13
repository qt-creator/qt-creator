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

using namespace CPlusPlus;

ASTMatcher::ASTMatcher(Control *control)
    : _control(control)
{ }

ASTMatcher::~ASTMatcher()
{ }

Control *ASTMatcher::control() const
{ return _control; }

TranslationUnit *ASTMatcher::translationUnit() const
{ return _control->translationUnit(); }

unsigned ASTMatcher::tokenCount() const
{ return translationUnit()->tokenCount(); }

const Token &ASTMatcher::tokenAt(unsigned index) const
{ return translationUnit()->tokenAt(index); }

int ASTMatcher::tokenKind(unsigned index) const
{ return translationUnit()->tokenKind(index); }

const char *ASTMatcher::spell(unsigned index) const
{ return translationUnit()->spell(index); }

Identifier *ASTMatcher::identifier(unsigned index) const
{ return translationUnit()->identifier(index); }

Literal *ASTMatcher::literal(unsigned index) const
{ return translationUnit()->literal(index); }

NumericLiteral *ASTMatcher::numericLiteral(unsigned index) const
{ return translationUnit()->numericLiteral(index); }

StringLiteral *ASTMatcher::stringLiteral(unsigned index) const
{ return translationUnit()->stringLiteral(index); }

void ASTMatcher::getPosition(unsigned offset,
                             unsigned *line,
                             unsigned *column,
                             StringLiteral **fileName) const
{ translationUnit()->getPosition(offset, line, column, fileName); }

void ASTMatcher::getTokenPosition(unsigned index,
                                  unsigned *line,
                                  unsigned *column,
                                  StringLiteral **fileName) const
{ translationUnit()->getTokenPosition(index, line, column, fileName); }

void ASTMatcher::getTokenStartPosition(unsigned index, unsigned *line, unsigned *column) const
{ getPosition(tokenAt(index).begin(), line, column); }

void ASTMatcher::getTokenEndPosition(unsigned index, unsigned *line, unsigned *column) const
{ getPosition(tokenAt(index).end(), line, column); }

bool ASTMatcher::match(SimpleSpecifierAST *node, SimpleSpecifierAST *pattern)
{
    if (node->specifier_token != pattern->specifier_token)
        return false;
    return true;
}

bool ASTMatcher::match(AttributeSpecifierAST *node, AttributeSpecifierAST *pattern)
{
    if (node->attribute_token != pattern->attribute_token)
        return false;
    if (node->first_lparen_token != pattern->first_lparen_token)
        return false;
    if (node->second_lparen_token != pattern->second_lparen_token)
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (node->first_rparen_token != pattern->first_rparen_token)
        return false;
    if (node->second_rparen_token != pattern->second_rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(AttributeAST *node, AttributeAST *pattern)
{
    if (node->identifier_token != pattern->identifier_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (node->tag_token != pattern->tag_token)
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(TypeofSpecifierAST *node, TypeofSpecifierAST *pattern)
{
    if (node->typeof_token != pattern->typeof_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
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
    if (node->equals_token != pattern->equals_token)
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    return true;
}

bool ASTMatcher::match(SimpleDeclarationAST *node, SimpleDeclarationAST *pattern)
{
    if (node->qt_invokable_token != pattern->qt_invokable_token)
        return false;
    if (! AST::match(node->decl_specifier_list, pattern->decl_specifier_list, this))
        return false;
    if (! AST::match(node->declarator_list, pattern->declarator_list, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(EmptyDeclarationAST *node, EmptyDeclarationAST *pattern)
{
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(AccessDeclarationAST *node, AccessDeclarationAST *pattern)
{
    if (node->access_specifier_token != pattern->access_specifier_token)
        return false;
    if (node->slots_token != pattern->slots_token)
        return false;
    if (node->colon_token != pattern->colon_token)
        return false;
    return true;
}

bool ASTMatcher::match(AsmDefinitionAST *node, AsmDefinitionAST *pattern)
{
    if (node->asm_token != pattern->asm_token)
        return false;
    if (node->volatile_token != pattern->volatile_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(BaseSpecifierAST *node, BaseSpecifierAST *pattern)
{
    if (node->virtual_token != pattern->virtual_token)
        return false;
    if (node->access_specifier_token != pattern->access_specifier_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    return true;
}

bool ASTMatcher::match(CompoundLiteralAST *node, CompoundLiteralAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    return true;
}

bool ASTMatcher::match(QtMethodAST *node, QtMethodAST *pattern)
{
    if (node->method_token != pattern->method_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(BinaryExpressionAST *node, BinaryExpressionAST *pattern)
{
    if (! AST::match(node->left_expression, pattern->left_expression, this))
        return false;
    if (node->binary_op_token != pattern->binary_op_token)
        return false;
    if (! AST::match(node->right_expression, pattern->right_expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(CastExpressionAST *node, CastExpressionAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(ClassSpecifierAST *node, ClassSpecifierAST *pattern)
{
    if (node->classkey_token != pattern->classkey_token)
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->colon_token != pattern->colon_token)
        return false;
    if (! AST::match(node->base_clause_list, pattern->base_clause_list, this))
        return false;
    if (node->lbrace_token != pattern->lbrace_token)
        return false;
    if (! AST::match(node->member_specifier_list, pattern->member_specifier_list, this))
        return false;
    if (node->rbrace_token != pattern->rbrace_token)
        return false;
    return true;
}

bool ASTMatcher::match(CaseStatementAST *node, CaseStatementAST *pattern)
{
    if (node->case_token != pattern->case_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->colon_token != pattern->colon_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(CompoundStatementAST *node, CompoundStatementAST *pattern)
{
    if (node->lbrace_token != pattern->lbrace_token)
        return false;
    if (! AST::match(node->statement_list, pattern->statement_list, this))
        return false;
    if (node->rbrace_token != pattern->rbrace_token)
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
    if (node->question_token != pattern->question_token)
        return false;
    if (! AST::match(node->left_expression, pattern->left_expression, this))
        return false;
    if (node->colon_token != pattern->colon_token)
        return false;
    if (! AST::match(node->right_expression, pattern->right_expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(CppCastExpressionAST *node, CppCastExpressionAST *pattern)
{
    if (node->cast_token != pattern->cast_token)
        return false;
    if (node->less_token != pattern->less_token)
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (node->greater_token != pattern->greater_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(CtorInitializerAST *node, CtorInitializerAST *pattern)
{
    if (node->colon_token != pattern->colon_token)
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
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(FunctionDeclaratorAST *node, FunctionDeclaratorAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->parameters, pattern->parameters, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
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
    if (node->lbracket_token != pattern->lbracket_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rbracket_token != pattern->rbracket_token)
        return false;
    return true;
}

bool ASTMatcher::match(DeleteExpressionAST *node, DeleteExpressionAST *pattern)
{
    if (node->scope_token != pattern->scope_token)
        return false;
    if (node->delete_token != pattern->delete_token)
        return false;
    if (node->lbracket_token != pattern->lbracket_token)
        return false;
    if (node->rbracket_token != pattern->rbracket_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(DoStatementAST *node, DoStatementAST *pattern)
{
    if (node->do_token != pattern->do_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    if (node->while_token != pattern->while_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
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
    if (node->classkey_token != pattern->classkey_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    return true;
}

bool ASTMatcher::match(EnumSpecifierAST *node, EnumSpecifierAST *pattern)
{
    if (node->enum_token != pattern->enum_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->lbrace_token != pattern->lbrace_token)
        return false;
    if (! AST::match(node->enumerator_list, pattern->enumerator_list, this))
        return false;
    if (node->rbrace_token != pattern->rbrace_token)
        return false;
    return true;
}

bool ASTMatcher::match(EnumeratorAST *node, EnumeratorAST *pattern)
{
    if (node->identifier_token != pattern->identifier_token)
        return false;
    if (node->equal_token != pattern->equal_token)
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
    if (node->dot_dot_dot_token != pattern->dot_dot_dot_token)
        return false;
    return true;
}

bool ASTMatcher::match(ExceptionSpecificationAST *node, ExceptionSpecificationAST *pattern)
{
    if (node->throw_token != pattern->throw_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (node->dot_dot_dot_token != pattern->dot_dot_dot_token)
        return false;
    if (! AST::match(node->type_id_list, pattern->type_id_list, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
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
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(FunctionDefinitionAST *node, FunctionDefinitionAST *pattern)
{
    if (node->qt_invokable_token != pattern->qt_invokable_token)
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
    if (node->foreach_token != pattern->foreach_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    if (node->comma_token != pattern->comma_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ForStatementAST *node, ForStatementAST *pattern)
{
    if (node->for_token != pattern->for_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(IfStatementAST *node, IfStatementAST *pattern)
{
    if (node->if_token != pattern->if_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    if (node->else_token != pattern->else_token)
        return false;
    if (! AST::match(node->else_statement, pattern->else_statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ArrayInitializerAST *node, ArrayInitializerAST *pattern)
{
    if (node->lbrace_token != pattern->lbrace_token)
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (node->rbrace_token != pattern->rbrace_token)
        return false;
    return true;
}

bool ASTMatcher::match(LabeledStatementAST *node, LabeledStatementAST *pattern)
{
    if (node->label_token != pattern->label_token)
        return false;
    if (node->colon_token != pattern->colon_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(LinkageBodyAST *node, LinkageBodyAST *pattern)
{
    if (node->lbrace_token != pattern->lbrace_token)
        return false;
    if (! AST::match(node->declaration_list, pattern->declaration_list, this))
        return false;
    if (node->rbrace_token != pattern->rbrace_token)
        return false;
    return true;
}

bool ASTMatcher::match(LinkageSpecificationAST *node, LinkageSpecificationAST *pattern)
{
    if (node->extern_token != pattern->extern_token)
        return false;
    if (node->extern_type_token != pattern->extern_type_token)
        return false;
    if (! AST::match(node->declaration, pattern->declaration, this))
        return false;
    return true;
}

bool ASTMatcher::match(MemInitializerAST *node, MemInitializerAST *pattern)
{
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(NestedNameSpecifierAST *node, NestedNameSpecifierAST *pattern)
{
    if (! AST::match(node->class_or_namespace_name, pattern->class_or_namespace_name, this))
        return false;
    if (node->scope_token != pattern->scope_token)
        return false;
    return true;
}

bool ASTMatcher::match(QualifiedNameAST *node, QualifiedNameAST *pattern)
{
    if (node->global_scope_token != pattern->global_scope_token)
        return false;
    if (! AST::match(node->nested_name_specifier_list, pattern->nested_name_specifier_list, this))
        return false;
    if (! AST::match(node->unqualified_name, pattern->unqualified_name, this))
        return false;
    return true;
}

bool ASTMatcher::match(OperatorFunctionIdAST *node, OperatorFunctionIdAST *pattern)
{
    if (node->operator_token != pattern->operator_token)
        return false;
    if (! AST::match(node->op, pattern->op, this))
        return false;
    return true;
}

bool ASTMatcher::match(ConversionFunctionIdAST *node, ConversionFunctionIdAST *pattern)
{
    if (node->operator_token != pattern->operator_token)
        return false;
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->ptr_operator_list, pattern->ptr_operator_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(SimpleNameAST *node, SimpleNameAST *pattern)
{
    if (node->identifier_token != pattern->identifier_token)
        return false;
    return true;
}

bool ASTMatcher::match(DestructorNameAST *node, DestructorNameAST *pattern)
{
    if (node->tilde_token != pattern->tilde_token)
        return false;
    if (node->identifier_token != pattern->identifier_token)
        return false;
    return true;
}

bool ASTMatcher::match(TemplateIdAST *node, TemplateIdAST *pattern)
{
    if (node->identifier_token != pattern->identifier_token)
        return false;
    if (node->less_token != pattern->less_token)
        return false;
    if (! AST::match(node->template_argument_list, pattern->template_argument_list, this))
        return false;
    if (node->greater_token != pattern->greater_token)
        return false;
    return true;
}

bool ASTMatcher::match(NamespaceAST *node, NamespaceAST *pattern)
{
    if (node->namespace_token != pattern->namespace_token)
        return false;
    if (node->identifier_token != pattern->identifier_token)
        return false;
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (! AST::match(node->linkage_body, pattern->linkage_body, this))
        return false;
    return true;
}

bool ASTMatcher::match(NamespaceAliasDefinitionAST *node, NamespaceAliasDefinitionAST *pattern)
{
    if (node->namespace_token != pattern->namespace_token)
        return false;
    if (node->namespace_name_token != pattern->namespace_name_token)
        return false;
    if (node->equal_token != pattern->equal_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(NewPlacementAST *node, NewPlacementAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(NewArrayDeclaratorAST *node, NewArrayDeclaratorAST *pattern)
{
    if (node->lbracket_token != pattern->lbracket_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rbracket_token != pattern->rbracket_token)
        return false;
    return true;
}

bool ASTMatcher::match(NewExpressionAST *node, NewExpressionAST *pattern)
{
    if (node->scope_token != pattern->scope_token)
        return false;
    if (node->new_token != pattern->new_token)
        return false;
    if (! AST::match(node->new_placement, pattern->new_placement, this))
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->new_type_id, pattern->new_type_id, this))
        return false;
    if (! AST::match(node->new_initializer, pattern->new_initializer, this))
        return false;
    return true;
}

bool ASTMatcher::match(NewInitializerAST *node, NewInitializerAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
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
    if (node->op_token != pattern->op_token)
        return false;
    if (node->open_token != pattern->open_token)
        return false;
    if (node->close_token != pattern->close_token)
        return false;
    return true;
}

bool ASTMatcher::match(ParameterDeclarationAST *node, ParameterDeclarationAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (node->equal_token != pattern->equal_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(ParameterDeclarationClauseAST *node, ParameterDeclarationClauseAST *pattern)
{
    if (! AST::match(node->parameter_declaration_list, pattern->parameter_declaration_list, this))
        return false;
    if (node->dot_dot_dot_token != pattern->dot_dot_dot_token)
        return false;
    return true;
}

bool ASTMatcher::match(CallAST *node, CallAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(ArrayAccessAST *node, ArrayAccessAST *pattern)
{
    if (node->lbracket_token != pattern->lbracket_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rbracket_token != pattern->rbracket_token)
        return false;
    return true;
}

bool ASTMatcher::match(PostIncrDecrAST *node, PostIncrDecrAST *pattern)
{
    if (node->incr_decr_token != pattern->incr_decr_token)
        return false;
    return true;
}

bool ASTMatcher::match(MemberAccessAST *node, MemberAccessAST *pattern)
{
    if (node->access_token != pattern->access_token)
        return false;
    if (node->template_token != pattern->template_token)
        return false;
    if (! AST::match(node->member_name, pattern->member_name, this))
        return false;
    return true;
}

bool ASTMatcher::match(TypeidExpressionAST *node, TypeidExpressionAST *pattern)
{
    if (node->typeid_token != pattern->typeid_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(TypenameCallExpressionAST *node, TypenameCallExpressionAST *pattern)
{
    if (node->typename_token != pattern->typename_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(TypeConstructorCallAST *node, TypeConstructorCallAST *pattern)
{
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression_list, pattern->expression_list, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
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
    if (node->global_scope_token != pattern->global_scope_token)
        return false;
    if (! AST::match(node->nested_name_specifier_list, pattern->nested_name_specifier_list, this))
        return false;
    if (node->star_token != pattern->star_token)
        return false;
    if (! AST::match(node->cv_qualifier_list, pattern->cv_qualifier_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(PointerAST *node, PointerAST *pattern)
{
    if (node->star_token != pattern->star_token)
        return false;
    if (! AST::match(node->cv_qualifier_list, pattern->cv_qualifier_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(ReferenceAST *node, ReferenceAST *pattern)
{
    if (node->amp_token != pattern->amp_token)
        return false;
    return true;
}

bool ASTMatcher::match(BreakStatementAST *node, BreakStatementAST *pattern)
{
    if (node->break_token != pattern->break_token)
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(ContinueStatementAST *node, ContinueStatementAST *pattern)
{
    if (node->continue_token != pattern->continue_token)
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(GotoStatementAST *node, GotoStatementAST *pattern)
{
    if (node->goto_token != pattern->goto_token)
        return false;
    if (node->identifier_token != pattern->identifier_token)
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(ReturnStatementAST *node, ReturnStatementAST *pattern)
{
    if (node->return_token != pattern->return_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(SizeofExpressionAST *node, SizeofExpressionAST *pattern)
{
    if (node->sizeof_token != pattern->sizeof_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(NumericLiteralAST *node, NumericLiteralAST *pattern)
{
    if (node->literal_token != pattern->literal_token)
        return false;
    return true;
}

bool ASTMatcher::match(BoolLiteralAST *node, BoolLiteralAST *pattern)
{
    if (node->literal_token != pattern->literal_token)
        return false;
    return true;
}

bool ASTMatcher::match(ThisExpressionAST *node, ThisExpressionAST *pattern)
{
    if (node->this_token != pattern->this_token)
        return false;
    return true;
}

bool ASTMatcher::match(NestedExpressionAST *node, NestedExpressionAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(StringLiteralAST *node, StringLiteralAST *pattern)
{
    if (node->literal_token != pattern->literal_token)
        return false;
    if (! AST::match(node->next, pattern->next, this))
        return false;
    return true;
}

bool ASTMatcher::match(SwitchStatementAST *node, SwitchStatementAST *pattern)
{
    if (node->switch_token != pattern->switch_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(TemplateDeclarationAST *node, TemplateDeclarationAST *pattern)
{
    if (node->export_token != pattern->export_token)
        return false;
    if (node->template_token != pattern->template_token)
        return false;
    if (node->less_token != pattern->less_token)
        return false;
    if (! AST::match(node->template_parameter_list, pattern->template_parameter_list, this))
        return false;
    if (node->greater_token != pattern->greater_token)
        return false;
    if (! AST::match(node->declaration, pattern->declaration, this))
        return false;
    return true;
}

bool ASTMatcher::match(ThrowExpressionAST *node, ThrowExpressionAST *pattern)
{
    if (node->throw_token != pattern->throw_token)
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
    if (node->try_token != pattern->try_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    if (! AST::match(node->catch_clause_list, pattern->catch_clause_list, this))
        return false;
    return true;
}

bool ASTMatcher::match(CatchClauseAST *node, CatchClauseAST *pattern)
{
    if (node->catch_token != pattern->catch_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->exception_declaration, pattern->exception_declaration, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
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
    if (node->classkey_token != pattern->classkey_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->equal_token != pattern->equal_token)
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    return true;
}

bool ASTMatcher::match(TemplateTypeParameterAST *node, TemplateTypeParameterAST *pattern)
{
    if (node->template_token != pattern->template_token)
        return false;
    if (node->less_token != pattern->less_token)
        return false;
    if (! AST::match(node->template_parameter_list, pattern->template_parameter_list, this))
        return false;
    if (node->greater_token != pattern->greater_token)
        return false;
    if (node->class_token != pattern->class_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->equal_token != pattern->equal_token)
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    return true;
}

bool ASTMatcher::match(UnaryExpressionAST *node, UnaryExpressionAST *pattern)
{
    if (node->unary_op_token != pattern->unary_op_token)
        return false;
    if (! AST::match(node->expression, pattern->expression, this))
        return false;
    return true;
}

bool ASTMatcher::match(UsingAST *node, UsingAST *pattern)
{
    if (node->using_token != pattern->using_token)
        return false;
    if (node->typename_token != pattern->typename_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(UsingDirectiveAST *node, UsingDirectiveAST *pattern)
{
    if (node->using_token != pattern->using_token)
        return false;
    if (node->namespace_token != pattern->namespace_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(WhileStatementAST *node, WhileStatementAST *pattern)
{
    if (node->while_token != pattern->while_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->condition, pattern->condition, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCClassForwardDeclarationAST *node, ObjCClassForwardDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (node->class_token != pattern->class_token)
        return false;
    if (! AST::match(node->identifier_list, pattern->identifier_list, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCClassDeclarationAST *node, ObjCClassDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (node->interface_token != pattern->interface_token)
        return false;
    if (node->implementation_token != pattern->implementation_token)
        return false;
    if (! AST::match(node->class_name, pattern->class_name, this))
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->category_name, pattern->category_name, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (node->colon_token != pattern->colon_token)
        return false;
    if (! AST::match(node->superclass, pattern->superclass, this))
        return false;
    if (! AST::match(node->protocol_refs, pattern->protocol_refs, this))
        return false;
    if (! AST::match(node->inst_vars_decl, pattern->inst_vars_decl, this))
        return false;
    if (! AST::match(node->member_declaration_list, pattern->member_declaration_list, this))
        return false;
    if (node->end_token != pattern->end_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolForwardDeclarationAST *node, ObjCProtocolForwardDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (node->protocol_token != pattern->protocol_token)
        return false;
    if (! AST::match(node->identifier_list, pattern->identifier_list, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolDeclarationAST *node, ObjCProtocolDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (node->protocol_token != pattern->protocol_token)
        return false;
    if (! AST::match(node->name, pattern->name, this))
        return false;
    if (! AST::match(node->protocol_refs, pattern->protocol_refs, this))
        return false;
    if (! AST::match(node->member_declaration_list, pattern->member_declaration_list, this))
        return false;
    if (node->end_token != pattern->end_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolRefsAST *node, ObjCProtocolRefsAST *pattern)
{
    if (node->less_token != pattern->less_token)
        return false;
    if (! AST::match(node->identifier_list, pattern->identifier_list, this))
        return false;
    if (node->greater_token != pattern->greater_token)
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
    if (node->lbracket_token != pattern->lbracket_token)
        return false;
    if (! AST::match(node->receiver_expression, pattern->receiver_expression, this))
        return false;
    if (! AST::match(node->selector, pattern->selector, this))
        return false;
    if (! AST::match(node->argument_list, pattern->argument_list, this))
        return false;
    if (node->rbracket_token != pattern->rbracket_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCProtocolExpressionAST *node, ObjCProtocolExpressionAST *pattern)
{
    if (node->protocol_token != pattern->protocol_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (node->identifier_token != pattern->identifier_token)
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCTypeNameAST *node, ObjCTypeNameAST *pattern)
{
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->type_id, pattern->type_id, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCEncodeExpressionAST *node, ObjCEncodeExpressionAST *pattern)
{
    if (node->encode_token != pattern->encode_token)
        return false;
    if (! AST::match(node->type_name, pattern->type_name, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSelectorWithoutArgumentsAST *node, ObjCSelectorWithoutArgumentsAST *pattern)
{
    if (node->name_token != pattern->name_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSelectorArgumentAST *node, ObjCSelectorArgumentAST *pattern)
{
    if (node->name_token != pattern->name_token)
        return false;
    if (node->colon_token != pattern->colon_token)
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
    if (node->selector_token != pattern->selector_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->selector, pattern->selector, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCInstanceVariablesDeclarationAST *node, ObjCInstanceVariablesDeclarationAST *pattern)
{
    if (node->lbrace_token != pattern->lbrace_token)
        return false;
    if (! AST::match(node->instance_variable_list, pattern->instance_variable_list, this))
        return false;
    if (node->rbrace_token != pattern->rbrace_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCVisibilityDeclarationAST *node, ObjCVisibilityDeclarationAST *pattern)
{
    if (node->visibility_token != pattern->visibility_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCPropertyAttributeAST *node, ObjCPropertyAttributeAST *pattern)
{
    if (node->attribute_identifier_token != pattern->attribute_identifier_token)
        return false;
    if (node->equals_token != pattern->equals_token)
        return false;
    if (! AST::match(node->method_selector, pattern->method_selector, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCPropertyDeclarationAST *node, ObjCPropertyDeclarationAST *pattern)
{
    if (! AST::match(node->attribute_list, pattern->attribute_list, this))
        return false;
    if (node->property_token != pattern->property_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->property_attribute_list, pattern->property_attribute_list, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
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
    if (node->param_name_token != pattern->param_name_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCMethodPrototypeAST *node, ObjCMethodPrototypeAST *pattern)
{
    if (node->method_type_token != pattern->method_type_token)
        return false;
    if (! AST::match(node->type_name, pattern->type_name, this))
        return false;
    if (! AST::match(node->selector, pattern->selector, this))
        return false;
    if (! AST::match(node->argument_list, pattern->argument_list, this))
        return false;
    if (node->dot_dot_dot_token != pattern->dot_dot_dot_token)
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
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSynthesizedPropertyAST *node, ObjCSynthesizedPropertyAST *pattern)
{
    if (node->equals_token != pattern->equals_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSynthesizedPropertiesDeclarationAST *node, ObjCSynthesizedPropertiesDeclarationAST *pattern)
{
    if (node->synthesized_token != pattern->synthesized_token)
        return false;
    if (! AST::match(node->property_identifier_list, pattern->property_identifier_list, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCDynamicPropertiesDeclarationAST *node, ObjCDynamicPropertiesDeclarationAST *pattern)
{
    if (node->dynamic_token != pattern->dynamic_token)
        return false;
    if (! AST::match(node->property_identifier_list, pattern->property_identifier_list, this))
        return false;
    if (node->semicolon_token != pattern->semicolon_token)
        return false;
    return true;
}

bool ASTMatcher::match(ObjCFastEnumerationAST *node, ObjCFastEnumerationAST *pattern)
{
    if (node->for_token != pattern->for_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->type_specifier_list, pattern->type_specifier_list, this))
        return false;
    if (! AST::match(node->declarator, pattern->declarator, this))
        return false;
    if (! AST::match(node->initializer, pattern->initializer, this))
        return false;
    if (node->in_token != pattern->in_token)
        return false;
    if (! AST::match(node->fast_enumeratable_expression, pattern->fast_enumeratable_expression, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->body_statement, pattern->body_statement, this))
        return false;
    return true;
}

bool ASTMatcher::match(ObjCSynchronizedStatementAST *node, ObjCSynchronizedStatementAST *pattern)
{
    if (node->synchronized_token != pattern->synchronized_token)
        return false;
    if (node->lparen_token != pattern->lparen_token)
        return false;
    if (! AST::match(node->synchronized_object, pattern->synchronized_object, this))
        return false;
    if (node->rparen_token != pattern->rparen_token)
        return false;
    if (! AST::match(node->statement, pattern->statement, this))
        return false;
    return true;
}

