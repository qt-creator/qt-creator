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

void SimpleSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit SimpleSpecifierAST
        // visit SpecifierAST
    }
    visitor->endVisit(this);
}

void AttributeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AttributeSpecifierAST
        for (AttributeAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        // visit SpecifierAST
    }
    visitor->endVisit(this);
}

void AttributeAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AttributeAST
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void TypeofSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypeofSpecifierAST
        accept(expression, visitor);
        // visit SpecifierAST
    }
    visitor->endVisit(this);
}

void DeclarationListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclarationListAST
        accept(declaration, visitor);
    }
    visitor->endVisit(this);
}

void DeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclaratorAST
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        for (PtrOperatorAST *it = ptr_operators; it; it = it->next)
            accept(it, visitor);
        accept(core_declarator, visitor);
        for (PostfixDeclaratorAST *it = postfix_declarators; it; it = it->next)
            accept(it, visitor);
        for (SpecifierAST *it = post_attributes; it; it = it->next)
            accept(it, visitor);
        accept(initializer, visitor);
    }
    visitor->endVisit(this);
}

void ExpressionListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExpressionListAST
        accept(expression, visitor);
    }
    visitor->endVisit(this);
}

void SimpleDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit SimpleDeclarationAST
        for (SpecifierAST *it = decl_specifier_seq; it; it = it->next)
            accept(it, visitor);
        for (DeclaratorListAST *it = declarators; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void EmptyDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit EmptyDeclarationAST
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void AccessDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AccessDeclarationAST
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void AsmDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit AsmDefinitionAST
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void BaseSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit BaseSpecifierAST
        accept(name, visitor);
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
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        accept(name, visitor);
        for (BaseSpecifierAST *it = base_clause; it; it = it->next)
            accept(it, visitor);
        for (DeclarationListAST *it = member_specifiers; it; it = it->next)
            accept(it, visitor);
        // visit SpecifierAST
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
    }
    visitor->endVisit(this);
}

void StatementListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit StatementListAST
        accept(statement, visitor);
    }
    visitor->endVisit(this);
}

void CompoundStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CompoundStatementAST
        for (StatementListAST *it = statements; it; it = it->next)
            accept(it, visitor);
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void ConditionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ConditionAST
        for (SpecifierAST *it = type_specifier; it; it = it->next)
            accept(it, visitor);
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
        for (MemInitializerAST *it = member_initializers; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void DeclarationStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclarationStatementAST
        accept(declaration, visitor);
        // visit StatementAST
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
        for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next)
            accept(it, visitor);
        accept(exception_specification, visitor);
        accept(as_cpp_initializer, visitor);
        // visit PostfixDeclaratorAST
    }
    visitor->endVisit(this);
}

void ArrayDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ArrayDeclaratorAST
        accept(expression, visitor);
        // visit PostfixDeclaratorAST
    }
    visitor->endVisit(this);
}

void DeclaratorListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit DeclaratorListAST
        accept(declarator, visitor);
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
    }
    visitor->endVisit(this);
}

void NamedTypeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NamedTypeSpecifierAST
        accept(name, visitor);
        // visit SpecifierAST
    }
    visitor->endVisit(this);
}

void ElaboratedTypeSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ElaboratedTypeSpecifierAST
        accept(name, visitor);
        // visit SpecifierAST
    }
    visitor->endVisit(this);
}

void EnumSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit EnumSpecifierAST
        accept(name, visitor);
        for (EnumeratorAST *it = enumerators; it; it = it->next)
            accept(it, visitor);
        // visit SpecifierAST
    }
    visitor->endVisit(this);
}

void EnumeratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit EnumeratorAST
        accept(expression, visitor);
    }
    visitor->endVisit(this);
}

void ExceptionDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExceptionDeclarationAST
        for (SpecifierAST *it = type_specifier; it; it = it->next)
            accept(it, visitor);
        accept(declarator, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ExceptionSpecificationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExceptionSpecificationAST
        for (ExpressionListAST *it = type_ids; it; it = it->next)
            accept(it, visitor);
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
    }
    visitor->endVisit(this);
}

void ExpressionStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ExpressionStatementAST
        accept(expression, visitor);
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void FunctionDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit FunctionDefinitionAST
        for (SpecifierAST *it = decl_specifier_seq; it; it = it->next)
            accept(it, visitor);
        accept(declarator, visitor);
        accept(ctor_initializer, visitor);
        accept(function_body, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ForeachStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ForeachStatementAST
        for (SpecifierAST *it = type_specifiers; it; it = it->next)
            accept(it, visitor);
        accept(declarator, visitor);
        accept(initializer, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
        // visit StatementAST
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
    }
    visitor->endVisit(this);
}

void ArrayInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ArrayInitializerAST
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it, visitor);
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
    }
    visitor->endVisit(this);
}

void LinkageBodyAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit LinkageBodyAST
        for (DeclarationListAST *it = declarations; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void LinkageSpecificationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit LinkageSpecificationAST
        accept(declaration, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void MemInitializerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit MemInitializerAST
        accept(name, visitor);
        accept(expression, visitor);
    }
    visitor->endVisit(this);
}

void NestedNameSpecifierAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NestedNameSpecifierAST
        accept(class_or_namespace_name, visitor);
    }
    visitor->endVisit(this);
}

void QualifiedNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit QualifiedNameAST
        for (NestedNameSpecifierAST *it = nested_name_specifier; it; it = it->next)
            accept(it, visitor);
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
        for (SpecifierAST *it = type_specifier; it; it = it->next)
            accept(it, visitor);
        for (PtrOperatorAST *it = ptr_operators; it; it = it->next)
            accept(it, visitor);
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
        for (TemplateArgumentListAST *it = template_arguments; it; it = it->next)
            accept(it, visitor);
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void NamespaceAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NamespaceAST
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        accept(linkage_body, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void NamespaceAliasDefinitionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NamespaceAliasDefinitionAST
        accept(name, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void NewPlacementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NewPlacementAST
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void NewArrayDeclaratorAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit NewArrayDeclaratorAST
        accept(expression, visitor);
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
        for (SpecifierAST *it = type_specifier; it; it = it->next)
            accept(it, visitor);
        for (PtrOperatorAST *it = ptr_operators; it; it = it->next)
            accept(it, visitor);
        for (NewArrayDeclaratorAST *it = new_array_declarators; it; it = it->next)
            accept(it, visitor);
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
        for (SpecifierAST *it = type_specifier; it; it = it->next)
            accept(it, visitor);
        accept(declarator, visitor);
        accept(expression, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ParameterDeclarationClauseAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ParameterDeclarationClauseAST
        for (DeclarationListAST *it = parameter_declarations; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void CallAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CallAST
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it, visitor);
        // visit PostfixAST
    }
    visitor->endVisit(this);
}

void ArrayAccessAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ArrayAccessAST
        accept(expression, visitor);
        // visit PostfixAST
    }
    visitor->endVisit(this);
}

void PostIncrDecrAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PostIncrDecrAST
        // visit PostfixAST
    }
    visitor->endVisit(this);
}

void MemberAccessAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit MemberAccessAST
        accept(member_name, visitor);
        // visit PostfixAST
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
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void TypeConstructorCallAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypeConstructorCallAST
        for (SpecifierAST *it = type_specifier; it; it = it->next)
            accept(it, visitor);
        for (ExpressionListAST *it = expression_list; it; it = it->next)
            accept(it, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void PostfixExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PostfixExpressionAST
        accept(base_expression, visitor);
        for (PostfixAST *it = postfix_expressions; it; it = it->next)
            accept(it, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void PointerToMemberAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PointerToMemberAST
        for (NestedNameSpecifierAST *it = nested_name_specifier; it; it = it->next)
            accept(it, visitor);
        for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next)
            accept(it, visitor);
        // visit PtrOperatorAST
    }
    visitor->endVisit(this);
}

void PointerAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit PointerAST
        for (SpecifierAST *it = cv_qualifier_seq; it; it = it->next)
            accept(it, visitor);
        // visit PtrOperatorAST
    }
    visitor->endVisit(this);
}

void ReferenceAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ReferenceAST
        // visit PtrOperatorAST
    }
    visitor->endVisit(this);
}

void BreakStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit BreakStatementAST
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void ContinueStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ContinueStatementAST
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void GotoStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit GotoStatementAST
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void ReturnStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ReturnStatementAST
        accept(expression, visitor);
        // visit StatementAST
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
    }
    visitor->endVisit(this);
}

void TemplateArgumentListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TemplateArgumentListAST
        accept(template_argument, visitor);
    }
    visitor->endVisit(this);
}

void TemplateDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TemplateDeclarationAST
        for (DeclarationListAST *it = template_parameters; it; it = it->next)
            accept(it, visitor);
        accept(declaration, visitor);
        // visit DeclarationAST
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
        for (DeclarationListAST *it = declarations; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void TryBlockStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TryBlockStatementAST
        accept(statement, visitor);
        for (CatchClauseAST *it = catch_clause_seq; it; it = it->next)
            accept(it, visitor);
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void CatchClauseAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit CatchClauseAST
        accept(exception_declaration, visitor);
        accept(statement, visitor);
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void TypeIdAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TypeIdAST
        for (SpecifierAST *it = type_specifier; it; it = it->next)
            accept(it, visitor);
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
    }
    visitor->endVisit(this);
}

void TemplateTypeParameterAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit TemplateTypeParameterAST
        for (DeclarationListAST *it = template_parameters; it; it = it->next)
            accept(it, visitor);
        accept(name, visitor);
        accept(type_id, visitor);
        // visit DeclarationAST
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
    }
    visitor->endVisit(this);
}

void UsingDirectiveAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit UsingDirectiveAST
        accept(name, visitor);
        // visit DeclarationAST
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
    }
    visitor->endVisit(this);
}

void IdentifierListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit IdentifierListAST
        accept(name, visitor);
    }
    visitor->endVisit(this);
}

void ObjCClassForwardDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCClassForwardDeclarationAST
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        for (IdentifierListAST *it = identifier_list; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCClassDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCClassInterfaceDefinitionAST
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        accept(class_name, visitor);
        accept(category_name, visitor);
        accept(superclass, visitor);
        accept(protocol_refs, visitor);
        accept(inst_vars_decl, visitor);
        for (DeclarationListAST *it = member_declarations; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCProtocolForwardDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCProtocolDeclarationAST
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        for (IdentifierListAST *it = identifier_list; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCProtocolDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCProtocolDeclarationAST
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        accept(name, visitor);
        accept(protocol_refs, visitor);
        for (DeclarationListAST *it = member_declarations; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCProtocolRefsAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCProtocolRefsAST
        for (IdentifierListAST *it = identifier_list; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void ObjCMessageArgumentAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCMessageArgumentAST
        accept(parameter_value_expression, visitor);
    }
    visitor->endVisit(this);
}

void ObjCMessageArgumentListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCMessageArgumentListAST
        accept(arg, visitor);
    }
    visitor->endVisit(this);
}

void ObjCMessageExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCMessageExpressionAST
        accept(receiver_expression, visitor);
        accept(selector, visitor);
        for (ObjCMessageArgumentListAST *it = argument_list; it; it = it->next)
            accept(it, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ObjCProtocolExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCProtocolExpressionAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ObjCTypeNameAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCTypeNameAST
        accept(type_id, visitor);
    }
    visitor->endVisit(this);
}

void ObjCEncodeExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCEncodeExpressionAST
        accept(type_name, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ObjCSelectorWithoutArgumentsAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSelectorWithoutArgumentsAST
        // visit ObjCSelectorAST
    }
    visitor->endVisit(this);
}

void ObjCSelectorArgumentAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSelectorArgumentAST
    }
    visitor->endVisit(this);
}

void ObjCSelectorArgumentListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSelectorArgumentListAST
        accept(argument, visitor);
    }
    visitor->endVisit(this);
}

void ObjCSelectorWithArgumentsAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSelectorWithArgumentsAST
        for (ObjCSelectorArgumentListAST *it = selector_arguments; it; it = it->next)
            accept(it, visitor);
        // visit ObjCSelectorAST
    }
    visitor->endVisit(this);
}

void ObjCSelectorExpressionAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSelectorExpressionAST
        accept(selector, visitor);
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ObjCInstanceVariablesDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCInstanceVariablesDeclarationAST
        for (DeclarationListAST *it = instance_variables; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void ObjCVisibilityDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCVisibilityDeclarationAST
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCPropertyAttributeAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCPropertyAttributeAST
        accept(method_selector, visitor);
    }
    visitor->endVisit(this);
}

void ObjCPropertyAttributeListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCPropertyAttributeListAST
        accept(attr, visitor);
    }
    visitor->endVisit(this);
}

void ObjCPropertyDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCPropertyDeclarationAST
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        for (ObjCPropertyAttributeListAST *it = property_attributes; it; it = it->next)
            accept(it, visitor);
        accept(simple_declaration, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCMessageArgumentDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCMessageArgumentDeclarationAST
        accept(type_name, visitor);
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
        // visit NameAST
        // visit ExpressionAST
    }
    visitor->endVisit(this);
}

void ObjCMessageArgumentDeclarationListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCMessageArgumentDeclarationListAST
        accept(argument_declaration, visitor);
    }
    visitor->endVisit(this);
}

void ObjCMethodPrototypeAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCMethodPrototypeAST
        accept(type_name, visitor);
        accept(selector, visitor);
        for (ObjCMessageArgumentDeclarationListAST *it = arguments; it; it = it->next)
            accept(it, visitor);
        for (SpecifierAST *it = attributes; it; it = it->next)
            accept(it, visitor);
    }
    visitor->endVisit(this);
}

void ObjCMethodDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCMethodDeclarationAST
        accept(method_prototype, visitor);
        accept(function_body, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCSynthesizedPropertyAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSynthesizedPropertyAST
    }
    visitor->endVisit(this);
}

void ObjCSynthesizedPropertyListAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSynthesizedPropertyListAST
        accept(synthesized_property, visitor);
    }
    visitor->endVisit(this);
}

void ObjCSynthesizedPropertiesDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSynthesizedPropertiesDeclarationAST
        for (ObjCSynthesizedPropertyListAST *it = property_identifiers; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCDynamicPropertiesDeclarationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCDynamicPropertiesDeclarationAST
        for (IdentifierListAST *it = property_identifiers; it; it = it->next)
            accept(it, visitor);
        // visit DeclarationAST
    }
    visitor->endVisit(this);
}

void ObjCFastEnumerationAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCFastEnumerationAST
        for (SpecifierAST *it = type_specifiers; it; it = it->next)
            accept(it, visitor);
        accept(declarator, visitor);
        accept(initializer, visitor);
        accept(fast_enumeratable_expression, visitor);
        accept(body_statement, visitor);
        // visit StatementAST
    }
    visitor->endVisit(this);
}

void ObjCSynchronizedStatementAST::accept0(ASTVisitor *visitor)
{
    if (visitor->visit(this)) {
        // visit ObjCSynchronizedStatementAST
        accept(synchronized_object, visitor);
        accept(statement, visitor);
        // visit StatementAST
    }
    visitor->endVisit(this);
}


