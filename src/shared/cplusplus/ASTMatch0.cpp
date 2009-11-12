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

using namespace CPlusPlus;

bool SimpleSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (SimpleSpecifierAST *_other = pattern->asSimpleSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool AttributeSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (AttributeSpecifierAST *_other = pattern->asAttributeSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool AttributeAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (AttributeAST *_other = pattern->asAttribute()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression_list, _other->expression_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool TypeofSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TypeofSpecifierAST *_other = pattern->asTypeofSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool DeclaratorAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (DeclaratorAST *_other = pattern->asDeclarator()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(ptr_operator_list, _other->ptr_operator_list, matcher))
            return false;
        if (! match(core_declarator, _other->core_declarator, matcher))
            return false;
        if (! match(postfix_declarator_list, _other->postfix_declarator_list, matcher))
            return false;
        if (! match(post_attribute_list, _other->post_attribute_list, matcher))
            return false;
        if (! match(initializer, _other->initializer, matcher))
            return false;
        return true;
    }
    return false;
}

bool SimpleDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (SimpleDeclarationAST *_other = pattern->asSimpleDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(decl_specifier_list, _other->decl_specifier_list, matcher))
            return false;
        if (! match(declarator_list, _other->declarator_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool EmptyDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (EmptyDeclarationAST *_other = pattern->asEmptyDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool AccessDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (AccessDeclarationAST *_other = pattern->asAccessDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool AsmDefinitionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (AsmDefinitionAST *_other = pattern->asAsmDefinition()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool BaseSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (BaseSpecifierAST *_other = pattern->asBaseSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        return true;
    }
    return false;
}

bool CompoundLiteralAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CompoundLiteralAST *_other = pattern->asCompoundLiteral()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_id, _other->type_id, matcher))
            return false;
        if (! match(initializer, _other->initializer, matcher))
            return false;
        return true;
    }
    return false;
}

bool QtMethodAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (QtMethodAST *_other = pattern->asQtMethod()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        return true;
    }
    return false;
}

bool BinaryExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (BinaryExpressionAST *_other = pattern->asBinaryExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(left_expression, _other->left_expression, matcher))
            return false;
        if (! match(right_expression, _other->right_expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool CastExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CastExpressionAST *_other = pattern->asCastExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_id, _other->type_id, matcher))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool ClassSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ClassSpecifierAST *_other = pattern->asClassSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        if (! match(base_clause_list, _other->base_clause_list, matcher))
            return false;
        if (! match(member_specifier_list, _other->member_specifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool CaseStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CaseStatementAST *_other = pattern->asCaseStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool CompoundStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CompoundStatementAST *_other = pattern->asCompoundStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(statement_list, _other->statement_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ConditionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ConditionAST *_other = pattern->asCondition()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        return true;
    }
    return false;
}

bool ConditionalExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ConditionalExpressionAST *_other = pattern->asConditionalExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(condition, _other->condition, matcher))
            return false;
        if (! match(left_expression, _other->left_expression, matcher))
            return false;
        if (! match(right_expression, _other->right_expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool CppCastExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CppCastExpressionAST *_other = pattern->asCppCastExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_id, _other->type_id, matcher))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool CtorInitializerAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CtorInitializerAST *_other = pattern->asCtorInitializer()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(member_initializer_list, _other->member_initializer_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool DeclarationStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (DeclarationStatementAST *_other = pattern->asDeclarationStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(declaration, _other->declaration, matcher))
            return false;
        return true;
    }
    return false;
}

bool DeclaratorIdAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (DeclaratorIdAST *_other = pattern->asDeclaratorId()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        return true;
    }
    return false;
}

bool NestedDeclaratorAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NestedDeclaratorAST *_other = pattern->asNestedDeclarator()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        return true;
    }
    return false;
}

