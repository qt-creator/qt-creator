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

#ifndef CPLUSPLUS_ASTFWD_H
#define CPLUSPLUS_ASTFWD_H

#include "CPlusPlusForwardDeclarations.h"

namespace CPlusPlus {

template <typename _Tp> class List;

class AST;
class ASTVisitor;
class ASTMatcher;

class AccessDeclarationAST;
class AliasDeclarationAST;
class AlignofExpressionAST;
class AnonymousNameAST;
class ArrayAccessAST;
class ArrayDeclaratorAST;
class ArrayInitializerAST;
class AsmDefinitionAST;
class AttributeAST;
class AttributeSpecifierAST;
class BaseSpecifierAST;
class BinaryExpressionAST;
class BoolLiteralAST;
class BracedInitializerAST;
class BreakStatementAST;
class CallAST;
class CaptureAST;
class CaseStatementAST;
class CastExpressionAST;
class CatchClauseAST;
class ClassSpecifierAST;
class CompoundExpressionAST;
class CompoundLiteralAST;
class CompoundStatementAST;
class ConditionAST;
class ConditionalExpressionAST;
class ContinueStatementAST;
class ConversionFunctionIdAST;
class CoreDeclaratorAST;
class CppCastExpressionAST;
class CtorInitializerAST;
class DeclarationAST;
class DeclarationStatementAST;
class DeclaratorAST;
class DeclaratorIdAST;
class DecltypeSpecifierAST;
class DeleteExpressionAST;
class DestructorNameAST;
class DoStatementAST;
class DynamicExceptionSpecificationAST;
class ElaboratedTypeSpecifierAST;
class EmptyDeclarationAST;
class EnumSpecifierAST;
class EnumeratorAST;
class ExceptionDeclarationAST;
class ExceptionSpecificationAST;
class ExpressionAST;
class ExpressionListParenAST;
class ExpressionOrDeclarationStatementAST;
class ExpressionStatementAST;
class ForStatementAST;
class ForeachStatementAST;
class FunctionDeclaratorAST;
class FunctionDefinitionAST;
class GotoStatementAST;
class IdExpressionAST;
class IfStatementAST;
class LabeledStatementAST;
class LambdaCaptureAST;
class LambdaDeclaratorAST;
class LambdaExpressionAST;
class LambdaIntroducerAST;
class LinkageBodyAST;
class LinkageSpecificationAST;
class MemInitializerAST;
class MemberAccessAST;
class NameAST;
class NamedTypeSpecifierAST;
class NamespaceAST;
class NamespaceAliasDefinitionAST;
class NestedDeclaratorAST;
class NestedExpressionAST;
class NestedNameSpecifierAST;
class NewArrayDeclaratorAST;
class NewExpressionAST;
class NewTypeIdAST;
class NoExceptSpecificationAST;
class NumericLiteralAST;
class ObjCClassDeclarationAST;
class ObjCClassForwardDeclarationAST;
class ObjCDynamicPropertiesDeclarationAST;
class ObjCEncodeExpressionAST;
class ObjCFastEnumerationAST;
class ObjCInstanceVariablesDeclarationAST;
class ObjCMessageArgumentAST;
class ObjCMessageArgumentDeclarationAST;
class ObjCMessageExpressionAST;
class ObjCMethodDeclarationAST;
class ObjCMethodPrototypeAST;
class ObjCPropertyAttributeAST;
class ObjCPropertyDeclarationAST;
class ObjCProtocolDeclarationAST;
class ObjCProtocolExpressionAST;
class ObjCProtocolForwardDeclarationAST;
class ObjCProtocolRefsAST;
class ObjCSelectorAST;
class ObjCSelectorArgumentAST;
class ObjCSelectorExpressionAST;
class ObjCSynchronizedStatementAST;
class ObjCSynthesizedPropertiesDeclarationAST;
class ObjCSynthesizedPropertyAST;
class ObjCTypeNameAST;
class ObjCVisibilityDeclarationAST;
class OperatorAST;
class OperatorFunctionIdAST;
class ParameterDeclarationAST;
class ParameterDeclarationClauseAST;
class PointerAST;
class PointerLiteralAST;
class PointerToMemberAST;
class PostIncrDecrAST;
class PostfixAST;
class PostfixDeclaratorAST;
class PtrOperatorAST;
class QtEnumDeclarationAST;
class QtFlagsDeclarationAST;
class QtInterfaceNameAST;
class QtInterfacesDeclarationAST;
class QtMemberDeclarationAST;
class QtMethodAST;
class QtObjectTagAST;
class QtPrivateSlotAST;
class QtPropertyDeclarationAST;
class QtPropertyDeclarationItemAST;
class QualifiedNameAST;
class RangeBasedForStatementAST;
class ReferenceAST;
class ReturnStatementAST;
class SimpleDeclarationAST;
class SimpleNameAST;
class SimpleSpecifierAST;
class SizeofExpressionAST;
class SpecifierAST;
class StatementAST;
class StaticAssertDeclarationAST;
class StringLiteralAST;
class SwitchStatementAST;
class TemplateDeclarationAST;
class TemplateIdAST;
class TemplateTypeParameterAST;
class ThisExpressionAST;
class ThrowExpressionAST;
class TrailingReturnTypeAST;
class TranslationUnitAST;
class TryBlockStatementAST;
class TypeConstructorCallAST;
class TypeIdAST;
class TypeidExpressionAST;
class TypenameCallExpressionAST;
class TypenameTypeParameterAST;
class TypeofSpecifierAST;
class UnaryExpressionAST;
class UsingAST;
class UsingDirectiveAST;
class WhileStatementAST;

typedef List<ExpressionAST *> ExpressionListAST;
typedef List<DeclarationAST *> DeclarationListAST;
typedef List<StatementAST *> StatementListAST;
typedef List<DeclaratorAST *> DeclaratorListAST;
typedef List<BaseSpecifierAST *> BaseSpecifierListAST;
typedef List<EnumeratorAST *> EnumeratorListAST;
typedef List<MemInitializerAST *> MemInitializerListAST;
typedef List<NewArrayDeclaratorAST *> NewArrayDeclaratorListAST;
typedef List<PostfixAST *> PostfixListAST;
typedef List<PostfixDeclaratorAST *> PostfixDeclaratorListAST;
typedef List<AttributeAST *> AttributeListAST;
typedef List<NestedNameSpecifierAST *> NestedNameSpecifierListAST;
typedef List<CatchClauseAST *> CatchClauseListAST;
typedef List<PtrOperatorAST *> PtrOperatorListAST;
typedef List<SpecifierAST *> SpecifierListAST;
typedef List<QtPropertyDeclarationItemAST *> QtPropertyDeclarationItemListAST;
typedef List<NameAST *> NameListAST;
typedef List<QtInterfaceNameAST *> QtInterfaceNameListAST;

typedef List<ParameterDeclarationAST *> ParameterDeclarationListAST;
typedef List<CaptureAST *> CaptureListAST;

typedef List<ObjCMessageArgumentAST *> ObjCMessageArgumentListAST;
typedef List<ObjCSelectorArgumentAST *> ObjCSelectorArgumentListAST;
typedef List<ObjCPropertyAttributeAST *> ObjCPropertyAttributeListAST;
typedef List<ObjCMessageArgumentDeclarationAST *> ObjCMessageArgumentDeclarationListAST;
typedef List<ObjCSynthesizedPropertyAST *> ObjCSynthesizedPropertyListAST;

} // namespace CPlusPlus


#endif // CPLUSPLUS_ASTFWD_H
