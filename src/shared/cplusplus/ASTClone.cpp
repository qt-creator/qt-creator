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
#include "ASTVisitor.h"

using namespace CPlusPlus;

SimpleSpecifierAST *SimpleSpecifierAST::clone(MemoryPool *pool) const
{
    SimpleSpecifierAST *ast = new (pool) SimpleSpecifierAST;
    // copy SpecifierAST
    if (next) ast->next = next->clone(pool);
    // copy SimpleSpecifierAST
    ast->specifier_token = specifier_token;
    return ast;
}

AttributeSpecifierAST *AttributeSpecifierAST::clone(MemoryPool *pool) const
{
    AttributeSpecifierAST *ast = new (pool) AttributeSpecifierAST;
    // copy SpecifierAST
    if (next) ast->next = next->clone(pool);
    // copy AttributeSpecifierAST
    ast->attribute_token = attribute_token;
    ast->first_lparen_token = first_lparen_token;
    ast->second_lparen_token = second_lparen_token;
    if (attributes) ast->attributes = attributes->clone(pool);
    ast->first_rparen_token = first_rparen_token;
    ast->second_rparen_token = second_rparen_token;
    return ast;
}

AttributeAST *AttributeAST::clone(MemoryPool *pool) const
{
    AttributeAST *ast = new (pool) AttributeAST;
    // copy AttributeAST
    ast->identifier_token = identifier_token;
    ast->lparen_token = lparen_token;
    ast->tag_token = tag_token;
    if (expression_list) ast->expression_list = expression_list->clone(pool);
    ast->rparen_token = rparen_token;
    if (next) ast->next = next->clone(pool);
    ast->comma_token = comma_token;
    return ast;
}

