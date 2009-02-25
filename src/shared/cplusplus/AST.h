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

#ifndef CPLUSPLUS_AST_H
#define CPLUSPLUS_AST_H

#include "CPlusPlusForwardDeclarations.h"
#include "ASTfwd.h"
#include "MemoryPool.h"

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class CPLUSPLUS_EXPORT AST: public Managed
{
    AST(const AST &other);
    void operator =(const AST &other);

public:
    AST();
    virtual ~AST();

    void accept(ASTVisitor *visitor);

    static void accept(AST *ast, ASTVisitor *visitor)
    { if (ast) ast->accept(visitor); }

    virtual unsigned firstToken() const = 0;
    virtual unsigned lastToken() const = 0;

    AccessDeclarationAST *asAccessDeclaration();
    ArrayAccessAST *asArrayAccess();
    ArrayDeclaratorAST *asArrayDeclarator();
    ArrayInitializerAST *asArrayInitializer();
    AsmDefinitionAST *asAsmDefinition();
    AttributeAST *asAttribute();
    AttributeSpecifierAST *asAttributeSpecifier();
    BaseSpecifierAST *asBaseSpecifier();
    QtMethodAST *asQtMethod();
    BinaryExpressionAST *asBinaryExpression();
    BoolLiteralAST *asBoolLiteral();
    BreakStatementAST *asBreakStatement();
    CallAST *asCall();
    CaseStatementAST *asCaseStatement();
    CastExpressionAST *asCastExpression();
    CatchClauseAST *asCatchClause();
    ClassSpecifierAST *asClassSpecifier();
    CompoundLiteralAST *asCompoundLiteral();
    CompoundStatementAST *asCompoundStatement();
    ConditionAST *asCondition();
    ConditionalExpressionAST *asConditionalExpression();
    ContinueStatementAST *asContinueStatement();
    ConversionFunctionIdAST *asConversionFunctionId();
    CoreDeclaratorAST *asCoreDeclarator();
    CppCastExpressionAST *asCppCastExpression();
    CtorInitializerAST *asCtorInitializer();
    DeclarationAST *asDeclaration();
    DeclarationStatementAST *asDeclarationStatement();
    DeclaratorAST *asDeclarator();
    DeclaratorIdAST *asDeclaratorId();
    DeclaratorListAST *asDeclaratorList();
    DeleteExpressionAST *asDeleteExpression();
    DestructorNameAST *asDestructorName();
    DoStatementAST *asDoStatement();
    ElaboratedTypeSpecifierAST *asElaboratedTypeSpecifier();
    EmptyDeclarationAST *asEmptyDeclaration();
    EnumSpecifierAST *asEnumSpecifier();
    EnumeratorAST *asEnumerator();
    ExceptionDeclarationAST *asExceptionDeclaration();
    ExceptionSpecificationAST *asExceptionSpecification();
    ExpressionAST *asExpression();
    ExpressionListAST *asExpressionList();
    ExpressionOrDeclarationStatementAST *asExpressionOrDeclarationStatement();
    ExpressionStatementAST *asExpressionStatement();
    ForStatementAST *asForStatement();
    FunctionDeclaratorAST *asFunctionDeclarator();
    FunctionDefinitionAST *asFunctionDefinition();
    GotoStatementAST *asGotoStatement();
    IfStatementAST *asIfStatement();
    LabeledStatementAST *asLabeledStatement();
    LinkageBodyAST *asLinkageBody();
    LinkageSpecificationAST *asLinkageSpecification();
    MemInitializerAST *asMemInitializer();
    MemberAccessAST *asMemberAccess();
    NameAST *asName();
    NamedTypeSpecifierAST *asNamedTypeSpecifier();
    NamespaceAST *asNamespace();
    NamespaceAliasDefinitionAST *asNamespaceAliasDefinition();
    NestedDeclaratorAST *asNestedDeclarator();
    NestedExpressionAST *asNestedExpression();
    NestedNameSpecifierAST *asNestedNameSpecifier();
    NewDeclaratorAST *asNewDeclarator();
    NewExpressionAST *asNewExpression();
    NewInitializerAST *asNewInitializer();
    NewTypeIdAST *asNewTypeId();
    NumericLiteralAST *asNumericLiteral();
    OperatorAST *asOperator();
    OperatorFunctionIdAST *asOperatorFunctionId();
    ParameterDeclarationAST *asParameterDeclaration();
    ParameterDeclarationClauseAST *asParameterDeclarationClause();
    PointerAST *asPointer();
    PointerToMemberAST *asPointerToMember();
    PostIncrDecrAST *asPostIncrDecr();
    PostfixAST *asPostfix();
    PostfixDeclaratorAST *asPostfixDeclarator();
    PostfixExpressionAST *asPostfixExpression();
    PtrOperatorAST *asPtrOperator();
    QualifiedNameAST *asQualifiedName();
    ReferenceAST *asReference();
    ReturnStatementAST *asReturnStatement();
    SimpleDeclarationAST *asSimpleDeclaration();
    SimpleNameAST *asSimpleName();
    SimpleSpecifierAST *asSimpleSpecifier();
    SizeofExpressionAST *asSizeofExpression();
    SpecifierAST *asSpecifier();
    StatementAST *asStatement();
    StringLiteralAST *asStringLiteral();
    SwitchStatementAST *asSwitchStatement();
    TemplateArgumentListAST *asTemplateArgumentList();
    TemplateDeclarationAST *asTemplateDeclaration();
    TemplateIdAST *asTemplateId();
    TemplateTypeParameterAST *asTemplateTypeParameter();
    ThisExpressionAST *asThisExpression();
    ThrowExpressionAST *asThrowExpression();
    TranslationUnitAST *asTranslationUnit();
    TryBlockStatementAST *asTryBlockStatement();
    TypeConstructorCallAST *asTypeConstructorCall();
    TypeIdAST *asTypeId();
    TypeidExpressionAST *asTypeidExpression();
    TypenameCallExpressionAST *asTypenameCallExpression();
    TypenameTypeParameterAST *asTypenameTypeParameter();
    TypeofSpecifierAST *asTypeofSpecifier();
    UnaryExpressionAST *asUnaryExpression();
    UsingAST *asUsing();
    UsingDirectiveAST *asUsingDirective();
    WhileStatementAST *asWhileStatement();

    virtual AST *clone(MemoryPool *pool) const = 0;

protected:
    virtual void accept0(ASTVisitor *visitor) = 0;
};

