/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_PRETTYPRINTER_H
#define CPLUSPLUS_PRETTYPRINTER_H

#include "CPlusPlusForwardDeclarations.h"
#include "ASTVisitor.h"

#include <iosfwd>

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class PrettyPrinter: protected ASTVisitor
{
public:
    PrettyPrinter(Control *control, std::ostream &out);

    void operator()(AST *ast);

protected:
    virtual bool visit(AccessDeclarationAST *ast);
    virtual bool visit(ArrayAccessAST *ast);
    virtual bool visit(ArrayDeclaratorAST *ast);
    virtual bool visit(ArrayInitializerAST *ast);
    virtual bool visit(AsmDefinitionAST *ast);
    virtual bool visit(AttributeSpecifierAST *ast);
    virtual bool visit(AttributeAST *ast);
    virtual bool visit(BaseSpecifierAST *ast);
    virtual bool visit(BinaryExpressionAST *ast);
    virtual bool visit(BoolLiteralAST *ast);
    virtual bool visit(BreakStatementAST *ast);
    virtual bool visit(CallAST *ast);
    virtual bool visit(CaseStatementAST *ast);
    virtual bool visit(CastExpressionAST *ast);
    virtual bool visit(CatchClauseAST *ast);
    virtual bool visit(ClassSpecifierAST *ast);
    virtual bool visit(CompoundLiteralAST *ast);
    virtual bool visit(CompoundStatementAST *ast);
    virtual bool visit(ConditionAST *ast);
    virtual bool visit(ConditionalExpressionAST *ast);
    virtual bool visit(ContinueStatementAST *ast);
    virtual bool visit(ConversionFunctionIdAST *ast);
    virtual bool visit(CppCastExpressionAST *ast);
    virtual bool visit(CtorInitializerAST *ast);
    virtual bool visit(DeclaratorAST *ast);
    virtual bool visit(DeclarationStatementAST *ast);
    virtual bool visit(DeclaratorIdAST *ast);
    virtual bool visit(DeclaratorListAST *ast);
    virtual bool visit(DeleteExpressionAST *ast);
    virtual bool visit(DestructorNameAST *ast);
    virtual bool visit(DoStatementAST *ast);
    virtual bool visit(ElaboratedTypeSpecifierAST *ast);
    virtual bool visit(EmptyDeclarationAST *ast);
    virtual bool visit(EnumSpecifierAST *ast);
    virtual bool visit(EnumeratorAST *ast);
    virtual bool visit(ExceptionDeclarationAST *ast);
    virtual bool visit(ExceptionSpecificationAST *ast);
    virtual bool visit(ExpressionListAST *ast);
    virtual bool visit(ExpressionOrDeclarationStatementAST *ast);
    virtual bool visit(ExpressionStatementAST *ast);
    virtual bool visit(ForStatementAST *ast);
    virtual bool visit(FunctionDeclaratorAST *ast);
    virtual bool visit(FunctionDefinitionAST *ast);
    virtual bool visit(GotoStatementAST *ast);
    virtual bool visit(IfStatementAST *ast);
    virtual bool visit(LabeledStatementAST *ast);
    virtual bool visit(LinkageBodyAST *ast);
    virtual bool visit(LinkageSpecificationAST *ast);
    virtual bool visit(MemInitializerAST *ast);
    virtual bool visit(MemberAccessAST *ast);
    virtual bool visit(NamedTypeSpecifierAST *ast);
    virtual bool visit(NamespaceAST *ast);
    virtual bool visit(NamespaceAliasDefinitionAST *ast);
    virtual bool visit(NestedDeclaratorAST *ast);
    virtual bool visit(NestedExpressionAST *ast);
    virtual bool visit(NestedNameSpecifierAST *ast);
    virtual bool visit(NewDeclaratorAST *ast);
    virtual bool visit(NewExpressionAST *ast);
    virtual bool visit(NewInitializerAST *ast);
    virtual bool visit(NewTypeIdAST *ast);
    virtual bool visit(NumericLiteralAST *ast);
    virtual bool visit(OperatorAST *ast);
    virtual bool visit(OperatorFunctionIdAST *ast);
    virtual bool visit(ParameterDeclarationAST *ast);
    virtual bool visit(ParameterDeclarationClauseAST *ast);
    virtual bool visit(PointerAST *ast);
    virtual bool visit(PointerToMemberAST *ast);
    virtual bool visit(PostIncrDecrAST *ast);
    virtual bool visit(PostfixExpressionAST *ast);
    virtual bool visit(QualifiedNameAST *ast);
    virtual bool visit(ReferenceAST *ast);
    virtual bool visit(ReturnStatementAST *ast);
    virtual bool visit(SimpleDeclarationAST *ast);
    virtual bool visit(SimpleNameAST *ast);
    virtual bool visit(SimpleSpecifierAST *ast);
    virtual bool visit(SizeofExpressionAST *ast);
    virtual bool visit(StringLiteralAST *ast);
    virtual bool visit(SwitchStatementAST *ast);
    virtual bool visit(TemplateArgumentListAST *ast);
    virtual bool visit(TemplateDeclarationAST *ast);
    virtual bool visit(TemplateIdAST *ast);
    virtual bool visit(TemplateTypeParameterAST *ast);
    virtual bool visit(ThisExpressionAST *ast);
    virtual bool visit(ThrowExpressionAST *ast);
    virtual bool visit(TranslationUnitAST *ast);
    virtual bool visit(TryBlockStatementAST *ast);
    virtual bool visit(TypeConstructorCallAST *ast);
    virtual bool visit(TypeIdAST *ast);
    virtual bool visit(TypeidExpressionAST *ast);
    virtual bool visit(TypeofSpecifierAST *ast);
    virtual bool visit(TypenameCallExpressionAST *ast);
    virtual bool visit(TypenameTypeParameterAST *ast);
    virtual bool visit(UnaryExpressionAST *ast);
    virtual bool visit(UsingAST *ast);
    virtual bool visit(UsingDirectiveAST *ast);
    virtual bool visit(WhileStatementAST *ast);
    virtual bool visit(QtMethodAST *ast);

    void indent();
    void deindent();
    void newline();

private:
    std::ostream &out;
    unsigned depth;
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_PRETTYPRINTER_H