TypeofSpecifierAST *TypeofSpecifierAST::clone(MemoryPool *pool) const
{
    TypeofSpecifierAST *ast = new (pool) TypeofSpecifierAST;
    // copy SpecifierAST
    if (next) ast->next = next->clone(pool);
    // copy TypeofSpecifierAST
    ast->typeof_token = typeof_token;
    ast->lparen_token = lparen_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

DeclarationListAST *DeclarationListAST::clone(MemoryPool *pool) const
{
    DeclarationListAST *ast = new (pool) DeclarationListAST;
    // copy DeclarationListAST
    if (declaration) ast->declaration = declaration->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

DeclaratorAST *DeclaratorAST::clone(MemoryPool *pool) const
{
    DeclaratorAST *ast = new (pool) DeclaratorAST;
    // copy DeclaratorAST
    if (attributes) ast->attributes = attributes->clone(pool);
    if (ptr_operators) ast->ptr_operators = ptr_operators->clone(pool);
    if (core_declarator) ast->core_declarator = core_declarator->clone(pool);
    if (postfix_declarators) ast->postfix_declarators = postfix_declarators->clone(pool);
    if (post_attributes) ast->post_attributes = post_attributes->clone(pool);
    ast->equals_token = equals_token;
    if (initializer) ast->initializer = initializer->clone(pool);
    return ast;
}

ExpressionListAST *ExpressionListAST::clone(MemoryPool *pool) const
{
    ExpressionListAST *ast = new (pool) ExpressionListAST;
    // copy ExpressionListAST
    ast->comma_token = comma_token;
    if (expression) ast->expression = expression->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

SimpleDeclarationAST *SimpleDeclarationAST::clone(MemoryPool *pool) const
{
    SimpleDeclarationAST *ast = new (pool) SimpleDeclarationAST;
    // copy DeclarationAST
    // copy SimpleDeclarationAST
    ast->qt_invokable_token = qt_invokable_token;
    if (decl_specifier_seq) ast->decl_specifier_seq = decl_specifier_seq->clone(pool);
    if (declarators) ast->declarators = declarators->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

EmptyDeclarationAST *EmptyDeclarationAST::clone(MemoryPool *pool) const
{
    EmptyDeclarationAST *ast = new (pool) EmptyDeclarationAST;
    // copy DeclarationAST
    // copy EmptyDeclarationAST
    ast->semicolon_token = semicolon_token;
    return ast;
}

AccessDeclarationAST *AccessDeclarationAST::clone(MemoryPool *pool) const
{
    AccessDeclarationAST *ast = new (pool) AccessDeclarationAST;
    // copy DeclarationAST
    // copy AccessDeclarationAST
    ast->access_specifier_token = access_specifier_token;
    ast->slots_token = slots_token;
    ast->colon_token = colon_token;
    return ast;
}

AsmDefinitionAST *AsmDefinitionAST::clone(MemoryPool *pool) const
{
    AsmDefinitionAST *ast = new (pool) AsmDefinitionAST;
    // copy DeclarationAST
    // copy AsmDefinitionAST
    ast->asm_token = asm_token;
    ast->volatile_token = volatile_token;
    ast->lparen_token = lparen_token;
    ast->rparen_token = rparen_token;
    ast->semicolon_token = semicolon_token;
    return ast;
}

BaseSpecifierAST *BaseSpecifierAST::clone(MemoryPool *pool) const
{
    BaseSpecifierAST *ast = new (pool) BaseSpecifierAST;
    // copy BaseSpecifierAST
    ast->comma_token = comma_token;
    ast->virtual_token = virtual_token;
    ast->access_specifier_token = access_specifier_token;
    if (name) ast->name = name->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

CompoundLiteralAST *CompoundLiteralAST::clone(MemoryPool *pool) const
{
    CompoundLiteralAST *ast = new (pool) CompoundLiteralAST;
    // copy ExpressionAST
    // copy CompoundLiteralAST
    ast->lparen_token = lparen_token;
    if (type_id) ast->type_id = type_id->clone(pool);
    ast->rparen_token = rparen_token;
    if (initializer) ast->initializer = initializer->clone(pool);
    return ast;
}

QtMethodAST *QtMethodAST::clone(MemoryPool *pool) const
{
    QtMethodAST *ast = new (pool) QtMethodAST;
    // copy ExpressionAST
    // copy QtMethodAST
    ast->method_token = method_token;
    ast->lparen_token = lparen_token;
    if (declarator) ast->declarator = declarator->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

BinaryExpressionAST *BinaryExpressionAST::clone(MemoryPool *pool) const
{
    BinaryExpressionAST *ast = new (pool) BinaryExpressionAST;
    // copy ExpressionAST
    // copy BinaryExpressionAST
    if (left_expression) ast->left_expression = left_expression->clone(pool);
    ast->binary_op_token = binary_op_token;
    if (right_expression) ast->right_expression = right_expression->clone(pool);
    return ast;
}

CastExpressionAST *CastExpressionAST::clone(MemoryPool *pool) const
{
    CastExpressionAST *ast = new (pool) CastExpressionAST;
    // copy ExpressionAST
    // copy CastExpressionAST
    ast->lparen_token = lparen_token;
    if (type_id) ast->type_id = type_id->clone(pool);
    ast->rparen_token = rparen_token;
    if (expression) ast->expression = expression->clone(pool);
    return ast;
}

ClassSpecifierAST *ClassSpecifierAST::clone(MemoryPool *pool) const
{
    ClassSpecifierAST *ast = new (pool) ClassSpecifierAST;
    // copy SpecifierAST
    if (next) ast->next = next->clone(pool);
    // copy ClassSpecifierAST
    ast->classkey_token = classkey_token;
    if (attributes) ast->attributes = attributes->clone(pool);
    if (name) ast->name = name->clone(pool);
    ast->colon_token = colon_token;
    if (base_clause) ast->base_clause = base_clause->clone(pool);
    ast->lbrace_token = lbrace_token;
    if (member_specifiers) ast->member_specifiers = member_specifiers->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
}

CaseStatementAST *CaseStatementAST::clone(MemoryPool *pool) const
{
    CaseStatementAST *ast = new (pool) CaseStatementAST;
    // copy StatementAST
    // copy CaseStatementAST
    ast->case_token = case_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->colon_token = colon_token;
    if (statement) ast->statement = statement->clone(pool);
    return ast;
}

StatementListAST *StatementListAST::clone(MemoryPool *pool) const
{
    StatementListAST *ast = new (pool) StatementListAST;
    // copy StatementListAST
    if (statement) ast->statement = statement->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

CompoundStatementAST *CompoundStatementAST::clone(MemoryPool *pool) const
{
    CompoundStatementAST *ast = new (pool) CompoundStatementAST;
    // copy StatementAST
    // copy CompoundStatementAST
    ast->lbrace_token = lbrace_token;
    if (statements) ast->statements = statements->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
}

ConditionAST *ConditionAST::clone(MemoryPool *pool) const
{
    ConditionAST *ast = new (pool) ConditionAST;
    // copy ExpressionAST
    // copy ConditionAST
    if (type_specifier) ast->type_specifier = type_specifier->clone(pool);
    if (declarator) ast->declarator = declarator->clone(pool);
    return ast;
}

ConditionalExpressionAST *ConditionalExpressionAST::clone(MemoryPool *pool) const
{
    ConditionalExpressionAST *ast = new (pool) ConditionalExpressionAST;
    // copy ExpressionAST
    // copy ConditionalExpressionAST
    if (condition) ast->condition = condition->clone(pool);
    ast->question_token = question_token;
    if (left_expression) ast->left_expression = left_expression->clone(pool);
    ast->colon_token = colon_token;
    if (right_expression) ast->right_expression = right_expression->clone(pool);
    return ast;
}

CppCastExpressionAST *CppCastExpressionAST::clone(MemoryPool *pool) const
{
    CppCastExpressionAST *ast = new (pool) CppCastExpressionAST;
    // copy ExpressionAST
    // copy CppCastExpressionAST
    ast->cast_token = cast_token;
    ast->less_token = less_token;
    if (type_id) ast->type_id = type_id->clone(pool);
    ast->greater_token = greater_token;
    ast->lparen_token = lparen_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

CtorInitializerAST *CtorInitializerAST::clone(MemoryPool *pool) const
{
    CtorInitializerAST *ast = new (pool) CtorInitializerAST;
    // copy CtorInitializerAST
    ast->colon_token = colon_token;
    if (member_initializers) ast->member_initializers = member_initializers->clone(pool);
    return ast;
}

DeclarationStatementAST *DeclarationStatementAST::clone(MemoryPool *pool) const
{
    DeclarationStatementAST *ast = new (pool) DeclarationStatementAST;
    // copy StatementAST
    // copy DeclarationStatementAST
    if (declaration) ast->declaration = declaration->clone(pool);
    return ast;
}

DeclaratorIdAST *DeclaratorIdAST::clone(MemoryPool *pool) const
{
    DeclaratorIdAST *ast = new (pool) DeclaratorIdAST;
    // copy CoreDeclaratorAST
    // copy DeclaratorIdAST
    if (name) ast->name = name->clone(pool);
    return ast;
}

NestedDeclaratorAST *NestedDeclaratorAST::clone(MemoryPool *pool) const
{
    NestedDeclaratorAST *ast = new (pool) NestedDeclaratorAST;
    // copy CoreDeclaratorAST
    // copy NestedDeclaratorAST
    ast->lparen_token = lparen_token;
    if (declarator) ast->declarator = declarator->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

FunctionDeclaratorAST *FunctionDeclaratorAST::clone(MemoryPool *pool) const
{
    FunctionDeclaratorAST *ast = new (pool) FunctionDeclaratorAST;
    // copy PostfixDeclaratorAST
    if (next) ast->next = next->clone(pool);
    // copy FunctionDeclaratorAST
    ast->lparen_token = lparen_token;
    if (parameters) ast->parameters = parameters->clone(pool);
    ast->rparen_token = rparen_token;
    if (cv_qualifier_seq) ast->cv_qualifier_seq = cv_qualifier_seq->clone(pool);
    if (exception_specification) ast->exception_specification = exception_specification->clone(pool);
    if (as_cpp_initializer) ast->as_cpp_initializer = as_cpp_initializer->clone(pool);
    return ast;
}

ArrayDeclaratorAST *ArrayDeclaratorAST::clone(MemoryPool *pool) const
{
    ArrayDeclaratorAST *ast = new (pool) ArrayDeclaratorAST;
    // copy PostfixDeclaratorAST
    if (next) ast->next = next->clone(pool);
    // copy ArrayDeclaratorAST
    ast->lbracket_token = lbracket_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rbracket_token = rbracket_token;
    return ast;
}

DeclaratorListAST *DeclaratorListAST::clone(MemoryPool *pool) const
{
    DeclaratorListAST *ast = new (pool) DeclaratorListAST;
    // copy DeclaratorListAST
    ast->comma_token = comma_token;
    if (declarator) ast->declarator = declarator->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

DeleteExpressionAST *DeleteExpressionAST::clone(MemoryPool *pool) const
{
    DeleteExpressionAST *ast = new (pool) DeleteExpressionAST;
    // copy ExpressionAST
    // copy DeleteExpressionAST
    ast->scope_token = scope_token;
    ast->delete_token = delete_token;
    ast->lbracket_token = lbracket_token;
    ast->rbracket_token = rbracket_token;
    if (expression) ast->expression = expression->clone(pool);
    return ast;
}

DoStatementAST *DoStatementAST::clone(MemoryPool *pool) const
{
    DoStatementAST *ast = new (pool) DoStatementAST;
    // copy StatementAST
    // copy DoStatementAST
    ast->do_token = do_token;
    if (statement) ast->statement = statement->clone(pool);
    ast->while_token = while_token;
    ast->lparen_token = lparen_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    ast->semicolon_token = semicolon_token;
    return ast;
}

NamedTypeSpecifierAST *NamedTypeSpecifierAST::clone(MemoryPool *pool) const
{
    NamedTypeSpecifierAST *ast = new (pool) NamedTypeSpecifierAST;
    // copy SpecifierAST
    if (next) ast->next = next->clone(pool);
    // copy NamedTypeSpecifierAST
    if (name) ast->name = name->clone(pool);
    return ast;
}

ElaboratedTypeSpecifierAST *ElaboratedTypeSpecifierAST::clone(MemoryPool *pool) const
{
    ElaboratedTypeSpecifierAST *ast = new (pool) ElaboratedTypeSpecifierAST;
    // copy SpecifierAST
    if (next) ast->next = next->clone(pool);
    // copy ElaboratedTypeSpecifierAST
    ast->classkey_token = classkey_token;
    if (name) ast->name = name->clone(pool);
    return ast;
}

EnumSpecifierAST *EnumSpecifierAST::clone(MemoryPool *pool) const
{
    EnumSpecifierAST *ast = new (pool) EnumSpecifierAST;
    // copy SpecifierAST
    if (next) ast->next = next->clone(pool);
    // copy EnumSpecifierAST
    ast->enum_token = enum_token;
    if (name) ast->name = name->clone(pool);
    ast->lbrace_token = lbrace_token;
    if (enumerators) ast->enumerators = enumerators->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
}

EnumeratorAST *EnumeratorAST::clone(MemoryPool *pool) const
{
    EnumeratorAST *ast = new (pool) EnumeratorAST;
    // copy EnumeratorAST
    ast->comma_token = comma_token;
    ast->identifier_token = identifier_token;
    ast->equal_token = equal_token;
    if (expression) ast->expression = expression->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

ExceptionDeclarationAST *ExceptionDeclarationAST::clone(MemoryPool *pool) const
{
    ExceptionDeclarationAST *ast = new (pool) ExceptionDeclarationAST;
    // copy DeclarationAST
    // copy ExceptionDeclarationAST
    if (type_specifier) ast->type_specifier = type_specifier->clone(pool);
    if (declarator) ast->declarator = declarator->clone(pool);
    ast->dot_dot_dot_token = dot_dot_dot_token;
    return ast;
}

ExceptionSpecificationAST *ExceptionSpecificationAST::clone(MemoryPool *pool) const
{
    ExceptionSpecificationAST *ast = new (pool) ExceptionSpecificationAST;
    // copy ExceptionSpecificationAST
    ast->throw_token = throw_token;
    ast->lparen_token = lparen_token;
    ast->dot_dot_dot_token = dot_dot_dot_token;
    if (type_ids) ast->type_ids = type_ids->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

ExpressionOrDeclarationStatementAST *ExpressionOrDeclarationStatementAST::clone(MemoryPool *pool) const
{
    ExpressionOrDeclarationStatementAST *ast = new (pool) ExpressionOrDeclarationStatementAST;
    // copy StatementAST
    // copy ExpressionOrDeclarationStatementAST
    if (expression) ast->expression = expression->clone(pool);
    if (declaration) ast->declaration = declaration->clone(pool);
    return ast;
}

ExpressionStatementAST *ExpressionStatementAST::clone(MemoryPool *pool) const
{
    ExpressionStatementAST *ast = new (pool) ExpressionStatementAST;
    // copy StatementAST
    // copy ExpressionStatementAST
    if (expression) ast->expression = expression->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

FunctionDefinitionAST *FunctionDefinitionAST::clone(MemoryPool *pool) const
{
    FunctionDefinitionAST *ast = new (pool) FunctionDefinitionAST;
    // copy DeclarationAST
    // copy FunctionDefinitionAST
    ast->qt_invokable_token = qt_invokable_token;
    if (decl_specifier_seq) ast->decl_specifier_seq = decl_specifier_seq->clone(pool);
    if (declarator) ast->declarator = declarator->clone(pool);
    if (ctor_initializer) ast->ctor_initializer = ctor_initializer->clone(pool);
    if (function_body) ast->function_body = function_body->clone(pool);
    return ast;
}

ForeachStatementAST *ForeachStatementAST::clone(MemoryPool *pool) const
{
    ForeachStatementAST *ast = new (pool) ForeachStatementAST;
    // copy StatementAST
    // copy ForeachStatementAST
    ast->foreach_token = foreach_token;
    ast->lparen_token = lparen_token;
    if (type_specifiers) ast->type_specifiers = type_specifiers->clone(pool);
    if (declarator) ast->declarator = declarator->clone(pool);
    if (initializer) ast->initializer = initializer->clone(pool);
    ast->comma_token = comma_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement) ast->statement = statement->clone(pool);
    return ast;
}

ForStatementAST *ForStatementAST::clone(MemoryPool *pool) const
{
    ForStatementAST *ast = new (pool) ForStatementAST;
    // copy StatementAST
    // copy ForStatementAST
    ast->for_token = for_token;
    ast->lparen_token = lparen_token;
    if (initializer) ast->initializer = initializer->clone(pool);
    if (condition) ast->condition = condition->clone(pool);
    ast->semicolon_token = semicolon_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement) ast->statement = statement->clone(pool);
    return ast;
}

IfStatementAST *IfStatementAST::clone(MemoryPool *pool) const
{
    IfStatementAST *ast = new (pool) IfStatementAST;
    // copy StatementAST
    // copy IfStatementAST
    ast->if_token = if_token;
    ast->lparen_token = lparen_token;
    if (condition) ast->condition = condition->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement) ast->statement = statement->clone(pool);
    ast->else_token = else_token;
    if (else_statement) ast->else_statement = else_statement->clone(pool);
    return ast;
}

ArrayInitializerAST *ArrayInitializerAST::clone(MemoryPool *pool) const
{
    ArrayInitializerAST *ast = new (pool) ArrayInitializerAST;
    // copy ExpressionAST
    // copy ArrayInitializerAST
    ast->lbrace_token = lbrace_token;
    if (expression_list) ast->expression_list = expression_list->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
}

LabeledStatementAST *LabeledStatementAST::clone(MemoryPool *pool) const
{
    LabeledStatementAST *ast = new (pool) LabeledStatementAST;
    // copy StatementAST
    // copy LabeledStatementAST
    ast->label_token = label_token;
    ast->colon_token = colon_token;
    if (statement) ast->statement = statement->clone(pool);
    return ast;
}

LinkageBodyAST *LinkageBodyAST::clone(MemoryPool *pool) const
{
    LinkageBodyAST *ast = new (pool) LinkageBodyAST;
    // copy DeclarationAST
    // copy LinkageBodyAST
    ast->lbrace_token = lbrace_token;
    if (declarations) ast->declarations = declarations->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
}

LinkageSpecificationAST *LinkageSpecificationAST::clone(MemoryPool *pool) const
{
    LinkageSpecificationAST *ast = new (pool) LinkageSpecificationAST;
    // copy DeclarationAST
    // copy LinkageSpecificationAST
    ast->extern_token = extern_token;
    ast->extern_type_token = extern_type_token;
    if (declaration) ast->declaration = declaration->clone(pool);
    return ast;
}

MemInitializerAST *MemInitializerAST::clone(MemoryPool *pool) const
{
    MemInitializerAST *ast = new (pool) MemInitializerAST;
    // copy MemInitializerAST
    ast->comma_token = comma_token;
    if (name) ast->name = name->clone(pool);
    ast->lparen_token = lparen_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    if (next) ast->next = next->clone(pool);
    return ast;
}

NestedNameSpecifierAST *NestedNameSpecifierAST::clone(MemoryPool *pool) const
{
    NestedNameSpecifierAST *ast = new (pool) NestedNameSpecifierAST;
    // copy NestedNameSpecifierAST
    if (class_or_namespace_name) ast->class_or_namespace_name = class_or_namespace_name->clone(pool);
    ast->scope_token = scope_token;
    if (next) ast->next = next->clone(pool);
    return ast;
}

QualifiedNameAST *QualifiedNameAST::clone(MemoryPool *pool) const
{
    QualifiedNameAST *ast = new (pool) QualifiedNameAST;
    // copy ExpressionAST
    // copy NameAST
    // copy QualifiedNameAST
    ast->global_scope_token = global_scope_token;
    if (nested_name_specifier) ast->nested_name_specifier = nested_name_specifier->clone(pool);
    if (unqualified_name) ast->unqualified_name = unqualified_name->clone(pool);
    return ast;
}

OperatorFunctionIdAST *OperatorFunctionIdAST::clone(MemoryPool *pool) const
{
    OperatorFunctionIdAST *ast = new (pool) OperatorFunctionIdAST;
    // copy ExpressionAST
    // copy NameAST
    // copy OperatorFunctionIdAST
    ast->operator_token = operator_token;
    if (op) ast->op = op->clone(pool);
    return ast;
}

ConversionFunctionIdAST *ConversionFunctionIdAST::clone(MemoryPool *pool) const
{
    ConversionFunctionIdAST *ast = new (pool) ConversionFunctionIdAST;
    // copy ExpressionAST
    // copy NameAST
    // copy ConversionFunctionIdAST
    ast->operator_token = operator_token;
    if (type_specifier) ast->type_specifier = type_specifier->clone(pool);
    if (ptr_operators) ast->ptr_operators = ptr_operators->clone(pool);
    return ast;
}

SimpleNameAST *SimpleNameAST::clone(MemoryPool *pool) const
{
    SimpleNameAST *ast = new (pool) SimpleNameAST;
    // copy ExpressionAST
    // copy NameAST
    // copy SimpleNameAST
    ast->identifier_token = identifier_token;
    return ast;
}

DestructorNameAST *DestructorNameAST::clone(MemoryPool *pool) const
{
    DestructorNameAST *ast = new (pool) DestructorNameAST;
    // copy ExpressionAST
    // copy NameAST
    // copy DestructorNameAST
    ast->tilde_token = tilde_token;
    ast->identifier_token = identifier_token;
    return ast;
}

TemplateIdAST *TemplateIdAST::clone(MemoryPool *pool) const
{
    TemplateIdAST *ast = new (pool) TemplateIdAST;
    // copy ExpressionAST
    // copy NameAST
    // copy TemplateIdAST
    ast->identifier_token = identifier_token;
    ast->less_token = less_token;
    if (template_arguments) ast->template_arguments = template_arguments->clone(pool);
    ast->greater_token = greater_token;
    return ast;
}

NamespaceAST *NamespaceAST::clone(MemoryPool *pool) const
{
    NamespaceAST *ast = new (pool) NamespaceAST;
    // copy DeclarationAST
    // copy NamespaceAST
    ast->namespace_token = namespace_token;
    ast->identifier_token = identifier_token;
    if (attributes) ast->attributes = attributes->clone(pool);
    if (linkage_body) ast->linkage_body = linkage_body->clone(pool);
    return ast;
}

NamespaceAliasDefinitionAST *NamespaceAliasDefinitionAST::clone(MemoryPool *pool) const
{
    NamespaceAliasDefinitionAST *ast = new (pool) NamespaceAliasDefinitionAST;
    // copy DeclarationAST
    // copy NamespaceAliasDefinitionAST
    ast->namespace_token = namespace_token;
    ast->namespace_name_token = namespace_name_token;
    ast->equal_token = equal_token;
    if (name) ast->name = name->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

NewPlacementAST *NewPlacementAST::clone(MemoryPool *pool) const
{
    NewPlacementAST *ast = new (pool) NewPlacementAST;
    // copy NewPlacementAST
    ast->lparen_token = lparen_token;
    if (expression_list) ast->expression_list = expression_list->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

NewArrayDeclaratorAST *NewArrayDeclaratorAST::clone(MemoryPool *pool) const
{
    NewArrayDeclaratorAST *ast = new (pool) NewArrayDeclaratorAST;
    // copy NewArrayDeclaratorAST
    ast->lbracket_token = lbracket_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rbracket_token = rbracket_token;
    if (next) ast->next = next->clone(pool);
    return ast;
}

NewExpressionAST *NewExpressionAST::clone(MemoryPool *pool) const
{
    NewExpressionAST *ast = new (pool) NewExpressionAST;
    // copy ExpressionAST
    // copy NewExpressionAST
    ast->scope_token = scope_token;
    ast->new_token = new_token;
    if (new_placement) ast->new_placement = new_placement->clone(pool);
    ast->lparen_token = lparen_token;
    if (type_id) ast->type_id = type_id->clone(pool);
    ast->rparen_token = rparen_token;
    if (new_type_id) ast->new_type_id = new_type_id->clone(pool);
    if (new_initializer) ast->new_initializer = new_initializer->clone(pool);
    return ast;
}

NewInitializerAST *NewInitializerAST::clone(MemoryPool *pool) const
{
    NewInitializerAST *ast = new (pool) NewInitializerAST;
    // copy NewInitializerAST
    ast->lparen_token = lparen_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

NewTypeIdAST *NewTypeIdAST::clone(MemoryPool *pool) const
{
    NewTypeIdAST *ast = new (pool) NewTypeIdAST;
    // copy NewTypeIdAST
    if (type_specifier) ast->type_specifier = type_specifier->clone(pool);
    if (ptr_operators) ast->ptr_operators = ptr_operators->clone(pool);
    if (new_array_declarators) ast->new_array_declarators = new_array_declarators->clone(pool);
    return ast;
}

OperatorAST *OperatorAST::clone(MemoryPool *pool) const
{
    OperatorAST *ast = new (pool) OperatorAST;
    // copy OperatorAST
    ast->op_token = op_token;
    ast->open_token = open_token;
    ast->close_token = close_token;
    return ast;
}

ParameterDeclarationAST *ParameterDeclarationAST::clone(MemoryPool *pool) const
{
    ParameterDeclarationAST *ast = new (pool) ParameterDeclarationAST;
    // copy DeclarationAST
    // copy ParameterDeclarationAST
    if (type_specifier) ast->type_specifier = type_specifier->clone(pool);
    if (declarator) ast->declarator = declarator->clone(pool);
    ast->equal_token = equal_token;
    if (expression) ast->expression = expression->clone(pool);
    return ast;
}

ParameterDeclarationClauseAST *ParameterDeclarationClauseAST::clone(MemoryPool *pool) const
{
    ParameterDeclarationClauseAST *ast = new (pool) ParameterDeclarationClauseAST;
    // copy ParameterDeclarationClauseAST
    if (parameter_declarations) ast->parameter_declarations = parameter_declarations->clone(pool);
    ast->dot_dot_dot_token = dot_dot_dot_token;
    return ast;
}

CallAST *CallAST::clone(MemoryPool *pool) const
{
    CallAST *ast = new (pool) CallAST;
    // copy PostfixAST
    if (next) ast->next = next->clone(pool);
    // copy CallAST
    ast->lparen_token = lparen_token;
    if (expression_list) ast->expression_list = expression_list->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

ArrayAccessAST *ArrayAccessAST::clone(MemoryPool *pool) const
{
    ArrayAccessAST *ast = new (pool) ArrayAccessAST;
    // copy PostfixAST
    if (next) ast->next = next->clone(pool);
    // copy ArrayAccessAST
    ast->lbracket_token = lbracket_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rbracket_token = rbracket_token;
    return ast;
}

PostIncrDecrAST *PostIncrDecrAST::clone(MemoryPool *pool) const
{
    PostIncrDecrAST *ast = new (pool) PostIncrDecrAST;
    // copy PostfixAST
    if (next) ast->next = next->clone(pool);
    // copy PostIncrDecrAST
    ast->incr_decr_token = incr_decr_token;
    return ast;
}

MemberAccessAST *MemberAccessAST::clone(MemoryPool *pool) const
{
    MemberAccessAST *ast = new (pool) MemberAccessAST;
    // copy PostfixAST
    if (next) ast->next = next->clone(pool);
    // copy MemberAccessAST
    ast->access_token = access_token;
    ast->template_token = template_token;
    if (member_name) ast->member_name = member_name->clone(pool);
    return ast;
}

TypeidExpressionAST *TypeidExpressionAST::clone(MemoryPool *pool) const
{
    TypeidExpressionAST *ast = new (pool) TypeidExpressionAST;
    // copy ExpressionAST
    // copy TypeidExpressionAST
    ast->typeid_token = typeid_token;
    ast->lparen_token = lparen_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

TypenameCallExpressionAST *TypenameCallExpressionAST::clone(MemoryPool *pool) const
{
    TypenameCallExpressionAST *ast = new (pool) TypenameCallExpressionAST;
    // copy ExpressionAST
    // copy TypenameCallExpressionAST
    ast->typename_token = typename_token;
    if (name) ast->name = name->clone(pool);
    ast->lparen_token = lparen_token;
    if (expression_list) ast->expression_list = expression_list->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

TypeConstructorCallAST *TypeConstructorCallAST::clone(MemoryPool *pool) const
{
    TypeConstructorCallAST *ast = new (pool) TypeConstructorCallAST;
    // copy ExpressionAST
    // copy TypeConstructorCallAST
    if (type_specifier) ast->type_specifier = type_specifier->clone(pool);
    ast->lparen_token = lparen_token;
    if (expression_list) ast->expression_list = expression_list->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

PostfixExpressionAST *PostfixExpressionAST::clone(MemoryPool *pool) const
{
    PostfixExpressionAST *ast = new (pool) PostfixExpressionAST;
    // copy ExpressionAST
    // copy PostfixExpressionAST
    if (base_expression) ast->base_expression = base_expression->clone(pool);
    if (postfix_expressions) ast->postfix_expressions = postfix_expressions->clone(pool);
    return ast;
}

PointerToMemberAST *PointerToMemberAST::clone(MemoryPool *pool) const
{
    PointerToMemberAST *ast = new (pool) PointerToMemberAST;
    // copy PtrOperatorAST
    if (next) ast->next = next->clone(pool);
    // copy PointerToMemberAST
    ast->global_scope_token = global_scope_token;
    if (nested_name_specifier) ast->nested_name_specifier = nested_name_specifier->clone(pool);
    ast->star_token = star_token;
    if (cv_qualifier_seq) ast->cv_qualifier_seq = cv_qualifier_seq->clone(pool);
    return ast;
}

PointerAST *PointerAST::clone(MemoryPool *pool) const
{
    PointerAST *ast = new (pool) PointerAST;
    // copy PtrOperatorAST
    if (next) ast->next = next->clone(pool);
    // copy PointerAST
    ast->star_token = star_token;
    if (cv_qualifier_seq) ast->cv_qualifier_seq = cv_qualifier_seq->clone(pool);
    return ast;
}

ReferenceAST *ReferenceAST::clone(MemoryPool *pool) const
{
    ReferenceAST *ast = new (pool) ReferenceAST;
    // copy PtrOperatorAST
    if (next) ast->next = next->clone(pool);
    // copy ReferenceAST
    ast->amp_token = amp_token;
    return ast;
}

BreakStatementAST *BreakStatementAST::clone(MemoryPool *pool) const
{
    BreakStatementAST *ast = new (pool) BreakStatementAST;
    // copy StatementAST
    // copy BreakStatementAST
    ast->break_token = break_token;
    ast->semicolon_token = semicolon_token;
    return ast;
}

ContinueStatementAST *ContinueStatementAST::clone(MemoryPool *pool) const
{
    ContinueStatementAST *ast = new (pool) ContinueStatementAST;
    // copy StatementAST
    // copy ContinueStatementAST
    ast->continue_token = continue_token;
    ast->semicolon_token = semicolon_token;
    return ast;
}

GotoStatementAST *GotoStatementAST::clone(MemoryPool *pool) const
{
    GotoStatementAST *ast = new (pool) GotoStatementAST;
    // copy StatementAST
    // copy GotoStatementAST
    ast->goto_token = goto_token;
    ast->identifier_token = identifier_token;
    ast->semicolon_token = semicolon_token;
    return ast;
}

ReturnStatementAST *ReturnStatementAST::clone(MemoryPool *pool) const
{
    ReturnStatementAST *ast = new (pool) ReturnStatementAST;
    // copy StatementAST
    // copy ReturnStatementAST
    ast->return_token = return_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

SizeofExpressionAST *SizeofExpressionAST::clone(MemoryPool *pool) const
{
    SizeofExpressionAST *ast = new (pool) SizeofExpressionAST;
    // copy ExpressionAST
    // copy SizeofExpressionAST
    ast->sizeof_token = sizeof_token;
    if (expression) ast->expression = expression->clone(pool);
    return ast;
}

NumericLiteralAST *NumericLiteralAST::clone(MemoryPool *pool) const
{
    NumericLiteralAST *ast = new (pool) NumericLiteralAST;
    // copy ExpressionAST
    // copy NumericLiteralAST
    ast->literal_token = literal_token;
    return ast;
}

BoolLiteralAST *BoolLiteralAST::clone(MemoryPool *pool) const
{
    BoolLiteralAST *ast = new (pool) BoolLiteralAST;
    // copy ExpressionAST
    // copy BoolLiteralAST
    ast->literal_token = literal_token;
    return ast;
}

ThisExpressionAST *ThisExpressionAST::clone(MemoryPool *pool) const
{
    ThisExpressionAST *ast = new (pool) ThisExpressionAST;
    // copy ExpressionAST
    // copy ThisExpressionAST
    ast->this_token = this_token;
    return ast;
}

NestedExpressionAST *NestedExpressionAST::clone(MemoryPool *pool) const
{
    NestedExpressionAST *ast = new (pool) NestedExpressionAST;
    // copy ExpressionAST
    // copy NestedExpressionAST
    ast->lparen_token = lparen_token;
    if (expression) ast->expression = expression->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

StringLiteralAST *StringLiteralAST::clone(MemoryPool *pool) const
{
    StringLiteralAST *ast = new (pool) StringLiteralAST;
    // copy ExpressionAST
    // copy StringLiteralAST
    ast->literal_token = literal_token;
    if (next) ast->next = next->clone(pool);
    return ast;
}

SwitchStatementAST *SwitchStatementAST::clone(MemoryPool *pool) const
{
    SwitchStatementAST *ast = new (pool) SwitchStatementAST;
    // copy StatementAST
    // copy SwitchStatementAST
    ast->switch_token = switch_token;
    ast->lparen_token = lparen_token;
    if (condition) ast->condition = condition->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement) ast->statement = statement->clone(pool);
    return ast;
}

TemplateArgumentListAST *TemplateArgumentListAST::clone(MemoryPool *pool) const
{
    TemplateArgumentListAST *ast = new (pool) TemplateArgumentListAST;
    // copy TemplateArgumentListAST
    ast->comma_token = comma_token;
    if (template_argument) ast->template_argument = template_argument->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

TemplateDeclarationAST *TemplateDeclarationAST::clone(MemoryPool *pool) const
{
    TemplateDeclarationAST *ast = new (pool) TemplateDeclarationAST;
    // copy DeclarationAST
    // copy TemplateDeclarationAST
    ast->export_token = export_token;
    ast->template_token = template_token;
    ast->less_token = less_token;
    if (template_parameters) ast->template_parameters = template_parameters->clone(pool);
    ast->greater_token = greater_token;
    if (declaration) ast->declaration = declaration->clone(pool);
    return ast;
}

ThrowExpressionAST *ThrowExpressionAST::clone(MemoryPool *pool) const
{
    ThrowExpressionAST *ast = new (pool) ThrowExpressionAST;
    // copy ExpressionAST
    // copy ThrowExpressionAST
    ast->throw_token = throw_token;
    if (expression) ast->expression = expression->clone(pool);
    return ast;
}

TranslationUnitAST *TranslationUnitAST::clone(MemoryPool *pool) const
{
    TranslationUnitAST *ast = new (pool) TranslationUnitAST;
    // copy TranslationUnitAST
    if (declarations) ast->declarations = declarations->clone(pool);
    return ast;
}

TryBlockStatementAST *TryBlockStatementAST::clone(MemoryPool *pool) const
{
    TryBlockStatementAST *ast = new (pool) TryBlockStatementAST;
    // copy StatementAST
    // copy TryBlockStatementAST
    ast->try_token = try_token;
    if (statement) ast->statement = statement->clone(pool);
    if (catch_clause_seq) ast->catch_clause_seq = catch_clause_seq->clone(pool);
    return ast;
}

CatchClauseAST *CatchClauseAST::clone(MemoryPool *pool) const
{
    CatchClauseAST *ast = new (pool) CatchClauseAST;
    // copy StatementAST
    // copy CatchClauseAST
    ast->catch_token = catch_token;
    ast->lparen_token = lparen_token;
    if (exception_declaration) ast->exception_declaration = exception_declaration->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement) ast->statement = statement->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

TypeIdAST *TypeIdAST::clone(MemoryPool *pool) const
{
    TypeIdAST *ast = new (pool) TypeIdAST;
    // copy ExpressionAST
    // copy TypeIdAST
    if (type_specifier) ast->type_specifier = type_specifier->clone(pool);
    if (declarator) ast->declarator = declarator->clone(pool);
    return ast;
}

TypenameTypeParameterAST *TypenameTypeParameterAST::clone(MemoryPool *pool) const
{
    TypenameTypeParameterAST *ast = new (pool) TypenameTypeParameterAST;
    // copy DeclarationAST
    // copy TypenameTypeParameterAST
    ast->classkey_token = classkey_token;
    if (name) ast->name = name->clone(pool);
    ast->equal_token = equal_token;
    if (type_id) ast->type_id = type_id->clone(pool);
    return ast;
}

TemplateTypeParameterAST *TemplateTypeParameterAST::clone(MemoryPool *pool) const
{
    TemplateTypeParameterAST *ast = new (pool) TemplateTypeParameterAST;
    // copy DeclarationAST
    // copy TemplateTypeParameterAST
    ast->template_token = template_token;
    ast->less_token = less_token;
    if (template_parameters) ast->template_parameters = template_parameters->clone(pool);
    ast->greater_token = greater_token;
    ast->class_token = class_token;
    if (name) ast->name = name->clone(pool);
    ast->equal_token = equal_token;
    if (type_id) ast->type_id = type_id->clone(pool);
    return ast;
}

UnaryExpressionAST *UnaryExpressionAST::clone(MemoryPool *pool) const
{
    UnaryExpressionAST *ast = new (pool) UnaryExpressionAST;
    // copy ExpressionAST
    // copy UnaryExpressionAST
    ast->unary_op_token = unary_op_token;
    if (expression) ast->expression = expression->clone(pool);
    return ast;
}

UsingAST *UsingAST::clone(MemoryPool *pool) const
{
    UsingAST *ast = new (pool) UsingAST;
    // copy DeclarationAST
    // copy UsingAST
    ast->using_token = using_token;
    ast->typename_token = typename_token;
    if (name) ast->name = name->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

UsingDirectiveAST *UsingDirectiveAST::clone(MemoryPool *pool) const
{
    UsingDirectiveAST *ast = new (pool) UsingDirectiveAST;
    // copy DeclarationAST
    // copy UsingDirectiveAST
    ast->using_token = using_token;
    ast->namespace_token = namespace_token;
    if (name) ast->name = name->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

WhileStatementAST *WhileStatementAST::clone(MemoryPool *pool) const
{
    WhileStatementAST *ast = new (pool) WhileStatementAST;
    // copy StatementAST
    // copy WhileStatementAST
    ast->while_token = while_token;
    ast->lparen_token = lparen_token;
    if (condition) ast->condition = condition->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement) ast->statement = statement->clone(pool);
    return ast;
}

IdentifierListAST *IdentifierListAST::clone(MemoryPool *pool) const
{
    IdentifierListAST *ast = new (pool) IdentifierListAST;
    // copy IdentifierListAST
    if (ast->name) ast->name = name->clone(pool);
    ast->comma_token = comma_token;
    if (next) ast->next = next->clone(pool);
    return ast;
}

ObjCClassForwardDeclarationAST *ObjCClassForwardDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCClassForwardDeclarationAST *ast = new (pool) ObjCClassForwardDeclarationAST;
    // copy DeclarationAST
    // copy ObjCClassForwardDeclarationAST
    if (attributes) ast->attributes = attributes->clone(pool);
    ast->class_token = class_token;
    if (identifier_list) ast->identifier_list = identifier_list->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

ObjCClassDeclarationAST *ObjCClassDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCClassDeclarationAST *ast = new (pool) ObjCClassDeclarationAST;
    // copy DeclarationAST
    // copy ObjCClassDeclarationAST
    if (attributes) ast->attributes = attributes->clone(pool);
    ast->interface_token = interface_token;
    ast->implementation_token = implementation_token;
    if (class_name) ast->class_name = class_name->clone(pool);
    ast->lparen_token = lparen_token;
    if (category_name) ast->category_name = category_name->clone(pool);
    ast->rparen_token = rparen_token;
    ast->colon_token = colon_token;
    if (superclass) ast->superclass = superclass->clone(pool);
    if (protocol_refs) ast->protocol_refs = protocol_refs->clone(pool);
    if (inst_vars_decl) ast->inst_vars_decl = inst_vars_decl->clone(pool);
    if (member_declarations) ast->member_declarations = member_declarations->clone(pool);
    ast->end_token = end_token;
    return ast;
}

ObjCProtocolForwardDeclarationAST *ObjCProtocolForwardDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCProtocolForwardDeclarationAST *ast = new (pool) ObjCProtocolForwardDeclarationAST;
    if (attributes) ast->attributes = attributes->clone(pool);
    ast->protocol_token = protocol_token;
    if (identifier_list) ast->identifier_list = identifier_list;
    ast->semicolon_token = semicolon_token;
    return ast;
}

ObjCProtocolDeclarationAST *ObjCProtocolDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCProtocolDeclarationAST *ast = new (pool) ObjCProtocolDeclarationAST;
    if (attributes) ast->attributes = attributes->clone(pool);
    ast->protocol_token = protocol_token;
    if (name) ast->name = name->clone(pool);
    if (protocol_refs) ast->protocol_refs = protocol_refs->clone(pool);
    if (member_declarations) ast->member_declarations = member_declarations->clone(pool);
    ast->end_token = end_token;
    return ast;
}

ObjCProtocolRefsAST *ObjCProtocolRefsAST::clone(MemoryPool *pool) const
{
    ObjCProtocolRefsAST *ast = new (pool) ObjCProtocolRefsAST;
    ast->less_token = less_token;
    if (ast->identifier_list) ast->identifier_list = identifier_list->clone(pool);
    ast->greater_token = greater_token;
    return ast;
}

ObjCMessageExpressionAST *ObjCMessageExpressionAST::clone(MemoryPool *pool) const
{
    ObjCMessageExpressionAST *ast = new (pool) ObjCMessageExpressionAST;
    ast->lbracket_token = lbracket_token;
    if (receiver_expression) ast->receiver_expression = receiver_expression->clone(pool);
    if (selector) ast->selector = selector->clone(pool);
    if (argument_list) ast->argument_list = argument_list->clone(pool);
    ast->rbracket_token = rbracket_token;
    return ast;
}

ObjCMessageArgumentListAST *ObjCMessageArgumentListAST::clone(MemoryPool *pool) const
{
    ObjCMessageArgumentListAST *ast = new (pool) ObjCMessageArgumentListAST;
    if (arg) ast->arg = arg->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

ObjCMessageArgumentAST *ObjCMessageArgumentAST::clone(MemoryPool *pool) const
{
    ObjCMessageArgumentAST *ast = new (pool) ObjCMessageArgumentAST;
    if (parameter_value_expression) ast->parameter_value_expression = parameter_value_expression->clone(pool);
    return ast;
}

ObjCProtocolExpressionAST *ObjCProtocolExpressionAST::clone(MemoryPool *pool) const
{
    ObjCProtocolExpressionAST *ast = new (pool) ObjCProtocolExpressionAST;
    ast->protocol_token = protocol_token;
    ast->lparen_token = lparen_token;
    ast->identifier_token = identifier_token;
    ast->rparen_token = rparen_token;
    return ast;
}

ObjCTypeNameAST *ObjCTypeNameAST::clone(MemoryPool *pool) const
{
    ObjCTypeNameAST *ast = new (pool) ObjCTypeNameAST;
    ast->lparen_token = lparen_token;
    ast->type_qualifier = type_qualifier;
    if (type_id) ast->type_id = type_id->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

ObjCEncodeExpressionAST *ObjCEncodeExpressionAST::clone(MemoryPool *pool) const
{
    ObjCEncodeExpressionAST *ast = new (pool) ObjCEncodeExpressionAST;
    ast->encode_token = encode_token;
    if (type_name) ast->type_name = type_name->clone(pool);
    return ast;
}

ObjCSelectorWithoutArgumentsAST *ObjCSelectorWithoutArgumentsAST::clone(MemoryPool *pool) const
{
    ObjCSelectorWithoutArgumentsAST *ast = new (pool) ObjCSelectorWithoutArgumentsAST;
    ast->name_token = name_token;
    return ast;
}

ObjCSelectorArgumentAST *ObjCSelectorArgumentAST::clone(MemoryPool *pool) const
{
    ObjCSelectorArgumentAST *ast = new (pool) ObjCSelectorArgumentAST;
    ast->name_token = name_token;
    ast->colon_token = colon_token;
    return ast;
}

ObjCSelectorArgumentListAST *ObjCSelectorArgumentListAST::clone(MemoryPool *pool) const
{
    ObjCSelectorArgumentListAST *ast = new (pool) ObjCSelectorArgumentListAST;
    if (argument) ast->argument = argument->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

ObjCSelectorWithArgumentsAST *ObjCSelectorWithArgumentsAST::clone(MemoryPool *pool) const
{
    ObjCSelectorWithArgumentsAST *ast = new (pool) ObjCSelectorWithArgumentsAST;
    if (selector_arguments) ast->selector_arguments = selector_arguments->clone(pool);
    return ast;
}

ObjCSelectorExpressionAST *ObjCSelectorExpressionAST::clone(MemoryPool *pool) const
{
    ObjCSelectorExpressionAST *ast = new (pool) ObjCSelectorExpressionAST;
    ast->selector_token = selector_token;
    ast->lparen_token = lparen_token;
    if (selector) ast->selector = selector->clone(pool);
    ast->rparen_token = rparen_token;
    return ast;
}

ObjCInstanceVariablesDeclarationAST *ObjCInstanceVariablesDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCInstanceVariablesDeclarationAST *ast = new (pool) ObjCInstanceVariablesDeclarationAST;
    ast->lbrace_token = lbrace_token;
    if (instance_variables) ast->instance_variables = instance_variables->clone(pool);
    ast->rbrace_token = rbrace_token;
    return ast;
}

ObjCVisibilityDeclarationAST *ObjCVisibilityDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCVisibilityDeclarationAST *ast = new (pool) ObjCVisibilityDeclarationAST;
    ast->visibility_token = visibility_token;
    return ast;
}

ObjCPropertyAttributeAST *ObjCPropertyAttributeAST::clone(MemoryPool *pool) const
{
    ObjCPropertyAttributeAST *ast = new (pool) ObjCPropertyAttributeAST;
    ast->attribute_identifier_token = attribute_identifier_token;
    ast->equals_token = equals_token;
    if (method_selector) ast->method_selector = method_selector->clone(pool);
    return ast;
}

ObjCPropertyAttributeListAST *ObjCPropertyAttributeListAST::clone(MemoryPool *pool) const
{
    ObjCPropertyAttributeListAST *ast = new (pool) ObjCPropertyAttributeListAST;
    if (attr) ast->attr = attr->clone(pool);
    ast->comma_token = comma_token;
    if (next) ast->next = next->clone(pool);
    return ast;
}

ObjCPropertyDeclarationAST *ObjCPropertyDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCPropertyDeclarationAST *ast = new (pool) ObjCPropertyDeclarationAST;
    if (attributes) ast->attributes = attributes->clone(pool);
    ast->property_token = property_token;
    ast->lparen_token = lparen_token;
    if (property_attributes) ast->property_attributes = property_attributes->clone(pool);
    ast->rparen_token = rparen_token;
    if (simple_declaration) ast->simple_declaration = simple_declaration->clone(pool);
    return ast;
}

ObjCMessageArgumentDeclarationAST *ObjCMessageArgumentDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCMessageArgumentDeclarationAST *ast = new (pool) ObjCMessageArgumentDeclarationAST;
    if (type_name) ast->type_name = type_name->clone(pool);
    if (attributes) ast->attributes = attributes->clone(pool);
    ast->param_name_token = param_name_token;
    return ast;
}

ObjCMessageArgumentDeclarationListAST *ObjCMessageArgumentDeclarationListAST::clone(MemoryPool *pool) const
{
    ObjCMessageArgumentDeclarationListAST *ast = new (pool) ObjCMessageArgumentDeclarationListAST;
    if (argument_declaration) ast->argument_declaration = argument_declaration->clone(pool);
    if (next) ast->next = next->clone(pool);
    return ast;
}

ObjCMethodPrototypeAST *ObjCMethodPrototypeAST::clone(MemoryPool *pool) const
{
    ObjCMethodPrototypeAST *ast = new (pool) ObjCMethodPrototypeAST;
    ast->method_type_token = method_type_token;
    if (type_name) ast->type_name = type_name->clone(pool);
    if (arguments) ast->arguments = arguments->clone(pool);
    if (attributes) ast->attributes = attributes->clone(pool);
    return ast;
}

ObjCMethodDeclarationAST *ObjCMethodDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCMethodDeclarationAST *ast = new (pool) ObjCMethodDeclarationAST;
    if (method_prototype) ast->method_prototype = method_prototype->clone(pool);
    if (function_body) ast->function_body = function_body->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

ObjCSynthesizedPropertyAST *ObjCSynthesizedPropertyAST::clone(MemoryPool *pool) const
{
    ObjCSynthesizedPropertyAST *ast = new (pool) ObjCSynthesizedPropertyAST;
    ast->property_identifier = property_identifier;
    ast->equals_token = equals_token;
    ast->property_alias_identifier = property_alias_identifier;
    return ast;
}

ObjCSynthesizedPropertyListAST *ObjCSynthesizedPropertyListAST::clone(MemoryPool *pool) const
{
    ObjCSynthesizedPropertyListAST *ast = new (pool) ObjCSynthesizedPropertyListAST;
    if (synthesized_property) ast->synthesized_property = synthesized_property->clone(pool);
    ast->comma_token = comma_token;
    if (next) ast->next = next->clone(pool);
    return ast;
}

ObjCSynthesizedPropertiesDeclarationAST *ObjCSynthesizedPropertiesDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCSynthesizedPropertiesDeclarationAST *ast = new (pool) ObjCSynthesizedPropertiesDeclarationAST;
    ast->synthesized_token = synthesized_token;
    if (property_identifiers) ast->property_identifiers = property_identifiers->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

ObjCDynamicPropertiesDeclarationAST *ObjCDynamicPropertiesDeclarationAST::clone(MemoryPool *pool) const
{
    ObjCDynamicPropertiesDeclarationAST *ast = new (pool) ObjCDynamicPropertiesDeclarationAST;
    ast->dynamic_token = dynamic_token;
    if (property_identifiers) ast->property_identifiers = property_identifiers->clone(pool);
    ast->semicolon_token = semicolon_token;
    return ast;
}

ObjCFastEnumerationAST *ObjCFastEnumerationAST::clone(MemoryPool *pool) const
{
    ObjCFastEnumerationAST *ast = new (pool) ObjCFastEnumerationAST;
    ast->for_token = for_token;
    ast->lparen_token = lparen_token;
    if (type_specifiers) ast->type_specifiers = type_specifiers->clone(pool);
    if (declarator) ast->declarator = declarator->clone(pool);
    if (initializer) ast->initializer = initializer->clone(pool);
    ast->in_token = in_token;
    if (fast_enumeratable_expression) ast->fast_enumeratable_expression = fast_enumeratable_expression->clone(pool);
    ast->rparen_token = rparen_token;
    if (body_statement) ast->body_statement = body_statement->clone(pool);
    return ast;
}

ObjCSynchronizedStatementAST *ObjCSynchronizedStatementAST::clone(MemoryPool *pool) const
{
    ObjCSynchronizedStatementAST *ast = new (pool) ObjCSynchronizedStatementAST;
    ast->synchronized_token = synchronized_token;
    ast->lparen_token = lparen_token;
    if (synchronized_object) ast->synchronized_object = synchronized_object->clone(pool);
    ast->rparen_token = rparen_token;
    if (statement) ast->statement = statement->clone(pool);
    return ast;
}