bool FunctionDeclaratorAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (FunctionDeclaratorAST *_other = pattern->asFunctionDeclarator()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(parameters, _other->parameters, matcher))
            return false;
        if (! match(cv_qualifier_list, _other->cv_qualifier_list, matcher))
            return false;
        if (! match(exception_specification, _other->exception_specification, matcher))
            return false;
        if (! match(as_cpp_initializer, _other->as_cpp_initializer, matcher))
            return false;
        return true;
    }
    return false;
}

bool ArrayDeclaratorAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ArrayDeclaratorAST *_other = pattern->asArrayDeclarator()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool DeleteExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (DeleteExpressionAST *_other = pattern->asDeleteExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool DoStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (DoStatementAST *_other = pattern->asDoStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool NamedTypeSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NamedTypeSpecifierAST *_other = pattern->asNamedTypeSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        return true;
    }
    return false;
}

bool ElaboratedTypeSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ElaboratedTypeSpecifierAST *_other = pattern->asElaboratedTypeSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        return true;
    }
    return false;
}

bool EnumSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (EnumSpecifierAST *_other = pattern->asEnumSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        if (! match(enumerator_list, _other->enumerator_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool EnumeratorAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (EnumeratorAST *_other = pattern->asEnumerator()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool ExceptionDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ExceptionDeclarationAST *_other = pattern->asExceptionDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        return true;
    }
    return false;
}

bool ExceptionSpecificationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ExceptionSpecificationAST *_other = pattern->asExceptionSpecification()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_id_list, _other->type_id_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ExpressionOrDeclarationStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ExpressionOrDeclarationStatementAST *_other = pattern->asExpressionOrDeclarationStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        if (! match(declaration, _other->declaration, matcher))
            return false;
        return true;
    }
    return false;
}

bool ExpressionStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ExpressionStatementAST *_other = pattern->asExpressionStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool FunctionDefinitionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (FunctionDefinitionAST *_other = pattern->asFunctionDefinition()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(decl_specifier_list, _other->decl_specifier_list, matcher))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        if (! match(ctor_initializer, _other->ctor_initializer, matcher))
            return false;
        if (! match(function_body, _other->function_body, matcher))
            return false;
        return true;
    }
    return false;
}

bool ForeachStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ForeachStatementAST *_other = pattern->asForeachStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        if (! match(initializer, _other->initializer, matcher))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool ForStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ForStatementAST *_other = pattern->asForStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(initializer, _other->initializer, matcher))
            return false;
        if (! match(condition, _other->condition, matcher))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool IfStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (IfStatementAST *_other = pattern->asIfStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(condition, _other->condition, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        if (! match(else_statement, _other->else_statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool ArrayInitializerAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ArrayInitializerAST *_other = pattern->asArrayInitializer()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression_list, _other->expression_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool LabeledStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (LabeledStatementAST *_other = pattern->asLabeledStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool LinkageBodyAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (LinkageBodyAST *_other = pattern->asLinkageBody()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(declaration_list, _other->declaration_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool LinkageSpecificationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (LinkageSpecificationAST *_other = pattern->asLinkageSpecification()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(declaration, _other->declaration, matcher))
            return false;
        return true;
    }
    return false;
}

bool MemInitializerAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (MemInitializerAST *_other = pattern->asMemInitializer()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool NestedNameSpecifierAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NestedNameSpecifierAST *_other = pattern->asNestedNameSpecifier()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(class_or_namespace_name, _other->class_or_namespace_name, matcher))
            return false;
        return true;
    }
    return false;
}

bool QualifiedNameAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (QualifiedNameAST *_other = pattern->asQualifiedName()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(nested_name_specifier_list, _other->nested_name_specifier_list, matcher))
            return false;
        if (! match(unqualified_name, _other->unqualified_name, matcher))
            return false;
        return true;
    }
    return false;
}

bool OperatorFunctionIdAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (OperatorFunctionIdAST *_other = pattern->asOperatorFunctionId()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(op, _other->op, matcher))
            return false;
        return true;
    }
    return false;
}

