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

#include "ASTMatcher.h"
#include "Control.h"
#include "TranslationUnit.h"

using namespace CPlusPlus;

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


bool ASTMatcher::match(AccessDeclarationAST *, AccessDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ArrayAccessAST *, ArrayAccessAST *)
{
    return true;
}

bool ASTMatcher::match(ArrayDeclaratorAST *, ArrayDeclaratorAST *)
{
    return true;
}

bool ASTMatcher::match(ArrayInitializerAST *, ArrayInitializerAST *)
{
    return true;
}

bool ASTMatcher::match(AsmDefinitionAST *, AsmDefinitionAST *)
{
    return true;
}

bool ASTMatcher::match(AttributeSpecifierAST *, AttributeSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(AttributeAST *, AttributeAST *)
{
    return true;
}

bool ASTMatcher::match(BaseSpecifierAST *, BaseSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(BinaryExpressionAST *, BinaryExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(BoolLiteralAST *, BoolLiteralAST *)
{
    return true;
}

bool ASTMatcher::match(BreakStatementAST *, BreakStatementAST *)
{
    return true;
}

bool ASTMatcher::match(CallAST *, CallAST *)
{
    return true;
}

bool ASTMatcher::match(CaseStatementAST *, CaseStatementAST *)
{
    return true;
}

bool ASTMatcher::match(CastExpressionAST *, CastExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(CatchClauseAST *, CatchClauseAST *)
{
    return true;
}

bool ASTMatcher::match(ClassSpecifierAST *, ClassSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(CompoundLiteralAST *, CompoundLiteralAST *)
{
    return true;
}

bool ASTMatcher::match(CompoundStatementAST *, CompoundStatementAST *)
{
    return true;
}

bool ASTMatcher::match(ConditionAST *, ConditionAST *)
{
    return true;
}

bool ASTMatcher::match(ConditionalExpressionAST *, ConditionalExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(ContinueStatementAST *, ContinueStatementAST *)
{
    return true;
}

bool ASTMatcher::match(ConversionFunctionIdAST *, ConversionFunctionIdAST *)
{
    return true;
}

bool ASTMatcher::match(CppCastExpressionAST *, CppCastExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(CtorInitializerAST *, CtorInitializerAST *)
{
    return true;
}

bool ASTMatcher::match(DeclaratorAST *, DeclaratorAST *)
{
    return true;
}

bool ASTMatcher::match(DeclarationStatementAST *, DeclarationStatementAST *)
{
    return true;
}

bool ASTMatcher::match(DeclaratorIdAST *, DeclaratorIdAST *)
{
    return true;
}

bool ASTMatcher::match(DeleteExpressionAST *, DeleteExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(DestructorNameAST *, DestructorNameAST *)
{
    return true;
}

bool ASTMatcher::match(DoStatementAST *, DoStatementAST *)
{
    return true;
}

bool ASTMatcher::match(ElaboratedTypeSpecifierAST *, ElaboratedTypeSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(EmptyDeclarationAST *, EmptyDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(EnumSpecifierAST *, EnumSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(EnumeratorAST *, EnumeratorAST *)
{
    return true;
}

bool ASTMatcher::match(ExceptionDeclarationAST *, ExceptionDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ExceptionSpecificationAST *, ExceptionSpecificationAST *)
{
    return true;
}

bool ASTMatcher::match(ExpressionOrDeclarationStatementAST *, ExpressionOrDeclarationStatementAST *)
{
    return true;
}

bool ASTMatcher::match(ExpressionStatementAST *, ExpressionStatementAST *)
{
    return true;
}

bool ASTMatcher::match(ForeachStatementAST *, ForeachStatementAST *)
{
    return true;
}

bool ASTMatcher::match(ForStatementAST *, ForStatementAST *)
{
    return true;
}

bool ASTMatcher::match(FunctionDeclaratorAST *, FunctionDeclaratorAST *)
{
    return true;
}

bool ASTMatcher::match(FunctionDefinitionAST *, FunctionDefinitionAST *)
{
    return true;
}

bool ASTMatcher::match(GotoStatementAST *, GotoStatementAST *)
{
    return true;
}

bool ASTMatcher::match(IfStatementAST *, IfStatementAST *)
{
    return true;
}

bool ASTMatcher::match(LabeledStatementAST *, LabeledStatementAST *)
{
    return true;
}

bool ASTMatcher::match(LinkageBodyAST *, LinkageBodyAST *)
{
    return true;
}

bool ASTMatcher::match(LinkageSpecificationAST *, LinkageSpecificationAST *)
{
    return true;
}

bool ASTMatcher::match(MemInitializerAST *, MemInitializerAST *)
{
    return true;
}

bool ASTMatcher::match(MemberAccessAST *, MemberAccessAST *)
{
    return true;
}

bool ASTMatcher::match(NamedTypeSpecifierAST *, NamedTypeSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(NamespaceAST *, NamespaceAST *)
{
    return true;
}

bool ASTMatcher::match(NamespaceAliasDefinitionAST *, NamespaceAliasDefinitionAST *)
{
    return true;
}

bool ASTMatcher::match(NestedDeclaratorAST *, NestedDeclaratorAST *)
{
    return true;
}

bool ASTMatcher::match(NestedExpressionAST *, NestedExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(NestedNameSpecifierAST *, NestedNameSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(NewPlacementAST *, NewPlacementAST *)
{
    return true;
}

bool ASTMatcher::match(NewArrayDeclaratorAST *, NewArrayDeclaratorAST *)
{
    return true;
}

bool ASTMatcher::match(NewExpressionAST *, NewExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(NewInitializerAST *, NewInitializerAST *)
{
    return true;
}

bool ASTMatcher::match(NewTypeIdAST *, NewTypeIdAST *)
{
    return true;
}

bool ASTMatcher::match(NumericLiteralAST *, NumericLiteralAST *)
{
    return true;
}

bool ASTMatcher::match(OperatorAST *, OperatorAST *)
{
    return true;
}

bool ASTMatcher::match(OperatorFunctionIdAST *, OperatorFunctionIdAST *)
{
    return true;
}

bool ASTMatcher::match(ParameterDeclarationAST *, ParameterDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ParameterDeclarationClauseAST *, ParameterDeclarationClauseAST *)
{
    return true;
}

bool ASTMatcher::match(PointerAST *, PointerAST *)
{
    return true;
}

bool ASTMatcher::match(PointerToMemberAST *, PointerToMemberAST *)
{
    return true;
}

bool ASTMatcher::match(PostIncrDecrAST *, PostIncrDecrAST *)
{
    return true;
}

bool ASTMatcher::match(PostfixExpressionAST *, PostfixExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(QualifiedNameAST *, QualifiedNameAST *)
{
    return true;
}

bool ASTMatcher::match(ReferenceAST *, ReferenceAST *)
{
    return true;
}

bool ASTMatcher::match(ReturnStatementAST *, ReturnStatementAST *)
{
    return true;
}

bool ASTMatcher::match(SimpleDeclarationAST *, SimpleDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(SimpleNameAST *, SimpleNameAST *)
{
    return true;
}

bool ASTMatcher::match(SimpleSpecifierAST *, SimpleSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(SizeofExpressionAST *, SizeofExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(StringLiteralAST *, StringLiteralAST *)
{
    return true;
}

bool ASTMatcher::match(SwitchStatementAST *, SwitchStatementAST *)
{
    return true;
}

bool ASTMatcher::match(TemplateDeclarationAST *, TemplateDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(TemplateIdAST *, TemplateIdAST *)
{
    return true;
}

bool ASTMatcher::match(TemplateTypeParameterAST *, TemplateTypeParameterAST *)
{
    return true;
}

bool ASTMatcher::match(ThisExpressionAST *, ThisExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(ThrowExpressionAST *, ThrowExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(TranslationUnitAST *, TranslationUnitAST *)
{
    return true;
}

bool ASTMatcher::match(TryBlockStatementAST *, TryBlockStatementAST *)
{
    return true;
}

bool ASTMatcher::match(TypeConstructorCallAST *, TypeConstructorCallAST *)
{
    return true;
}

bool ASTMatcher::match(TypeIdAST *, TypeIdAST *)
{
    return true;
}

bool ASTMatcher::match(TypeidExpressionAST *, TypeidExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(TypeofSpecifierAST *, TypeofSpecifierAST *)
{
    return true;
}

bool ASTMatcher::match(TypenameCallExpressionAST *, TypenameCallExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(TypenameTypeParameterAST *, TypenameTypeParameterAST *)
{
    return true;
}

bool ASTMatcher::match(UnaryExpressionAST *, UnaryExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(UsingAST *, UsingAST *)
{
    return true;
}

bool ASTMatcher::match(UsingDirectiveAST *, UsingDirectiveAST *)
{
    return true;
}

bool ASTMatcher::match(WhileStatementAST *, WhileStatementAST *)
{
    return true;
}

bool ASTMatcher::match(QtMethodAST *, QtMethodAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCClassDeclarationAST *, ObjCClassDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCClassForwardDeclarationAST *, ObjCClassForwardDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCProtocolDeclarationAST *, ObjCProtocolDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCProtocolForwardDeclarationAST *, ObjCProtocolForwardDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCProtocolRefsAST *, ObjCProtocolRefsAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCMessageExpressionAST *, ObjCMessageExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCMessageArgumentAST *, ObjCMessageArgumentAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCProtocolExpressionAST *, ObjCProtocolExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCTypeNameAST *, ObjCTypeNameAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCEncodeExpressionAST *, ObjCEncodeExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCSelectorWithoutArgumentsAST *, ObjCSelectorWithoutArgumentsAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCSelectorArgumentAST *, ObjCSelectorArgumentAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCSelectorWithArgumentsAST *, ObjCSelectorWithArgumentsAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCSelectorExpressionAST *, ObjCSelectorExpressionAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCInstanceVariablesDeclarationAST *, ObjCInstanceVariablesDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCVisibilityDeclarationAST *, ObjCVisibilityDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCPropertyAttributeAST *, ObjCPropertyAttributeAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCPropertyDeclarationAST *, ObjCPropertyDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCMethodPrototypeAST *, ObjCMethodPrototypeAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCMethodDeclarationAST *, ObjCMethodDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCMessageArgumentDeclarationAST *, ObjCMessageArgumentDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCSynthesizedPropertyAST *, ObjCSynthesizedPropertyAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCSynthesizedPropertiesDeclarationAST *, ObjCSynthesizedPropertiesDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCDynamicPropertiesDeclarationAST *, ObjCDynamicPropertiesDeclarationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCFastEnumerationAST *, ObjCFastEnumerationAST *)
{
    return true;
}

bool ASTMatcher::match(ObjCSynchronizedStatementAST *, ObjCSynchronizedStatementAST *)
{
    return true;
}
