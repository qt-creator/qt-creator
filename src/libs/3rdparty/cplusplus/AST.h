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

#pragma once

#include "CPlusPlusForwardDeclarations.h"
#include "ASTfwd.h"
#include "MemoryPool.h"

namespace CPlusPlus {

template <typename Tptr>
class CPLUSPLUS_EXPORT List: public Managed
{
    List(const List &other);
    void operator =(const List &other);

public:
    List()
        : value(Tptr()), next(0)
    { }

    List(const Tptr &value)
        : value(value), next(0)
    { }

    int firstToken() const
    {
        if (value)
            return value->firstToken();

        // ### CPP_CHECK(0);
        return 0;
    }

    int lastToken() const
    {
        Tptr lv = lastValue();

        if (lv)
            return lv->lastToken();

        // ### CPP_CHECK(0);
        return 0;
    }

    Tptr lastValue() const
    {
        Tptr lastValue = 0;

        for (const List *it = this; it; it = it->next) {
            if (it->value)
                lastValue = it->value;
        }

        return lastValue;
    }

    Tptr value;
    List *next;
};

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

    template <typename Tptr>
    static void accept(List<Tptr> *it, ASTVisitor *visitor)
    {
        for (; it; it = it->next)
            accept(it->value, visitor);
    }

    static bool match(AST *ast, AST *pattern, ASTMatcher *matcher);
    bool match(AST *pattern, ASTMatcher *matcher);

    template <typename Tptr>
    static bool match(List<Tptr> *it, List<Tptr> *patternIt, ASTMatcher *matcher)
    {
        while (it && patternIt) {
            if (! match(it->value, patternIt->value, matcher))
                return false;

            it = it->next;
            patternIt = patternIt->next;
        }

        if (! it && ! patternIt)
            return true;

        return false;
    }

    virtual int firstToken() const = 0;
    virtual int lastToken() const = 0;

    virtual AST *clone(MemoryPool *pool) const = 0;

    virtual AccessDeclarationAST *asAccessDeclaration() { return nullptr; }
    virtual AliasDeclarationAST *asAliasDeclaration() { return nullptr; }
    virtual AlignmentSpecifierAST *asAlignmentSpecifier() { return nullptr; }
    virtual AlignofExpressionAST *asAlignofExpression() { return nullptr; }
    virtual AnonymousNameAST *asAnonymousName() { return nullptr; }
    virtual ArrayAccessAST *asArrayAccess() { return nullptr; }
    virtual ArrayDeclaratorAST *asArrayDeclarator() { return nullptr; }
    virtual ArrayInitializerAST *asArrayInitializer() { return nullptr; }
    virtual AsmDefinitionAST *asAsmDefinition() { return nullptr; }
    virtual AttributeSpecifierAST *asAttributeSpecifier() { return nullptr; }
    virtual BaseSpecifierAST *asBaseSpecifier() { return nullptr; }
    virtual BinaryExpressionAST *asBinaryExpression() { return nullptr; }
    virtual BoolLiteralAST *asBoolLiteral() { return nullptr; }
    virtual BracedInitializerAST *asBracedInitializer() { return nullptr; }
    virtual BracketDesignatorAST *asBracketDesignator() { return nullptr; }
    virtual BreakStatementAST *asBreakStatement() { return nullptr; }
    virtual CallAST *asCall() { return nullptr; }
    virtual CaptureAST *asCapture() { return nullptr; }
    virtual CaseStatementAST *asCaseStatement() { return nullptr; }
    virtual CastExpressionAST *asCastExpression() { return nullptr; }
    virtual CatchClauseAST *asCatchClause() { return nullptr; }
    virtual ClassSpecifierAST *asClassSpecifier() { return nullptr; }
    virtual CompoundExpressionAST *asCompoundExpression() { return nullptr; }
    virtual CompoundLiteralAST *asCompoundLiteral() { return nullptr; }
    virtual CompoundStatementAST *asCompoundStatement() { return nullptr; }
    virtual ConditionAST *asCondition() { return nullptr; }
    virtual ConditionalExpressionAST *asConditionalExpression() { return nullptr; }
    virtual ContinueStatementAST *asContinueStatement() { return nullptr; }
    virtual ConversionFunctionIdAST *asConversionFunctionId() { return nullptr; }
    virtual CoreDeclaratorAST *asCoreDeclarator() { return nullptr; }
    virtual CppCastExpressionAST *asCppCastExpression() { return nullptr; }
    virtual CtorInitializerAST *asCtorInitializer() { return nullptr; }
    virtual DeclarationAST *asDeclaration() { return nullptr; }
    virtual DeclarationStatementAST *asDeclarationStatement() { return nullptr; }
    virtual DeclaratorAST *asDeclarator() { return nullptr; }
    virtual DeclaratorIdAST *asDeclaratorId() { return nullptr; }
    virtual DecltypeSpecifierAST *asDecltypeSpecifier() { return nullptr; }
    virtual DeleteExpressionAST *asDeleteExpression() { return nullptr; }
    virtual DesignatedInitializerAST *asDesignatedInitializer() { return nullptr; }
    virtual DesignatorAST *asDesignator() { return nullptr; }
    virtual DestructorNameAST *asDestructorName() { return nullptr; }
    virtual DoStatementAST *asDoStatement() { return nullptr; }
    virtual DotDesignatorAST *asDotDesignator() { return nullptr; }
    virtual DynamicExceptionSpecificationAST *asDynamicExceptionSpecification() { return nullptr; }
    virtual ElaboratedTypeSpecifierAST *asElaboratedTypeSpecifier() { return nullptr; }
    virtual EmptyDeclarationAST *asEmptyDeclaration() { return nullptr; }
    virtual EnumSpecifierAST *asEnumSpecifier() { return nullptr; }
    virtual EnumeratorAST *asEnumerator() { return nullptr; }
    virtual ExceptionDeclarationAST *asExceptionDeclaration() { return nullptr; }
    virtual ExceptionSpecificationAST *asExceptionSpecification() { return nullptr; }
    virtual ExpressionAST *asExpression() { return nullptr; }
    virtual ExpressionListParenAST *asExpressionListParen() { return nullptr; }
    virtual ExpressionOrDeclarationStatementAST *asExpressionOrDeclarationStatement() { return nullptr; }
    virtual ExpressionStatementAST *asExpressionStatement() { return nullptr; }
    virtual ForStatementAST *asForStatement() { return nullptr; }
    virtual ForeachStatementAST *asForeachStatement() { return nullptr; }
    virtual FunctionDeclaratorAST *asFunctionDeclarator() { return nullptr; }
    virtual FunctionDefinitionAST *asFunctionDefinition() { return nullptr; }
    virtual GnuAttributeAST *asGnuAttribute() { return nullptr; }
    virtual GnuAttributeSpecifierAST *asGnuAttributeSpecifier() { return nullptr; }
    virtual GotoStatementAST *asGotoStatement() { return nullptr; }
    virtual IdExpressionAST *asIdExpression() { return nullptr; }
    virtual IfStatementAST *asIfStatement() { return nullptr; }
    virtual LabeledStatementAST *asLabeledStatement() { return nullptr; }
    virtual LambdaCaptureAST *asLambdaCapture() { return nullptr; }
    virtual LambdaDeclaratorAST *asLambdaDeclarator() { return nullptr; }
    virtual LambdaExpressionAST *asLambdaExpression() { return nullptr; }
    virtual LambdaIntroducerAST *asLambdaIntroducer() { return nullptr; }
    virtual LinkageBodyAST *asLinkageBody() { return nullptr; }
    virtual LinkageSpecificationAST *asLinkageSpecification() { return nullptr; }
    virtual MemInitializerAST *asMemInitializer() { return nullptr; }
    virtual MemberAccessAST *asMemberAccess() { return nullptr; }
    virtual NameAST *asName() { return nullptr; }
    virtual NamedTypeSpecifierAST *asNamedTypeSpecifier() { return nullptr; }
    virtual NamespaceAST *asNamespace() { return nullptr; }
    virtual NamespaceAliasDefinitionAST *asNamespaceAliasDefinition() { return nullptr; }
    virtual NestedDeclaratorAST *asNestedDeclarator() { return nullptr; }
    virtual NestedExpressionAST *asNestedExpression() { return nullptr; }
    virtual NestedNameSpecifierAST *asNestedNameSpecifier() { return nullptr; }
    virtual NewArrayDeclaratorAST *asNewArrayDeclarator() { return nullptr; }
    virtual NewExpressionAST *asNewExpression() { return nullptr; }
    virtual NewTypeIdAST *asNewTypeId() { return nullptr; }
    virtual NoExceptOperatorExpressionAST *asNoExceptOperatorExpression() { return nullptr; }
    virtual NoExceptSpecificationAST *asNoExceptSpecification() { return nullptr; }
    virtual NumericLiteralAST *asNumericLiteral() { return nullptr; }
    virtual ObjCClassDeclarationAST *asObjCClassDeclaration() { return nullptr; }
    virtual ObjCClassForwardDeclarationAST *asObjCClassForwardDeclaration() { return nullptr; }
    virtual ObjCDynamicPropertiesDeclarationAST *asObjCDynamicPropertiesDeclaration() { return nullptr; }
    virtual ObjCEncodeExpressionAST *asObjCEncodeExpression() { return nullptr; }
    virtual ObjCFastEnumerationAST *asObjCFastEnumeration() { return nullptr; }
    virtual ObjCInstanceVariablesDeclarationAST *asObjCInstanceVariablesDeclaration() { return nullptr; }
    virtual ObjCMessageArgumentAST *asObjCMessageArgument() { return nullptr; }
    virtual ObjCMessageArgumentDeclarationAST *asObjCMessageArgumentDeclaration() { return nullptr; }
    virtual ObjCMessageExpressionAST *asObjCMessageExpression() { return nullptr; }
    virtual ObjCMethodDeclarationAST *asObjCMethodDeclaration() { return nullptr; }
    virtual ObjCMethodPrototypeAST *asObjCMethodPrototype() { return nullptr; }
    virtual ObjCPropertyAttributeAST *asObjCPropertyAttribute() { return nullptr; }
    virtual ObjCPropertyDeclarationAST *asObjCPropertyDeclaration() { return nullptr; }
    virtual ObjCProtocolDeclarationAST *asObjCProtocolDeclaration() { return nullptr; }
    virtual ObjCProtocolExpressionAST *asObjCProtocolExpression() { return nullptr; }
    virtual ObjCProtocolForwardDeclarationAST *asObjCProtocolForwardDeclaration() { return nullptr; }
    virtual ObjCProtocolRefsAST *asObjCProtocolRefs() { return nullptr; }
    virtual ObjCSelectorAST *asObjCSelector() { return nullptr; }
    virtual ObjCSelectorArgumentAST *asObjCSelectorArgument() { return nullptr; }
    virtual ObjCSelectorExpressionAST *asObjCSelectorExpression() { return nullptr; }
    virtual ObjCSynchronizedStatementAST *asObjCSynchronizedStatement() { return nullptr; }
    virtual ObjCSynthesizedPropertiesDeclarationAST *asObjCSynthesizedPropertiesDeclaration() { return nullptr; }
    virtual ObjCSynthesizedPropertyAST *asObjCSynthesizedProperty() { return nullptr; }
    virtual ObjCTypeNameAST *asObjCTypeName() { return nullptr; }
    virtual ObjCVisibilityDeclarationAST *asObjCVisibilityDeclaration() { return nullptr; }
    virtual OperatorAST *asOperator() { return nullptr; }
    virtual OperatorFunctionIdAST *asOperatorFunctionId() { return nullptr; }
    virtual ParameterDeclarationAST *asParameterDeclaration() { return nullptr; }
    virtual ParameterDeclarationClauseAST *asParameterDeclarationClause() { return nullptr; }
    virtual PointerAST *asPointer() { return nullptr; }
    virtual PointerLiteralAST *asPointerLiteral() { return nullptr; }
    virtual PointerToMemberAST *asPointerToMember() { return nullptr; }
    virtual PostIncrDecrAST *asPostIncrDecr() { return nullptr; }
    virtual PostfixAST *asPostfix() { return nullptr; }
    virtual PostfixDeclaratorAST *asPostfixDeclarator() { return nullptr; }
    virtual PtrOperatorAST *asPtrOperator() { return nullptr; }
    virtual QtEnumDeclarationAST *asQtEnumDeclaration() { return nullptr; }
    virtual QtFlagsDeclarationAST *asQtFlagsDeclaration() { return nullptr; }
    virtual QtInterfaceNameAST *asQtInterfaceName() { return nullptr; }
    virtual QtInterfacesDeclarationAST *asQtInterfacesDeclaration() { return nullptr; }
    virtual QtMemberDeclarationAST *asQtMemberDeclaration() { return nullptr; }
    virtual QtMethodAST *asQtMethod() { return nullptr; }
    virtual QtObjectTagAST *asQtObjectTag() { return nullptr; }
    virtual QtPrivateSlotAST *asQtPrivateSlot() { return nullptr; }
    virtual QtPropertyDeclarationAST *asQtPropertyDeclaration() { return nullptr; }
    virtual QtPropertyDeclarationItemAST *asQtPropertyDeclarationItem() { return nullptr; }
    virtual QualifiedNameAST *asQualifiedName() { return nullptr; }
    virtual RangeBasedForStatementAST *asRangeBasedForStatement() { return nullptr; }
    virtual ReferenceAST *asReference() { return nullptr; }
    virtual ReturnStatementAST *asReturnStatement() { return nullptr; }
    virtual SimpleDeclarationAST *asSimpleDeclaration() { return nullptr; }
    virtual SimpleNameAST *asSimpleName() { return nullptr; }
    virtual SimpleSpecifierAST *asSimpleSpecifier() { return nullptr; }
    virtual SizeofExpressionAST *asSizeofExpression() { return nullptr; }
    virtual SpecifierAST *asSpecifier() { return nullptr; }
    virtual StatementAST *asStatement() { return nullptr; }
    virtual StaticAssertDeclarationAST *asStaticAssertDeclaration() { return nullptr; }
    virtual StringLiteralAST *asStringLiteral() { return nullptr; }
    virtual SwitchStatementAST *asSwitchStatement() { return nullptr; }
    virtual TemplateDeclarationAST *asTemplateDeclaration() { return nullptr; }
    virtual TemplateIdAST *asTemplateId() { return nullptr; }
    virtual TemplateTypeParameterAST *asTemplateTypeParameter() { return nullptr; }
    virtual ThisExpressionAST *asThisExpression() { return nullptr; }
    virtual ThrowExpressionAST *asThrowExpression() { return nullptr; }
    virtual TrailingReturnTypeAST *asTrailingReturnType() { return nullptr; }
    virtual TranslationUnitAST *asTranslationUnit() { return nullptr; }
    virtual TryBlockStatementAST *asTryBlockStatement() { return nullptr; }
    virtual TypeConstructorCallAST *asTypeConstructorCall() { return nullptr; }
    virtual TypeIdAST *asTypeId() { return nullptr; }
    virtual TypeidExpressionAST *asTypeidExpression() { return nullptr; }
    virtual TypenameCallExpressionAST *asTypenameCallExpression() { return nullptr; }
    virtual TypenameTypeParameterAST *asTypenameTypeParameter() { return nullptr; }
    virtual TypeofSpecifierAST *asTypeofSpecifier() { return nullptr; }
    virtual UnaryExpressionAST *asUnaryExpression() { return nullptr; }
    virtual UsingAST *asUsing() { return nullptr; }
    virtual UsingDirectiveAST *asUsingDirective() { return nullptr; }
    virtual WhileStatementAST *asWhileStatement() { return nullptr; }

protected:
    virtual void accept0(ASTVisitor *visitor) = 0;
    virtual bool match0(AST *, ASTMatcher *) = 0;
};

