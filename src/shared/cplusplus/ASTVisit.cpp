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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "AST.h"
#include "ASTVisitor.h"

CPLUSPLUS_BEGIN_NAMESPACE

void SimpleSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit SimpleSpecifierAST
        // visit SpecifierAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void AttributeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AttributeSpecifierAST
        accept(attributes, visitor);
        // visit SpecifierAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void AttributeAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AttributeAST
        accept(expression_list, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void TypeofSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypeofSpecifierAST
        accept(expression, visitor);
        // visit SpecifierAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void DeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclaratorAST
        accept(ptr_operators, visitor);
        accept(core_declarator, visitor);
        accept(postfix_declarators, visitor);
        accept(attributes, visitor);
        accept(initializer, visitor);
    }
    visitor->endVisit(this);
}

void ExpressionListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExpressionListAST
        accept(expression, visitor);
        accept(next, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void SimpleDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit SimpleDeclarationAST
        accept(decl_specifier_seq, visitor);
        accept(declarators, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void EmptyDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit EmptyDeclarationAST
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void AccessDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AccessDeclarationAST
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void AsmDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AsmDefinitionAST
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void BaseSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit BaseSpecifierAST
        accept(name, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void CompoundLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CompoundLiteralAST
        accept(type_id, visitor);
        accept(initializer, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void QtMethodAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit QtMethodAST
        accept(declarator, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void BinaryExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit BinaryExpressionAST
        accept(left_expression, visitor);
        accept(right_expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void CastExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CastExpressionAST
        accept(type_id, visitor);
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ClassSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ClassSpecifierAST
        accept(attributes, visitor);
        accept(name, visitor);
        accept(base_clause, visitor);
        accept(member_specifiers, visitor);
        // visit SpecifierAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void CaseStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CaseStatementAST
        accept(expression, visitor);
        accept(statement, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void CompoundStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CompoundStatementAST
        accept(statements, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ConditionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ConditionAST
        accept(type_specifier, visitor);
        accept(declarator, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ConditionalExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ConditionalExpressionAST
        accept(condition, visitor);
        accept(left_expression, visitor);
        accept(right_expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void CppCastExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CppCastExpressionAST
        accept(type_id, visitor);
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void CtorInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CtorInitializerAST
        accept(member_initializers, visitor);
    }
    visitor->endVisit(this);
}

void DeclarationStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclarationStatementAST
        accept(declaration, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void DeclaratorIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclaratorIdAST
        accept(name, visitor);
        // visit CoreDeclaratorAST
    }
    visitor->endVisit(this);
}

void NestedDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NestedDeclaratorAST
        accept(declarator, visitor);
        // visit CoreDeclaratorAST
    }
    visitor->endVisit(this);
}

void FunctionDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit FunctionDeclaratorAST
        accept(parameters, visitor);
        accept(cv_qualifier_seq, visitor);
        accept(exception_specification, visitor);
        accept(as_cpp_initializer, visitor);
        // visit PostfixDeclaratorAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ArrayDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ArrayDeclaratorAST
        accept(expression, visitor);
        // visit PostfixDeclaratorAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void DeclaratorListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclaratorListAST
        accept(declarator, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void DeleteExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeleteExpressionAST
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void DoStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DoStatementAST
        accept(statement, visitor);
        accept(expression, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void NamedTypeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NamedTypeSpecifierAST
        accept(name, visitor);
        // visit SpecifierAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ElaboratedTypeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ElaboratedTypeSpecifierAST
        accept(name, visitor);
        // visit SpecifierAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void EnumSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit EnumSpecifierAST
        accept(name, visitor);
        accept(enumerators, visitor);
        // visit SpecifierAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void EnumeratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit EnumeratorAST
        accept(expression, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ExceptionDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExceptionDeclarationAST
        accept(type_specifier, visitor);
        accept(declarator, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ExceptionSpecificationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExceptionSpecificationAST
        accept(type_ids, visitor);
    }
    visitor->endVisit(this);
}

void ExpressionOrDeclarationStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExpressionOrDeclarationStatementAST
        accept(expression, visitor);
        accept(declaration, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ExpressionStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExpressionStatementAST
        accept(expression, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void FunctionDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit FunctionDefinitionAST
        accept(decl_specifier_seq, visitor);
        accept(declarator, visitor);
        accept(ctor_initializer, visitor);
        accept(function_body, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ForStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ForStatementAST
        accept(initializer, visitor);
        accept(condition, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void IfStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit IfStatementAST
        accept(condition, visitor);
        accept(statement, visitor);
        accept(else_statement, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ArrayInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ArrayInitializerAST
        accept(expression_list, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void LabeledStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit LabeledStatementAST
        accept(statement, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void LinkageBodyAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit LinkageBodyAST
        accept(declarations, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void LinkageSpecificationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit LinkageSpecificationAST
        accept(declaration, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void MemInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit MemInitializerAST
        accept(name, visitor);
        accept(expression, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void NestedNameSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NestedNameSpecifierAST
        accept(class_or_namespace_name, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void QualifiedNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit QualifiedNameAST
        accept(nested_name_specifier, visitor);
        accept(unqualified_name, visitor);
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void OperatorFunctionIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit OperatorFunctionIdAST
        accept(op, visitor);
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ConversionFunctionIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ConversionFunctionIdAST
        accept(type_specifier, visitor);
        accept(ptr_operators, visitor);
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void SimpleNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit SimpleNameAST
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void DestructorNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DestructorNameAST
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void TemplateIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TemplateIdAST
        accept(template_arguments, visitor);
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void NamespaceAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NamespaceAST
        accept(attributes, visitor);
        accept(linkage_body, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void NamespaceAliasDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NamespaceAliasDefinitionAST
        accept(name, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void NewPlacementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NewPlacementAST
        accept(expression_list, visitor);
    }
    visitor->endVisit(this);
}

void NewArrayDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NewArrayDeclaratorAST
        accept(expression, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void NewExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NewExpressionAST
        accept(new_placement, visitor);
        accept(type_id, visitor);
        accept(new_type_id, visitor);
        accept(new_initializer, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void NewInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NewInitializerAST
        accept(expression, visitor);
    }
    visitor->endVisit(this);
}

void NewTypeIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NewTypeIdAST
        accept(type_specifier, visitor);
        accept(ptr_operators, visitor);
        accept(new_array_declarators, visitor);
    }
    visitor->endVisit(this);
}

void OperatorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit OperatorAST
    }
    visitor->endVisit(this);
}

void ParameterDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ParameterDeclarationAST
        accept(type_specifier, visitor);
        accept(declarator, visitor);
        accept(expression, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ParameterDeclarationClauseAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ParameterDeclarationClauseAST
        accept(parameter_declarations, visitor);
    }
    visitor->endVisit(this);
}

void CallAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CallAST
        accept(expression_list, visitor);
        // visit PostfixAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ArrayAccessAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ArrayAccessAST
        accept(expression, visitor);
        // visit PostfixAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void PostIncrDecrAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PostIncrDecrAST
        // visit PostfixAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void MemberAccessAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit MemberAccessAST
        accept(member_name, visitor);
        // visit PostfixAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void TypeidExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypeidExpressionAST
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void TypenameCallExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypenameCallExpressionAST
        accept(name, visitor);
        accept(expression_list, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void TypeConstructorCallAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypeConstructorCallAST
        accept(type_specifier, visitor);
        accept(expression_list, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void PostfixExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PostfixExpressionAST
        accept(base_expression, visitor);
        accept(postfix_expressions, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void PointerToMemberAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PointerToMemberAST
        accept(nested_name_specifier, visitor);
        accept(cv_qualifier_seq, visitor);
        // visit PtrOperatorAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void PointerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PointerAST
        accept(cv_qualifier_seq, visitor);
        // visit PtrOperatorAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ReferenceAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ReferenceAST
        // visit PtrOperatorAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void BreakStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit BreakStatementAST
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ContinueStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ContinueStatementAST
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void GotoStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit GotoStatementAST
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ReturnStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ReturnStatementAST
        accept(expression, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void SizeofExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit SizeofExpressionAST
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void NumericLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NumericLiteralAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void BoolLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit BoolLiteralAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ThisExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ThisExpressionAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void NestedExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NestedExpressionAST
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void StringLiteralAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit StringLiteralAST
        accept(next, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void SwitchStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit SwitchStatementAST
        accept(condition, visitor);
        accept(statement, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void TemplateArgumentListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TemplateArgumentListAST
        accept(template_argument, visitor);
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void TemplateDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TemplateDeclarationAST
        accept(template_parameters, visitor);
        accept(declaration, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ThrowExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ThrowExpressionAST
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void TranslationUnitAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TranslationUnitAST
        accept(declarations, visitor);
    }
    visitor->endVisit(this);
}

void TryBlockStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TryBlockStatementAST
        accept(statement, visitor);
        accept(catch_clause_seq, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void CatchClauseAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CatchClauseAST
        accept(exception_declaration, visitor);
        accept(statement, visitor);
        accept(next, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void TypeIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypeIdAST
        accept(type_specifier, visitor);
        accept(declarator, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void TypenameTypeParameterAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypenameTypeParameterAST
        accept(name, visitor);
        accept(type_id, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void TemplateTypeParameterAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TemplateTypeParameterAST
        accept(template_parameters, visitor);
        accept(name, visitor);
        accept(type_id, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void UnaryExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit UnaryExpressionAST
        accept(expression, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void UsingAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit UsingAST
        accept(name, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void UsingDirectiveAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit UsingDirectiveAST
        accept(name, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void WhileStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit WhileStatementAST
        accept(condition, visitor);
        accept(statement, visitor);
        // visit StatementAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void IdentifierListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit IdentifierListAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

void ObjCClassDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCClassDeclarationAST
        accept(attributes, visitor);
        accept(identifier_list, visitor);
        // visit DeclarationAST
        accept(next, visitor);
    }
    visitor->endVisit(this);
}

CPLUSPLUS_END_NAMESPACE
