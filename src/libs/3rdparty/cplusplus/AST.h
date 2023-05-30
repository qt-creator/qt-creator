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

// clang-cl needs an export for the subclass, while msvc fails to build in debug mode if
// the export is present.
#if defined(Q_CC_CLANG) && defined(Q_CC_MSVC)
#define CPLUSPLUS_EXPORT_SUBCLASS CPLUSPLUS_EXPORT
#else
#define CPLUSPLUS_EXPORT_SUBCLASS
#endif

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

    class CPLUSPLUS_EXPORT_SUBCLASS ListIterator
    {
        List<Tptr> *iter;

    public:
        ListIterator(List<Tptr> *iter)
            : iter(iter)
        {}
        Tptr operator*() { return iter->value; }
        ListIterator &operator++()
        {
            if (iter)
                iter = iter->next;
            return *this;
        }
        bool operator==(const ListIterator &other) { return iter == other.iter; }
        bool operator!=(const ListIterator &other) { return iter != other.iter; }
    };
    ListIterator begin() { return {this}; }
    ListIterator end() { return {nullptr}; }

    int size() { return next ? next->size() + 1 : 1; }
};

template<typename Tptr>
typename List<Tptr>::ListIterator begin(List<Tptr> *list)
{
    return list ? list->begin() : typename List<Tptr>::ListIterator(nullptr);
}

template<typename Tptr>
typename List<Tptr>::ListIterator end(List<Tptr> *list)
{
    return list ? list->end() : typename List<Tptr>::ListIterator(nullptr);
}

template<typename Tptr>
int size(List<Tptr> *list)
{
    if (list)
        return list->size();
    return 0;
}

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
    virtual AwaitExpressionAST *asAwaitExpression() { return nullptr; }
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
    virtual DecompositionDeclaratorAST *asDecompositionDeclarator() { return nullptr; }
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
    virtual MsvcDeclspecSpecifierAST *asMsvcDeclspecSpecifier() { return nullptr; }
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
    virtual PlaceholderTypeSpecifierAST *asPlaceholderTypeSpecifier() { return nullptr; }
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
    virtual StdAttributeSpecifierAST *asStdAttributeSpecifier() { return nullptr; }
    virtual StringLiteralAST *asStringLiteral() { return nullptr; }
    virtual SwitchStatementAST *asSwitchStatement() { return nullptr; }
    virtual TemplateDeclarationAST *asTemplateDeclaration() { return nullptr; }
    virtual ConceptDeclarationAST *asConceptDeclaration() { return nullptr; }
    virtual TemplateIdAST *asTemplateId() { return nullptr; }
    virtual TemplateTypeParameterAST *asTemplateTypeParameter() { return nullptr; }
    virtual ThisExpressionAST *asThisExpression() { return nullptr; }
    virtual ThrowExpressionAST *asThrowExpression() { return nullptr; }
    virtual TrailingReturnTypeAST *asTrailingReturnType() { return nullptr; }
    virtual TranslationUnitAST *asTranslationUnit() { return nullptr; }
    virtual TryBlockStatementAST *asTryBlockStatement() { return nullptr; }
    virtual TypeConstraintAST *asTypeConstraint() { return nullptr; }
    virtual TypeConstructorCallAST *asTypeConstructorCall() { return nullptr; }
    virtual TypeIdAST *asTypeId() { return nullptr; }
    virtual TypeidExpressionAST *asTypeidExpression() { return nullptr; }
    virtual TypenameCallExpressionAST *asTypenameCallExpression() { return nullptr; }
    virtual TypenameTypeParameterAST *asTypenameTypeParameter() { return nullptr; }
    virtual TypeofSpecifierAST *asTypeofSpecifier() { return nullptr; }
    virtual UnaryExpressionAST *asUnaryExpression() { return nullptr; }
    virtual RequiresExpressionAST *asRequiresExpression() { return nullptr; }
    virtual RequiresClauseAST *asRequiresClause() { return nullptr; }
    virtual UsingAST *asUsing() { return nullptr; }
    virtual UsingDirectiveAST *asUsingDirective() { return nullptr; }
    virtual WhileStatementAST *asWhileStatement() { return nullptr; }
    virtual YieldExpressionAST *asYieldExpression() { return nullptr; }

protected:
    virtual void accept0(ASTVisitor *visitor) = 0;
    virtual bool match0(AST *, ASTMatcher *) = 0;
};

class CPLUSPLUS_EXPORT StatementAST: public AST
{
public:
    StatementAST *asStatement() override { return this; }

    StatementAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT ExpressionAST: public AST
{
public:
    ExpressionAST *asExpression() override { return this; }

    ExpressionAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT DeclarationAST: public AST
{
public:
    DeclarationAST *asDeclaration() override { return this; }

    DeclarationAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT NameAST: public AST
{
public: // annotations
    const Name *name = nullptr;

public:
    NameAST *asName() override { return this; }

    NameAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT SpecifierAST: public AST
{
public:
    SpecifierAST *asSpecifier() override { return this; }

    SpecifierAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT PtrOperatorAST: public AST
{
public:
    PtrOperatorAST *asPtrOperator() override { return this; }

    PtrOperatorAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT PostfixAST: public ExpressionAST
{
public:
    PostfixAST *asPostfix() override { return this; }

    PostfixAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT CoreDeclaratorAST: public AST
{
public:
    CoreDeclaratorAST *asCoreDeclarator() override { return this; }

    CoreDeclaratorAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT PostfixDeclaratorAST: public AST
{
public:
    PostfixDeclaratorAST *asPostfixDeclarator() override { return this; }

    PostfixDeclaratorAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT ObjCSelectorArgumentAST: public AST
{
public:
    int name_token = 0;
    int colon_token = 0;

public:
    ObjCSelectorArgumentAST *asObjCSelectorArgument() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCSelectorArgumentAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCSelectorAST: public NameAST
{
public:
    ObjCSelectorArgumentListAST *selector_argument_list = nullptr;

public:
    ObjCSelectorAST *asObjCSelector() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCSelectorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT SimpleSpecifierAST: public SpecifierAST
{
public:
    int specifier_token = 0;

public:
    SimpleSpecifierAST *asSimpleSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    SimpleSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT AttributeSpecifierAST: public SpecifierAST
{
public:
    AttributeSpecifierAST *asAttributeSpecifier() override { return this; }

    AttributeSpecifierAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT AlignmentSpecifierAST: public AttributeSpecifierAST
{
public:
    int align_token = 0;
    int lparen_token = 0;
    ExpressionAST *typeIdExprOrAlignmentExpr = nullptr;
    int ellipses_token = 0;
    int rparen_token = 0;

public:
    AlignmentSpecifierAST *asAlignmentSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    AlignmentSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};


class CPLUSPLUS_EXPORT GnuAttributeSpecifierAST: public AttributeSpecifierAST
{
public:
    int attribute_token = 0;
    int first_lparen_token = 0;
    int second_lparen_token = 0;
    GnuAttributeListAST *attribute_list = nullptr;
    int first_rparen_token = 0;
    int second_rparen_token = 0;

public:
    GnuAttributeSpecifierAST *asGnuAttributeSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    GnuAttributeSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT MsvcDeclspecSpecifierAST: public AttributeSpecifierAST
{
public:
    int attribute_token = 0;
    int lparen_token = 0;
    GnuAttributeListAST *attribute_list = nullptr;
    int rparen_token = 0;

public:
    MsvcDeclspecSpecifierAST *asMsvcDeclspecSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    MsvcDeclspecSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT StdAttributeSpecifierAST: public AttributeSpecifierAST
{
public:
    int first_lbracket_token = 0;
    int second_lbracket_token = 0;
    GnuAttributeListAST *attribute_list = nullptr;
    int first_rbracket_token = 0;
    int second_rbracket_token = 0;

public:
    StdAttributeSpecifierAST *asStdAttributeSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    StdAttributeSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT GnuAttributeAST: public AST
{
public:
    int identifier_token = 0;
    int lparen_token = 0;
    int tag_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int rparen_token = 0;

public:
    GnuAttributeAST *asGnuAttribute() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    GnuAttributeAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TypeofSpecifierAST: public SpecifierAST
{
public:
    int typeof_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
    TypeofSpecifierAST *asTypeofSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TypeofSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DecltypeSpecifierAST: public SpecifierAST
{
public:
    int decltype_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
    DecltypeSpecifierAST *asDecltypeSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DecltypeSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TypeConstraintAST: public AST
{
public:
    NestedNameSpecifierListAST *nestedName = nullptr;
    NameAST *conceptName = nullptr;
    int lessToken = 0;
    ExpressionListAST *templateArgs = nullptr;
    int greaterToken = 0;

    TypeConstraintAST *asTypeConstraint() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TypeConstraintAST *clone(MemoryPool *pool) const override;

    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT PlaceholderTypeSpecifierAST: public SpecifierAST
{
public:
    TypeConstraintAST *typeConstraint = nullptr;
    int declTypetoken = 0;
    int lparenToken = 0;
    int decltypeToken = 0;
    int autoToken = 0;
    int rparenToken = 0;

public:
    PlaceholderTypeSpecifierAST *asPlaceholderTypeSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    PlaceholderTypeSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DeclaratorAST: public AST
{
public:
    SpecifierListAST *attribute_list = nullptr;
    PtrOperatorListAST *ptr_operator_list = nullptr;
    CoreDeclaratorAST *core_declarator = nullptr;
    PostfixDeclaratorListAST *postfix_declarator_list = nullptr;
    SpecifierListAST *post_attribute_list = nullptr;
    int equal_token = 0;
    ExpressionAST *initializer = nullptr;
    RequiresClauseAST *requiresClause = nullptr;

public:
    DeclaratorAST *asDeclarator() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DeclaratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT SimpleDeclarationAST: public DeclarationAST
{
public:
    int qt_invokable_token = 0;
    SpecifierListAST *decl_specifier_list = nullptr;
    DeclaratorListAST *declarator_list = nullptr;
    int semicolon_token = 0;

public:
    List<Symbol *> *symbols = nullptr;

public:
    SimpleDeclarationAST *asSimpleDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    SimpleDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT EmptyDeclarationAST: public DeclarationAST
{
public:
    int semicolon_token = 0;

public:
    EmptyDeclarationAST *asEmptyDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    EmptyDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT AccessDeclarationAST: public DeclarationAST
{
public:
    int access_specifier_token = 0;
    int slots_token = 0;
    int colon_token = 0;

public:
    AccessDeclarationAST *asAccessDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    AccessDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtObjectTagAST: public DeclarationAST
{
public:
    int q_object_token = 0;

public:
    QtObjectTagAST *asQtObjectTag() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtObjectTagAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtPrivateSlotAST: public DeclarationAST
{
public:
    int q_private_slot_token = 0;
    int lparen_token = 0;
    int dptr_token = 0;
    int dptr_lparen_token = 0;
    int dptr_rparen_token = 0;
    int comma_token = 0;
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    int rparen_token = 0;

public:
    QtPrivateSlotAST *asQtPrivateSlot() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtPrivateSlotAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class QtPropertyDeclarationItemAST: public AST
{
public:
    int item_name_token = 0;
    ExpressionAST *expression = nullptr;

public:
    QtPropertyDeclarationItemAST *asQtPropertyDeclarationItem() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtPropertyDeclarationItemAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtPropertyDeclarationAST: public DeclarationAST
{
public:
    int property_specifier_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr; // for Q_PRIVATE_PROPERTY(expression, ...)
    int comma_token = 0;
    ExpressionAST *type_id = nullptr;
    NameAST *property_name = nullptr;
    QtPropertyDeclarationItemListAST *property_declaration_item_list = nullptr;
    int rparen_token = 0;

public:
    QtPropertyDeclarationAST *asQtPropertyDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtPropertyDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtEnumDeclarationAST: public DeclarationAST
{
public:
    int enum_specifier_token = 0;
    int lparen_token = 0;
    NameListAST *enumerator_list = nullptr;
    int rparen_token = 0;

public:
    QtEnumDeclarationAST *asQtEnumDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtEnumDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtFlagsDeclarationAST: public DeclarationAST
{
public:
    int flags_specifier_token = 0;
    int lparen_token = 0;
    NameListAST *flag_enums_list = nullptr;
    int rparen_token = 0;

public:
    QtFlagsDeclarationAST *asQtFlagsDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtFlagsDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtInterfaceNameAST: public AST
{
public:
    NameAST *interface_name = nullptr;
    NameListAST *constraint_list = nullptr;

public:
    QtInterfaceNameAST *asQtInterfaceName() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtInterfaceNameAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtInterfacesDeclarationAST: public DeclarationAST
{
public:
    int interfaces_token = 0;
    int lparen_token = 0;
    QtInterfaceNameListAST *interface_name_list = nullptr;
    int rparen_token = 0;

public:
    QtInterfacesDeclarationAST *asQtInterfacesDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtInterfacesDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT AsmDefinitionAST: public DeclarationAST
{
public:
    int asm_token = 0;
    int volatile_token = 0;
    int lparen_token = 0;
    // ### string literals
    // ### asm operand list
    int rparen_token = 0;
    int semicolon_token = 0;

public:
    AsmDefinitionAST *asAsmDefinition() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    AsmDefinitionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT BaseSpecifierAST: public AST
{
public:
    int virtual_token = 0;
    int access_specifier_token = 0;
    NameAST *name = nullptr;
    int ellipsis_token = 0;

public: // annotations
    BaseClass *symbol = nullptr;

public:
    BaseSpecifierAST *asBaseSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    BaseSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT IdExpressionAST: public ExpressionAST
{
public:
    NameAST *name = nullptr;

public:
    IdExpressionAST *asIdExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    IdExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CompoundExpressionAST: public ExpressionAST
{
public:
    int lparen_token = 0;
    CompoundStatementAST *statement = nullptr;
    int rparen_token = 0;

public:
    CompoundExpressionAST *asCompoundExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CompoundExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CompoundLiteralAST: public ExpressionAST
{
public:
    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;
    ExpressionAST *initializer = nullptr;

public:
    CompoundLiteralAST *asCompoundLiteral() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CompoundLiteralAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtMethodAST: public ExpressionAST
{
public:
    int method_token = 0;
    int lparen_token = 0;
    DeclaratorAST *declarator = nullptr;
    int rparen_token = 0;

public:
    QtMethodAST *asQtMethod() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtMethodAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QtMemberDeclarationAST: public StatementAST
{
public:
    int q_token = 0;
    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;

public:
    QtMemberDeclarationAST *asQtMemberDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QtMemberDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT BinaryExpressionAST: public ExpressionAST
{
public:
    ExpressionAST *left_expression = nullptr;
    int binary_op_token = 0;
    ExpressionAST *right_expression = nullptr;

public:
    BinaryExpressionAST *asBinaryExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    BinaryExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CastExpressionAST: public ExpressionAST
{
public:
    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;
    ExpressionAST *expression = nullptr;

public:
    CastExpressionAST *asCastExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CastExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ClassSpecifierAST: public SpecifierAST
{
public:
    int classkey_token = 0;
    SpecifierListAST *attribute_list = nullptr;
    NameAST *name = nullptr;
    int final_token = 0;
    int colon_token = 0;
    BaseSpecifierListAST *base_clause_list = nullptr;
    int dot_dot_dot_token = 0;
    int lbrace_token = 0;
    DeclarationListAST *member_specifier_list = nullptr;
    int rbrace_token = 0;

public: // annotations
    Class *symbol = nullptr;

public:
    ClassSpecifierAST *asClassSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ClassSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CaseStatementAST: public StatementAST
{
public:
    int case_token = 0;
    ExpressionAST *expression = nullptr;
    int colon_token = 0;
    StatementAST *statement = nullptr;

public:
    CaseStatementAST *asCaseStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CaseStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CompoundStatementAST: public StatementAST
{
public:
    int lbrace_token = 0;
    StatementListAST *statement_list = nullptr;
    int rbrace_token = 0;

public: // annotations
    Block *symbol = nullptr;

public:
    CompoundStatementAST *asCompoundStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CompoundStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ConditionAST: public ExpressionAST
{
public:
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;

public:
    ConditionAST *asCondition() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ConditionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ConditionalExpressionAST: public ExpressionAST
{
public:
    ExpressionAST *condition = nullptr;
    int question_token = 0;
    ExpressionAST *left_expression = nullptr;
    int colon_token = 0;
    ExpressionAST *right_expression = nullptr;

public:
    ConditionalExpressionAST *asConditionalExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ConditionalExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CppCastExpressionAST: public ExpressionAST
{
public:
    int cast_token = 0;
    int less_token = 0;
    ExpressionAST *type_id = nullptr;
    int greater_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
    CppCastExpressionAST *asCppCastExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CppCastExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CtorInitializerAST: public AST
{
public:
    int colon_token = 0;
    MemInitializerListAST *member_initializer_list = nullptr;
    int dot_dot_dot_token = 0;

public:
    CtorInitializerAST *asCtorInitializer() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CtorInitializerAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DeclarationStatementAST: public StatementAST
{
public:
    DeclarationAST *declaration = nullptr;

public:
    DeclarationStatementAST *asDeclarationStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DeclarationStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DeclaratorIdAST: public CoreDeclaratorAST
{
public:
    int dot_dot_dot_token = 0;
    NameAST *name = nullptr;

public:
    DeclaratorIdAST *asDeclaratorId() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DeclaratorIdAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DecompositionDeclaratorAST: public CoreDeclaratorAST
{
public:
    NameListAST *identifiers = nullptr;

public:
    DecompositionDeclaratorAST *asDecompositionDeclarator() override { return this; }

    int firstToken() const override { return identifiers->firstToken(); }
    int lastToken() const override { return identifiers->lastToken(); }

    DecompositionDeclaratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NestedDeclaratorAST: public CoreDeclaratorAST
{
public:
    int lparen_token = 0;
    DeclaratorAST *declarator = nullptr;
    int rparen_token = 0;

public:
    NestedDeclaratorAST *asNestedDeclarator() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NestedDeclaratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT FunctionDeclaratorAST: public PostfixDeclaratorAST
{
public:
    SpecifierListAST *decl_specifier_list = nullptr;
    int lparen_token = 0;
    ParameterDeclarationClauseAST *parameter_declaration_clause = nullptr;
    int rparen_token = 0;
    SpecifierListAST *cv_qualifier_list = nullptr;
    int ref_qualifier_token = 0;
    ExceptionSpecificationAST *exception_specification = nullptr;
    TrailingReturnTypeAST *trailing_return_type = nullptr;
    // Some FunctionDeclarators can also be interpreted as an initializer, like for 'A b(c);'
    ExpressionAST *as_cpp_initializer = nullptr;

public: // annotations
    Function *symbol = nullptr;

public:
    FunctionDeclaratorAST *asFunctionDeclarator() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    FunctionDeclaratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ArrayDeclaratorAST: public PostfixDeclaratorAST
{
public:
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
    ArrayDeclaratorAST *asArrayDeclarator() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ArrayDeclaratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DeleteExpressionAST: public ExpressionAST
{
public:
    int scope_token = 0;
    int delete_token = 0;
    int lbracket_token = 0;
    int rbracket_token = 0;
    ExpressionAST *expression = nullptr;

public:
    DeleteExpressionAST *asDeleteExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DeleteExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DoStatementAST: public StatementAST
{
public:
    int do_token = 0;
    StatementAST *statement = nullptr;
    int while_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;
    int semicolon_token = 0;

public:
    DoStatementAST *asDoStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DoStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NamedTypeSpecifierAST: public SpecifierAST
{
public:
    NameAST *name = nullptr;

public:
    NamedTypeSpecifierAST *asNamedTypeSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NamedTypeSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ElaboratedTypeSpecifierAST: public SpecifierAST
{
public:
    int classkey_token = 0;
    SpecifierListAST *attribute_list = nullptr;
    NameAST *name = nullptr;

public:
    ElaboratedTypeSpecifierAST *asElaboratedTypeSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ElaboratedTypeSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT EnumSpecifierAST: public SpecifierAST
{
public:
    int enum_token = 0;
    int key_token = 0; // struct, class or 0
    NameAST *name = nullptr;
    int colon_token = 0; // can be 0 if there is no enum-base
    SpecifierListAST *type_specifier_list = nullptr; // ditto
    int lbrace_token = 0;
    EnumeratorListAST *enumerator_list = nullptr;
    int stray_comma_token = 0;
    int rbrace_token = 0;

public: // annotations
    Enum *symbol = nullptr;

public:
    EnumSpecifierAST *asEnumSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    EnumSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT EnumeratorAST: public AST
{
public:
    int identifier_token = 0;
    int equal_token = 0;
    ExpressionAST *expression = nullptr;

public:
    EnumeratorAST *asEnumerator() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    EnumeratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ExceptionDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    int dot_dot_dot_token = 0;

public:
    ExceptionDeclarationAST *asExceptionDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ExceptionDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ExceptionSpecificationAST: public AST
{
public:
    ExceptionSpecificationAST *asExceptionSpecification() override { return this; }

    ExceptionSpecificationAST *clone(MemoryPool *pool) const override = 0;
};

class CPLUSPLUS_EXPORT DynamicExceptionSpecificationAST: public ExceptionSpecificationAST
{
public:
    int throw_token = 0;
    int lparen_token = 0;
    int dot_dot_dot_token = 0;
    ExpressionListAST *type_id_list = nullptr;
    int rparen_token = 0;

public:
    DynamicExceptionSpecificationAST *asDynamicExceptionSpecification() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DynamicExceptionSpecificationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NoExceptSpecificationAST: public ExceptionSpecificationAST
{
public:
    int noexcept_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
    NoExceptSpecificationAST *asNoExceptSpecification() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NoExceptSpecificationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ExpressionOrDeclarationStatementAST: public StatementAST
{
public:
    ExpressionStatementAST *expression = nullptr;
    DeclarationStatementAST *declaration = nullptr;

public:
    ExpressionOrDeclarationStatementAST *asExpressionOrDeclarationStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ExpressionOrDeclarationStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ExpressionStatementAST: public StatementAST
{
public:
    ExpressionAST *expression = nullptr;
    int semicolon_token = 0;

public:
    ExpressionStatementAST *asExpressionStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ExpressionStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT FunctionDefinitionAST: public DeclarationAST
{
public:
    int qt_invokable_token = 0;
    SpecifierListAST *decl_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    CtorInitializerAST *ctor_initializer = nullptr;
    StatementAST *function_body = nullptr;

public: // annotations
    Function *symbol = nullptr;

public:
    FunctionDefinitionAST *asFunctionDefinition() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    FunctionDefinitionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ForeachStatementAST: public StatementAST
{
public:
    int foreach_token = 0;
    int lparen_token = 0;
    // declaration
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    // or an expression
    ExpressionAST *initializer = nullptr;
    int comma_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    ForeachStatementAST *asForeachStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ForeachStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT RangeBasedForStatementAST : public StatementAST
{
public:
    int for_token = 0;
    int lparen_token = 0;
    // declaration
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    // or an expression
    int colon_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    RangeBasedForStatementAST *asRangeBasedForStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    RangeBasedForStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ForStatementAST: public StatementAST
{
public:
    int for_token = 0;
    int lparen_token = 0;
    StatementAST *initializer = nullptr;
    ExpressionAST *condition = nullptr;
    int semicolon_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    ForStatementAST *asForStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ForStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT IfStatementAST: public StatementAST
{
public:
    int if_token = 0;
    int constexpr_token = 0;
    int lparen_token = 0;
    StatementAST *initStmt = nullptr;
    ExpressionAST *condition = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;
    int else_token = 0;
    StatementAST *else_statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    IfStatementAST *asIfStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    IfStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ArrayInitializerAST: public ExpressionAST
{
public:
    int lbrace_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int rbrace_token = 0;

public:
    ArrayInitializerAST *asArrayInitializer() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ArrayInitializerAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT LabeledStatementAST: public StatementAST
{
public:
    int label_token = 0;
    int colon_token = 0;
    StatementAST *statement = nullptr;

public:
    LabeledStatementAST *asLabeledStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    LabeledStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT LinkageBodyAST: public DeclarationAST
{
public:
    int lbrace_token = 0;
    DeclarationListAST *declaration_list = nullptr;
    int rbrace_token = 0;

public:
    LinkageBodyAST *asLinkageBody() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    LinkageBodyAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT LinkageSpecificationAST: public DeclarationAST
{
public:
    int extern_token = 0;
    int extern_type_token = 0;
    DeclarationAST *declaration = nullptr;

public:
    LinkageSpecificationAST *asLinkageSpecification() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    LinkageSpecificationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT MemInitializerAST: public AST
{
public:
    NameAST *name = nullptr;
    // either a BracedInitializerAST or a ExpressionListParenAST
    ExpressionAST *expression = nullptr;

public:
    MemInitializerAST *asMemInitializer() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    MemInitializerAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NestedNameSpecifierAST: public AST
{
public:
    NameAST *class_or_namespace_name = nullptr;
    int scope_token = 0;

public:
    NestedNameSpecifierAST *asNestedNameSpecifier() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NestedNameSpecifierAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT QualifiedNameAST: public NameAST
{
public:
    int global_scope_token = 0;
    NestedNameSpecifierListAST *nested_name_specifier_list = nullptr;
    NameAST *unqualified_name = nullptr;

public:
    QualifiedNameAST *asQualifiedName() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    QualifiedNameAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT OperatorFunctionIdAST: public NameAST
{
public:
    int operator_token = 0;
    OperatorAST *op = nullptr;

public:
    OperatorFunctionIdAST *asOperatorFunctionId() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    OperatorFunctionIdAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ConversionFunctionIdAST: public NameAST
{
public:
    int operator_token = 0;
    SpecifierListAST *type_specifier_list = nullptr;
    PtrOperatorListAST *ptr_operator_list = nullptr;

public:
    ConversionFunctionIdAST *asConversionFunctionId() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ConversionFunctionIdAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT AnonymousNameAST: public NameAST
{
public:
    int class_token = 0;

public:
    AnonymousNameAST *asAnonymousName() override { return this; }
    int firstToken() const override { return 0; }
    int lastToken() const override { return 0; }

    AnonymousNameAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT SimpleNameAST: public NameAST
{
public:
    int identifier_token = 0;

public:
    SimpleNameAST *asSimpleName() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    SimpleNameAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT DestructorNameAST: public NameAST
{
public:
    int tilde_token = 0;
    NameAST *unqualified_name = nullptr;

public:
    DestructorNameAST *asDestructorName() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    DestructorNameAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TemplateIdAST: public NameAST
{
public:
    int template_token = 0;
    int identifier_token = 0;
    int less_token = 0;
    ExpressionListAST *template_argument_list = nullptr;
    int greater_token = 0;

public:
    TemplateIdAST *asTemplateId() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TemplateIdAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NamespaceAST: public DeclarationAST
{
public:
    int inline_token = 0;
    int namespace_token = 0;
    int identifier_token = 0;
    SpecifierListAST *attribute_list = nullptr;
    DeclarationAST *linkage_body = nullptr;

public: // annotations
    Namespace *symbol = nullptr;

public:
    NamespaceAST *asNamespace() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NamespaceAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NamespaceAliasDefinitionAST: public DeclarationAST
{
public:
    int namespace_token = 0;
    int namespace_name_token = 0;
    int equal_token = 0;
    NameAST *name = nullptr;
    int semicolon_token = 0;

public:
    NamespaceAliasDefinitionAST *asNamespaceAliasDefinition() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NamespaceAliasDefinitionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT AliasDeclarationAST: public DeclarationAST
{
public:
    int using_token = 0;
    NameAST *name = nullptr;
    int equal_token = 0;
    TypeIdAST *typeId = nullptr;
    int semicolon_token = 0;

public: // annotations
    Declaration *symbol = nullptr;

public:
    AliasDeclarationAST *asAliasDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    AliasDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ExpressionListParenAST: public ExpressionAST
{
public:
    int lparen_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int rparen_token = 0;

public:
    ExpressionListParenAST *asExpressionListParen() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ExpressionListParenAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NewArrayDeclaratorAST: public AST
{
public:
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
    NewArrayDeclaratorAST *asNewArrayDeclarator() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NewArrayDeclaratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NewExpressionAST: public ExpressionAST
{
public:
    int scope_token = 0;
    int new_token = 0;
    ExpressionListParenAST *new_placement = nullptr;

    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;

    NewTypeIdAST *new_type_id = nullptr;

    ExpressionAST *new_initializer = nullptr; // either ExpressionListParenAST or BracedInitializerAST

public:
    NewExpressionAST *asNewExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NewExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NewTypeIdAST: public AST
{
public:
    SpecifierListAST *type_specifier_list = nullptr;
    PtrOperatorListAST *ptr_operator_list = nullptr;
    NewArrayDeclaratorListAST *new_array_declarator_list = nullptr;

public:
    NewTypeIdAST *asNewTypeId() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NewTypeIdAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT OperatorAST: public AST
{
public:
    int op_token = 0;
    int open_token = 0;
    int close_token = 0;

public:
    OperatorAST *asOperator()  override{ return this; }

    int firstToken() const override;
    int lastToken() const override;

    OperatorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ParameterDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    int equal_token = 0;
    ExpressionAST *expression = nullptr;

public: // annotations
    Argument *symbol = nullptr;

public:
    ParameterDeclarationAST *asParameterDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ParameterDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ParameterDeclarationClauseAST: public AST
{
public:
    ParameterDeclarationListAST *parameter_declaration_list = nullptr;
    int dot_dot_dot_token = 0;

public:
    ParameterDeclarationClauseAST *asParameterDeclarationClause() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ParameterDeclarationClauseAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CallAST: public PostfixAST
{
public:
    ExpressionAST *base_expression = nullptr;
    int lparen_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int rparen_token = 0;

public:
    CallAST *asCall() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CallAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ArrayAccessAST: public PostfixAST
{
public:
    ExpressionAST *base_expression = nullptr;
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
    ArrayAccessAST *asArrayAccess() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ArrayAccessAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT PostIncrDecrAST: public PostfixAST
{
public:
    ExpressionAST *base_expression = nullptr;
    int incr_decr_token = 0;

public:
    PostIncrDecrAST *asPostIncrDecr()  override{ return this; }

    int firstToken() const override;
    int lastToken() const override;

    PostIncrDecrAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT MemberAccessAST: public PostfixAST
{
public:
    ExpressionAST *base_expression = nullptr;
    int access_token = 0;
    int template_token = 0;
    NameAST *member_name = nullptr;

public:
    MemberAccessAST *asMemberAccess() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    MemberAccessAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TypeidExpressionAST: public ExpressionAST
{
public:
    int typeid_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
    TypeidExpressionAST *asTypeidExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TypeidExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TypenameCallExpressionAST: public ExpressionAST
{
public:
    int typename_token = 0;
    NameAST *name = nullptr;
    ExpressionAST *expression = nullptr; // either ExpressionListParenAST or BracedInitializerAST

public:
    TypenameCallExpressionAST *asTypenameCallExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TypenameCallExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TypeConstructorCallAST: public ExpressionAST
{
public:
    SpecifierListAST *type_specifier_list = nullptr;
    ExpressionAST *expression = nullptr; // either ExpressionListParenAST or BracedInitializerAST

public:
    TypeConstructorCallAST *asTypeConstructorCall() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TypeConstructorCallAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT PointerToMemberAST: public PtrOperatorAST
{
public:
    int global_scope_token = 0;
    NestedNameSpecifierListAST *nested_name_specifier_list = nullptr;
    int star_token = 0;
    SpecifierListAST *cv_qualifier_list = nullptr;
    int ref_qualifier_token = 0;

public:
    PointerToMemberAST *asPointerToMember() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    PointerToMemberAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT PointerAST: public PtrOperatorAST
{
public:
    int star_token = 0;
    SpecifierListAST *cv_qualifier_list = nullptr;

public:
    PointerAST *asPointer() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    PointerAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ReferenceAST: public PtrOperatorAST
{
public:
    int reference_token = 0;

public:
    ReferenceAST *asReference() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ReferenceAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT BreakStatementAST: public StatementAST
{
public:
    int break_token = 0;
    int semicolon_token = 0;

public:
    BreakStatementAST *asBreakStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    BreakStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ContinueStatementAST: public StatementAST
{
public:
    int continue_token = 0;
    int semicolon_token = 0;

public:
    ContinueStatementAST *asContinueStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ContinueStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT GotoStatementAST: public StatementAST
{
public:
    int goto_token = 0;
    int identifier_token = 0;
    int semicolon_token = 0;

public:
    GotoStatementAST *asGotoStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    GotoStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ReturnStatementAST: public StatementAST
{
public:
    int return_token = 0;
    ExpressionAST *expression = nullptr;
    int semicolon_token = 0;

public:
    ReturnStatementAST *asReturnStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ReturnStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT SizeofExpressionAST: public ExpressionAST
{
public:
    int sizeof_token = 0;
    int dot_dot_dot_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
    SizeofExpressionAST *asSizeofExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    SizeofExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT AlignofExpressionAST: public ExpressionAST
{
public:
    int alignof_token = 0;
    int lparen_token = 0;
    TypeIdAST *typeId = nullptr;
    int rparen_token = 0;

public:
    AlignofExpressionAST *asAlignofExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    AlignofExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT PointerLiteralAST: public ExpressionAST
{
public:
    int literal_token = 0;

public:
    PointerLiteralAST *asPointerLiteral() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    PointerLiteralAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NumericLiteralAST: public ExpressionAST
{
public:
    int literal_token = 0;

public:
    NumericLiteralAST *asNumericLiteral() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NumericLiteralAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT BoolLiteralAST: public ExpressionAST
{
public:
    int literal_token = 0;

public:
    BoolLiteralAST *asBoolLiteral() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    BoolLiteralAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ThisExpressionAST: public ExpressionAST
{
public:
    int this_token = 0;

public:
    ThisExpressionAST *asThisExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ThisExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NestedExpressionAST: public ExpressionAST
{
public:
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
    NestedExpressionAST *asNestedExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NestedExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT StaticAssertDeclarationAST: public DeclarationAST
{
public:
    int static_assert_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int comma_token = 0;
    ExpressionAST *string_literal = nullptr;
    int rparen_token = 0;
    int semicolon_token = 0;

public:
    StaticAssertDeclarationAST *asStaticAssertDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    StaticAssertDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT StringLiteralAST: public ExpressionAST
{
public:
    int literal_token = 0;
    StringLiteralAST *next = nullptr;

public:
    StringLiteralAST *asStringLiteral() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    StringLiteralAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT SwitchStatementAST: public StatementAST
{
public:
    int switch_token = 0;
    int lparen_token = 0;
    ExpressionAST *condition = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    SwitchStatementAST *asSwitchStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    SwitchStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TemplateDeclarationAST: public DeclarationAST
{
public:
    int export_token = 0;
    int template_token = 0;
    int less_token = 0;
    DeclarationListAST *template_parameter_list = nullptr;
    int greater_token = 0;
    RequiresClauseAST *requiresClause = nullptr;
    DeclarationAST *declaration = nullptr;

public: // annotations
    Template *symbol = nullptr;

public:
    TemplateDeclarationAST *asTemplateDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TemplateDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ConceptDeclarationAST: public DeclarationAST
{
public:
    int concept_token = 0;
    NameAST *name = nullptr;
    SpecifierListAST *attributes = nullptr;
    int equals_token = 0;
    ExpressionAST *constraint = nullptr;
    int semicolon_token = 0;

public:
    ConceptDeclarationAST *asConceptDeclaration() override { return this; }

    int firstToken() const override { return concept_token; }
    int lastToken() const override { return semicolon_token + 1; }

    ConceptDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ThrowExpressionAST: public ExpressionAST
{
public:
    int throw_token = 0;
    ExpressionAST *expression = nullptr;

public:
    ThrowExpressionAST *asThrowExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ThrowExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT YieldExpressionAST: public ExpressionAST
{
public:
    int yield_token = 0;
    ExpressionAST *expression = nullptr;

public:
    YieldExpressionAST *asYieldExpression() override { return this; }

    int firstToken() const override { return yield_token; }
    int lastToken() const override { return expression->lastToken(); }

    YieldExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT AwaitExpressionAST: public ExpressionAST
{
public:
    int await_token = 0;
    ExpressionAST *castExpression = nullptr;

public:
    AwaitExpressionAST *asAwaitExpression() override { return this; }

    int firstToken() const override { return await_token; }
    int lastToken() const override { return castExpression->lastToken(); }

    AwaitExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT NoExceptOperatorExpressionAST: public ExpressionAST
{
public:
    int noexcept_token = 0;
    ExpressionAST *expression = nullptr;

public:
    NoExceptOperatorExpressionAST *asNoExceptOperatorExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    NoExceptOperatorExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TranslationUnitAST: public AST
{
public:
    DeclarationListAST *declaration_list = nullptr;

public:
    TranslationUnitAST *asTranslationUnit() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TranslationUnitAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TryBlockStatementAST: public StatementAST
{
public:
    int try_token = 0;
    StatementAST *statement = nullptr;
    CatchClauseListAST *catch_clause_list = nullptr;

public:
    TryBlockStatementAST *asTryBlockStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TryBlockStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT CatchClauseAST: public StatementAST
{
public:
    int catch_token = 0;
    int lparen_token = 0;
    ExceptionDeclarationAST *exception_declaration = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    CatchClauseAST *asCatchClause() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    CatchClauseAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TypeIdAST: public ExpressionAST
{
public:
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;

public:
    TypeIdAST *asTypeId() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TypeIdAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TypenameTypeParameterAST: public DeclarationAST
{
public:
    int classkey_token = 0;
    int dot_dot_dot_token = 0;
    NameAST *name = nullptr;
    int equal_token = 0;
    ExpressionAST *type_id = nullptr;

public: // annotations
    TypenameArgument *symbol = nullptr;

public:
    TypenameTypeParameterAST *asTypenameTypeParameter() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TypenameTypeParameterAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT TemplateTypeParameterAST: public DeclarationAST
{
public:
    int template_token = 0;
    TypeConstraintAST *typeConstraint = nullptr;
    int less_token = 0;
    DeclarationListAST *template_parameter_list = nullptr;
    int greater_token = 0;
    int class_token = 0;
    int dot_dot_dot_token = 0;
    NameAST *name = nullptr;
    int equal_token = 0;
    ExpressionAST *type_id = nullptr;

public:
    TypenameArgument *symbol = nullptr;

public:
    TemplateTypeParameterAST *asTemplateTypeParameter() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    TemplateTypeParameterAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT UnaryExpressionAST: public ExpressionAST
{
public:
    int unary_op_token = 0;
    ExpressionAST *expression = nullptr;

public:
    UnaryExpressionAST *asUnaryExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    UnaryExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT RequiresExpressionAST: public ExpressionAST
{
public:
    int requires_token = 0;
    int lparen_token = 0;
    ParameterDeclarationClauseAST *parameters = nullptr;
    int rparen_token = 0;
    int lbrace_token = 0;
    int rbrace_token = 0;

public:
    RequiresExpressionAST *asRequiresExpression() override { return this; }

    int firstToken() const override { return requires_token; }
    int lastToken() const override { return rbrace_token + 1; }

    RequiresExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT RequiresClauseAST: public AST
{
public:
    int requires_token = 0;
    ExpressionAST *constraint = nullptr;

public:
    RequiresClauseAST *asRequiresClause() override { return this; }

    int firstToken() const override { return requires_token; }
    int lastToken() const override { return constraint->lastToken(); }

    RequiresClauseAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT UsingAST: public DeclarationAST
{
public:
    int using_token = 0;
    int typename_token = 0;
    NameAST *name = nullptr;
    int semicolon_token = 0;

public: // annotations
    UsingDeclaration *symbol = nullptr;

public:
    UsingAST *asUsing() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    UsingAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT UsingDirectiveAST: public DeclarationAST
{
public:
    int using_token = 0;
    int namespace_token = 0;
    NameAST *name = nullptr;
    int semicolon_token = 0;

public:
    UsingNamespaceDirective *symbol = nullptr;

public:
    UsingDirectiveAST *asUsingDirective() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    UsingDirectiveAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT WhileStatementAST: public StatementAST
{
public:
    int while_token = 0;
    int lparen_token = 0;
    ExpressionAST *condition = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    WhileStatementAST *asWhileStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    WhileStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCClassForwardDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list = nullptr;
    int class_token = 0;
    NameListAST *identifier_list = nullptr;
    int semicolon_token = 0;

public: // annotations
    List<ObjCForwardClassDeclaration *> *symbols = nullptr;

public:
    ObjCClassForwardDeclarationAST *asObjCClassForwardDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCClassForwardDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCClassDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list = nullptr;
    int interface_token = 0;
    int implementation_token = 0;
    NameAST *class_name = nullptr;
    int lparen_token = 0;
    NameAST *category_name = nullptr;
    int rparen_token = 0;
    int colon_token = 0;
    NameAST *superclass = nullptr;
    ObjCProtocolRefsAST *protocol_refs = nullptr;
    ObjCInstanceVariablesDeclarationAST *inst_vars_decl = nullptr;
    DeclarationListAST *member_declaration_list = nullptr;
    int end_token = 0;

public: // annotations
    ObjCClass *symbol = nullptr;

public:
    ObjCClassDeclarationAST *asObjCClassDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCClassDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCProtocolForwardDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list = nullptr;
    int protocol_token = 0;
    NameListAST *identifier_list = nullptr;
    int semicolon_token = 0;

public: // annotations
    List<ObjCForwardProtocolDeclaration *> *symbols = nullptr;

public:
    ObjCProtocolForwardDeclarationAST *asObjCProtocolForwardDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCProtocolForwardDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCProtocolDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list = nullptr;
    int protocol_token = 0;
    NameAST *name = nullptr;
    ObjCProtocolRefsAST *protocol_refs = nullptr;
    DeclarationListAST *member_declaration_list = nullptr;
    int end_token = 0;

public: // annotations
    ObjCProtocol *symbol = nullptr;

public:
    ObjCProtocolDeclarationAST *asObjCProtocolDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCProtocolDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCProtocolRefsAST: public AST
{
public:
    int less_token = 0;
    NameListAST *identifier_list = nullptr;
    int greater_token = 0;

public:
    ObjCProtocolRefsAST *asObjCProtocolRefs() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCProtocolRefsAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCMessageArgumentAST: public AST
{
public:
    ExpressionAST *parameter_value_expression = nullptr;

public:
    ObjCMessageArgumentAST *asObjCMessageArgument() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCMessageArgumentAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCMessageExpressionAST: public ExpressionAST
{
public:
    int lbracket_token = 0;
    ExpressionAST *receiver_expression = nullptr;
    ObjCSelectorAST *selector = nullptr;
    ObjCMessageArgumentListAST *argument_list = nullptr;
    int rbracket_token = 0;

public:
    ObjCMessageExpressionAST *asObjCMessageExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCMessageExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCProtocolExpressionAST: public ExpressionAST
{
public:
    int protocol_token = 0;
    int lparen_token = 0;
    int identifier_token = 0;
    int rparen_token = 0;

public:
    ObjCProtocolExpressionAST *asObjCProtocolExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCProtocolExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCTypeNameAST: public AST
{
public:
    int lparen_token = 0;
    int type_qualifier_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;

public:
    ObjCTypeNameAST *asObjCTypeName() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCTypeNameAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCEncodeExpressionAST: public ExpressionAST
{
public:
    int encode_token = 0;
    ObjCTypeNameAST *type_name = nullptr;

public:
    ObjCEncodeExpressionAST *asObjCEncodeExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCEncodeExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCSelectorExpressionAST: public ExpressionAST
{
public:
    int selector_token = 0;
    int lparen_token = 0;
    ObjCSelectorAST *selector = nullptr;
    int rparen_token = 0;

public:
    ObjCSelectorExpressionAST *asObjCSelectorExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCSelectorExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCInstanceVariablesDeclarationAST: public AST
{
public:
    int lbrace_token = 0;
    DeclarationListAST *instance_variable_list = nullptr;
    int rbrace_token = 0;

public:
    ObjCInstanceVariablesDeclarationAST *asObjCInstanceVariablesDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCInstanceVariablesDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCVisibilityDeclarationAST: public DeclarationAST
{
public:
    int visibility_token = 0;

public:
    ObjCVisibilityDeclarationAST *asObjCVisibilityDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCVisibilityDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCPropertyAttributeAST: public AST
{
public:
    int attribute_identifier_token = 0;
    int equals_token = 0;
    ObjCSelectorAST *method_selector = nullptr;

public:
    ObjCPropertyAttributeAST *asObjCPropertyAttribute() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCPropertyAttributeAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCPropertyDeclarationAST: public DeclarationAST
{
public:
    SpecifierListAST *attribute_list = nullptr;
    int property_token = 0;
    int lparen_token = 0;
    ObjCPropertyAttributeListAST *property_attribute_list = nullptr;
    int rparen_token = 0;
    DeclarationAST *simple_declaration = nullptr;

public: // annotations
    List<ObjCPropertyDeclaration *> *symbols = nullptr;

public:
    ObjCPropertyDeclarationAST *asObjCPropertyDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCPropertyDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCMessageArgumentDeclarationAST: public AST
{
public:
    ObjCTypeNameAST *type_name = nullptr;
    SpecifierListAST *attribute_list = nullptr;
    NameAST *param_name = nullptr;

public: // annotations
    Argument *argument = nullptr;

public:
    ObjCMessageArgumentDeclarationAST *asObjCMessageArgumentDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCMessageArgumentDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCMethodPrototypeAST: public AST
{
public:
    int method_type_token = 0;
    ObjCTypeNameAST *type_name = nullptr;
    ObjCSelectorAST *selector = nullptr;
    ObjCMessageArgumentDeclarationListAST *argument_list = nullptr;
    int dot_dot_dot_token = 0;
    SpecifierListAST *attribute_list = nullptr;

public: // annotations
    ObjCMethod *symbol = nullptr;

public:
    ObjCMethodPrototypeAST *asObjCMethodPrototype() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCMethodPrototypeAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCMethodDeclarationAST: public DeclarationAST
{
public:
    ObjCMethodPrototypeAST *method_prototype = nullptr;
    StatementAST *function_body = nullptr;
    int semicolon_token = 0;

public:
    ObjCMethodDeclarationAST *asObjCMethodDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCMethodDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCSynthesizedPropertyAST: public AST
{
public:
    int property_identifier_token = 0;
    int equals_token = 0;
    int alias_identifier_token = 0;

public:
    ObjCSynthesizedPropertyAST *asObjCSynthesizedProperty() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCSynthesizedPropertyAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCSynthesizedPropertiesDeclarationAST: public DeclarationAST
{
public:
    int synthesized_token = 0;
    ObjCSynthesizedPropertyListAST *property_identifier_list = nullptr;
    int semicolon_token = 0;

public:
    ObjCSynthesizedPropertiesDeclarationAST *asObjCSynthesizedPropertiesDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCSynthesizedPropertiesDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCDynamicPropertiesDeclarationAST: public DeclarationAST
{
public:
    int dynamic_token = 0;
    NameListAST *property_identifier_list = nullptr;
    int semicolon_token = 0;

public:
    ObjCDynamicPropertiesDeclarationAST *asObjCDynamicPropertiesDeclaration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCDynamicPropertiesDeclarationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCFastEnumerationAST: public StatementAST
{
public:
    int for_token = 0;
    int lparen_token = 0;

    // declaration
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    // or an expression
    ExpressionAST *initializer = nullptr;

    int in_token = 0;
    ExpressionAST *fast_enumeratable_expression = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
    ObjCFastEnumerationAST *asObjCFastEnumeration() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCFastEnumerationAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CPLUSPLUS_EXPORT ObjCSynchronizedStatementAST: public StatementAST
{
public:
    int synchronized_token = 0;
    int lparen_token = 0;
    ExpressionAST *synchronized_object = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public:
    ObjCSynchronizedStatementAST *asObjCSynchronizedStatement() override { return this; }

    int firstToken() const override;
    int lastToken() const override;

    ObjCSynchronizedStatementAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};


class LambdaExpressionAST: public ExpressionAST
{
public:
    LambdaIntroducerAST *lambda_introducer = nullptr;
    DeclarationListAST *templateParameters = nullptr;
    RequiresClauseAST *requiresClause = nullptr;
    SpecifierListAST *attributes = nullptr;
    LambdaDeclaratorAST *lambda_declarator = nullptr;
    StatementAST *statement = nullptr;

public:
    LambdaExpressionAST *asLambdaExpression() override { return this; }

    int firstToken() const override;
    int lastToken() const override;
    LambdaExpressionAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class LambdaIntroducerAST: public AST
{
public:
    int lbracket_token = 0;
    LambdaCaptureAST *lambda_capture = nullptr;
    int rbracket_token = 0;

public:
    LambdaIntroducerAST *asLambdaIntroducer() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    LambdaIntroducerAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class LambdaCaptureAST: public AST
{
public:
    int default_capture_token = 0;
    CaptureListAST *capture_list = nullptr;

public:
    LambdaCaptureAST *asLambdaCapture() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    LambdaCaptureAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class CaptureAST: public AST
{
public:
    int amper_token = 0;
    NameAST *identifier = nullptr;

public:
    CaptureAST *asCapture() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    CaptureAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class LambdaDeclaratorAST: public AST
{
public:
    int lparen_token = 0;
    ParameterDeclarationClauseAST *parameter_declaration_clause = nullptr;
    int rparen_token = 0;
    SpecifierListAST *attributes = nullptr;
    int mutable_token = 0;
    ExceptionSpecificationAST *exception_specification = nullptr;
    TrailingReturnTypeAST *trailing_return_type = nullptr;
    RequiresClauseAST *requiresClause = nullptr;

public: // annotations
    Function *symbol = nullptr;

public:
    LambdaDeclaratorAST *asLambdaDeclarator() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    LambdaDeclaratorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class TrailingReturnTypeAST: public AST
{
public:
    int arrow_token = 0;
    SpecifierListAST *attributes = nullptr;
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;

public:
    TrailingReturnTypeAST *asTrailingReturnType() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    TrailingReturnTypeAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class BracedInitializerAST: public ExpressionAST
{
public:
    int lbrace_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int comma_token = 0;
    int rbrace_token = 0;

public:
    BracedInitializerAST *asBracedInitializer() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    BracedInitializerAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class DesignatorAST: public AST
{
public:
    DesignatorAST *asDesignator() override { return this; }
    DesignatorAST *clone(MemoryPool *pool) const override = 0;
};

class DotDesignatorAST: public DesignatorAST
{
public:
    int dot_token = 0;
    int identifier_token = 0;

public:
    DotDesignatorAST *asDotDesignator() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    DotDesignatorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class BracketDesignatorAST: public DesignatorAST
{
public:
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
    BracketDesignatorAST *asBracketDesignator() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    BracketDesignatorAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

class DesignatedInitializerAST: public ExpressionAST
{
public:
    DesignatorListAST *designator_list = nullptr;
    int equal_token = 0;
    ExpressionAST *initializer = nullptr;

public:
    DesignatedInitializerAST *asDesignatedInitializer() override { return this; }
    int firstToken() const override;
    int lastToken() const override;

    DesignatedInitializerAST *clone(MemoryPool *pool) const override;

protected:
    void accept0(ASTVisitor *visitor) override;
    bool match0(AST *, ASTMatcher *) override;
};

} // namespace CPlusPlus