class CPLUSPLUS_EXPORT StatementAST: public AST
{
public:
    StatementAST()
    {}

    virtual StatementAST *asStatement() { return this; }

    virtual StatementAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT ExpressionAST: public AST
{
public:
    ExpressionAST()
    {}

    virtual ExpressionAST *asExpression() { return this; }

    virtual ExpressionAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT DeclarationAST: public AST
{
public:
    DeclarationAST()
    {}

    virtual DeclarationAST *asDeclaration() { return this; }

    virtual DeclarationAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT NameAST: public AST
{
public: // annotations
    const Name *name;

public:
    NameAST()
        : name(0)
    {}

    virtual NameAST *asName() { return this; }

    virtual NameAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT SpecifierAST: public AST
{
public:
    SpecifierAST()
    {}

    virtual SpecifierAST *asSpecifier() { return this; }

    virtual SpecifierAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PtrOperatorAST: public AST
{
public:
    PtrOperatorAST()
    {}

    virtual PtrOperatorAST *asPtrOperator() { return this; }

    virtual PtrOperatorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PostfixAST: public ExpressionAST
{
public:
    PostfixAST()
    {}

    virtual PostfixAST *asPostfix() { return this; }

    virtual PostfixAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT CoreDeclaratorAST: public AST
{
public:
    CoreDeclaratorAST()
    {}

    virtual CoreDeclaratorAST *asCoreDeclarator() { return this; }

    virtual CoreDeclaratorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PostfixDeclaratorAST: public AST
{
public:
    PostfixDeclaratorAST()
    {}

    virtual PostfixDeclaratorAST *asPostfixDeclarator() { return this; }

    virtual PostfixDeclaratorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT ObjCSelectorArgumentAST: public AST
{
public:
    int name_token;
    int colon_token;

public:
    ObjCSelectorArgumentAST()
        : name_token(0)
        , colon_token(0)
    {}

    virtual ObjCSelectorArgumentAST *asObjCSelectorArgument() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCSelectorArgumentAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCSelectorAST: public NameAST
{
public:
    ObjCSelectorArgumentListAST *selector_argument_list;

public:
    ObjCSelectorAST()
        : selector_argument_list(0)
    {}

    virtual ObjCSelectorAST *asObjCSelector() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCSelectorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT SimpleSpecifierAST: public SpecifierAST
{
public:
    int specifier_token;

public:
    SimpleSpecifierAST()
        : specifier_token(0)
    {}

    virtual SimpleSpecifierAST *asSimpleSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual SimpleSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT AttributeSpecifierAST: public SpecifierAST
{
public:
    AttributeSpecifierAST()
    {}

    virtual AttributeSpecifierAST *asAttributeSpecifier() { return this; }

    virtual AttributeSpecifierAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT AlignmentSpecifierAST: public AttributeSpecifierAST
{
public:
    int align_token;
    int lparen_token;
    ExpressionAST *typeIdExprOrAlignmentExpr;
    int ellipses_token;
    int rparen_token;

public:
    AlignmentSpecifierAST()
        : align_token(0)
        , lparen_token(0)
        , typeIdExprOrAlignmentExpr(0)
        , ellipses_token(0)
        , rparen_token(0)
    {}

    virtual AlignmentSpecifierAST *asAlignmentSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual AlignmentSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};


class CPLUSPLUS_EXPORT GnuAttributeSpecifierAST: public AttributeSpecifierAST
{
public:
    int attribute_token;
    int first_lparen_token;
    int second_lparen_token;
    GnuAttributeListAST *attribute_list;
    int first_rparen_token;
    int second_rparen_token;

public:
    GnuAttributeSpecifierAST()
        : attribute_token(0)
        , first_lparen_token(0)
        , second_lparen_token(0)
        , attribute_list(0)
        , first_rparen_token(0)
        , second_rparen_token(0)
    {}

    virtual GnuAttributeSpecifierAST *asGnuAttributeSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual GnuAttributeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT GnuAttributeAST: public AST
{
public:
    int identifier_token;
    int lparen_token;
    int tag_token;
    ExpressionListAST *expression_list;
    int rparen_token;

public:
    GnuAttributeAST()
        : identifier_token(0)
        , lparen_token(0)
        , tag_token(0)
        , expression_list(0)
        , rparen_token(0)
    {}

    virtual GnuAttributeAST *asGnuAttribute() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual GnuAttributeAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TypeofSpecifierAST: public SpecifierAST
{
public:
    int typeof_token;
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;

public:
    TypeofSpecifierAST()
        : typeof_token(0)
        , lparen_token(0)
        , expression(0)
        , rparen_token(0)
    {}

    virtual TypeofSpecifierAST *asTypeofSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TypeofSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT DecltypeSpecifierAST: public SpecifierAST
{
public:
    int decltype_token;
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;

public:
    DecltypeSpecifierAST()
        : decltype_token(0)
        , lparen_token(0)
        , expression(0)
        , rparen_token(0)
    {}

    virtual DecltypeSpecifierAST *asDecltypeSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DecltypeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT DeclaratorAST: public AST
{
public:
    SpecifierListAST *attribute_list;
    PtrOperatorListAST *ptr_operator_list;
    CoreDeclaratorAST *core_declarator;
    PostfixDeclaratorListAST *postfix_declarator_list;
    SpecifierListAST *post_attribute_list;
    int equal_token;
    ExpressionAST *initializer;

public:
    DeclaratorAST()
        : attribute_list(0)
        , ptr_operator_list(0)
        , core_declarator(0)
        , postfix_declarator_list(0)
        , post_attribute_list(0)
        , equal_token(0)
        , initializer(0)
    {}

    virtual DeclaratorAST *asDeclarator() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT SimpleDeclarationAST: public DeclarationAST
{
public:
    int qt_invokable_token;
    SpecifierListAST *decl_specifier_list;
    DeclaratorListAST *declarator_list;
    int semicolon_token;

public:
    List<Symbol *> *symbols;

public:
    SimpleDeclarationAST()
        : qt_invokable_token(0)
        , decl_specifier_list(0)
        , declarator_list(0)
        , semicolon_token(0)
        , symbols(0)
    {}

    virtual SimpleDeclarationAST *asSimpleDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual SimpleDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT EmptyDeclarationAST: public DeclarationAST
{
public:
    int semicolon_token;

public:
    EmptyDeclarationAST()
        : semicolon_token(0)
    {}

    virtual EmptyDeclarationAST *asEmptyDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual EmptyDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT AccessDeclarationAST: public DeclarationAST
{
public:
    int access_specifier_token;
    int slots_token;
    int colon_token;

public:
    AccessDeclarationAST()
        : access_specifier_token(0)
        , slots_token(0)
        , colon_token(0)
    {}

    virtual AccessDeclarationAST *asAccessDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual AccessDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtObjectTagAST: public DeclarationAST
{
public:
    int q_object_token;

public:
    QtObjectTagAST()
        : q_object_token(0)
    {}

    virtual QtObjectTagAST *asQtObjectTag() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtObjectTagAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtPrivateSlotAST: public DeclarationAST
{
public:
    int q_private_slot_token;
    int lparen_token;
    int dptr_token;
    int dptr_lparen_token;
    int dptr_rparen_token;
    int comma_token;
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;
    int rparen_token;

public:
    QtPrivateSlotAST()
        : q_private_slot_token(0)
        , lparen_token(0)
        , dptr_token(0)
        , dptr_lparen_token(0)
        , dptr_rparen_token(0)
        , comma_token(0)
        , type_specifier_list(0)
        , declarator(0)
        , rparen_token(0)
    {}

    virtual QtPrivateSlotAST *asQtPrivateSlot() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtPrivateSlotAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class QtPropertyDeclarationItemAST: public AST
{
public:
    int item_name_token;
    ExpressionAST *expression;

public:
    QtPropertyDeclarationItemAST()
        : item_name_token(0)
        , expression(0)
    {}

    virtual QtPropertyDeclarationItemAST *asQtPropertyDeclarationItem() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtPropertyDeclarationItemAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtPropertyDeclarationAST: public DeclarationAST
{
public:
    int property_specifier_token;
    int lparen_token;
    ExpressionAST *expression; // for Q_PRIVATE_PROPERTY(expression, ...)
    int comma_token;
    ExpressionAST *type_id;
    NameAST *property_name;
    QtPropertyDeclarationItemListAST *property_declaration_item_list;
    int rparen_token;

public:
    QtPropertyDeclarationAST()
        : property_specifier_token(0)
        , lparen_token(0)
        , expression(0)
        , comma_token(0)
        , type_id(0)
        , property_name(0)
        , property_declaration_item_list(0)
        , rparen_token(0)
    {}

    virtual QtPropertyDeclarationAST *asQtPropertyDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtPropertyDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtEnumDeclarationAST: public DeclarationAST
{
public:
    int enum_specifier_token;
    int lparen_token;
    NameListAST *enumerator_list;
    int rparen_token;

public:
    QtEnumDeclarationAST()
        : enum_specifier_token(0)
        , lparen_token(0)
        , enumerator_list(0)
        , rparen_token(0)
    {}

    virtual QtEnumDeclarationAST *asQtEnumDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtEnumDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtFlagsDeclarationAST: public DeclarationAST
{
public:
    int flags_specifier_token;
    int lparen_token;
    NameListAST *flag_enums_list;
    int rparen_token;

public:
    QtFlagsDeclarationAST()
        : flags_specifier_token(0)
        , lparen_token(0)
        , flag_enums_list(0)
        , rparen_token(0)
    {}

    virtual QtFlagsDeclarationAST *asQtFlagsDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtFlagsDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtInterfaceNameAST: public AST
{
public:
    NameAST *interface_name;
    NameListAST *constraint_list;

public:
    QtInterfaceNameAST()
        : interface_name(0)
        , constraint_list(0)
    {}

    virtual QtInterfaceNameAST *asQtInterfaceName() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtInterfaceNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtInterfacesDeclarationAST: public DeclarationAST
{
public:
    int interfaces_token;
    int lparen_token;
    QtInterfaceNameListAST *interface_name_list;
    int rparen_token;

public:
    QtInterfacesDeclarationAST()
        : interfaces_token(0)
        , lparen_token(0)
        , interface_name_list(0)
        , rparen_token(0)
    {}

    virtual QtInterfacesDeclarationAST *asQtInterfacesDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtInterfacesDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT AsmDefinitionAST: public DeclarationAST
{
public:
    int asm_token;
    int volatile_token;
    int lparen_token;
    // ### string literals
    // ### asm operand list
    int rparen_token;
    int semicolon_token;

public:
    AsmDefinitionAST()
        : asm_token(0)
        , volatile_token(0)
        , lparen_token(0)
        , rparen_token(0)
        , semicolon_token(0)
    {}

    virtual AsmDefinitionAST *asAsmDefinition() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual AsmDefinitionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT BaseSpecifierAST: public AST
{
public:
    int virtual_token;
    int access_specifier_token;
    NameAST *name;
    int ellipsis_token;

public: // annotations
    BaseClass *symbol;

public:
    BaseSpecifierAST()
        : virtual_token(0)
        , access_specifier_token(0)
        , name(0)
        , ellipsis_token(0)
        , symbol(0)
    {}

    virtual BaseSpecifierAST *asBaseSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual BaseSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT IdExpressionAST: public ExpressionAST
{
public:
    NameAST *name;

public:
    IdExpressionAST()
        : name(0)
    {}

    virtual IdExpressionAST *asIdExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual IdExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CompoundExpressionAST: public ExpressionAST
{
public:
    int lparen_token;
    CompoundStatementAST *statement;
    int rparen_token;

public:
    CompoundExpressionAST()
        : lparen_token(0)
        , statement(0)
        , rparen_token(0)
    {}

    virtual CompoundExpressionAST *asCompoundExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CompoundExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CompoundLiteralAST: public ExpressionAST
{
public:
    int lparen_token;
    ExpressionAST *type_id;
    int rparen_token;
    ExpressionAST *initializer;

public:
    CompoundLiteralAST()
        : lparen_token(0)
        , type_id(0)
        , rparen_token(0)
        , initializer(0)
    {}

    virtual CompoundLiteralAST *asCompoundLiteral() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CompoundLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtMethodAST: public ExpressionAST
{
public:
    int method_token;
    int lparen_token;
    DeclaratorAST *declarator;
    int rparen_token;

public:
    QtMethodAST()
        : method_token(0)
        , lparen_token(0)
        , declarator(0)
        , rparen_token(0)
    {}

    virtual QtMethodAST *asQtMethod() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtMethodAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QtMemberDeclarationAST: public StatementAST
{
public:
    int q_token;
    int lparen_token;
    ExpressionAST *type_id;
    int rparen_token;

public:
    QtMemberDeclarationAST()
        : q_token(0)
        , lparen_token(0)
        , type_id(0)
        , rparen_token(0)
    {}

    virtual QtMemberDeclarationAST *asQtMemberDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QtMemberDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT BinaryExpressionAST: public ExpressionAST
{
public:
    ExpressionAST *left_expression;
    int binary_op_token;
    ExpressionAST *right_expression;

public:
    BinaryExpressionAST()
        : left_expression(0)
        , binary_op_token(0)
        , right_expression(0)
    {}

    virtual BinaryExpressionAST *asBinaryExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual BinaryExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CastExpressionAST: public ExpressionAST
{
public:
    int lparen_token;
    ExpressionAST *type_id;
    int rparen_token;
    ExpressionAST *expression;

public:
    CastExpressionAST()
        : lparen_token(0)
        , type_id(0)
        , rparen_token(0)
        , expression(0)
    {}

    virtual CastExpressionAST *asCastExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CastExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ClassSpecifierAST: public SpecifierAST
{
public:
    int classkey_token;
    SpecifierListAST *attribute_list;
    NameAST *name;
    int final_token;
    int colon_token;
    BaseSpecifierListAST *base_clause_list;
    int dot_dot_dot_token;
    int lbrace_token;
    DeclarationListAST *member_specifier_list;
    int rbrace_token;

public: // annotations
    Class *symbol;

public:
    ClassSpecifierAST()
        : classkey_token(0)
        , attribute_list(0)
        , name(0)
        , final_token(0)
        , colon_token(0)
        , base_clause_list(0)
        , dot_dot_dot_token(0)
        , lbrace_token(0)
        , member_specifier_list(0)
        , rbrace_token(0)
        , symbol(0)
    {}

    virtual ClassSpecifierAST *asClassSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ClassSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CaseStatementAST: public StatementAST
{
public:
    int case_token;
    ExpressionAST *expression;
    int colon_token;
    StatementAST *statement;

public:
    CaseStatementAST()
        : case_token(0)
        , expression(0)
        , colon_token(0)
        , statement(0)
    {}

    virtual CaseStatementAST *asCaseStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CaseStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CompoundStatementAST: public StatementAST
{
public:
    int lbrace_token;
    StatementListAST *statement_list;
    int rbrace_token;

public: // annotations
    Block *symbol;

public:
    CompoundStatementAST()
        : lbrace_token(0)
        , statement_list(0)
        , rbrace_token(0)
        , symbol(0)
    {}

    virtual CompoundStatementAST *asCompoundStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CompoundStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ConditionAST: public ExpressionAST
{
public:
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;

public:
    ConditionAST()
        : type_specifier_list(0)
        , declarator(0)
    {}

    virtual ConditionAST *asCondition() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ConditionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ConditionalExpressionAST: public ExpressionAST
{
public:
    ExpressionAST *condition;
    int question_token;
    ExpressionAST *left_expression;
    int colon_token;
    ExpressionAST *right_expression;

public:
    ConditionalExpressionAST()
        : condition(0)
        , question_token(0)
        , left_expression(0)
        , colon_token(0)
        , right_expression(0)
    {}

    virtual ConditionalExpressionAST *asConditionalExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ConditionalExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CppCastExpressionAST: public ExpressionAST
{
public:
    int cast_token;
    int less_token;
    ExpressionAST *type_id;
    int greater_token;
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;

public:
    CppCastExpressionAST()
        : cast_token(0)
        , less_token(0)
        , type_id(0)
        , greater_token(0)
        , lparen_token(0)
        , expression(0)
        , rparen_token(0)
    {}

    virtual CppCastExpressionAST *asCppCastExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CppCastExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CtorInitializerAST: public AST
{
public:
    int colon_token;
    MemInitializerListAST *member_initializer_list;
    int dot_dot_dot_token;

public:
    CtorInitializerAST()
        : colon_token(0)
        , member_initializer_list(0)
        , dot_dot_dot_token(0)
    {}

    virtual CtorInitializerAST *asCtorInitializer() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CtorInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT DeclarationStatementAST: public StatementAST
{
public:
    DeclarationAST *declaration;

public:
    DeclarationStatementAST()
        : declaration(0)
    {}

    virtual DeclarationStatementAST *asDeclarationStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DeclarationStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT DeclaratorIdAST: public CoreDeclaratorAST
{
public:
    int dot_dot_dot_token;
    NameAST *name;

public:
    DeclaratorIdAST()
        : dot_dot_dot_token(0)
        , name(0)
    {}

    virtual DeclaratorIdAST *asDeclaratorId() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DeclaratorIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NestedDeclaratorAST: public CoreDeclaratorAST
{
public:
    int lparen_token;
    DeclaratorAST *declarator;
    int rparen_token;

public:
    NestedDeclaratorAST()
        : lparen_token(0)
        , declarator(0)
        , rparen_token(0)
    {}

    virtual NestedDeclaratorAST *asNestedDeclarator() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NestedDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT FunctionDeclaratorAST: public PostfixDeclaratorAST
{
public:
    int lparen_token;
    ParameterDeclarationClauseAST *parameter_declaration_clause;
    int rparen_token;
    SpecifierListAST *cv_qualifier_list;
    int ref_qualifier_token;
    ExceptionSpecificationAST *exception_specification;
    TrailingReturnTypeAST *trailing_return_type;
    // Some FunctionDeclarators can also be interpreted as an initializer, like for 'A b(c);'
    ExpressionAST *as_cpp_initializer;

public: // annotations
    Function *symbol;

public:
    FunctionDeclaratorAST()
        : lparen_token(0)
        , parameter_declaration_clause(0)
        , rparen_token(0)
        , cv_qualifier_list(0)
        , ref_qualifier_token(0)
        , exception_specification(0)
        , trailing_return_type(0)
        , as_cpp_initializer(0)
        , symbol(0)
    {}

    virtual FunctionDeclaratorAST *asFunctionDeclarator() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual FunctionDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ArrayDeclaratorAST: public PostfixDeclaratorAST
{
public:
    int lbracket_token;
    ExpressionAST *expression;
    int rbracket_token;

public:
    ArrayDeclaratorAST()
        : lbracket_token(0)
        , expression(0)
        , rbracket_token(0)
    {}

    virtual ArrayDeclaratorAST *asArrayDeclarator() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ArrayDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT DeleteExpressionAST: public ExpressionAST
{
public:
    int scope_token;
    int delete_token;
    int lbracket_token;
    int rbracket_token;
    ExpressionAST *expression;

public:
    DeleteExpressionAST()
        : scope_token(0)
        , delete_token(0)
        , lbracket_token(0)
        , rbracket_token(0)
        , expression(0)
    {}

    virtual DeleteExpressionAST *asDeleteExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DeleteExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT DoStatementAST: public StatementAST
{
public:
    int do_token;
    StatementAST *statement;
    int while_token;
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;
    int semicolon_token;

public:
    DoStatementAST()
        : do_token(0)
        , statement(0)
        , while_token(0)
        , lparen_token(0)
        , expression(0)
        , rparen_token(0)
        , semicolon_token(0)
    {}

    virtual DoStatementAST *asDoStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DoStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NamedTypeSpecifierAST: public SpecifierAST
{
public:
    NameAST *name;

public:
    NamedTypeSpecifierAST()
        : name(0)
    {}

    virtual NamedTypeSpecifierAST *asNamedTypeSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NamedTypeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ElaboratedTypeSpecifierAST: public SpecifierAST
{
public:
    int classkey_token;
    SpecifierListAST *attribute_list;
    NameAST *name;

public:
    ElaboratedTypeSpecifierAST()
        : classkey_token(0)
        , attribute_list(0)
        , name(0)
    {}

    virtual ElaboratedTypeSpecifierAST *asElaboratedTypeSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ElaboratedTypeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT EnumSpecifierAST: public SpecifierAST
{
public:
    int enum_token;
    int key_token; // struct, class or 0
    NameAST *name;
    int colon_token; // can be 0 if there is no enum-base
    SpecifierListAST *type_specifier_list; // ditto
    int lbrace_token;
    EnumeratorListAST *enumerator_list;
    int stray_comma_token;
    int rbrace_token;

public: // annotations
    Enum *symbol;

public:
    EnumSpecifierAST()
        : enum_token(0)
        , key_token(0)
        , name(0)
        , colon_token(0)
        , type_specifier_list(0)
        , lbrace_token(0)
        , enumerator_list(0)
        , stray_comma_token(0)
        , rbrace_token(0)
        , symbol(0)
    {}

    virtual EnumSpecifierAST *asEnumSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual EnumSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT EnumeratorAST: public AST
{
public:
    int identifier_token;
    int equal_token;
    ExpressionAST *expression;

public:
    EnumeratorAST()
        : identifier_token(0)
        , equal_token(0)
        , expression(0)
    {}

    virtual EnumeratorAST *asEnumerator() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual EnumeratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ExceptionDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;
    int dot_dot_dot_token;

public:
    ExceptionDeclarationAST()
        : type_specifier_list(0)
        , declarator(0)
        , dot_dot_dot_token(0)
    {}

    virtual ExceptionDeclarationAST *asExceptionDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ExceptionDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ExceptionSpecificationAST: public AST
{
public:
    ExceptionSpecificationAST()
    {}

    virtual ExceptionSpecificationAST *asExceptionSpecification() { return this; }

    virtual ExceptionSpecificationAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT DynamicExceptionSpecificationAST: public ExceptionSpecificationAST
{
public:
    int throw_token;
    int lparen_token;
    int dot_dot_dot_token;
    ExpressionListAST *type_id_list;
    int rparen_token;

public:
    DynamicExceptionSpecificationAST()
        : throw_token(0)
        , lparen_token(0)
        , dot_dot_dot_token(0)
        , type_id_list(0)
        , rparen_token(0)
    {}

    virtual DynamicExceptionSpecificationAST *asDynamicExceptionSpecification() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DynamicExceptionSpecificationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NoExceptSpecificationAST: public ExceptionSpecificationAST
{
public:
    int noexcept_token;
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;

public:
    NoExceptSpecificationAST()
        : noexcept_token(0)
        , lparen_token(0)
        , expression(0)
        , rparen_token(0)
    {}

    virtual NoExceptSpecificationAST *asNoExceptSpecification() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NoExceptSpecificationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ExpressionOrDeclarationStatementAST: public StatementAST
{
public:
    ExpressionStatementAST *expression;
    DeclarationStatementAST *declaration;

public:
    ExpressionOrDeclarationStatementAST()
        : expression(0)
        , declaration(0)
    {}

    virtual ExpressionOrDeclarationStatementAST *asExpressionOrDeclarationStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ExpressionOrDeclarationStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ExpressionStatementAST: public StatementAST
{
public:
    ExpressionAST *expression;
    int semicolon_token;

public:
    ExpressionStatementAST()
        : expression(0)
        , semicolon_token(0)
    {}

    virtual ExpressionStatementAST *asExpressionStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ExpressionStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT FunctionDefinitionAST: public DeclarationAST
{
public:
    int qt_invokable_token;
    SpecifierListAST *decl_specifier_list;
    DeclaratorAST *declarator;
    CtorInitializerAST *ctor_initializer;
    StatementAST *function_body;

public: // annotations
    Function *symbol;

public:
    FunctionDefinitionAST()
        : qt_invokable_token(0)
        , decl_specifier_list(0)
        , declarator(0)
        , ctor_initializer(0)
        , function_body(0)
        , symbol(0)
    {}

    virtual FunctionDefinitionAST *asFunctionDefinition() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual FunctionDefinitionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ForeachStatementAST: public StatementAST
{
public:
    int foreach_token;
    int lparen_token;
    // declaration
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;
    // or an expression
    ExpressionAST *initializer;
    int comma_token;
    ExpressionAST *expression;
    int rparen_token;
    StatementAST *statement;

public: // annotations
    Block *symbol;

public:
    ForeachStatementAST()
        : foreach_token(0)
        , lparen_token(0)
        , type_specifier_list(0)
        , declarator(0)
        , initializer(0)
        , comma_token(0)
        , expression(0)
        , rparen_token(0)
        , statement(0)
        , symbol(0)
    {}

    virtual ForeachStatementAST *asForeachStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ForeachStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT RangeBasedForStatementAST : public StatementAST
{
public:
    int for_token;
    int lparen_token;
    // declaration
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;
    // or an expression
    int colon_token;
    ExpressionAST *expression;
    int rparen_token;
    StatementAST *statement;

public: // annotations
    Block *symbol;

public:
    RangeBasedForStatementAST()
        : for_token(0)
        , lparen_token(0)
        , type_specifier_list(0)
        , declarator(0)
        , colon_token(0)
        , expression(0)
        , rparen_token(0)
        , statement(0)
        , symbol(0)
    {}

    virtual RangeBasedForStatementAST *asRangeBasedForStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual RangeBasedForStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ForStatementAST: public StatementAST
{
public:
    int for_token;
    int lparen_token;
    StatementAST *initializer;
    ExpressionAST *condition;
    int semicolon_token;
    ExpressionAST *expression;
    int rparen_token;
    StatementAST *statement;

public: // annotations
    Block *symbol;

public:
    ForStatementAST()
        : for_token(0)
        , lparen_token(0)
        , initializer(0)
        , condition(0)
        , semicolon_token(0)
        , expression(0)
        , rparen_token(0)
        , statement(0)
        , symbol(0)
    {}

    virtual ForStatementAST *asForStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ForStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT IfStatementAST: public StatementAST
{
public:
    int if_token;
    int lparen_token;
    ExpressionAST *condition;
    int rparen_token;
    StatementAST *statement;
    int else_token;
    StatementAST *else_statement;

public: // annotations
    Block *symbol;

public:
    IfStatementAST()
        : if_token(0)
        , lparen_token(0)
        , condition(0)
        , rparen_token(0)
        , statement(0)
        , else_token(0)
        , else_statement(0)
        , symbol(0)
    {}

    virtual IfStatementAST *asIfStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual IfStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ArrayInitializerAST: public ExpressionAST
{
public:
    int lbrace_token;
    ExpressionListAST *expression_list;
    int rbrace_token;

public:
    ArrayInitializerAST()
        : lbrace_token(0)
        , expression_list(0)
        , rbrace_token(0)
    {}

    virtual ArrayInitializerAST *asArrayInitializer() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ArrayInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT LabeledStatementAST: public StatementAST
{
public:
    int label_token;
    int colon_token;
    StatementAST *statement;

public:
    LabeledStatementAST()
        : label_token(0)
        , colon_token(0)
        , statement(0)
    {}

    virtual LabeledStatementAST *asLabeledStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual LabeledStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT LinkageBodyAST: public DeclarationAST
{
public:
    int lbrace_token;
    DeclarationListAST *declaration_list;
    int rbrace_token;

public:
    LinkageBodyAST()
        : lbrace_token(0)
        , declaration_list(0)
        , rbrace_token(0)
    {}

    virtual LinkageBodyAST *asLinkageBody() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual LinkageBodyAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT LinkageSpecificationAST: public DeclarationAST
{
public:
    int extern_token;
    int extern_type_token;
    DeclarationAST *declaration;

public:
    LinkageSpecificationAST()
        : extern_token(0)
        , extern_type_token(0)
        , declaration(0)
    {}

    virtual LinkageSpecificationAST *asLinkageSpecification() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual LinkageSpecificationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT MemInitializerAST: public AST
{
public:
    NameAST *name;
    // either a BracedInitializerAST or a ExpressionListParenAST
    ExpressionAST *expression;

public:
    MemInitializerAST()
        : name(0)
        , expression(0)
    {}

    virtual MemInitializerAST *asMemInitializer() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual MemInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NestedNameSpecifierAST: public AST
{
public:
    NameAST *class_or_namespace_name;
    int scope_token;

public:
    NestedNameSpecifierAST()
        : class_or_namespace_name(0)
        , scope_token(0)
    {}

    virtual NestedNameSpecifierAST *asNestedNameSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NestedNameSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT QualifiedNameAST: public NameAST
{
public:
    int global_scope_token;
    NestedNameSpecifierListAST *nested_name_specifier_list;
    NameAST *unqualified_name;

public:
    QualifiedNameAST()
        : global_scope_token(0)
        , nested_name_specifier_list(0)
        , unqualified_name(0)
    {}

    virtual QualifiedNameAST *asQualifiedName() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual QualifiedNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT OperatorFunctionIdAST: public NameAST
{
public:
    int operator_token;
    OperatorAST *op;

public:
    OperatorFunctionIdAST()
        : operator_token(0)
        , op(0)
    {}

    virtual OperatorFunctionIdAST *asOperatorFunctionId() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual OperatorFunctionIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ConversionFunctionIdAST: public NameAST
{
public:
    int operator_token;
    SpecifierListAST *type_specifier_list;
    PtrOperatorListAST *ptr_operator_list;

public:
    ConversionFunctionIdAST()
        : operator_token(0)
        , type_specifier_list(0)
        , ptr_operator_list(0)
    {}

    virtual ConversionFunctionIdAST *asConversionFunctionId() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ConversionFunctionIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT AnonymousNameAST: public NameAST
{
public:
    int class_token;
public:
    AnonymousNameAST()
        : class_token(0)
    {}

    virtual AnonymousNameAST *asAnonymousName() { return this; }
    virtual int firstToken() const { return 0; }
    virtual int lastToken() const { return 0; }

    virtual AnonymousNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT SimpleNameAST: public NameAST
{
public:
    int identifier_token;

public:
    SimpleNameAST()
        : identifier_token(0)
    {}

    virtual SimpleNameAST *asSimpleName() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual SimpleNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT DestructorNameAST: public NameAST
{
public:
    int tilde_token;
    NameAST *unqualified_name;

public:
    DestructorNameAST()
        : tilde_token(0)
        , unqualified_name(0)
    {}

    virtual DestructorNameAST *asDestructorName() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DestructorNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TemplateIdAST: public NameAST
{
public:
    int template_token;
    int identifier_token;
    int less_token;
    ExpressionListAST *template_argument_list;
    int greater_token;

public:
    TemplateIdAST()
        : template_token(0)
        , identifier_token(0)
        , less_token(0)
        , template_argument_list(0)
        , greater_token(0)
    {}

    virtual TemplateIdAST *asTemplateId() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TemplateIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NamespaceAST: public DeclarationAST
{
public:
    int inline_token;
    int namespace_token;
    int identifier_token;
    SpecifierListAST *attribute_list;
    DeclarationAST *linkage_body;

public: // annotations
    Namespace *symbol;

public:
    NamespaceAST()
        : inline_token(0)
        , namespace_token(0)
        , identifier_token(0)
        , attribute_list(0)
        , linkage_body(0)
        , symbol(0)
    {}

    virtual NamespaceAST *asNamespace() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NamespaceAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NamespaceAliasDefinitionAST: public DeclarationAST
{
public:
    int namespace_token;
    int namespace_name_token;
    int equal_token;
    NameAST *name;
    int semicolon_token;

public:
    NamespaceAliasDefinitionAST()
        : namespace_token(0)
        , namespace_name_token(0)
        , equal_token(0)
        , name(0)
        , semicolon_token(0)
    {}

    virtual NamespaceAliasDefinitionAST *asNamespaceAliasDefinition() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NamespaceAliasDefinitionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT AliasDeclarationAST: public DeclarationAST
{
public:
    int using_token;
    NameAST *name;
    int equal_token;
    TypeIdAST *typeId;
    int semicolon_token;

public: // annotations
    Declaration *symbol;

public:
    AliasDeclarationAST()
        : using_token(0)
        , name(0)
        , equal_token(0)
        , typeId(0)
        , semicolon_token(0)
        , symbol(0)
    {}

    virtual AliasDeclarationAST *asAliasDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual AliasDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ExpressionListParenAST: public ExpressionAST
{
public:
    int lparen_token;
    ExpressionListAST *expression_list;
    int rparen_token;

public:
    ExpressionListParenAST()
        : lparen_token(0)
        , expression_list(0)
        , rparen_token(0)
    {}

    virtual ExpressionListParenAST *asExpressionListParen() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ExpressionListParenAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NewArrayDeclaratorAST: public AST
{
public:
    int lbracket_token;
    ExpressionAST *expression;
    int rbracket_token;

public:
    NewArrayDeclaratorAST()
        : lbracket_token(0)
        , expression(0)
        , rbracket_token(0)
    {}

    virtual NewArrayDeclaratorAST *asNewArrayDeclarator() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NewArrayDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NewExpressionAST: public ExpressionAST
{
public:
    int scope_token;
    int new_token;
    ExpressionListParenAST *new_placement;

    int lparen_token;
    ExpressionAST *type_id;
    int rparen_token;

    NewTypeIdAST *new_type_id;

    ExpressionAST *new_initializer; // either ExpressionListParenAST or BracedInitializerAST

public:
    NewExpressionAST()
        : scope_token(0)
        , new_token(0)
        , new_placement(0)
        , lparen_token(0)
        , type_id(0)
        , rparen_token(0)
        , new_type_id(0)
        , new_initializer(0)
    {}

    virtual NewExpressionAST *asNewExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NewExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NewTypeIdAST: public AST
{
public:
    SpecifierListAST *type_specifier_list;
    PtrOperatorListAST *ptr_operator_list;
    NewArrayDeclaratorListAST *new_array_declarator_list;

public:
    NewTypeIdAST()
        : type_specifier_list(0)
        , ptr_operator_list(0)
        , new_array_declarator_list(0)
    {}

    virtual NewTypeIdAST *asNewTypeId() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NewTypeIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT OperatorAST: public AST
{
public:
    int op_token;
    int open_token;
    int close_token;

public:
    OperatorAST()
        : op_token(0)
        , open_token(0)
        , close_token(0)
    {}

    virtual OperatorAST *asOperator() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual OperatorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ParameterDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;
    int equal_token;
    ExpressionAST *expression;

public: // annotations
    Argument *symbol;

public:
    ParameterDeclarationAST()
        : type_specifier_list(0)
        , declarator(0)
        , equal_token(0)
        , expression(0)
        , symbol(0)
    {}

    virtual ParameterDeclarationAST *asParameterDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ParameterDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ParameterDeclarationClauseAST: public AST
{
public:
    ParameterDeclarationListAST *parameter_declaration_list;
    int dot_dot_dot_token;

public:
    ParameterDeclarationClauseAST()
        : parameter_declaration_list(0)
        , dot_dot_dot_token(0)
    {}

    virtual ParameterDeclarationClauseAST *asParameterDeclarationClause() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ParameterDeclarationClauseAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CallAST: public PostfixAST
{
public:
    ExpressionAST *base_expression;
    int lparen_token;
    ExpressionListAST *expression_list;
    int rparen_token;

public:
    CallAST()
        : base_expression(0)
        , lparen_token(0)
        , expression_list(0)
        , rparen_token(0)
    {}

    virtual CallAST *asCall() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CallAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ArrayAccessAST: public PostfixAST
{
public:
    ExpressionAST *base_expression;
    int lbracket_token;
    ExpressionAST *expression;
    int rbracket_token;

public:
    ArrayAccessAST()
        : base_expression(0)
        , lbracket_token(0)
        , expression(0)
        , rbracket_token(0)
    {}

    virtual ArrayAccessAST *asArrayAccess() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ArrayAccessAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT PostIncrDecrAST: public PostfixAST
{
public:
    ExpressionAST *base_expression;
    int incr_decr_token;

public:
    PostIncrDecrAST()
        : base_expression(0)
        , incr_decr_token(0)
    {}

    virtual PostIncrDecrAST *asPostIncrDecr() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual PostIncrDecrAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT MemberAccessAST: public PostfixAST
{
public:
    ExpressionAST *base_expression;
    int access_token;
    int template_token;
    NameAST *member_name;

public:
    MemberAccessAST()
        : base_expression(0)
        , access_token(0)
        , template_token(0)
        , member_name(0)
    {}

    virtual MemberAccessAST *asMemberAccess() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual MemberAccessAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TypeidExpressionAST: public ExpressionAST
{
public:
    int typeid_token;
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;

public:
    TypeidExpressionAST()
        : typeid_token(0)
        , lparen_token(0)
        , expression(0)
        , rparen_token(0)
    {}

    virtual TypeidExpressionAST *asTypeidExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TypeidExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TypenameCallExpressionAST: public ExpressionAST
{
public:
    int typename_token;
    NameAST *name;
    ExpressionAST *expression; // either ExpressionListParenAST or BracedInitializerAST

public:
    TypenameCallExpressionAST()
        : typename_token(0)
        , name(0)
        , expression(0)
    {}

    virtual TypenameCallExpressionAST *asTypenameCallExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TypenameCallExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TypeConstructorCallAST: public ExpressionAST
{
public:
    SpecifierListAST *type_specifier_list;
    ExpressionAST *expression; // either ExpressionListParenAST or BracedInitializerAST

public:
    TypeConstructorCallAST()
        : type_specifier_list(0)
        , expression(0)
    {}

    virtual TypeConstructorCallAST *asTypeConstructorCall() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TypeConstructorCallAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT PointerToMemberAST: public PtrOperatorAST
{
public:
    int global_scope_token;
    NestedNameSpecifierListAST *nested_name_specifier_list;
    int star_token;
    SpecifierListAST *cv_qualifier_list;
    int ref_qualifier_token;

public:
    PointerToMemberAST()
        : global_scope_token(0)
        , nested_name_specifier_list(0)
        , star_token(0)
        , cv_qualifier_list(0)
        , ref_qualifier_token(0)
    {}

    virtual PointerToMemberAST *asPointerToMember() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual PointerToMemberAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT PointerAST: public PtrOperatorAST
{
public:
    int star_token;
    SpecifierListAST *cv_qualifier_list;

public:
    PointerAST()
        : star_token(0)
        , cv_qualifier_list(0)
    {}

    virtual PointerAST *asPointer() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual PointerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ReferenceAST: public PtrOperatorAST
{
public:
    int reference_token;

public:
    ReferenceAST()
        : reference_token(0)
    {}

    virtual ReferenceAST *asReference() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ReferenceAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT BreakStatementAST: public StatementAST
{
public:
    int break_token;
    int semicolon_token;

public:
    BreakStatementAST()
        : break_token(0)
        , semicolon_token(0)
    {}

    virtual BreakStatementAST *asBreakStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual BreakStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ContinueStatementAST: public StatementAST
{
public:
    int continue_token;
    int semicolon_token;

public:
    ContinueStatementAST()
        : continue_token(0)
        , semicolon_token(0)
    {}

    virtual ContinueStatementAST *asContinueStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ContinueStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT GotoStatementAST: public StatementAST
{
public:
    int goto_token;
    int identifier_token;
    int semicolon_token;

public:
    GotoStatementAST()
        : goto_token(0)
        , identifier_token(0)
        , semicolon_token(0)
    {}

    virtual GotoStatementAST *asGotoStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual GotoStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ReturnStatementAST: public StatementAST
{
public:
    int return_token;
    ExpressionAST *expression;
    int semicolon_token;

public:
    ReturnStatementAST()
        : return_token(0)
        , expression(0)
        , semicolon_token(0)
    {}

    virtual ReturnStatementAST *asReturnStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ReturnStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT SizeofExpressionAST: public ExpressionAST
{
public:
    int sizeof_token;
    int dot_dot_dot_token;
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;

public:
    SizeofExpressionAST()
        : sizeof_token(0)
        , dot_dot_dot_token(0)
        , lparen_token(0)
        , expression(0)
        , rparen_token(0)
    {}

    virtual SizeofExpressionAST *asSizeofExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual SizeofExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT AlignofExpressionAST: public ExpressionAST
{
public:
    int alignof_token;
    int lparen_token;
    TypeIdAST *typeId;
    int rparen_token;

public:
    AlignofExpressionAST()
        : alignof_token(0)
        , lparen_token(0)
        , typeId(0)
        , rparen_token(0)
    {}

    virtual AlignofExpressionAST *asAlignofExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual AlignofExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT PointerLiteralAST: public ExpressionAST
{
public:
    int literal_token;

public:
    PointerLiteralAST()
        : literal_token(0)
    {}

    virtual PointerLiteralAST *asPointerLiteral() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual PointerLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NumericLiteralAST: public ExpressionAST
{
public:
    int literal_token;

public:
    NumericLiteralAST()
        : literal_token(0)
    {}

    virtual NumericLiteralAST *asNumericLiteral() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NumericLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT BoolLiteralAST: public ExpressionAST
{
public:
    int literal_token;

public:
    BoolLiteralAST()
        : literal_token(0)
    {}

    virtual BoolLiteralAST *asBoolLiteral() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual BoolLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ThisExpressionAST: public ExpressionAST
{
public:
    int this_token;

public:
    ThisExpressionAST()
        : this_token(0)
    {}

    virtual ThisExpressionAST *asThisExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ThisExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NestedExpressionAST: public ExpressionAST
{
public:
    int lparen_token;
    ExpressionAST *expression;
    int rparen_token;

public:
    NestedExpressionAST()
        : lparen_token(0)
        , expression(0)
        , rparen_token(0)
    {}

    virtual NestedExpressionAST *asNestedExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NestedExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT StaticAssertDeclarationAST: public DeclarationAST
{
public:
    int static_assert_token;
    int lparen_token;
    ExpressionAST *expression;
    int comma_token;
    ExpressionAST *string_literal;
    int rparen_token;
    int semicolon_token;

public:
    StaticAssertDeclarationAST()
        : static_assert_token(0)
        , lparen_token(0)
        , expression(0)
        , comma_token(0)
        , string_literal(0)
        , rparen_token(0)
        , semicolon_token(0)
    {}

    virtual StaticAssertDeclarationAST *asStaticAssertDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual StaticAssertDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT StringLiteralAST: public ExpressionAST
{
public:
    int literal_token;
    StringLiteralAST *next;

public:
    StringLiteralAST()
        : literal_token(0)
        , next(0)
    {}

    virtual StringLiteralAST *asStringLiteral() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual StringLiteralAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT SwitchStatementAST: public StatementAST
{
public:
    int switch_token;
    int lparen_token;
    ExpressionAST *condition;
    int rparen_token;
    StatementAST *statement;

public: // annotations
    Block *symbol;

public:
    SwitchStatementAST()
        : switch_token(0)
        , lparen_token(0)
        , condition(0)
        , rparen_token(0)
        , statement(0)
        , symbol(0)
    {}

    virtual SwitchStatementAST *asSwitchStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual SwitchStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TemplateDeclarationAST: public DeclarationAST
{
public:
    int export_token;
    int template_token;
    int less_token;
    DeclarationListAST *template_parameter_list;
    int greater_token;
    DeclarationAST *declaration;

public: // annotations
    Template *symbol;

public:
    TemplateDeclarationAST()
        : export_token(0)
        , template_token(0)
        , less_token(0)
        , template_parameter_list(0)
        , greater_token(0)
        , declaration(0)
        , symbol(0)
    {}

    virtual TemplateDeclarationAST *asTemplateDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TemplateDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ThrowExpressionAST: public ExpressionAST
{
public:
    int throw_token;
    ExpressionAST *expression;

public:
    ThrowExpressionAST()
        : throw_token(0)
        , expression(0)
    {}

    virtual ThrowExpressionAST *asThrowExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ThrowExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT NoExceptOperatorExpressionAST: public ExpressionAST
{
public:
    int noexcept_token;
    ExpressionAST *expression;

public:
    NoExceptOperatorExpressionAST()
        : noexcept_token(0)
        , expression(0)
    {}

    virtual NoExceptOperatorExpressionAST *asNoExceptOperatorExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual NoExceptOperatorExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TranslationUnitAST: public AST
{
public:
    DeclarationListAST *declaration_list;

public:
    TranslationUnitAST()
        : declaration_list(0)
    {}

    virtual TranslationUnitAST *asTranslationUnit() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TranslationUnitAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TryBlockStatementAST: public StatementAST
{
public:
    int try_token;
    StatementAST *statement;
    CatchClauseListAST *catch_clause_list;

public:
    TryBlockStatementAST()
        : try_token(0)
        , statement(0)
        , catch_clause_list(0)
    {}

    virtual TryBlockStatementAST *asTryBlockStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TryBlockStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT CatchClauseAST: public StatementAST
{
public:
    int catch_token;
    int lparen_token;
    ExceptionDeclarationAST *exception_declaration;
    int rparen_token;
    StatementAST *statement;

public: // annotations
    Block *symbol;

public:
    CatchClauseAST()
        : catch_token(0)
        , lparen_token(0)
        , exception_declaration(0)
        , rparen_token(0)
        , statement(0)
        , symbol(0)
    {}

    virtual CatchClauseAST *asCatchClause() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CatchClauseAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TypeIdAST: public ExpressionAST
{
public:
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;

public:
    TypeIdAST()
        : type_specifier_list(0)
        , declarator(0)
    {}

    virtual TypeIdAST *asTypeId() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TypeIdAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TypenameTypeParameterAST: public DeclarationAST
{
public:
    int classkey_token;
    int dot_dot_dot_token;
    NameAST *name;
    int equal_token;
    ExpressionAST *type_id;

public: // annotations
    TypenameArgument *symbol;

public:
    TypenameTypeParameterAST()
        : classkey_token(0)
        , dot_dot_dot_token(0)
        , name(0)
        , equal_token(0)
        , type_id(0)
        , symbol(0)
    {}

    virtual TypenameTypeParameterAST *asTypenameTypeParameter() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TypenameTypeParameterAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT TemplateTypeParameterAST: public DeclarationAST
{
public:
    int template_token;
    int less_token;
    DeclarationListAST *template_parameter_list;
    int greater_token;
    int class_token;
    int dot_dot_dot_token;
    NameAST *name;
    int equal_token;
    ExpressionAST *type_id;

public:
    TypenameArgument *symbol;

public:
    TemplateTypeParameterAST()
        : template_token(0)
        , less_token(0)
        , template_parameter_list(0)
        , greater_token(0)
        , class_token(0)
        , dot_dot_dot_token(0)
        , name(0)
        , equal_token(0)
        , type_id(0)
        , symbol(0)
    {}

    virtual TemplateTypeParameterAST *asTemplateTypeParameter() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TemplateTypeParameterAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT UnaryExpressionAST: public ExpressionAST
{
public:
    int unary_op_token;
    ExpressionAST *expression;

public:
    UnaryExpressionAST()
        : unary_op_token(0)
        , expression(0)
    {}

    virtual UnaryExpressionAST *asUnaryExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual UnaryExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT UsingAST: public DeclarationAST
{
public:
    int using_token;
    int typename_token;
    NameAST *name;
    int semicolon_token;

public: // annotations
    UsingDeclaration *symbol;

public:
    UsingAST()
        : using_token(0)
        , typename_token(0)
        , name(0)
        , semicolon_token(0)
        , symbol(0)
    {}

    virtual UsingAST *asUsing() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual UsingAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT UsingDirectiveAST: public DeclarationAST
{
public:
    int using_token;
    int namespace_token;
    NameAST *name;
    int semicolon_token;

public:
    UsingNamespaceDirective *symbol;

public:
    UsingDirectiveAST()
        : using_token(0)
        , namespace_token(0)
        , name(0)
        , semicolon_token(0)
        , symbol(0)
    {}

    virtual UsingDirectiveAST *asUsingDirective() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual UsingDirectiveAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT WhileStatementAST: public StatementAST
{
public:
    int while_token;
    int lparen_token;
    ExpressionAST *condition;
    int rparen_token;
    StatementAST *statement;

public: // annotations
    Block *symbol;

public:
    WhileStatementAST()
        : while_token(0)
        , lparen_token(0)
        , condition(0)
        , rparen_token(0)
        , statement(0)
        , symbol(0)
    {}

    virtual WhileStatementAST *asWhileStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual WhileStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCClassForwardDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list;
    int class_token;
    NameListAST *identifier_list;
    int semicolon_token;

public: // annotations
    List<ObjCForwardClassDeclaration *> *symbols;

public:
    ObjCClassForwardDeclarationAST()
        : attribute_list(0)
        , class_token(0)
        , identifier_list(0)
        , semicolon_token(0)
        , symbols(0)
    {}

    virtual ObjCClassForwardDeclarationAST *asObjCClassForwardDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCClassForwardDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCClassDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list;
    int interface_token;
    int implementation_token;
    NameAST *class_name;
    int lparen_token;
    NameAST *category_name;
    int rparen_token;
    int colon_token;
    NameAST *superclass;
    ObjCProtocolRefsAST *protocol_refs;
    ObjCInstanceVariablesDeclarationAST *inst_vars_decl;
    DeclarationListAST *member_declaration_list;
    int end_token;

public: // annotations
    ObjCClass *symbol;

public:
    ObjCClassDeclarationAST()
        : attribute_list(0)
        , interface_token(0)
        , implementation_token(0)
        , class_name(0)
        , lparen_token(0)
        , category_name(0)
        , rparen_token(0)
        , colon_token(0)
        , superclass(0)
        , protocol_refs(0)
        , inst_vars_decl(0)
        , member_declaration_list(0)
        , end_token(0)
        , symbol(0)
    {}

    virtual ObjCClassDeclarationAST *asObjCClassDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCClassDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCProtocolForwardDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list;
    int protocol_token;
    NameListAST *identifier_list;
    int semicolon_token;

public: // annotations
    List<ObjCForwardProtocolDeclaration *> *symbols;

public:
    ObjCProtocolForwardDeclarationAST()
        : attribute_list(0)
        , protocol_token(0)
        , identifier_list(0)
        , semicolon_token(0)
        , symbols(0)
    {}

    virtual ObjCProtocolForwardDeclarationAST *asObjCProtocolForwardDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCProtocolForwardDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCProtocolDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list;
    int protocol_token;
    NameAST *name;
    ObjCProtocolRefsAST *protocol_refs;
    DeclarationListAST *member_declaration_list;
    int end_token;

public: // annotations
    ObjCProtocol *symbol;

public:
    ObjCProtocolDeclarationAST()
        : attribute_list(0)
        , protocol_token(0)
        , name(0)
        , protocol_refs(0)
        , member_declaration_list(0)
        , end_token(0)
        , symbol(0)
    {}

    virtual ObjCProtocolDeclarationAST *asObjCProtocolDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCProtocolDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCProtocolRefsAST: public AST
{
public:
    int less_token;
    NameListAST *identifier_list;
    int greater_token;

public:
    ObjCProtocolRefsAST()
        : less_token(0)
        , identifier_list(0)
        , greater_token(0)
    {}

    virtual ObjCProtocolRefsAST *asObjCProtocolRefs() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCProtocolRefsAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCMessageArgumentAST: public AST
{
public:
    ExpressionAST *parameter_value_expression;

public:
    ObjCMessageArgumentAST()
        : parameter_value_expression(0)
    {}

    virtual ObjCMessageArgumentAST *asObjCMessageArgument() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCMessageArgumentAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCMessageExpressionAST: public ExpressionAST
{
public:
    int lbracket_token;
    ExpressionAST *receiver_expression;
    ObjCSelectorAST *selector;
    ObjCMessageArgumentListAST *argument_list;
    int rbracket_token;

public:
    ObjCMessageExpressionAST()
        : lbracket_token(0)
        , receiver_expression(0)
        , selector(0)
        , argument_list(0)
        , rbracket_token(0)
    {}

    virtual ObjCMessageExpressionAST *asObjCMessageExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCMessageExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCProtocolExpressionAST: public ExpressionAST
{
public:
    int protocol_token;
    int lparen_token;
    int identifier_token;
    int rparen_token;

public:
    ObjCProtocolExpressionAST()
        : protocol_token(0)
        , lparen_token(0)
        , identifier_token(0)
        , rparen_token(0)
    {}

    virtual ObjCProtocolExpressionAST *asObjCProtocolExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCProtocolExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCTypeNameAST: public AST
{
public:
    int lparen_token;
    int type_qualifier_token;
    ExpressionAST *type_id;
    int rparen_token;

public:
    ObjCTypeNameAST()
        : lparen_token(0)
        , type_qualifier_token(0)
        , type_id(0)
        , rparen_token(0)
    {}

    virtual ObjCTypeNameAST *asObjCTypeName() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCTypeNameAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCEncodeExpressionAST: public ExpressionAST
{
public:
    int encode_token;
    ObjCTypeNameAST *type_name;

public:
    ObjCEncodeExpressionAST()
        : encode_token(0)
        , type_name(0)
    {}

    virtual ObjCEncodeExpressionAST *asObjCEncodeExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCEncodeExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCSelectorExpressionAST: public ExpressionAST
{
public:
    int selector_token;
    int lparen_token;
    ObjCSelectorAST *selector;
    int rparen_token;

public:
    ObjCSelectorExpressionAST()
        : selector_token(0)
        , lparen_token(0)
        , selector(0)
        , rparen_token(0)
    {}

    virtual ObjCSelectorExpressionAST *asObjCSelectorExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCSelectorExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCInstanceVariablesDeclarationAST: public AST
{
public:
    int lbrace_token;
    DeclarationListAST *instance_variable_list;
    int rbrace_token;

public:
    ObjCInstanceVariablesDeclarationAST()
        : lbrace_token(0)
        , instance_variable_list(0)
        , rbrace_token(0)
    {}

    virtual ObjCInstanceVariablesDeclarationAST *asObjCInstanceVariablesDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCInstanceVariablesDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCVisibilityDeclarationAST: public DeclarationAST
{
public:
    int visibility_token;

public:
    ObjCVisibilityDeclarationAST()
        : visibility_token(0)
    {}

    virtual ObjCVisibilityDeclarationAST *asObjCVisibilityDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCVisibilityDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCPropertyAttributeAST: public AST
{
public:
    int attribute_identifier_token;
    int equals_token;
    ObjCSelectorAST *method_selector;

public:
    ObjCPropertyAttributeAST()
        : attribute_identifier_token(0)
        , equals_token(0)
        , method_selector(0)
    {}

    virtual ObjCPropertyAttributeAST *asObjCPropertyAttribute() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCPropertyAttributeAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCPropertyDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list;
    int property_token;
    int lparen_token;
    ObjCPropertyAttributeListAST *property_attribute_list;
    int rparen_token;
    DeclarationAST *simple_declaration;

public: // annotations
    List<ObjCPropertyDeclaration *> *symbols;

public:
    ObjCPropertyDeclarationAST()
        : attribute_list(0)
        , property_token(0)
        , lparen_token(0)
        , property_attribute_list(0)
        , rparen_token(0)
        , simple_declaration(0)
        , symbols(0)
    {}

    virtual ObjCPropertyDeclarationAST *asObjCPropertyDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCPropertyDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCMessageArgumentDeclarationAST: public AST
{
public:
    ObjCTypeNameAST* type_name;
    SpecifierListAST *attribute_list;
    NameAST *param_name;

public: // annotations
    Argument *argument;

public:
    ObjCMessageArgumentDeclarationAST()
        : type_name(0)
        , attribute_list(0)
        , param_name(0)
        , argument(0)
    {}

    virtual ObjCMessageArgumentDeclarationAST *asObjCMessageArgumentDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCMessageArgumentDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCMethodPrototypeAST: public AST
{
public:
    int method_type_token;
    ObjCTypeNameAST *type_name;
    ObjCSelectorAST *selector;
    ObjCMessageArgumentDeclarationListAST *argument_list;
    int dot_dot_dot_token;
    SpecifierListAST *attribute_list;

public: // annotations
    ObjCMethod *symbol;

public:
    ObjCMethodPrototypeAST()
        : method_type_token(0)
        , type_name(0)
        , selector(0)
        , argument_list(0)
        , dot_dot_dot_token(0)
        , attribute_list(0)
        , symbol(0)
    {}

    virtual ObjCMethodPrototypeAST *asObjCMethodPrototype() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCMethodPrototypeAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCMethodDeclarationAST: public DeclarationAST
{
public:
    ObjCMethodPrototypeAST *method_prototype;
    StatementAST *function_body;
    int semicolon_token;

public:
    ObjCMethodDeclarationAST()
        : method_prototype(0)
        , function_body(0)
        , semicolon_token(0)
    {}

    virtual ObjCMethodDeclarationAST *asObjCMethodDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCMethodDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCSynthesizedPropertyAST: public AST
{
public:
    int property_identifier_token;
    int equals_token;
    int alias_identifier_token;

public:
    ObjCSynthesizedPropertyAST()
        : property_identifier_token(0)
        , equals_token(0)
        , alias_identifier_token(0)
    {}

    virtual ObjCSynthesizedPropertyAST *asObjCSynthesizedProperty() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCSynthesizedPropertyAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCSynthesizedPropertiesDeclarationAST: public DeclarationAST
{
public:
    int synthesized_token;
    ObjCSynthesizedPropertyListAST *property_identifier_list;
    int semicolon_token;

public:
    ObjCSynthesizedPropertiesDeclarationAST()
        : synthesized_token(0)
        , property_identifier_list(0)
        , semicolon_token(0)
    {}

    virtual ObjCSynthesizedPropertiesDeclarationAST *asObjCSynthesizedPropertiesDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCSynthesizedPropertiesDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCDynamicPropertiesDeclarationAST: public DeclarationAST
{
public:
    int dynamic_token;
    NameListAST *property_identifier_list;
    int semicolon_token;

public:
    ObjCDynamicPropertiesDeclarationAST()
        : dynamic_token(0)
        , property_identifier_list(0)
        , semicolon_token(0)
    {}

    virtual ObjCDynamicPropertiesDeclarationAST *asObjCDynamicPropertiesDeclaration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCDynamicPropertiesDeclarationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCFastEnumerationAST: public StatementAST
{
public:
    int for_token;
    int lparen_token;

    // declaration
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;
    // or an expression
    ExpressionAST *initializer;

    int in_token;
    ExpressionAST *fast_enumeratable_expression;
    int rparen_token;
    StatementAST *statement;

public: // annotations
    Block *symbol;

public:
    ObjCFastEnumerationAST()
        : for_token(0)
        , lparen_token(0)
        , type_specifier_list(0)
        , declarator(0)
        , initializer(0)
        , in_token(0)
        , fast_enumeratable_expression(0)
        , rparen_token(0)
        , statement(0)
        , symbol(0)
    {}

    virtual ObjCFastEnumerationAST *asObjCFastEnumeration() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCFastEnumerationAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT ObjCSynchronizedStatementAST: public StatementAST
{
public:
    int synchronized_token;
    int lparen_token;
    ExpressionAST *synchronized_object;
    int rparen_token;
    StatementAST *statement;

public:
    ObjCSynchronizedStatementAST()
        : synchronized_token(0)
        , lparen_token(0)
        , synchronized_object(0)
        , rparen_token(0)
        , statement(0)
    {}

    virtual ObjCSynchronizedStatementAST *asObjCSynchronizedStatement() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual ObjCSynchronizedStatementAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};


class LambdaExpressionAST: public ExpressionAST
{
public:
    LambdaIntroducerAST *lambda_introducer;
    LambdaDeclaratorAST *lambda_declarator;
    StatementAST *statement;

public:
    LambdaExpressionAST()
        : lambda_introducer(0)
        , lambda_declarator(0)
        , statement(0)
    {}

    virtual LambdaExpressionAST *asLambdaExpression() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;
    virtual LambdaExpressionAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class LambdaIntroducerAST: public AST
{
public:
    int lbracket_token;
    LambdaCaptureAST *lambda_capture;
    int rbracket_token;

public:
    LambdaIntroducerAST()
        : lbracket_token(0)
        , lambda_capture(0)
        , rbracket_token(0)
    {}

    virtual LambdaIntroducerAST *asLambdaIntroducer() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual LambdaIntroducerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class LambdaCaptureAST: public AST
{
public:
    int default_capture_token;
    CaptureListAST *capture_list;

public:
    LambdaCaptureAST()
        : default_capture_token(0)
        , capture_list(0)
    {}

    virtual LambdaCaptureAST *asLambdaCapture() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual LambdaCaptureAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CaptureAST: public AST
{
public:
    int amper_token;
    NameAST *identifier;

public:
    CaptureAST()
        : amper_token(0)
        , identifier(0)
    {}

    virtual CaptureAST *asCapture() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual CaptureAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class LambdaDeclaratorAST: public AST
{
public:
    int lparen_token;
    ParameterDeclarationClauseAST *parameter_declaration_clause;
    int rparen_token;
    SpecifierListAST *attributes;
    int mutable_token;
    ExceptionSpecificationAST *exception_specification;
    TrailingReturnTypeAST *trailing_return_type;

public: // annotations
    Function *symbol;

public:
    LambdaDeclaratorAST()
        : lparen_token(0)
        , parameter_declaration_clause(0)
        , rparen_token(0)
        , attributes(0)
        , mutable_token(0)
        , exception_specification(0)
        , trailing_return_type(0)
        , symbol(0)
    {}

    virtual LambdaDeclaratorAST *asLambdaDeclarator() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual LambdaDeclaratorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class TrailingReturnTypeAST: public AST
{
public:
    int arrow_token;
    SpecifierListAST *attributes;
    SpecifierListAST *type_specifier_list;
    DeclaratorAST *declarator;

public:
    TrailingReturnTypeAST()
        : arrow_token(0)
        , attributes(0)
        , type_specifier_list(0)
        , declarator(0)
    {}

    virtual TrailingReturnTypeAST *asTrailingReturnType() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual TrailingReturnTypeAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class BracedInitializerAST: public ExpressionAST
{
public:
    int lbrace_token;
    ExpressionListAST *expression_list;
    int comma_token;
    int rbrace_token;

public:
    BracedInitializerAST()
        : lbrace_token(0)
        , expression_list(0)
        , comma_token(0)
        , rbrace_token(0)
    {}

    virtual BracedInitializerAST *asBracedInitializer() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual BracedInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class DesignatorAST: public AST
{
public:
    DesignatorAST()
    {}

    virtual DesignatorAST *asDesignator() { return this; }
    virtual DesignatorAST *clone(MemoryPool *pool) const = 0;
};

class DotDesignatorAST: public DesignatorAST
{
public:
    int dot_token;
    int identifier_token;
public:
    DotDesignatorAST()
        : dot_token(0)
        , identifier_token(0)
    {}

    virtual DotDesignatorAST *asDotDesignator() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DotDesignatorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class BracketDesignatorAST: public DesignatorAST
{
public:
    int lbracket_token;
    ExpressionAST *expression;
    int rbracket_token;
public:
    BracketDesignatorAST()
        : lbracket_token(0)
        , expression(0)
        , rbracket_token(0)
    {}

    virtual BracketDesignatorAST *asBracketDesignator() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual BracketDesignatorAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class DesignatedInitializerAST: public ExpressionAST
{
public:
    DesignatorListAST *designator_list;
    int equal_token;
    ExpressionAST *initializer;

public:
    DesignatedInitializerAST()
        : designator_list(0)
        , equal_token(0)
        , initializer(0)
    {}

    virtual DesignatedInitializerAST *asDesignatedInitializer() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DesignatedInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

} // namespace CPlusPlus
