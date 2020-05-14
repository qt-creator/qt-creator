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
    virtual StatementAST *asStatement() { return this; }

    virtual StatementAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT ExpressionAST: public AST
{
public:
    virtual ExpressionAST *asExpression() { return this; }

    virtual ExpressionAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT DeclarationAST: public AST
{
public:
    virtual DeclarationAST *asDeclaration() { return this; }

    virtual DeclarationAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT NameAST: public AST
{
public: // annotations
    const Name *name = nullptr;

public:
    virtual NameAST *asName() { return this; }

    virtual NameAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT SpecifierAST: public AST
{
public:
    virtual SpecifierAST *asSpecifier() { return this; }

    virtual SpecifierAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PtrOperatorAST: public AST
{
public:
    virtual PtrOperatorAST *asPtrOperator() { return this; }

    virtual PtrOperatorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PostfixAST: public ExpressionAST
{
public:
    virtual PostfixAST *asPostfix() { return this; }

    virtual PostfixAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT CoreDeclaratorAST: public AST
{
public:
    virtual CoreDeclaratorAST *asCoreDeclarator() { return this; }

    virtual CoreDeclaratorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT PostfixDeclaratorAST: public AST
{
public:
    virtual PostfixDeclaratorAST *asPostfixDeclarator() { return this; }

    virtual PostfixDeclaratorAST *clone(MemoryPool *pool) const = 0;
};

class CPLUSPLUS_EXPORT ObjCSelectorArgumentAST: public AST
{
public:
    int name_token = 0;
    int colon_token = 0;

public:
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
    ObjCSelectorArgumentListAST *selector_argument_list = nullptr;

public:
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
    int specifier_token = 0;

public:
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
    virtual AttributeSpecifierAST *asAttributeSpecifier() { return this; }

    virtual AttributeSpecifierAST *clone(MemoryPool *pool) const = 0;
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
    int attribute_token = 0;
    int first_lparen_token = 0;
    int second_lparen_token = 0;
    GnuAttributeListAST *attribute_list = nullptr;
    int first_rparen_token = 0;
    int second_rparen_token = 0;

public:
    virtual GnuAttributeSpecifierAST *asGnuAttributeSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual GnuAttributeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

class CPLUSPLUS_EXPORT MsvcDeclspecSpecifierAST: public AttributeSpecifierAST
{
public:
    int attribute_token = 0;
    int lparen_token = 0;
    GnuAttributeListAST *attribute_list = nullptr;
    int rparen_token = 0;

public:
    virtual MsvcDeclspecSpecifierAST *asMsvcDeclspecSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual MsvcDeclspecSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
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
    virtual StdAttributeSpecifierAST *asStdAttributeSpecifier() { return this; }

    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual StdAttributeSpecifierAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
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
    int typeof_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
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
    int decltype_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
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
    SpecifierListAST *attribute_list = nullptr;
    PtrOperatorListAST *ptr_operator_list = nullptr;
    CoreDeclaratorAST *core_declarator = nullptr;
    PostfixDeclaratorListAST *postfix_declarator_list = nullptr;
    SpecifierListAST *post_attribute_list = nullptr;
    int equal_token = 0;
    ExpressionAST *initializer = nullptr;

public:
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
    int qt_invokable_token = 0;
    SpecifierListAST *decl_specifier_list = nullptr;
    DeclaratorListAST *declarator_list = nullptr;
    int semicolon_token = 0;

public:
    List<Symbol *> *symbols = nullptr;

public:
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
    int semicolon_token = 0;

public:
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
    int access_specifier_token = 0;
    int slots_token = 0;
    int colon_token = 0;

public:
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
    int q_object_token = 0;

public:
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
    int item_name_token = 0;
    ExpressionAST *expression = nullptr;

public:
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
    int property_specifier_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr; // for Q_PRIVATE_PROPERTY(expression, ...)
    int comma_token = 0;
    ExpressionAST *type_id = nullptr;
    NameAST *property_name = nullptr;
    QtPropertyDeclarationItemListAST *property_declaration_item_list = nullptr;
    int rparen_token = 0;

public:
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
    int enum_specifier_token = 0;
    int lparen_token = 0;
    NameListAST *enumerator_list = nullptr;
    int rparen_token = 0;

public:
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
    int flags_specifier_token = 0;
    int lparen_token = 0;
    NameListAST *flag_enums_list = nullptr;
    int rparen_token = 0;

public:
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
    NameAST *interface_name = nullptr;
    NameListAST *constraint_list = nullptr;

public:
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
    int interfaces_token = 0;
    int lparen_token = 0;
    QtInterfaceNameListAST *interface_name_list = nullptr;
    int rparen_token = 0;

public:
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
    int asm_token = 0;
    int volatile_token = 0;
    int lparen_token = 0;
    // ### string literals
    // ### asm operand list
    int rparen_token = 0;
    int semicolon_token = 0;

public:
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
    int virtual_token = 0;
    int access_specifier_token = 0;
    NameAST *name = nullptr;
    int ellipsis_token = 0;

public: // annotations
    BaseClass *symbol = nullptr;

public:
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
    NameAST *name = nullptr;

public:
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
    int lparen_token = 0;
    CompoundStatementAST *statement = nullptr;
    int rparen_token = 0;

public:
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
    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;
    ExpressionAST *initializer = nullptr;

public:
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
    int method_token = 0;
    int lparen_token = 0;
    DeclaratorAST *declarator = nullptr;
    int rparen_token = 0;

public:
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
    int q_token = 0;
    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;

public:
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
    ExpressionAST *left_expression = nullptr;
    int binary_op_token = 0;
    ExpressionAST *right_expression = nullptr;

public:
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
    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;
    ExpressionAST *expression = nullptr;

public:
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
    int case_token = 0;
    ExpressionAST *expression = nullptr;
    int colon_token = 0;
    StatementAST *statement = nullptr;

public:
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
    int lbrace_token = 0;
    StatementListAST *statement_list = nullptr;
    int rbrace_token = 0;

public: // annotations
    Block *symbol = nullptr;

public:
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
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;

public:
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
    ExpressionAST *condition = nullptr;
    int question_token = 0;
    ExpressionAST *left_expression = nullptr;
    int colon_token = 0;
    ExpressionAST *right_expression = nullptr;

public:
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
    int cast_token = 0;
    int less_token = 0;
    ExpressionAST *type_id = nullptr;
    int greater_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
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
    int colon_token = 0;
    MemInitializerListAST *member_initializer_list = nullptr;
    int dot_dot_dot_token = 0;

public:
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
    DeclarationAST *declaration = nullptr;

public:
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
    int dot_dot_dot_token = 0;
    NameAST *name = nullptr;

public:
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
    int lparen_token = 0;
    DeclaratorAST *declarator = nullptr;
    int rparen_token = 0;

public:
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
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
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
    int scope_token = 0;
    int delete_token = 0;
    int lbracket_token = 0;
    int rbracket_token = 0;
    ExpressionAST *expression = nullptr;

public:
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
    int do_token = 0;
    StatementAST *statement = nullptr;
    int while_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;
    int semicolon_token = 0;

public:
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
    NameAST *name = nullptr;

public:
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
    int classkey_token = 0;
    SpecifierListAST *attribute_list = nullptr;
    NameAST *name = nullptr;

public:
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
    int identifier_token = 0;
    int equal_token = 0;
    ExpressionAST *expression = nullptr;

public:
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
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    int dot_dot_dot_token = 0;

public:
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
    virtual ExceptionSpecificationAST *asExceptionSpecification() { return this; }

    virtual ExceptionSpecificationAST *clone(MemoryPool *pool) const = 0;
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
    int noexcept_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
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
    ExpressionStatementAST *expression = nullptr;
    DeclarationStatementAST *declaration = nullptr;

public:
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
    ExpressionAST *expression = nullptr;
    int semicolon_token = 0;

public:
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
    int qt_invokable_token = 0;
    SpecifierListAST *decl_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    CtorInitializerAST *ctor_initializer = nullptr;
    StatementAST *function_body = nullptr;

public: // annotations
    Function *symbol = nullptr;

public:
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
    int if_token = 0;
    int constexpr_token = 0;
    int lparen_token = 0;
    ExpressionAST *condition = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;
    int else_token = 0;
    StatementAST *else_statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
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
    int lbrace_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int rbrace_token = 0;

public:
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
    int label_token = 0;
    int colon_token = 0;
    StatementAST *statement = nullptr;

public:
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
    int lbrace_token = 0;
    DeclarationListAST *declaration_list = nullptr;
    int rbrace_token = 0;

public:
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
    int extern_token = 0;
    int extern_type_token = 0;
    DeclarationAST *declaration = nullptr;

public:
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
    NameAST *name = nullptr;
    // either a BracedInitializerAST or a ExpressionListParenAST
    ExpressionAST *expression = nullptr;

public:
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
    NameAST *class_or_namespace_name = nullptr;
    int scope_token = 0;

public:
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
    int global_scope_token = 0;
    NestedNameSpecifierListAST *nested_name_specifier_list = nullptr;
    NameAST *unqualified_name = nullptr;

public:
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
    int operator_token = 0;
    OperatorAST *op = nullptr;

public:
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
    int operator_token = 0;
    SpecifierListAST *type_specifier_list = nullptr;
    PtrOperatorListAST *ptr_operator_list = nullptr;

public:
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
    int class_token = 0;

public:
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
    int identifier_token = 0;

public:
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
    int tilde_token = 0;
    NameAST *unqualified_name = nullptr;

public:
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
    int template_token = 0;
    int identifier_token = 0;
    int less_token = 0;
    ExpressionListAST *template_argument_list = nullptr;
    int greater_token = 0;

public:
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
    int inline_token = 0;
    int namespace_token = 0;
    int identifier_token = 0;
    SpecifierListAST *attribute_list = nullptr;
    DeclarationAST *linkage_body = nullptr;

public: // annotations
    Namespace *symbol = nullptr;

public:
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
    int namespace_token = 0;
    int namespace_name_token = 0;
    int equal_token = 0;
    NameAST *name = nullptr;
    int semicolon_token = 0;

public:
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
    int using_token = 0;
    NameAST *name = nullptr;
    int equal_token = 0;
    TypeIdAST *typeId = nullptr;
    int semicolon_token = 0;

public: // annotations
    Declaration *symbol = nullptr;

public:
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
    int lparen_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int rparen_token = 0;

public:
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
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
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
    int scope_token = 0;
    int new_token = 0;
    ExpressionListParenAST *new_placement = nullptr;

    int lparen_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;

    NewTypeIdAST *new_type_id = nullptr;

    ExpressionAST *new_initializer = nullptr; // either ExpressionListParenAST or BracedInitializerAST

public:
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
    SpecifierListAST *type_specifier_list = nullptr;
    PtrOperatorListAST *ptr_operator_list = nullptr;
    NewArrayDeclaratorListAST *new_array_declarator_list = nullptr;

public:
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
    int op_token = 0;
    int open_token = 0;
    int close_token = 0;

public:
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
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;
    int equal_token = 0;
    ExpressionAST *expression = nullptr;

public: // annotations
    Argument *symbol = nullptr;

public:
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
    ParameterDeclarationListAST *parameter_declaration_list = nullptr;
    int dot_dot_dot_token = 0;

public:
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
    ExpressionAST *base_expression = nullptr;
    int lparen_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int rparen_token = 0;

public:
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
    ExpressionAST *base_expression = nullptr;
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
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
    ExpressionAST *base_expression = nullptr;
    int incr_decr_token = 0;

public:
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
    ExpressionAST *base_expression = nullptr;
    int access_token = 0;
    int template_token = 0;
    NameAST *member_name = nullptr;

public:
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
    int typeid_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
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
    int typename_token = 0;
    NameAST *name = nullptr;
    ExpressionAST *expression = nullptr; // either ExpressionListParenAST or BracedInitializerAST

public:
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
    SpecifierListAST *type_specifier_list = nullptr;
    ExpressionAST *expression = nullptr; // either ExpressionListParenAST or BracedInitializerAST

public:
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
    int global_scope_token = 0;
    NestedNameSpecifierListAST *nested_name_specifier_list = nullptr;
    int star_token = 0;
    SpecifierListAST *cv_qualifier_list = nullptr;
    int ref_qualifier_token = 0;

public:
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
    int star_token = 0;
    SpecifierListAST *cv_qualifier_list = nullptr;

public:
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
    int reference_token = 0;

public:
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
    int break_token = 0;
    int semicolon_token = 0;

public:
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
    int continue_token = 0;
    int semicolon_token = 0;

public:
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
    int goto_token = 0;
    int identifier_token = 0;
    int semicolon_token = 0;

public:
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
    int return_token = 0;
    ExpressionAST *expression = nullptr;
    int semicolon_token = 0;

public:
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
    int sizeof_token = 0;
    int dot_dot_dot_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
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
    int alignof_token = 0;
    int lparen_token = 0;
    TypeIdAST *typeId = nullptr;
    int rparen_token = 0;

public:
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
    int literal_token = 0;

public:
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
    int literal_token = 0;

public:
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
    int literal_token = 0;

public:
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
    int this_token = 0;

public:
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
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int rparen_token = 0;

public:
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
    int static_assert_token = 0;
    int lparen_token = 0;
    ExpressionAST *expression = nullptr;
    int comma_token = 0;
    ExpressionAST *string_literal = nullptr;
    int rparen_token = 0;
    int semicolon_token = 0;

public:
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
    int literal_token = 0;
    StringLiteralAST *next = nullptr;

public:
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
    int switch_token = 0;
    int lparen_token = 0;
    ExpressionAST *condition = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
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
    int export_token = 0;
    int template_token = 0;
    int less_token = 0;
    DeclarationListAST *template_parameter_list = nullptr;
    int greater_token = 0;
    DeclarationAST *declaration = nullptr;

public: // annotations
    Template *symbol = nullptr;

public:
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
    int throw_token = 0;
    ExpressionAST *expression = nullptr;

public:
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
    int noexcept_token = 0;
    ExpressionAST *expression = nullptr;

public:
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
    DeclarationListAST *declaration_list = nullptr;

public:
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
    int try_token = 0;
    StatementAST *statement = nullptr;
    CatchClauseListAST *catch_clause_list = nullptr;

public:
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
    int catch_token = 0;
    int lparen_token = 0;
    ExceptionDeclarationAST *exception_declaration = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
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
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;

public:
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
    int classkey_token = 0;
    int dot_dot_dot_token = 0;
    NameAST *name = nullptr;
    int equal_token = 0;
    ExpressionAST *type_id = nullptr;

public: // annotations
    TypenameArgument *symbol = nullptr;

public:
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
    int template_token = 0;
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
    int unary_op_token = 0;
    ExpressionAST *expression = nullptr;

public:
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
    int using_token = 0;
    int typename_token = 0;
    NameAST *name = nullptr;
    int semicolon_token = 0;

public: // annotations
    UsingDeclaration *symbol = nullptr;

public:
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
    int using_token = 0;
    int namespace_token = 0;
    NameAST *name = nullptr;
    int semicolon_token = 0;

public:
    UsingNamespaceDirective *symbol = nullptr;

public:
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
    int while_token = 0;
    int lparen_token = 0;
    ExpressionAST *condition = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public: // annotations
    Block *symbol = nullptr;

public:
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
    SpecifierListAST *attribute_list = nullptr;
    int class_token = 0;
    NameListAST *identifier_list = nullptr;
    int semicolon_token = 0;

public: // annotations
    List<ObjCForwardClassDeclaration *> *symbols = nullptr;

public:
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
    SpecifierListAST *attribute_list = nullptr;
    int protocol_token = 0;
    NameListAST *identifier_list = nullptr;
    int semicolon_token = 0;

public: // annotations
    List<ObjCForwardProtocolDeclaration *> *symbols = nullptr;

public:
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
    SpecifierListAST *attribute_list = nullptr;
    int protocol_token = 0;
    NameAST *name = nullptr;
    ObjCProtocolRefsAST *protocol_refs = nullptr;
    DeclarationListAST *member_declaration_list = nullptr;
    int end_token = 0;

public: // annotations
    ObjCProtocol *symbol = nullptr;

public:
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
    int less_token = 0;
    NameListAST *identifier_list = nullptr;
    int greater_token = 0;

public:
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
    ExpressionAST *parameter_value_expression = nullptr;

public:
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
    int lbracket_token = 0;
    ExpressionAST *receiver_expression = nullptr;
    ObjCSelectorAST *selector = nullptr;
    ObjCMessageArgumentListAST *argument_list = nullptr;
    int rbracket_token = 0;

public:
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
    int protocol_token = 0;
    int lparen_token = 0;
    int identifier_token = 0;
    int rparen_token = 0;

public:
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
    int lparen_token = 0;
    int type_qualifier_token = 0;
    ExpressionAST *type_id = nullptr;
    int rparen_token = 0;

public:
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
    int encode_token = 0;
    ObjCTypeNameAST *type_name = nullptr;

public:
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
    int selector_token = 0;
    int lparen_token = 0;
    ObjCSelectorAST *selector = nullptr;
    int rparen_token = 0;

public:
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
    int lbrace_token = 0;
    DeclarationListAST *instance_variable_list = nullptr;
    int rbrace_token = 0;

public:
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
    int visibility_token = 0;

public:
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
    int attribute_identifier_token = 0;
    int equals_token = 0;
    ObjCSelectorAST *method_selector = nullptr;

public:
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
    SpecifierListAST *attribute_list = nullptr;
    int property_token = 0;
    int lparen_token = 0;
    ObjCPropertyAttributeListAST *property_attribute_list = nullptr;
    int rparen_token = 0;
    DeclarationAST *simple_declaration = nullptr;

public: // annotations
    List<ObjCPropertyDeclaration *> *symbols = nullptr;

public:
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
    ObjCTypeNameAST *type_name = nullptr;
    SpecifierListAST *attribute_list = nullptr;
    NameAST *param_name = nullptr;

public: // annotations
    Argument *argument = nullptr;

public:
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
    int method_type_token = 0;
    ObjCTypeNameAST *type_name = nullptr;
    ObjCSelectorAST *selector = nullptr;
    ObjCMessageArgumentDeclarationListAST *argument_list = nullptr;
    int dot_dot_dot_token = 0;
    SpecifierListAST *attribute_list = nullptr;

public: // annotations
    ObjCMethod *symbol = nullptr;

public:
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
    ObjCMethodPrototypeAST *method_prototype = nullptr;
    StatementAST *function_body = nullptr;
    int semicolon_token = 0;

public:
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
    int property_identifier_token = 0;
    int equals_token = 0;
    int alias_identifier_token = 0;

public:
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
    int synthesized_token = 0;
    ObjCSynthesizedPropertyListAST *property_identifier_list = nullptr;
    int semicolon_token = 0;

public:
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
    int dynamic_token = 0;
    NameListAST *property_identifier_list = nullptr;
    int semicolon_token = 0;

public:
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
    int synchronized_token = 0;
    int lparen_token = 0;
    ExpressionAST *synchronized_object = nullptr;
    int rparen_token = 0;
    StatementAST *statement = nullptr;

public:
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
    LambdaIntroducerAST *lambda_introducer = nullptr;
    LambdaDeclaratorAST *lambda_declarator = nullptr;
    StatementAST *statement = nullptr;

public:
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
    int lbracket_token = 0;
    LambdaCaptureAST *lambda_capture = nullptr;
    int rbracket_token = 0;

public:
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
    int default_capture_token = 0;
    CaptureListAST *capture_list = nullptr;

public:
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
    int amper_token = 0;
    NameAST *identifier = nullptr;

public:
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
    int lparen_token = 0;
    ParameterDeclarationClauseAST *parameter_declaration_clause = nullptr;
    int rparen_token = 0;
    SpecifierListAST *attributes = nullptr;
    int mutable_token = 0;
    ExceptionSpecificationAST *exception_specification = nullptr;
    TrailingReturnTypeAST *trailing_return_type = nullptr;

public: // annotations
    Function *symbol = nullptr;

public:
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
    int arrow_token = 0;
    SpecifierListAST *attributes = nullptr;
    SpecifierListAST *type_specifier_list = nullptr;
    DeclaratorAST *declarator = nullptr;

public:
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
    int lbrace_token = 0;
    ExpressionListAST *expression_list = nullptr;
    int comma_token = 0;
    int rbrace_token = 0;

public:
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
    virtual DesignatorAST *asDesignator() { return this; }
    virtual DesignatorAST *clone(MemoryPool *pool) const = 0;
};

class DotDesignatorAST: public DesignatorAST
{
public:
    int dot_token = 0;
    int identifier_token = 0;

public:
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
    int lbracket_token = 0;
    ExpressionAST *expression = nullptr;
    int rbracket_token = 0;

public:
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
    DesignatorListAST *designator_list = nullptr;
    int equal_token = 0;
    ExpressionAST *initializer = nullptr;

public:
    virtual DesignatedInitializerAST *asDesignatedInitializer() { return this; }
    virtual int firstToken() const;
    virtual int lastToken() const;

    virtual DesignatedInitializerAST *clone(MemoryPool *pool) const;

protected:
    virtual void accept0(ASTVisitor *visitor);
    virtual bool match0(AST *, ASTMatcher *);
};

} // namespace CPlusPlus