bool ConversionFunctionIdAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ConversionFunctionIdAST *_other = pattern->asConversionFunctionId()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(ptr_operator_list, _other->ptr_operator_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool SimpleNameAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (SimpleNameAST *_other = pattern->asSimpleName()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool DestructorNameAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (DestructorNameAST *_other = pattern->asDestructorName()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool TemplateIdAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TemplateIdAST *_other = pattern->asTemplateId()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(template_argument_list, _other->template_argument_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool NamespaceAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NamespaceAST *_other = pattern->asNamespace()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(linkage_body, _other->linkage_body, matcher))
            return false;
        return true;
    }
    return false;
}

bool NamespaceAliasDefinitionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NamespaceAliasDefinitionAST *_other = pattern->asNamespaceAliasDefinition()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        return true;
    }
    return false;
}

bool NewPlacementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NewPlacementAST *_other = pattern->asNewPlacement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression_list, _other->expression_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool NewArrayDeclaratorAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NewArrayDeclaratorAST *_other = pattern->asNewArrayDeclarator()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool NewExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NewExpressionAST *_other = pattern->asNewExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(new_placement, _other->new_placement, matcher))
            return false;
        if (! match(type_id, _other->type_id, matcher))
            return false;
        if (! match(new_type_id, _other->new_type_id, matcher))
            return false;
        if (! match(new_initializer, _other->new_initializer, matcher))
            return false;
        return true;
    }
    return false;
}

