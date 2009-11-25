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
#ifndef ASTMATCHER_H
#define ASTMATCHER_H

#include "ASTfwd.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTMatcher
{
public:
    ASTMatcher();
    virtual ~ASTMatcher();

    virtual bool match(AccessDeclarationAST *node, AccessDeclarationAST *pattern);
    virtual bool match(ArrayAccessAST *node, ArrayAccessAST *pattern);
    virtual bool match(ArrayDeclaratorAST *node, ArrayDeclaratorAST *pattern);
    virtual bool match(ArrayInitializerAST *node, ArrayInitializerAST *pattern);
    virtual bool match(AsmDefinitionAST *node, AsmDefinitionAST *pattern);
    virtual bool match(AttributeSpecifierAST *node, AttributeSpecifierAST *pattern);
    virtual bool match(AttributeAST *node, AttributeAST *pattern);
    virtual bool match(BaseSpecifierAST *node, BaseSpecifierAST *pattern);
    virtual bool match(BinaryExpressionAST *node, BinaryExpressionAST *pattern);
    virtual bool match(BoolLiteralAST *node, BoolLiteralAST *pattern);
    virtual bool match(BreakStatementAST *node, BreakStatementAST *pattern);
    virtual bool match(CallAST *node, CallAST *pattern);
    virtual bool match(CaseStatementAST *node, CaseStatementAST *pattern);
    virtual bool match(CastExpressionAST *node, CastExpressionAST *pattern);
    virtual bool match(CatchClauseAST *node, CatchClauseAST *pattern);
    virtual bool match(ClassSpecifierAST *node, ClassSpecifierAST *pattern);
    virtual bool match(CompoundLiteralAST *node, CompoundLiteralAST *pattern);
    virtual bool match(CompoundStatementAST *node, CompoundStatementAST *pattern);
    virtual bool match(ConditionAST *node, ConditionAST *pattern);
    virtual bool match(ConditionalExpressionAST *node, ConditionalExpressionAST *pattern);
    virtual bool match(ContinueStatementAST *node, ContinueStatementAST *pattern);
    virtual bool match(ConversionFunctionIdAST *node, ConversionFunctionIdAST *pattern);
    virtual bool match(CppCastExpressionAST *node, CppCastExpressionAST *pattern);
    virtual bool match(CtorInitializerAST *node, CtorInitializerAST *pattern);
    virtual bool match(DeclaratorAST *node, DeclaratorAST *pattern);
    virtual bool match(DeclarationStatementAST *node, DeclarationStatementAST *pattern);
    virtual bool match(DeclaratorIdAST *node, DeclaratorIdAST *pattern);
    virtual bool match(DeleteExpressionAST *node, DeleteExpressionAST *pattern);
    virtual bool match(DestructorNameAST *node, DestructorNameAST *pattern);
    virtual bool match(DoStatementAST *node, DoStatementAST *pattern);
    virtual bool match(ElaboratedTypeSpecifierAST *node, ElaboratedTypeSpecifierAST *pattern);
    virtual bool match(EmptyDeclarationAST *node, EmptyDeclarationAST *pattern);
    virtual bool match(EnumSpecifierAST *node, EnumSpecifierAST *pattern);
    virtual bool match(EnumeratorAST *node, EnumeratorAST *pattern);
    virtual bool match(ExceptionDeclarationAST *node, ExceptionDeclarationAST *pattern);
    virtual bool match(ExceptionSpecificationAST *node, ExceptionSpecificationAST *pattern);
    virtual bool match(ExpressionOrDeclarationStatementAST *node, ExpressionOrDeclarationStatementAST *pattern);
    virtual bool match(ExpressionStatementAST *node, ExpressionStatementAST *pattern);
    virtual bool match(ForeachStatementAST *node, ForeachStatementAST *pattern);
    virtual bool match(ForStatementAST *node, ForStatementAST *pattern);
    virtual bool match(FunctionDeclaratorAST *node, FunctionDeclaratorAST *pattern);
    virtual bool match(FunctionDefinitionAST *node, FunctionDefinitionAST *pattern);
    virtual bool match(GotoStatementAST *node, GotoStatementAST *pattern);
    virtual bool match(IfStatementAST *node, IfStatementAST *pattern);
    virtual bool match(LabeledStatementAST *node, LabeledStatementAST *pattern);
    virtual bool match(LinkageBodyAST *node, LinkageBodyAST *pattern);
    virtual bool match(LinkageSpecificationAST *node, LinkageSpecificationAST *pattern);
    virtual bool match(MemInitializerAST *node, MemInitializerAST *pattern);
    virtual bool match(MemberAccessAST *node, MemberAccessAST *pattern);
    virtual bool match(NamedTypeSpecifierAST *node, NamedTypeSpecifierAST *pattern);
    virtual bool match(NamespaceAST *node, NamespaceAST *pattern);
    virtual bool match(NamespaceAliasDefinitionAST *node, NamespaceAliasDefinitionAST *pattern);
    virtual bool match(NestedDeclaratorAST *node, NestedDeclaratorAST *pattern);
    virtual bool match(NestedExpressionAST *node, NestedExpressionAST *pattern);
    virtual bool match(NestedNameSpecifierAST *node, NestedNameSpecifierAST *pattern);
    virtual bool match(NewPlacementAST *node, NewPlacementAST *pattern);
    virtual bool match(NewArrayDeclaratorAST *node, NewArrayDeclaratorAST *pattern);
    virtual bool match(NewExpressionAST *node, NewExpressionAST *pattern);
    virtual bool match(NewInitializerAST *node, NewInitializerAST *pattern);
    virtual bool match(NewTypeIdAST *node, NewTypeIdAST *pattern);
    virtual bool match(NumericLiteralAST *node, NumericLiteralAST *pattern);
    virtual bool match(OperatorAST *node, OperatorAST *pattern);
    virtual bool match(OperatorFunctionIdAST *node, OperatorFunctionIdAST *pattern);
    virtual bool match(ParameterDeclarationAST *node, ParameterDeclarationAST *pattern);
    virtual bool match(ParameterDeclarationClauseAST *node, ParameterDeclarationClauseAST *pattern);
    virtual bool match(PointerAST *node, PointerAST *pattern);
    virtual bool match(PointerToMemberAST *node, PointerToMemberAST *pattern);
    virtual bool match(PostIncrDecrAST *node, PostIncrDecrAST *pattern);
    virtual bool match(PostfixExpressionAST *node, PostfixExpressionAST *pattern);
    virtual bool match(QualifiedNameAST *node, QualifiedNameAST *pattern);
    virtual bool match(ReferenceAST *node, ReferenceAST *pattern);
    virtual bool match(ReturnStatementAST *node, ReturnStatementAST *pattern);
    virtual bool match(SimpleDeclarationAST *node, SimpleDeclarationAST *pattern);
    virtual bool match(SimpleNameAST *node, SimpleNameAST *pattern);
    virtual bool match(SimpleSpecifierAST *node, SimpleSpecifierAST *pattern);
    virtual bool match(SizeofExpressionAST *node, SizeofExpressionAST *pattern);
    virtual bool match(StringLiteralAST *node, StringLiteralAST *pattern);
    virtual bool match(SwitchStatementAST *node, SwitchStatementAST *pattern);
    virtual bool match(TemplateDeclarationAST *node, TemplateDeclarationAST *pattern);
    virtual bool match(TemplateIdAST *node, TemplateIdAST *pattern);
    virtual bool match(TemplateTypeParameterAST *node, TemplateTypeParameterAST *pattern);
    virtual bool match(ThisExpressionAST *node, ThisExpressionAST *pattern);
    virtual bool match(ThrowExpressionAST *node, ThrowExpressionAST *pattern);
    virtual bool match(TranslationUnitAST *node, TranslationUnitAST *pattern);
    virtual bool match(TryBlockStatementAST *node, TryBlockStatementAST *pattern);
    virtual bool match(TypeConstructorCallAST *node, TypeConstructorCallAST *pattern);
    virtual bool match(TypeIdAST *node, TypeIdAST *pattern);
    virtual bool match(TypeidExpressionAST *node, TypeidExpressionAST *pattern);
    virtual bool match(TypeofSpecifierAST *node, TypeofSpecifierAST *pattern);
    virtual bool match(TypenameCallExpressionAST *node, TypenameCallExpressionAST *pattern);
    virtual bool match(TypenameTypeParameterAST *node, TypenameTypeParameterAST *pattern);
    virtual bool match(UnaryExpressionAST *node, UnaryExpressionAST *pattern);
    virtual bool match(UsingAST *node, UsingAST *pattern);
    virtual bool match(UsingDirectiveAST *node, UsingDirectiveAST *pattern);
    virtual bool match(WhileStatementAST *node, WhileStatementAST *pattern);
    virtual bool match(QtMethodAST *node, QtMethodAST *pattern);
    virtual bool match(ObjCClassDeclarationAST *node, ObjCClassDeclarationAST *pattern);
    virtual bool match(ObjCClassForwardDeclarationAST *node, ObjCClassForwardDeclarationAST *pattern);
    virtual bool match(ObjCProtocolDeclarationAST *node, ObjCProtocolDeclarationAST *pattern);
    virtual bool match(ObjCProtocolForwardDeclarationAST *node, ObjCProtocolForwardDeclarationAST *pattern);
    virtual bool match(ObjCProtocolRefsAST *node, ObjCProtocolRefsAST *pattern);
    virtual bool match(ObjCMessageExpressionAST *node, ObjCMessageExpressionAST *pattern);
    virtual bool match(ObjCMessageArgumentAST *node, ObjCMessageArgumentAST *pattern);
    virtual bool match(ObjCProtocolExpressionAST *node, ObjCProtocolExpressionAST *pattern);
    virtual bool match(ObjCTypeNameAST *node, ObjCTypeNameAST *pattern);
    virtual bool match(ObjCEncodeExpressionAST *node, ObjCEncodeExpressionAST *pattern);
    virtual bool match(ObjCSelectorWithoutArgumentsAST *node, ObjCSelectorWithoutArgumentsAST *pattern);
    virtual bool match(ObjCSelectorArgumentAST *node, ObjCSelectorArgumentAST *pattern);
    virtual bool match(ObjCSelectorWithArgumentsAST *node, ObjCSelectorWithArgumentsAST *pattern);
    virtual bool match(ObjCSelectorExpressionAST *node, ObjCSelectorExpressionAST *pattern);
    virtual bool match(ObjCInstanceVariablesDeclarationAST *node, ObjCInstanceVariablesDeclarationAST *pattern);
    virtual bool match(ObjCVisibilityDeclarationAST *node, ObjCVisibilityDeclarationAST *pattern);
    virtual bool match(ObjCPropertyAttributeAST *node, ObjCPropertyAttributeAST *pattern);
    virtual bool match(ObjCPropertyDeclarationAST *node, ObjCPropertyDeclarationAST *pattern);
    virtual bool match(ObjCMethodPrototypeAST *node, ObjCMethodPrototypeAST *pattern);
    virtual bool match(ObjCMethodDeclarationAST *node, ObjCMethodDeclarationAST *pattern);
    virtual bool match(ObjCMessageArgumentDeclarationAST *node, ObjCMessageArgumentDeclarationAST *pattern);
    virtual bool match(ObjCSynthesizedPropertyAST *node, ObjCSynthesizedPropertyAST *pattern);
    virtual bool match(ObjCSynthesizedPropertiesDeclarationAST *node, ObjCSynthesizedPropertiesDeclarationAST *pattern);
    virtual bool match(ObjCDynamicPropertiesDeclarationAST *node, ObjCDynamicPropertiesDeclarationAST *pattern);
    virtual bool match(ObjCFastEnumerationAST *node, ObjCFastEnumerationAST *pattern);
    virtual bool match(ObjCSynchronizedStatementAST *node, ObjCSynchronizedStatementAST *pattern);
};

} // end of namespace CPlusPlus

#endif // ASTMATCHER_H