class CPLUSPLUS_EXPORT SpecifierAST: public AST
{
public:
    SpecifierAST *next;

public:
    virtual SpecifierAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT SimpleSpecifierAST: public SpecifierAST
{
public:
    unsigned specifier_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual SimpleSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT AttributeSpecifierAST: public SpecifierAST
{
public:
    unsigned attribute_token;
    unsigned first_lparen_token;
    unsigned second_lparen_token;
    AttributeAST *attributes;
    unsigned first_rparen_token;
    unsigned second_rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual AttributeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT AttributeAST: public AST
{
public:
    unsigned identifier_token;
    unsigned lparen_token;
    unsigned tag_token;
    ExpressionListAST *expression_list;
    unsigned rparen_token;
    AttributeAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual AttributeAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TypeofSpecifierAST: public SpecifierAST
{
public:
    unsigned typeof_token;
    ExpressionAST *expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TypeofSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT StatementAST: public AST
{
public:
    StatementAST *next;

public:
    virtual StatementAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT ExpressionAST: public AST
{
public:
    virtual ExpressionAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT DeclarationAST: public AST
{
public:
    DeclarationAST *next;

public:
    virtual DeclarationAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT CoreDeclaratorAST: public AST
{
public:
    virtual CoreDeclaratorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PostfixDeclaratorAST: public AST
{
public:
    PostfixDeclaratorAST *next;

public:
    virtual PostfixDeclaratorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT DeclaratorAST: public AST
{
public:
    PtrOperatorAST *ptr_operators;
    CoreDeclaratorAST *core_declarator;
    PostfixDeclaratorAST *postfix_declarators;
    SpecifierAST *attributes;
    ExpressionAST *initializer;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual DeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ExpressionListAST: public ExpressionAST
{
public:
    ExpressionAST *expression;
    ExpressionListAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ExpressionListAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT SimpleDeclarationAST: public DeclarationAST
{
public:
    SpecifierAST *decl_specifier_seq;
    DeclaratorListAST *declarators;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual SimpleDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT EmptyDeclarationAST: public DeclarationAST
{
public:
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual EmptyDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT AccessDeclarationAST: public DeclarationAST
{
public:
    unsigned access_specifier_token;
    unsigned slots_token;
    unsigned colon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual AccessDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT AsmDefinitionAST: public DeclarationAST
{
public:
    unsigned asm_token;
    SpecifierAST *cv_qualifier_seq;
    unsigned lparen_token;
    unsigned rparen_token;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual AsmDefinitionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT BaseSpecifierAST: public AST
{
public:
    unsigned token_virtual;
    unsigned token_access_specifier;
    NameAST *name;
    BaseSpecifierAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual BaseSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT CompoundLiteralAST: public ExpressionAST
{
public:
    unsigned lparen_token;
    ExpressionAST *type_id;
    unsigned rparen_token;
    ExpressionAST *initializer;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CompoundLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT QtMethodAST: public ExpressionAST
{
public:
    unsigned method_token;
    unsigned lparen_token;
    DeclaratorAST *declarator;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual QtMethodAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT BinaryExpressionAST: public ExpressionAST
{
public:
    ExpressionAST *left_expression;
    unsigned binary_op_token;
    ExpressionAST *right_expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual BinaryExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT CastExpressionAST: public ExpressionAST
{
public:
    unsigned lparen_token;
    ExpressionAST *type_id;
    unsigned rparen_token;
    ExpressionAST *expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CastExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ClassSpecifierAST: public SpecifierAST
{
public:
    unsigned classkey_token;
    SpecifierAST *attributes;
    NameAST *name;
    unsigned colon_token;
    BaseSpecifierAST *base_clause;
    unsigned lbrace_token;
    DeclarationAST *member_specifiers;
    unsigned rbrace_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ClassSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT CaseStatementAST: public StatementAST
{
public:
    unsigned case_token;
    ExpressionAST *expression;
    unsigned colon_token;
    StatementAST *statement;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CaseStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT CompoundStatementAST: public StatementAST
{
public:
    unsigned lbrace_token;
    StatementAST *statements;
    unsigned rbrace_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CompoundStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ConditionAST: public ExpressionAST
{
public:
    SpecifierAST *type_specifier;
    DeclaratorAST *declarator;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ConditionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ConditionalExpressionAST: public ExpressionAST
{
public:
    ExpressionAST *condition;
    unsigned question_token;
    ExpressionAST *left_expression;
    unsigned colon_token;
    ExpressionAST *right_expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ConditionalExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT CppCastExpressionAST: public ExpressionAST
{
public:
    unsigned cast_token;
    unsigned less_token;
    ExpressionAST *type_id;
    unsigned greater_token;
    unsigned lparen_token;
    ExpressionAST *expression;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CppCastExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT CtorInitializerAST: public AST
{
public:
    unsigned colon_token;
    MemInitializerAST *member_initializers;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CtorInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT DeclarationStatementAST: public StatementAST
{
public:
    DeclarationAST *declaration;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual DeclarationStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT DeclaratorIdAST: public CoreDeclaratorAST
{
public:
    NameAST *name;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual DeclaratorIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NestedDeclaratorAST: public CoreDeclaratorAST
{
public:
    unsigned lparen_token;
    DeclaratorAST *declarator;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NestedDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT FunctionDeclaratorAST: public PostfixDeclaratorAST
{
public:
    unsigned lparen_token;
    ParameterDeclarationClauseAST *parameters;
    unsigned rparen_token;
    SpecifierAST *cv_qualifier_seq;
    ExceptionSpecificationAST *exception_specification;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual FunctionDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ArrayDeclaratorAST: public PostfixDeclaratorAST
{
public:
    unsigned lbracket_token;
    ExpressionAST *expression;
    unsigned rbracket_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ArrayDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT DeclaratorListAST: public AST
{
public:
    DeclaratorAST *declarator;
    DeclaratorListAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual DeclaratorListAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT DeleteExpressionAST: public ExpressionAST
{
public:
    unsigned scope_token;
    unsigned delete_token;
    unsigned lbracket_token;
    unsigned rbracket_token;
    ExpressionAST *expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual DeleteExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT DoStatementAST: public StatementAST
{
public:
    unsigned do_token;
    StatementAST *statement;
    unsigned while_token;
    unsigned lparen_token;
    ExpressionAST *expression;
    unsigned rparen_token;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual DoStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NamedTypeSpecifierAST: public SpecifierAST
{
public:
    NameAST *name;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NamedTypeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ElaboratedTypeSpecifierAST: public SpecifierAST
{
public:
    unsigned classkey_token;
    NameAST *name;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ElaboratedTypeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT EnumSpecifierAST: public SpecifierAST
{
public:
    unsigned enum_token;
    NameAST *name;
    unsigned lbrace_token;
    EnumeratorAST *enumerators;
    unsigned rbrace_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual EnumSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT EnumeratorAST: public AST
{
public:
    unsigned identifier_token;
    unsigned equal_token;
    ExpressionAST *expression;
    EnumeratorAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual EnumeratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ExceptionDeclarationAST: public DeclarationAST
{
public:
    SpecifierAST *type_specifier;
    DeclaratorAST *declarator;
    unsigned dot_dot_dot_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ExceptionDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ExceptionSpecificationAST: public AST
{
public:
    unsigned throw_token;
    unsigned lparen_token;
    unsigned dot_dot_dot_token;
    ExpressionListAST *type_ids;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ExceptionSpecificationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ExpressionOrDeclarationStatementAST: public StatementAST
{
public:
    StatementAST *expression;
    StatementAST *declaration;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ExpressionOrDeclarationStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ExpressionStatementAST: public StatementAST
{
public:
    ExpressionAST *expression;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ExpressionStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT FunctionDefinitionAST: public DeclarationAST
{
public:
    SpecifierAST *decl_specifier_seq;
    DeclaratorAST *declarator;
    CtorInitializerAST *ctor_initializer;
    StatementAST *function_body;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual FunctionDefinitionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ForStatementAST: public StatementAST
{
public:
    unsigned for_token;
    unsigned lparen_token;
    StatementAST *initializer;
    ExpressionAST *condition;
    unsigned semicolon_token;
    ExpressionAST *expression;
    unsigned rparen_token;
    StatementAST *statement;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ForStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT IfStatementAST: public StatementAST
{
public:
    unsigned if_token;
    unsigned lparen_token;
    ExpressionAST *condition;
    unsigned rparen_token;
    StatementAST *statement;
    unsigned else_token;
    StatementAST *else_statement;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual IfStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ArrayInitializerAST: public ExpressionAST
{
public:
    unsigned lbrace_token;
    ExpressionListAST *expression_list;
    unsigned rbrace_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ArrayInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT LabeledStatementAST: public StatementAST
{
public:
    unsigned label_token;
    unsigned colon_token;
    StatementAST *statement;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual LabeledStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT LinkageBodyAST: public DeclarationAST
{
public:
    unsigned lbrace_token;
    DeclarationAST *declarations;
    unsigned rbrace_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual LinkageBodyAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT LinkageSpecificationAST: public DeclarationAST
{
public:
    unsigned extern_token;
    unsigned extern_type;
    DeclarationAST *declaration;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual LinkageSpecificationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT MemInitializerAST: public AST
{
public:
    NameAST *name;
    unsigned lparen_token;
    ExpressionAST *expression;
    unsigned rparen_token;
    MemInitializerAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual MemInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NameAST: public ExpressionAST
{
public:
    virtual NameAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT NestedNameSpecifierAST: public AST
{
public:
    NameAST *class_or_namespace_name;
    unsigned scope_token;
    NestedNameSpecifierAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NestedNameSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT QualifiedNameAST: public NameAST
{
public:
    unsigned global_scope_token;
    NestedNameSpecifierAST *nested_name_specifier;
    NameAST *unqualified_name;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual QualifiedNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT OperatorFunctionIdAST: public NameAST
{
public:
    unsigned operator_token;
    OperatorAST *op;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual OperatorFunctionIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ConversionFunctionIdAST: public NameAST
{
public:
    unsigned operator_token;
    SpecifierAST *type_specifier;
    PtrOperatorAST *ptr_operators;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ConversionFunctionIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT SimpleNameAST: public NameAST
{
public:
    unsigned identifier_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual SimpleNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT DestructorNameAST: public NameAST
{
public:
    unsigned tilde_token;
    unsigned identifier_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual DestructorNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TemplateIdAST: public NameAST
{
public:
    unsigned identifier_token;
    unsigned less_token;
    TemplateArgumentListAST *template_arguments;
    unsigned greater_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TemplateIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NamespaceAST: public DeclarationAST
{
public:
    unsigned namespace_token;
    unsigned identifier_token;
    SpecifierAST *attributes;
    DeclarationAST *linkage_body;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NamespaceAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NamespaceAliasDefinitionAST: public DeclarationAST
{
public:
    unsigned namespace_token;
    unsigned namespace_name;
    unsigned equal_token;
    NameAST *name;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NamespaceAliasDefinitionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NewDeclaratorAST: public AST
{
public:
    PtrOperatorAST *ptr_operators;
    NewDeclaratorAST *declarator;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NewDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NewExpressionAST: public ExpressionAST
{
public:
    unsigned scope_token;
    unsigned new_token;
    ExpressionAST *expression;
    ExpressionAST *type_id;
    NewTypeIdAST *new_type_id;
    NewInitializerAST *new_initializer;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NewExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NewInitializerAST: public AST
{
public:
    unsigned lparen_token;
    ExpressionAST *expression;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NewInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NewTypeIdAST: public AST
{
public:
    SpecifierAST *type_specifier;
    NewInitializerAST *new_initializer;
    NewDeclaratorAST *new_declarator;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NewTypeIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT OperatorAST: public AST
{
public:
    unsigned op_token;
    unsigned open_token;
    unsigned close_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual OperatorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ParameterDeclarationAST: public DeclarationAST
{
public:
    SpecifierAST *type_specifier;
    DeclaratorAST *declarator;
    unsigned equal_token;
    ExpressionAST *expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ParameterDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ParameterDeclarationClauseAST: public AST
{
public:
    DeclarationAST *parameter_declarations;
    unsigned dot_dot_dot_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ParameterDeclarationClauseAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT PostfixAST: public AST
{
public:
    PostfixAST *next;

public:
    virtual PostfixAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT CallAST: public PostfixAST
{
public:
    unsigned lparen_token;
    ExpressionListAST *expression_list;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CallAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ArrayAccessAST: public PostfixAST
{
public:
    unsigned lbracket_token;
    ExpressionAST *expression;
    unsigned rbracket_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ArrayAccessAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT PostIncrDecrAST: public PostfixAST
{
public:
    unsigned incr_decr_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual PostIncrDecrAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT MemberAccessAST: public PostfixAST
{
public:
    unsigned access_token;
    unsigned template_token;
    NameAST *member_name;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual MemberAccessAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TypeidExpressionAST: public ExpressionAST
{
public:
    unsigned typeid_token;
    unsigned lparen_token;
    ExpressionAST *expression;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TypeidExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TypenameCallExpressionAST: public ExpressionAST
{
public:
    unsigned typename_token;
    NameAST *name;
    unsigned lparen_token;
    ExpressionListAST *expression_list;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TypenameCallExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TypeConstructorCallAST: public ExpressionAST
{
public:
    SpecifierAST *type_specifier;
    unsigned lparen_token;
    ExpressionListAST *expression_list;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TypeConstructorCallAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT PostfixExpressionAST: public ExpressionAST
{
public:
    ExpressionAST *base_expression;
    PostfixAST *postfix_expressions;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual PostfixExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT PtrOperatorAST: public AST
{
public:
    PtrOperatorAST *next;

public:
    virtual PtrOperatorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PointerToMemberAST: public PtrOperatorAST
{
public:
    unsigned global_scope_token;
    NestedNameSpecifierAST *nested_name_specifier;
    unsigned star_token;
    SpecifierAST *cv_qualifier_seq;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual PointerToMemberAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT PointerAST: public PtrOperatorAST
{
public:
    unsigned star_token;
    SpecifierAST *cv_qualifier_seq;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual PointerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ReferenceAST: public PtrOperatorAST
{
public:
    unsigned amp_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ReferenceAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT BreakStatementAST: public StatementAST
{
public:
    unsigned break_token;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual BreakStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ContinueStatementAST: public StatementAST
{
public:
    unsigned continue_token;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ContinueStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT GotoStatementAST: public StatementAST
{
public:
    unsigned goto_token;
    unsigned identifier_token;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual GotoStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ReturnStatementAST: public StatementAST
{
public:
    unsigned return_token;
    ExpressionAST *expression;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ReturnStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT SizeofExpressionAST: public ExpressionAST
{
public:
    unsigned sizeof_token;
    ExpressionAST *expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual SizeofExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NumericLiteralAST: public ExpressionAST
{
public:
    unsigned token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NumericLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT BoolLiteralAST: public ExpressionAST
{
public:
    unsigned token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual BoolLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ThisExpressionAST: public ExpressionAST
{
public:
    unsigned this_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ThisExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT NestedExpressionAST: public ExpressionAST
{
public:
    unsigned lparen_token;
    ExpressionAST *expression;
    unsigned rparen_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual NestedExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT StringLiteralAST: public ExpressionAST
{
public:
    unsigned token;
    StringLiteralAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual StringLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT SwitchStatementAST: public StatementAST
{
public:
    unsigned switch_token;
    unsigned lparen_token;
    ExpressionAST *condition;
    unsigned rparen_token;
    StatementAST *statement;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual SwitchStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TemplateArgumentListAST: public AST
{
public:
    ExpressionAST *template_argument;
    TemplateArgumentListAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TemplateArgumentListAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TemplateDeclarationAST: public DeclarationAST
{
public:
    unsigned export_token;
    unsigned template_token;
    unsigned less_token;
    DeclarationAST *template_parameters;
    unsigned greater_token;
    DeclarationAST *declaration;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TemplateDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ThrowExpressionAST: public ExpressionAST
{
public:
    unsigned throw_token;
    ExpressionAST *expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ThrowExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TranslationUnitAST: public AST
{
public:
    DeclarationAST *declarations;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TranslationUnitAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TryBlockStatementAST: public StatementAST
{
public:
    unsigned try_token;
    StatementAST *statement;
    CatchClauseAST *catch_clause_seq;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TryBlockStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT CatchClauseAST: public StatementAST
{
public:
    unsigned catch_token;
    unsigned lparen_token;
    ExceptionDeclarationAST *exception_declaration;
    unsigned rparen_token;
    StatementAST *statement;
    CatchClauseAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual CatchClauseAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TypeIdAST: public ExpressionAST
{
public:
    SpecifierAST *type_specifier;
    DeclaratorAST *declarator;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TypeIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TypenameTypeParameterAST: public DeclarationAST
{
public:
    unsigned classkey_token;
    NameAST *name;
    unsigned equal_token;
    ExpressionAST *type_id;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TypenameTypeParameterAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT TemplateTypeParameterAST: public DeclarationAST
{
public:
    unsigned template_token;
    unsigned less_token;
    DeclarationAST *template_parameters;
    unsigned greater_token;
    unsigned class_token;
    NameAST *name;
    unsigned equal_token;
    ExpressionAST *type_id;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual TemplateTypeParameterAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT UnaryExpressionAST: public ExpressionAST
{
public:
    unsigned unary_op_token;
    ExpressionAST *expression;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual UnaryExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT UsingAST: public DeclarationAST
{
public:
    unsigned using_token;
    unsigned typename_token;
    NameAST *name;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual UsingAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT UsingDirectiveAST: public DeclarationAST
{
public:
    unsigned using_token;
    unsigned namespace_token;
    NameAST *name;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual UsingDirectiveAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT WhileStatementAST: public StatementAST
{
public:
    unsigned while_token;
    unsigned lparen_token;
    ExpressionAST *condition;
    unsigned rparen_token;
    StatementAST *statement;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual WhileStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};


// ObjC++
class CPLUSPLUS_EXPORT IdentifierListAST: public AST
{
public:
    unsigned identifier_token;
    IdentifierListAST *next;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual IdentifierListAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

class CPLUSPLUS_EXPORT ObjCClassDeclarationAST: public DeclarationAST
{
public:
    SpecifierAST *attributes;
    unsigned class_token;
    IdentifierListAST *identifier_list;
    unsigned semicolon_token;

public:
    virtual unsigned firstToken() const;
    virtual unsigned lastToken() const;

    virtual ObjCClassDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_AST_H