bool NewInitializerAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NewInitializerAST *_other = pattern->asNewInitializer()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool NewTypeIdAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NewTypeIdAST *_other = pattern->asNewTypeId()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(ptr_operator_list, _other->ptr_operator_list, matcher))
            return false;
        if (! match(new_array_declarator_list, _other->new_array_declarator_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool OperatorAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (OperatorAST *_other = pattern->asOperator()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ParameterDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ParameterDeclarationAST *_other = pattern->asParameterDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool ParameterDeclarationClauseAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ParameterDeclarationClauseAST *_other = pattern->asParameterDeclarationClause()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(parameter_declaration_list, _other->parameter_declaration_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool CallAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CallAST *_other = pattern->asCall()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression_list, _other->expression_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ArrayAccessAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ArrayAccessAST *_other = pattern->asArrayAccess()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool PostIncrDecrAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (PostIncrDecrAST *_other = pattern->asPostIncrDecr()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool MemberAccessAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (MemberAccessAST *_other = pattern->asMemberAccess()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(member_name, _other->member_name, matcher))
            return false;
        return true;
    }
    return false;
}

bool TypeidExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TypeidExpressionAST *_other = pattern->asTypeidExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool TypenameCallExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TypenameCallExpressionAST *_other = pattern->asTypenameCallExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        if (! match(expression_list, _other->expression_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool TypeConstructorCallAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TypeConstructorCallAST *_other = pattern->asTypeConstructorCall()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(expression_list, _other->expression_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool PostfixExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (PostfixExpressionAST *_other = pattern->asPostfixExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(base_expression, _other->base_expression, matcher))
            return false;
        if (! match(postfix_expression_list, _other->postfix_expression_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool PointerToMemberAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (PointerToMemberAST *_other = pattern->asPointerToMember()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(nested_name_specifier_list, _other->nested_name_specifier_list, matcher))
            return false;
        if (! match(cv_qualifier_list, _other->cv_qualifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool PointerAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (PointerAST *_other = pattern->asPointer()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(cv_qualifier_list, _other->cv_qualifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ReferenceAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ReferenceAST *_other = pattern->asReference()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool BreakStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (BreakStatementAST *_other = pattern->asBreakStatement()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ContinueStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ContinueStatementAST *_other = pattern->asContinueStatement()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool GotoStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (GotoStatementAST *_other = pattern->asGotoStatement()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ReturnStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ReturnStatementAST *_other = pattern->asReturnStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool SizeofExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (SizeofExpressionAST *_other = pattern->asSizeofExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool NumericLiteralAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NumericLiteralAST *_other = pattern->asNumericLiteral()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool BoolLiteralAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (BoolLiteralAST *_other = pattern->asBoolLiteral()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ThisExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ThisExpressionAST *_other = pattern->asThisExpression()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool NestedExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (NestedExpressionAST *_other = pattern->asNestedExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool StringLiteralAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (StringLiteralAST *_other = pattern->asStringLiteral()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(next, _other->next, matcher))
            return false;
        return true;
    }
    return false;
}

bool SwitchStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (SwitchStatementAST *_other = pattern->asSwitchStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(condition, _other->condition, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool TemplateDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TemplateDeclarationAST *_other = pattern->asTemplateDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(template_parameter_list, _other->template_parameter_list, matcher))
            return false;
        if (! match(declaration, _other->declaration, matcher))
            return false;
        return true;
    }
    return false;
}

bool ThrowExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ThrowExpressionAST *_other = pattern->asThrowExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool TranslationUnitAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TranslationUnitAST *_other = pattern->asTranslationUnit()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(declaration_list, _other->declaration_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool TryBlockStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TryBlockStatementAST *_other = pattern->asTryBlockStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        if (! match(catch_clause_list, _other->catch_clause_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool CatchClauseAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (CatchClauseAST *_other = pattern->asCatchClause()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(exception_declaration, _other->exception_declaration, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool TypeIdAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TypeIdAST *_other = pattern->asTypeId()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        return true;
    }
    return false;
}

bool TypenameTypeParameterAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TypenameTypeParameterAST *_other = pattern->asTypenameTypeParameter()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        if (! match(type_id, _other->type_id, matcher))
            return false;
        return true;
    }
    return false;
}

bool TemplateTypeParameterAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (TemplateTypeParameterAST *_other = pattern->asTemplateTypeParameter()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(template_parameter_list, _other->template_parameter_list, matcher))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        if (! match(type_id, _other->type_id, matcher))
            return false;
        return true;
    }
    return false;
}

bool UnaryExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (UnaryExpressionAST *_other = pattern->asUnaryExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(expression, _other->expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool UsingAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (UsingAST *_other = pattern->asUsing()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        return true;
    }
    return false;
}

bool UsingDirectiveAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (UsingDirectiveAST *_other = pattern->asUsingDirective()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        return true;
    }
    return false;
}

bool WhileStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (WhileStatementAST *_other = pattern->asWhileStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(condition, _other->condition, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCClassForwardDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCClassForwardDeclarationAST *_other = pattern->asObjCClassForwardDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(identifier_list, _other->identifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCClassDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCClassDeclarationAST *_other = pattern->asObjCClassDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(class_name, _other->class_name, matcher))
            return false;
        if (! match(category_name, _other->category_name, matcher))
            return false;
        if (! match(superclass, _other->superclass, matcher))
            return false;
        if (! match(protocol_refs, _other->protocol_refs, matcher))
            return false;
        if (! match(inst_vars_decl, _other->inst_vars_decl, matcher))
            return false;
        if (! match(member_declaration_list, _other->member_declaration_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCProtocolForwardDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCProtocolForwardDeclarationAST *_other = pattern->asObjCProtocolForwardDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(identifier_list, _other->identifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCProtocolDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCProtocolDeclarationAST *_other = pattern->asObjCProtocolDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(name, _other->name, matcher))
            return false;
        if (! match(protocol_refs, _other->protocol_refs, matcher))
            return false;
        if (! match(member_declaration_list, _other->member_declaration_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCProtocolRefsAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCProtocolRefsAST *_other = pattern->asObjCProtocolRefs()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(identifier_list, _other->identifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCMessageArgumentAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCMessageArgumentAST *_other = pattern->asObjCMessageArgument()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(parameter_value_expression, _other->parameter_value_expression, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCMessageExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCMessageExpressionAST *_other = pattern->asObjCMessageExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(receiver_expression, _other->receiver_expression, matcher))
            return false;
        if (! match(selector, _other->selector, matcher))
            return false;
        if (! match(argument_list, _other->argument_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCProtocolExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCProtocolExpressionAST *_other = pattern->asObjCProtocolExpression()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ObjCTypeNameAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCTypeNameAST *_other = pattern->asObjCTypeName()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_id, _other->type_id, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCEncodeExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCEncodeExpressionAST *_other = pattern->asObjCEncodeExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_name, _other->type_name, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCSelectorWithoutArgumentsAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCSelectorWithoutArgumentsAST *_other = pattern->asObjCSelectorWithoutArguments()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ObjCSelectorArgumentAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCSelectorArgumentAST *_other = pattern->asObjCSelectorArgument()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ObjCSelectorWithArgumentsAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCSelectorWithArgumentsAST *_other = pattern->asObjCSelectorWithArguments()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(selector_argument_list, _other->selector_argument_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCSelectorExpressionAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCSelectorExpressionAST *_other = pattern->asObjCSelectorExpression()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(selector, _other->selector, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCInstanceVariablesDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCInstanceVariablesDeclarationAST *_other = pattern->asObjCInstanceVariablesDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(instance_variable_list, _other->instance_variable_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCVisibilityDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCVisibilityDeclarationAST *_other = pattern->asObjCVisibilityDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ObjCPropertyAttributeAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCPropertyAttributeAST *_other = pattern->asObjCPropertyAttribute()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(method_selector, _other->method_selector, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCPropertyDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCPropertyDeclarationAST *_other = pattern->asObjCPropertyDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        if (! match(property_attribute_list, _other->property_attribute_list, matcher))
            return false;
        if (! match(simple_declaration, _other->simple_declaration, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCMessageArgumentDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCMessageArgumentDeclarationAST *_other = pattern->asObjCMessageArgumentDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_name, _other->type_name, matcher))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCMethodPrototypeAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCMethodPrototypeAST *_other = pattern->asObjCMethodPrototype()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_name, _other->type_name, matcher))
            return false;
        if (! match(selector, _other->selector, matcher))
            return false;
        if (! match(argument_list, _other->argument_list, matcher))
            return false;
        if (! match(attribute_list, _other->attribute_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCMethodDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCMethodDeclarationAST *_other = pattern->asObjCMethodDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(method_prototype, _other->method_prototype, matcher))
            return false;
        if (! match(function_body, _other->function_body, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCSynthesizedPropertyAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCSynthesizedPropertyAST *_other = pattern->asObjCSynthesizedProperty()) {
        if (! matcher->match(this, _other))
            return false;
        return true;
    }
    return false;
}

bool ObjCSynthesizedPropertiesDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCSynthesizedPropertiesDeclarationAST *_other = pattern->asObjCSynthesizedPropertiesDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(property_identifier_list, _other->property_identifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCDynamicPropertiesDeclarationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCDynamicPropertiesDeclarationAST *_other = pattern->asObjCDynamicPropertiesDeclaration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(property_identifier_list, _other->property_identifier_list, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCFastEnumerationAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCFastEnumerationAST *_other = pattern->asObjCFastEnumeration()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(type_specifier_list, _other->type_specifier_list, matcher))
            return false;
        if (! match(declarator, _other->declarator, matcher))
            return false;
        if (! match(initializer, _other->initializer, matcher))
            return false;
        if (! match(fast_enumeratable_expression, _other->fast_enumeratable_expression, matcher))
            return false;
        if (! match(body_statement, _other->body_statement, matcher))
            return false;
        return true;
    }
    return false;
}

bool ObjCSynchronizedStatementAST::match0(AST *pattern, ASTMatcher *matcher)
{
    if (ObjCSynchronizedStatementAST *_other = pattern->asObjCSynchronizedStatement()) {
        if (! matcher->match(this, _other))
            return false;
        if (! match(synchronized_object, _other->synchronized_object, matcher))
            return false;
        if (! match(statement, _other->statement, matcher))
            return false;
        return true;
    }
    return false;
}

