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
#ifndef CPLUSPLUS_AST_PATTERN_BUILDER_H
#define CPLUSPLUS_AST_PATTERN_BUILDER_H

#include "CPlusPlusForwardDeclarations.h"
#include "AST.h"
#include "MemoryPool.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTPatternBuilder
{
    MemoryPool pool;
    MemoryPool::State state;

public:
    ASTPatternBuilder(): state(pool.state()) {}

    void reset() { pool.rewind(state); };

    SimpleSpecifierAST *SimpleSpecifier()
    {
        SimpleSpecifierAST *__ast = new (&pool) SimpleSpecifierAST;
        return __ast;
    }

    AttributeSpecifierAST *AttributeSpecifier()
    {
        AttributeSpecifierAST *__ast = new (&pool) AttributeSpecifierAST;
        return __ast;
    }

    AttributeAST *Attribute()
    {
        AttributeAST *__ast = new (&pool) AttributeAST;
        return __ast;
    }

    TypeofSpecifierAST *TypeofSpecifier(ExpressionAST *expression = 0)
    {
        TypeofSpecifierAST *__ast = new (&pool) TypeofSpecifierAST;
        __ast->expression = expression;
        return __ast;
    }

    DeclaratorAST *Declarator(CoreDeclaratorAST *core_declarator = 0, ExpressionAST *initializer = 0)
    {
        DeclaratorAST *__ast = new (&pool) DeclaratorAST;
        __ast->core_declarator = core_declarator;
        __ast->initializer = initializer;
        return __ast;
    }

    SimpleDeclarationAST *SimpleDeclaration()
    {
        SimpleDeclarationAST *__ast = new (&pool) SimpleDeclarationAST;
        return __ast;
    }

    EmptyDeclarationAST *EmptyDeclaration()
    {
        EmptyDeclarationAST *__ast = new (&pool) EmptyDeclarationAST;
        return __ast;
    }

    AccessDeclarationAST *AccessDeclaration()
    {
        AccessDeclarationAST *__ast = new (&pool) AccessDeclarationAST;
        return __ast;
    }

    AsmDefinitionAST *AsmDefinition()
    {
        AsmDefinitionAST *__ast = new (&pool) AsmDefinitionAST;
        return __ast;
    }

    BaseSpecifierAST *BaseSpecifier(NameAST *name = 0)
    {
        BaseSpecifierAST *__ast = new (&pool) BaseSpecifierAST;
        __ast->name = name;
        return __ast;
    }

    CompoundLiteralAST *CompoundLiteral(ExpressionAST *type_id = 0, ExpressionAST *initializer = 0)
    {
        CompoundLiteralAST *__ast = new (&pool) CompoundLiteralAST;
        __ast->type_id = type_id;
        __ast->initializer = initializer;
        return __ast;
    }

    QtMethodAST *QtMethod(DeclaratorAST *declarator = 0)
    {
        QtMethodAST *__ast = new (&pool) QtMethodAST;
        __ast->declarator = declarator;
        return __ast;
    }

    BinaryExpressionAST *BinaryExpression(ExpressionAST *left_expression = 0, ExpressionAST *right_expression = 0)
    {
        BinaryExpressionAST *__ast = new (&pool) BinaryExpressionAST;
        __ast->left_expression = left_expression;
        __ast->right_expression = right_expression;
        return __ast;
    }

    CastExpressionAST *CastExpression(ExpressionAST *type_id = 0, ExpressionAST *expression = 0)
    {
        CastExpressionAST *__ast = new (&pool) CastExpressionAST;
        __ast->type_id = type_id;
        __ast->expression = expression;
        return __ast;
    }

    ClassSpecifierAST *ClassSpecifier(NameAST *name = 0)
    {
        ClassSpecifierAST *__ast = new (&pool) ClassSpecifierAST;
        __ast->name = name;
        return __ast;
    }

    CaseStatementAST *CaseStatement(ExpressionAST *expression = 0, StatementAST *statement = 0)
    {
        CaseStatementAST *__ast = new (&pool) CaseStatementAST;
        __ast->expression = expression;
        __ast->statement = statement;
        return __ast;
    }

    CompoundStatementAST *CompoundStatement()
    {
        CompoundStatementAST *__ast = new (&pool) CompoundStatementAST;
        return __ast;
    }

    ConditionAST *Condition(DeclaratorAST *declarator = 0)
    {
        ConditionAST *__ast = new (&pool) ConditionAST;
        __ast->declarator = declarator;
        return __ast;
    }

    ConditionalExpressionAST *ConditionalExpression(ExpressionAST *condition = 0, ExpressionAST *left_expression = 0, ExpressionAST *right_expression = 0)
    {
        ConditionalExpressionAST *__ast = new (&pool) ConditionalExpressionAST;
        __ast->condition = condition;
        __ast->left_expression = left_expression;
        __ast->right_expression = right_expression;
        return __ast;
    }

    CppCastExpressionAST *CppCastExpression(ExpressionAST *type_id = 0, ExpressionAST *expression = 0)
    {
        CppCastExpressionAST *__ast = new (&pool) CppCastExpressionAST;
        __ast->type_id = type_id;
        __ast->expression = expression;
        return __ast;
    }

    CtorInitializerAST *CtorInitializer()
    {
        CtorInitializerAST *__ast = new (&pool) CtorInitializerAST;
        return __ast;
    }

    DeclarationStatementAST *DeclarationStatement(DeclarationAST *declaration = 0)
    {
        DeclarationStatementAST *__ast = new (&pool) DeclarationStatementAST;
        __ast->declaration = declaration;
        return __ast;
    }

    DeclaratorIdAST *DeclaratorId(NameAST *name = 0)
    {
        DeclaratorIdAST *__ast = new (&pool) DeclaratorIdAST;
        __ast->name = name;
        return __ast;
    }

    NestedDeclaratorAST *NestedDeclarator(DeclaratorAST *declarator = 0)
    {
        NestedDeclaratorAST *__ast = new (&pool) NestedDeclaratorAST;
        __ast->declarator = declarator;
        return __ast;
    }

    FunctionDeclaratorAST *FunctionDeclarator(ParameterDeclarationClauseAST *parameters = 0, ExceptionSpecificationAST *exception_specification = 0, ExpressionAST *as_cpp_initializer = 0)
    {
        FunctionDeclaratorAST *__ast = new (&pool) FunctionDeclaratorAST;
        __ast->parameters = parameters;
        __ast->exception_specification = exception_specification;
        __ast->as_cpp_initializer = as_cpp_initializer;
        return __ast;
    }

    ArrayDeclaratorAST *ArrayDeclarator(ExpressionAST *expression = 0)
    {
        ArrayDeclaratorAST *__ast = new (&pool) ArrayDeclaratorAST;
        __ast->expression = expression;
        return __ast;
    }

    DeleteExpressionAST *DeleteExpression(ExpressionAST *expression = 0)
    {
        DeleteExpressionAST *__ast = new (&pool) DeleteExpressionAST;
        __ast->expression = expression;
        return __ast;
    }

    DoStatementAST *DoStatement(StatementAST *statement = 0, ExpressionAST *expression = 0)
    {
        DoStatementAST *__ast = new (&pool) DoStatementAST;
        __ast->statement = statement;
        __ast->expression = expression;
        return __ast;
    }

    NamedTypeSpecifierAST *NamedTypeSpecifier(NameAST *name = 0)
    {
        NamedTypeSpecifierAST *__ast = new (&pool) NamedTypeSpecifierAST;
        __ast->name = name;
        return __ast;
    }

    ElaboratedTypeSpecifierAST *ElaboratedTypeSpecifier(NameAST *name = 0)
    {
        ElaboratedTypeSpecifierAST *__ast = new (&pool) ElaboratedTypeSpecifierAST;
        __ast->name = name;
        return __ast;
    }

    EnumSpecifierAST *EnumSpecifier(NameAST *name = 0)
    {
        EnumSpecifierAST *__ast = new (&pool) EnumSpecifierAST;
        __ast->name = name;
        return __ast;
    }

    EnumeratorAST *Enumerator(ExpressionAST *expression = 0)
    {
        EnumeratorAST *__ast = new (&pool) EnumeratorAST;
        __ast->expression = expression;
        return __ast;
    }

    ExceptionDeclarationAST *ExceptionDeclaration(DeclaratorAST *declarator = 0)
    {
        ExceptionDeclarationAST *__ast = new (&pool) ExceptionDeclarationAST;
        __ast->declarator = declarator;
        return __ast;
    }

    ExceptionSpecificationAST *ExceptionSpecification()
    {
        ExceptionSpecificationAST *__ast = new (&pool) ExceptionSpecificationAST;
        return __ast;
    }

    ExpressionOrDeclarationStatementAST *ExpressionOrDeclarationStatement(StatementAST *expression = 0, StatementAST *declaration = 0)
    {
        ExpressionOrDeclarationStatementAST *__ast = new (&pool) ExpressionOrDeclarationStatementAST;
        __ast->expression = expression;
        __ast->declaration = declaration;
        return __ast;
    }

    ExpressionStatementAST *ExpressionStatement(ExpressionAST *expression = 0)
    {
        ExpressionStatementAST *__ast = new (&pool) ExpressionStatementAST;
        __ast->expression = expression;
        return __ast;
    }

    FunctionDefinitionAST *FunctionDefinition(DeclaratorAST *declarator = 0, CtorInitializerAST *ctor_initializer = 0, StatementAST *function_body = 0)
    {
        FunctionDefinitionAST *__ast = new (&pool) FunctionDefinitionAST;
        __ast->declarator = declarator;
        __ast->ctor_initializer = ctor_initializer;
        __ast->function_body = function_body;
        return __ast;
    }

    ForeachStatementAST *ForeachStatement(DeclaratorAST *declarator = 0, ExpressionAST *initializer = 0, ExpressionAST *expression = 0, StatementAST *statement = 0)
    {
        ForeachStatementAST *__ast = new (&pool) ForeachStatementAST;
        __ast->declarator = declarator;
        __ast->initializer = initializer;
        __ast->expression = expression;
        __ast->statement = statement;
        return __ast;
    }

    ForStatementAST *ForStatement(StatementAST *initializer = 0, ExpressionAST *condition = 0, ExpressionAST *expression = 0, StatementAST *statement = 0)
    {
        ForStatementAST *__ast = new (&pool) ForStatementAST;
        __ast->initializer = initializer;
        __ast->condition = condition;
        __ast->expression = expression;
        __ast->statement = statement;
        return __ast;
    }

    IfStatementAST *IfStatement(ExpressionAST *condition = 0, StatementAST *statement = 0, StatementAST *else_statement = 0)
    {
        IfStatementAST *__ast = new (&pool) IfStatementAST;
        __ast->condition = condition;
        __ast->statement = statement;
        __ast->else_statement = else_statement;
        return __ast;
    }

    ArrayInitializerAST *ArrayInitializer()
    {
        ArrayInitializerAST *__ast = new (&pool) ArrayInitializerAST;
        return __ast;
    }

    LabeledStatementAST *LabeledStatement(StatementAST *statement = 0)
    {
        LabeledStatementAST *__ast = new (&pool) LabeledStatementAST;
        __ast->statement = statement;
        return __ast;
    }

    LinkageBodyAST *LinkageBody()
    {
        LinkageBodyAST *__ast = new (&pool) LinkageBodyAST;
        return __ast;
    }

    LinkageSpecificationAST *LinkageSpecification(DeclarationAST *declaration = 0)
    {
        LinkageSpecificationAST *__ast = new (&pool) LinkageSpecificationAST;
        __ast->declaration = declaration;
        return __ast;
    }

    MemInitializerAST *MemInitializer(NameAST *name = 0)
    {
        MemInitializerAST *__ast = new (&pool) MemInitializerAST;
        __ast->name = name;
        return __ast;
    }

    NestedNameSpecifierAST *NestedNameSpecifier(NameAST *class_or_namespace_name = 0)
    {
        NestedNameSpecifierAST *__ast = new (&pool) NestedNameSpecifierAST;
        __ast->class_or_namespace_name = class_or_namespace_name;
        return __ast;
    }

    QualifiedNameAST *QualifiedName(NameAST *unqualified_name = 0)
    {
        QualifiedNameAST *__ast = new (&pool) QualifiedNameAST;
        __ast->unqualified_name = unqualified_name;
        return __ast;
    }

    OperatorFunctionIdAST *OperatorFunctionId(OperatorAST *op = 0)
    {
        OperatorFunctionIdAST *__ast = new (&pool) OperatorFunctionIdAST;
        __ast->op = op;
        return __ast;
    }

    ConversionFunctionIdAST *ConversionFunctionId()
    {
        ConversionFunctionIdAST *__ast = new (&pool) ConversionFunctionIdAST;
        return __ast;
    }

    SimpleNameAST *SimpleName()
    {
        SimpleNameAST *__ast = new (&pool) SimpleNameAST;
        return __ast;
    }

    DestructorNameAST *DestructorName()
    {
        DestructorNameAST *__ast = new (&pool) DestructorNameAST;
        return __ast;
    }

    TemplateIdAST *TemplateId()
    {
        TemplateIdAST *__ast = new (&pool) TemplateIdAST;
        return __ast;
    }

    NamespaceAST *Namespace(DeclarationAST *linkage_body = 0)
    {
        NamespaceAST *__ast = new (&pool) NamespaceAST;
        __ast->linkage_body = linkage_body;
        return __ast;
    }

    NamespaceAliasDefinitionAST *NamespaceAliasDefinition(NameAST *name = 0)
    {
        NamespaceAliasDefinitionAST *__ast = new (&pool) NamespaceAliasDefinitionAST;
        __ast->name = name;
        return __ast;
    }

    NewPlacementAST *NewPlacement()
    {
        NewPlacementAST *__ast = new (&pool) NewPlacementAST;
        return __ast;
    }

    NewArrayDeclaratorAST *NewArrayDeclarator(ExpressionAST *expression = 0)
    {
        NewArrayDeclaratorAST *__ast = new (&pool) NewArrayDeclaratorAST;
        __ast->expression = expression;
        return __ast;
    }

    NewExpressionAST *NewExpression(NewPlacementAST *new_placement = 0, ExpressionAST *type_id = 0, NewTypeIdAST *new_type_id = 0, NewInitializerAST *new_initializer = 0)
    {
        NewExpressionAST *__ast = new (&pool) NewExpressionAST;
        __ast->new_placement = new_placement;
        __ast->type_id = type_id;
        __ast->new_type_id = new_type_id;
        __ast->new_initializer = new_initializer;
        return __ast;
    }

    NewInitializerAST *NewInitializer(ExpressionAST *expression = 0)
    {
        NewInitializerAST *__ast = new (&pool) NewInitializerAST;
        __ast->expression = expression;
        return __ast;
    }

    NewTypeIdAST *NewTypeId()
    {
        NewTypeIdAST *__ast = new (&pool) NewTypeIdAST;
        return __ast;
    }

    OperatorAST *Operator()
    {
        OperatorAST *__ast = new (&pool) OperatorAST;
        return __ast;
    }

    ParameterDeclarationAST *ParameterDeclaration(DeclaratorAST *declarator = 0, ExpressionAST *expression = 0)
    {
        ParameterDeclarationAST *__ast = new (&pool) ParameterDeclarationAST;
        __ast->declarator = declarator;
        __ast->expression = expression;
        return __ast;
    }

    ParameterDeclarationClauseAST *ParameterDeclarationClause()
    {
        ParameterDeclarationClauseAST *__ast = new (&pool) ParameterDeclarationClauseAST;
        return __ast;
    }

    CallAST *Call()
    {
        CallAST *__ast = new (&pool) CallAST;
        return __ast;
    }

    ArrayAccessAST *ArrayAccess(ExpressionAST *expression = 0)
    {
        ArrayAccessAST *__ast = new (&pool) ArrayAccessAST;
        __ast->expression = expression;
        return __ast;
    }

    PostIncrDecrAST *PostIncrDecr()
    {
        PostIncrDecrAST *__ast = new (&pool) PostIncrDecrAST;
        return __ast;
    }

    MemberAccessAST *MemberAccess(NameAST *member_name = 0)
    {
        MemberAccessAST *__ast = new (&pool) MemberAccessAST;
        __ast->member_name = member_name;
        return __ast;
    }

    TypeidExpressionAST *TypeidExpression(ExpressionAST *expression = 0)
    {
        TypeidExpressionAST *__ast = new (&pool) TypeidExpressionAST;
        __ast->expression = expression;
        return __ast;
    }

    TypenameCallExpressionAST *TypenameCallExpression(NameAST *name = 0)
    {
        TypenameCallExpressionAST *__ast = new (&pool) TypenameCallExpressionAST;
        __ast->name = name;
        return __ast;
    }

    TypeConstructorCallAST *TypeConstructorCall()
    {
        TypeConstructorCallAST *__ast = new (&pool) TypeConstructorCallAST;
        return __ast;
    }

    PostfixExpressionAST *PostfixExpression(ExpressionAST *base_expression = 0)
    {
        PostfixExpressionAST *__ast = new (&pool) PostfixExpressionAST;
        __ast->base_expression = base_expression;
        return __ast;
    }

    PointerToMemberAST *PointerToMember()
    {
        PointerToMemberAST *__ast = new (&pool) PointerToMemberAST;
        return __ast;
    }

    PointerAST *Pointer()
    {
        PointerAST *__ast = new (&pool) PointerAST;
        return __ast;
    }

    ReferenceAST *Reference()
    {
        ReferenceAST *__ast = new (&pool) ReferenceAST;
        return __ast;
    }

    BreakStatementAST *BreakStatement()
    {
        BreakStatementAST *__ast = new (&pool) BreakStatementAST;
        return __ast;
    }

    ContinueStatementAST *ContinueStatement()
    {
        ContinueStatementAST *__ast = new (&pool) ContinueStatementAST;
        return __ast;
    }

    GotoStatementAST *GotoStatement()
    {
        GotoStatementAST *__ast = new (&pool) GotoStatementAST;
        return __ast;
    }

    ReturnStatementAST *ReturnStatement(ExpressionAST *expression = 0)
    {
        ReturnStatementAST *__ast = new (&pool) ReturnStatementAST;
        __ast->expression = expression;
        return __ast;
    }

    SizeofExpressionAST *SizeofExpression(ExpressionAST *expression = 0)
    {
        SizeofExpressionAST *__ast = new (&pool) SizeofExpressionAST;
        __ast->expression = expression;
        return __ast;
    }

    NumericLiteralAST *NumericLiteral()
    {
        NumericLiteralAST *__ast = new (&pool) NumericLiteralAST;
        return __ast;
    }

    BoolLiteralAST *BoolLiteral()
    {
        BoolLiteralAST *__ast = new (&pool) BoolLiteralAST;
        return __ast;
    }

    ThisExpressionAST *ThisExpression()
    {
        ThisExpressionAST *__ast = new (&pool) ThisExpressionAST;
        return __ast;
    }

    NestedExpressionAST *NestedExpression(ExpressionAST *expression = 0)
    {
        NestedExpressionAST *__ast = new (&pool) NestedExpressionAST;
        __ast->expression = expression;
        return __ast;
    }

    StringLiteralAST *StringLiteral(StringLiteralAST *next = 0)
    {
        StringLiteralAST *__ast = new (&pool) StringLiteralAST;
        __ast->next = next;
        return __ast;
    }

    SwitchStatementAST *SwitchStatement(ExpressionAST *condition = 0, StatementAST *statement = 0)
    {
        SwitchStatementAST *__ast = new (&pool) SwitchStatementAST;
        __ast->condition = condition;
        __ast->statement = statement;
        return __ast;
    }

    TemplateDeclarationAST *TemplateDeclaration(DeclarationAST *declaration = 0)
    {
        TemplateDeclarationAST *__ast = new (&pool) TemplateDeclarationAST;
        __ast->declaration = declaration;
        return __ast;
    }

    ThrowExpressionAST *ThrowExpression(ExpressionAST *expression = 0)
    {
        ThrowExpressionAST *__ast = new (&pool) ThrowExpressionAST;
        __ast->expression = expression;
        return __ast;
    }

    TranslationUnitAST *TranslationUnit()
    {
        TranslationUnitAST *__ast = new (&pool) TranslationUnitAST;
        return __ast;
    }

    TryBlockStatementAST *TryBlockStatement(StatementAST *statement = 0)
    {
        TryBlockStatementAST *__ast = new (&pool) TryBlockStatementAST;
        __ast->statement = statement;
        return __ast;
    }

    CatchClauseAST *CatchClause(ExceptionDeclarationAST *exception_declaration = 0, StatementAST *statement = 0)
    {
        CatchClauseAST *__ast = new (&pool) CatchClauseAST;
        __ast->exception_declaration = exception_declaration;
        __ast->statement = statement;
        return __ast;
    }

    TypeIdAST *TypeId(DeclaratorAST *declarator = 0)
    {
        TypeIdAST *__ast = new (&pool) TypeIdAST;
        __ast->declarator = declarator;
        return __ast;
    }

    TypenameTypeParameterAST *TypenameTypeParameter(NameAST *name = 0, ExpressionAST *type_id = 0)
    {
        TypenameTypeParameterAST *__ast = new (&pool) TypenameTypeParameterAST;
        __ast->name = name;
        __ast->type_id = type_id;
        return __ast;
    }

    TemplateTypeParameterAST *TemplateTypeParameter(NameAST *name = 0, ExpressionAST *type_id = 0)
    {
        TemplateTypeParameterAST *__ast = new (&pool) TemplateTypeParameterAST;
        __ast->name = name;
        __ast->type_id = type_id;
        return __ast;
    }

    UnaryExpressionAST *UnaryExpression(ExpressionAST *expression = 0)
    {
        UnaryExpressionAST *__ast = new (&pool) UnaryExpressionAST;
        __ast->expression = expression;
        return __ast;
    }

    UsingAST *Using(NameAST *name = 0)
    {
        UsingAST *__ast = new (&pool) UsingAST;
        __ast->name = name;
        return __ast;
    }

    UsingDirectiveAST *UsingDirective(NameAST *name = 0)
    {
        UsingDirectiveAST *__ast = new (&pool) UsingDirectiveAST;
        __ast->name = name;
        return __ast;
    }

    WhileStatementAST *WhileStatement(ExpressionAST *condition = 0, StatementAST *statement = 0)
    {
        WhileStatementAST *__ast = new (&pool) WhileStatementAST;
        __ast->condition = condition;
        __ast->statement = statement;
        return __ast;
    }

    ObjCClassForwardDeclarationAST *ObjCClassForwardDeclaration()
    {
        ObjCClassForwardDeclarationAST *__ast = new (&pool) ObjCClassForwardDeclarationAST;
        return __ast;
    }

    ObjCClassDeclarationAST *ObjCClassDeclaration(NameAST *class_name = 0, NameAST *category_name = 0, NameAST *superclass = 0, ObjCProtocolRefsAST *protocol_refs = 0, ObjCInstanceVariablesDeclarationAST *inst_vars_decl = 0)
    {
        ObjCClassDeclarationAST *__ast = new (&pool) ObjCClassDeclarationAST;
        __ast->class_name = class_name;
        __ast->category_name = category_name;
        __ast->superclass = superclass;
        __ast->protocol_refs = protocol_refs;
        __ast->inst_vars_decl = inst_vars_decl;
        return __ast;
    }

    ObjCProtocolForwardDeclarationAST *ObjCProtocolForwardDeclaration()
    {
        ObjCProtocolForwardDeclarationAST *__ast = new (&pool) ObjCProtocolForwardDeclarationAST;
        return __ast;
    }

    ObjCProtocolDeclarationAST *ObjCProtocolDeclaration(NameAST *name = 0, ObjCProtocolRefsAST *protocol_refs = 0)
    {
        ObjCProtocolDeclarationAST *__ast = new (&pool) ObjCProtocolDeclarationAST;
        __ast->name = name;
        __ast->protocol_refs = protocol_refs;
        return __ast;
    }

    ObjCProtocolRefsAST *ObjCProtocolRefs()
    {
        ObjCProtocolRefsAST *__ast = new (&pool) ObjCProtocolRefsAST;
        return __ast;
    }

    ObjCMessageArgumentAST *ObjCMessageArgument(ExpressionAST *parameter_value_expression = 0)
    {
        ObjCMessageArgumentAST *__ast = new (&pool) ObjCMessageArgumentAST;
        __ast->parameter_value_expression = parameter_value_expression;
        return __ast;
    }

    ObjCMessageExpressionAST *ObjCMessageExpression(ExpressionAST *receiver_expression = 0, ObjCSelectorAST *selector = 0)
    {
        ObjCMessageExpressionAST *__ast = new (&pool) ObjCMessageExpressionAST;
        __ast->receiver_expression = receiver_expression;
        __ast->selector = selector;
        return __ast;
    }

    ObjCProtocolExpressionAST *ObjCProtocolExpression()
    {
        ObjCProtocolExpressionAST *__ast = new (&pool) ObjCProtocolExpressionAST;
        return __ast;
    }

    ObjCTypeNameAST *ObjCTypeName(ExpressionAST *type_id = 0)
    {
        ObjCTypeNameAST *__ast = new (&pool) ObjCTypeNameAST;
        __ast->type_id = type_id;
        return __ast;
    }

    ObjCEncodeExpressionAST *ObjCEncodeExpression(ObjCTypeNameAST *type_name = 0)
    {
        ObjCEncodeExpressionAST *__ast = new (&pool) ObjCEncodeExpressionAST;
        __ast->type_name = type_name;
        return __ast;
    }

    ObjCSelectorWithoutArgumentsAST *ObjCSelectorWithoutArguments()
    {
        ObjCSelectorWithoutArgumentsAST *__ast = new (&pool) ObjCSelectorWithoutArgumentsAST;
        return __ast;
    }

    ObjCSelectorArgumentAST *ObjCSelectorArgument()
    {
        ObjCSelectorArgumentAST *__ast = new (&pool) ObjCSelectorArgumentAST;
        return __ast;
    }

    ObjCSelectorWithArgumentsAST *ObjCSelectorWithArguments()
    {
        ObjCSelectorWithArgumentsAST *__ast = new (&pool) ObjCSelectorWithArgumentsAST;
        return __ast;
    }

    ObjCSelectorExpressionAST *ObjCSelectorExpression(ObjCSelectorAST *selector = 0)
    {
        ObjCSelectorExpressionAST *__ast = new (&pool) ObjCSelectorExpressionAST;
        __ast->selector = selector;
        return __ast;
    }

    ObjCInstanceVariablesDeclarationAST *ObjCInstanceVariablesDeclaration()
    {
        ObjCInstanceVariablesDeclarationAST *__ast = new (&pool) ObjCInstanceVariablesDeclarationAST;
        return __ast;
    }

    ObjCVisibilityDeclarationAST *ObjCVisibilityDeclaration()
    {
        ObjCVisibilityDeclarationAST *__ast = new (&pool) ObjCVisibilityDeclarationAST;
        return __ast;
    }

    ObjCPropertyAttributeAST *ObjCPropertyAttribute(ObjCSelectorAST *method_selector = 0)
    {
        ObjCPropertyAttributeAST *__ast = new (&pool) ObjCPropertyAttributeAST;
        __ast->method_selector = method_selector;
        return __ast;
    }

    ObjCPropertyDeclarationAST *ObjCPropertyDeclaration(DeclarationAST *simple_declaration = 0)
    {
        ObjCPropertyDeclarationAST *__ast = new (&pool) ObjCPropertyDeclarationAST;
        __ast->simple_declaration = simple_declaration;
        return __ast;
    }

    ObjCMessageArgumentDeclarationAST *ObjCMessageArgumentDeclaration(ObjCTypeNameAST *type_name = 0)
    {
        ObjCMessageArgumentDeclarationAST *__ast = new (&pool) ObjCMessageArgumentDeclarationAST;
        __ast->type_name = type_name;
        return __ast;
    }

    ObjCMethodPrototypeAST *ObjCMethodPrototype(ObjCTypeNameAST *type_name = 0, ObjCSelectorAST *selector = 0)
    {
        ObjCMethodPrototypeAST *__ast = new (&pool) ObjCMethodPrototypeAST;
        __ast->type_name = type_name;
        __ast->selector = selector;
        return __ast;
    }

    ObjCMethodDeclarationAST *ObjCMethodDeclaration(ObjCMethodPrototypeAST *method_prototype = 0, StatementAST *function_body = 0)
    {
        ObjCMethodDeclarationAST *__ast = new (&pool) ObjCMethodDeclarationAST;
        __ast->method_prototype = method_prototype;
        __ast->function_body = function_body;
        return __ast;
    }

    ObjCSynthesizedPropertyAST *ObjCSynthesizedProperty()
    {
        ObjCSynthesizedPropertyAST *__ast = new (&pool) ObjCSynthesizedPropertyAST;
        return __ast;
    }

    ObjCSynthesizedPropertiesDeclarationAST *ObjCSynthesizedPropertiesDeclaration()
    {
        ObjCSynthesizedPropertiesDeclarationAST *__ast = new (&pool) ObjCSynthesizedPropertiesDeclarationAST;
        return __ast;
    }

    ObjCDynamicPropertiesDeclarationAST *ObjCDynamicPropertiesDeclaration()
    {
        ObjCDynamicPropertiesDeclarationAST *__ast = new (&pool) ObjCDynamicPropertiesDeclarationAST;
        return __ast;
    }

    ObjCFastEnumerationAST *ObjCFastEnumeration(DeclaratorAST *declarator = 0, ExpressionAST *initializer = 0, ExpressionAST *fast_enumeratable_expression = 0, StatementAST *body_statement = 0)
    {
        ObjCFastEnumerationAST *__ast = new (&pool) ObjCFastEnumerationAST;
        __ast->declarator = declarator;
        __ast->initializer = initializer;
        __ast->fast_enumeratable_expression = fast_enumeratable_expression;
        __ast->body_statement = body_statement;
        return __ast;
    }

    ObjCSynchronizedStatementAST *ObjCSynchronizedStatement(ExpressionAST *synchronized_object = 0, StatementAST *statement = 0)
    {
        ObjCSynchronizedStatementAST *__ast = new (&pool) ObjCSynchronizedStatementAST;
        __ast->synchronized_object = synchronized_object;
        __ast->statement = statement;
        return __ast;
    }

};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_AST_PATTERN_BUILDER_H
