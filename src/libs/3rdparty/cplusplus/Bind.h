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

#include "ASTVisitor.h"
#include "FullySpecifiedType.h"
#include "Names.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Bind: protected ASTVisitor
{
public:
    Bind(TranslationUnit *unit);

    void operator()(TranslationUnitAST *ast, Namespace *globalNamespace);
    void operator()(DeclarationAST *ast, Scope *scope);
    void operator()(StatementAST *ast, Scope *scope);
    FullySpecifiedType operator()(ExpressionAST *ast, Scope *scope);
    FullySpecifiedType operator()(NewTypeIdAST *ast, Scope *scope);

    bool skipFunctionBodies() const;
    void setSkipFunctionBodies(bool skipFunctionBodies);

protected:
    using ASTVisitor::translationUnit;

    int location(DeclaratorAST *ast, int defaultLocation) const;
    int location(CoreDeclaratorAST *ast, int defaultLocation) const;
    int location(NameAST *name, int defaultLocation) const;

    static int visibilityForAccessSpecifier(int tokenKind);
    static int visibilityForClassKey(int tokenKind);
    static int visibilityForObjCAccessSpecifier(int tokenKind);
    static bool isObjCClassMethod(int tokenKind);

    void setDeclSpecifiers(Symbol *symbol, const FullySpecifiedType &declSpecifiers);

    typedef FullySpecifiedType ExpressionTy;
    ExpressionTy expression(ExpressionAST *ast);

    const Name *name(NameAST *ast);

    void statement(StatementAST *ast);
    void declaration(DeclarationAST *ast);

    FullySpecifiedType specifier(SpecifierAST *ast, const FullySpecifiedType &init);
    FullySpecifiedType ptrOperator(PtrOperatorAST *ast, const FullySpecifiedType &init);
    FullySpecifiedType coreDeclarator(CoreDeclaratorAST *ast, const FullySpecifiedType &init);
    FullySpecifiedType postfixDeclarator(PostfixDeclaratorAST *ast, const FullySpecifiedType &init);

    Scope *switchScope(Scope *scope);
    int switchVisibility(int visibility);
    int switchMethodKey(int methodKey);
    int switchObjCVisibility(int visibility);

    int calculateScopeStart(ObjCClassDeclarationAST *ast) const;
    int calculateScopeStart(ObjCProtocolDeclarationAST *ast) const;

    const Name *objCSelectorArgument(ObjCSelectorArgumentAST *ast, bool *hasArg);
    void attribute(GnuAttributeAST *ast);
    FullySpecifiedType declarator(DeclaratorAST *ast, const FullySpecifiedType &init,
                                  DeclaratorIdAST **declaratorId,
                                  DecompositionDeclaratorAST **decompDeclarator = nullptr);
    void qtInterfaceName(QtInterfaceNameAST *ast);
    void baseSpecifier(BaseSpecifierAST *ast, int colon_token, Class *klass);
    void ctorInitializer(CtorInitializerAST *ast, Function *fun);
    void enumerator(EnumeratorAST *ast, Enum *symbol);
    FullySpecifiedType exceptionSpecification(ExceptionSpecificationAST *ast, const FullySpecifiedType &init);
    void memInitializer(MemInitializerAST *ast, Function *fun);
    const Name *nestedNameSpecifier(NestedNameSpecifierAST *ast);
    void newPlacement(ExpressionListParenAST *ast);
    FullySpecifiedType newArrayDeclarator(NewArrayDeclaratorAST *ast, const FullySpecifiedType &init);
    FullySpecifiedType newTypeId(NewTypeIdAST *ast);
    OperatorNameId::Kind cppOperator(OperatorAST *ast);
    void parameterDeclarationClause(ParameterDeclarationClauseAST *ast, int lparen_token, Function *fun);
    void translationUnit(TranslationUnitAST *ast);
    void objCProtocolRefs(ObjCProtocolRefsAST *ast, Symbol *objcClassOrProtocol);
    void objCMessageArgument(ObjCMessageArgumentAST *ast);
    FullySpecifiedType objCTypeName(ObjCTypeNameAST *ast);
    void objCInstanceVariablesDeclaration(ObjCInstanceVariablesDeclarationAST *ast, ObjCClass *klass);
    void objCPropertyAttribute(ObjCPropertyAttributeAST *ast);
    void objCMessageArgumentDeclaration(ObjCMessageArgumentDeclarationAST *ast, ObjCMethod *method);
    ObjCMethod *objCMethodPrototype(ObjCMethodPrototypeAST *ast);
    void objCSynthesizedProperty(ObjCSynthesizedPropertyAST *ast);
    void lambdaIntroducer(LambdaIntroducerAST *ast);
    void lambdaCapture(LambdaCaptureAST *ast);
    void capture(CaptureAST *ast);
    Function *lambdaDeclarator(LambdaDeclaratorAST *ast);
    FullySpecifiedType trailingReturnType(TrailingReturnTypeAST *ast, const FullySpecifiedType &init);
    const StringLiteral *asStringLiteral(const AST *ast);

    bool preVisit(AST *) override;
    void postVisit(AST *) override;

    // AST
    bool visit(ObjCSelectorArgumentAST *ast) override;
    bool visit(GnuAttributeAST *ast) override;
    bool visit(DeclaratorAST *ast) override;
    bool visit(QtPropertyDeclarationItemAST *ast) override;
    bool visit(QtInterfaceNameAST *ast) override;
    bool visit(BaseSpecifierAST *ast) override;
    bool visit(CtorInitializerAST *ast) override;
    bool visit(EnumeratorAST *ast) override;
    bool visit(DynamicExceptionSpecificationAST *ast) override;
    bool visit(MemInitializerAST *ast) override;
    bool visit(NestedNameSpecifierAST *ast) override;
    bool visit(NoExceptSpecificationAST *) override { return true; }
    bool visit(NewArrayDeclaratorAST *ast) override;
    bool visit(NewTypeIdAST *ast) override;
    bool visit(OperatorAST *ast) override;
    bool visit(ParameterDeclarationClauseAST *ast) override;
    bool visit(RequiresExpressionAST *ast) override;
    bool visit(TranslationUnitAST *ast) override;
    bool visit(ObjCProtocolRefsAST *ast) override;
    bool visit(ObjCMessageArgumentAST *ast) override;
    bool visit(ObjCTypeNameAST *ast) override;
    bool visit(ObjCInstanceVariablesDeclarationAST *ast) override;
    bool visit(ObjCPropertyAttributeAST *ast) override;
    bool visit(ObjCMessageArgumentDeclarationAST *ast) override;
    bool visit(ObjCMethodPrototypeAST *ast) override;
    bool visit(ObjCSynthesizedPropertyAST *ast) override;
    bool visit(LambdaIntroducerAST *ast) override;
    bool visit(LambdaCaptureAST *ast) override;
    bool visit(CaptureAST *ast) override;
    bool visit(LambdaDeclaratorAST *ast) override;
    bool visit(TrailingReturnTypeAST *ast) override;
    bool visit(DotDesignatorAST *) override { return true; }
    bool visit(BracketDesignatorAST *) override { return true; }

    // StatementAST
    bool visit(QtMemberDeclarationAST *ast) override;
    bool visit(CaseStatementAST *ast) override;
    bool visit(CompoundStatementAST *ast) override;
    bool visit(DeclarationStatementAST *ast) override;
    bool visit(DoStatementAST *ast) override;
    bool visit(ExpressionOrDeclarationStatementAST *ast) override;
    bool visit(ExpressionStatementAST *ast) override;
    bool visit(ForeachStatementAST *ast) override;
    bool visit(RangeBasedForStatementAST *ast) override;
    bool visit(ForStatementAST *ast) override;
    bool visit(IfStatementAST *ast) override;
    bool visit(LabeledStatementAST *ast) override;
    bool visit(BreakStatementAST *ast) override;
    bool visit(ContinueStatementAST *ast) override;
    bool visit(GotoStatementAST *ast) override;
    bool visit(ReturnStatementAST *ast) override;
    bool visit(SwitchStatementAST *ast) override;
    bool visit(TryBlockStatementAST *ast) override;
    bool visit(CatchClauseAST *ast) override;
    bool visit(WhileStatementAST *ast) override;
    bool visit(ObjCFastEnumerationAST *ast) override;
    bool visit(ObjCSynchronizedStatementAST *ast) override;

    // ExpressionAST
    bool visit(AlignofExpressionAST *) override { return true; }
    bool visit(IdExpressionAST *ast) override;
    bool visit(CompoundExpressionAST *ast) override;
    bool visit(CompoundLiteralAST *ast) override;
    bool visit(QtMethodAST *ast) override;
    bool visit(BinaryExpressionAST *ast) override;
    bool visit(CastExpressionAST *ast) override;
    bool visit(ConditionAST *ast) override;
    bool visit(ConditionalExpressionAST *ast) override;
    bool visit(CppCastExpressionAST *ast) override;
    bool visit(DeleteExpressionAST *ast) override;
    bool visit(DesignatedInitializerAST *) override { return true; }
    bool visit(ArrayInitializerAST *ast) override;
    bool visit(NewExpressionAST *ast) override;
    bool visit(NoExceptOperatorExpressionAST *) override { return true; }
    bool visit(TypeidExpressionAST *ast) override;
    bool visit(TypenameCallExpressionAST *ast) override;
    bool visit(TypeConstructorCallAST *ast) override;
    bool visit(SizeofExpressionAST *ast) override;
    bool visit(StaticAssertDeclarationAST *) override { return true; }
    bool visit(PointerLiteralAST *ast) override;
    bool visit(NumericLiteralAST *ast) override;
    bool visit(BoolLiteralAST *ast) override;
    bool visit(ThisExpressionAST *ast) override;
    bool visit(NestedExpressionAST *ast) override;
    bool visit(StringLiteralAST *ast) override;
    bool visit(ThrowExpressionAST *ast) override;
    bool visit(TypeIdAST *ast) override;
    bool visit(UnaryExpressionAST *ast) override;
    bool visit(ObjCMessageExpressionAST *ast) override;
    bool visit(ObjCProtocolExpressionAST *ast) override;
    bool visit(ObjCEncodeExpressionAST *ast) override;
    bool visit(ObjCSelectorExpressionAST *ast) override;
    bool visit(LambdaExpressionAST *ast) override;
    bool visit(BracedInitializerAST *ast) override;
    bool visit(ExpressionListParenAST *ast) override;

    // DeclarationAST
    bool visit(SimpleDeclarationAST *ast) override;
    bool visit(EmptyDeclarationAST *ast) override;
    bool visit(AccessDeclarationAST *ast) override;
    bool visit(QtObjectTagAST *ast) override;
    bool visit(QtPrivateSlotAST *ast) override;
    bool visit(QtPropertyDeclarationAST *ast) override;
    bool visit(QtEnumDeclarationAST *ast) override;
    bool visit(QtFlagsDeclarationAST *ast) override;
    bool visit(QtInterfacesDeclarationAST *ast) override;
    bool visit(AliasDeclarationAST *ast) override;
    bool visit(AsmDefinitionAST *ast) override;
    bool visit(ExceptionDeclarationAST *ast) override;
    bool visit(FunctionDefinitionAST *ast) override;
    bool visit(LinkageBodyAST *ast) override;
    bool visit(LinkageSpecificationAST *ast) override;
    bool visit(NamespaceAST *ast) override;
    bool visit(NamespaceAliasDefinitionAST *ast) override;
    bool visit(ParameterDeclarationAST *ast) override;
    bool visit(TemplateDeclarationAST *ast) override;
    bool visit(TypenameTypeParameterAST *ast) override;
    bool visit(TemplateTypeParameterAST *ast) override;
    bool visit(TypeConstraintAST *ast) override;
    bool visit(UsingAST *ast) override;
    bool visit(UsingDirectiveAST *ast) override;
    bool visit(ObjCClassForwardDeclarationAST *ast) override;
    bool visit(ObjCClassDeclarationAST *ast) override;
    bool visit(ObjCProtocolForwardDeclarationAST *ast) override;
    bool visit(ObjCProtocolDeclarationAST *ast) override;
    bool visit(ObjCVisibilityDeclarationAST *ast) override;
    bool visit(ObjCPropertyDeclarationAST *ast) override;
    bool visit(ObjCMethodDeclarationAST *ast) override;
    bool visit(ObjCSynthesizedPropertiesDeclarationAST *ast) override;
    bool visit(ObjCDynamicPropertiesDeclarationAST *ast) override;

    // NameAST
    bool visit(ObjCSelectorAST *ast) override;
    bool visit(QualifiedNameAST *ast) override;
    bool visit(OperatorFunctionIdAST *ast) override;
    bool visit(ConversionFunctionIdAST *ast) override;
    bool visit(AnonymousNameAST *ast) override;
    bool visit(SimpleNameAST *ast) override;
    bool visit(DestructorNameAST *ast) override;
    bool visit(TemplateIdAST *ast) override;

    // SpecifierAST
    bool visit(SimpleSpecifierAST *ast) override;
    bool visit(AlignmentSpecifierAST *ast) override;
    bool visit(GnuAttributeSpecifierAST *ast) override;
    bool visit(MsvcDeclspecSpecifierAST *ast) override;
    bool visit(StdAttributeSpecifierAST *ast) override;
    bool visit(TypeofSpecifierAST *ast) override;
    bool visit(DecltypeSpecifierAST *ast) override;
    bool visit(ClassSpecifierAST *ast) override;
    bool visit(NamedTypeSpecifierAST *ast) override;
    bool visit(ElaboratedTypeSpecifierAST *ast) override;
    bool visit(EnumSpecifierAST *ast) override;

    // PtrOperatorAST
    bool visit(PointerToMemberAST *ast) override;
    bool visit(PointerAST *ast) override;
    bool visit(ReferenceAST *ast) override;

    // PostfixAST
    bool visit(CallAST *ast) override;
    bool visit(ArrayAccessAST *ast) override;
    bool visit(PostIncrDecrAST *ast) override;
    bool visit(MemberAccessAST *ast) override;

    // CoreDeclaratorAST
    bool visit(DeclaratorIdAST *ast) override;
    bool visit(DecompositionDeclaratorAST *ast) override;
    bool visit(NestedDeclaratorAST *ast) override;

    // PostfixDeclaratorAST
    bool visit(FunctionDeclaratorAST *ast) override;
    bool visit(ArrayDeclaratorAST *ast) override;

private:
    static const int kMaxDepth;

    void ensureValidClassName(const Name **name, int sourceLocation);

    Scope *_scope;
    ExpressionTy _expression;
    const Name *_name;
    FullySpecifiedType _type;
    DeclaratorIdAST **_declaratorId;
    DecompositionDeclaratorAST **_decompositionDeclarator;
    int _visibility;
    int _objcVisibility;
    int _methodKey;
    bool _skipFunctionBodies;
    int _depth;
    bool _typeWasUnsignedOrSigned = false;
};

} // namespace CPlusPlus
