// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs/parser/qmljssourcelocation_p.h"

#include "qmljsglobal_p.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_QML_BEGIN_NAMESPACE

namespace QmlJS { namespace AST {

class BaseVisitor;
class Visitor;
class Node;
class ExpressionNode;
class Statement;
class TypeExpression;
class ThisExpression;
class IdentifierExpression;
class NullExpression;
class TrueLiteral;
class FalseLiteral;
class SuperLiteral;
class NumericLiteral;
class StringLiteral;
class TemplateLiteral;
class RegExpLiteral;
class Pattern;
class ArrayPattern;
class ObjectPattern;
class PatternElement;
class PatternElementList;
class PatternProperty;
class PatternPropertyList;
class Elision;
class PropertyName;
class IdentifierPropertyName;
class StringLiteralPropertyName;
class NumericLiteralPropertyName;
class ComputedPropertyName;
class ArrayMemberExpression;
class FieldMemberExpression;
class TaggedTemplate;
class NewMemberExpression;
class NewExpression;
class CallExpression;
class ArgumentList;
class PostIncrementExpression;
class PostDecrementExpression;
class DeleteExpression;
class VoidExpression;
class TypeOfExpression;
class PreIncrementExpression;
class PreDecrementExpression;
class UnaryPlusExpression;
class UnaryMinusExpression;
class TildeExpression;
class NotExpression;
class BinaryExpression;
class ConditionalExpression;
class Expression; // ### rename
class YieldExpression;
class Block;
class LeftHandSideExpression;
class StatementList;
class VariableStatement;
class VariableDeclarationList;
class EmptyStatement;
class ExpressionStatement;
class IfStatement;
class DoWhileStatement;
class WhileStatement;
class ForStatement;
class ForEachStatement;
class ContinueStatement;
class BreakStatement;
class ReturnStatement;
class WithStatement;
class SwitchStatement;
class CaseBlock;
class CaseClauses;
class CaseClause;
class DefaultClause;
class LabelledStatement;
class ThrowStatement;
class TryStatement;
class Catch;
class Finally;
class FunctionDeclaration;
class FunctionExpression;
class FormalParameterList;
class ExportSpecifier;
class ExportsList;
class ExportClause;
class ExportDeclaration;
class Program;
class ImportSpecifier;
class ImportsList;
class NamedImports;
class NameSpaceImport;
class NamedImport;
class ImportClause;
class FromClause;
class ImportDeclaration;
class ESModule;
class DebuggerStatement;
class NestedExpression;
class ClassExpression;
class ClassDeclaration;
class ClassElementList;
class TypeArgument;
class Type;
class TypeAnnotation;

// ui elements
class UiProgram;
class UiPragma;
class UiImport;
class UiPublicMember;
class UiParameterList;
class UiObjectDefinition;
class UiInlineComponent;
class UiObjectInitializer;
class UiObjectBinding;
class UiScriptBinding;
class UiSourceElement;
class UiArrayBinding;
class UiObjectMember;
class UiObjectMemberList;
class UiArrayMemberList;
class UiQualifiedId;
class UiHeaderItemList;
class UiEnumDeclaration;
class UiEnumMemberList;
class UiVersionSpecifier;
class UiRequired;
class UiAnnotation;
class UiAnnotationList;

} // namespace AST
} // namespace QmlJS

QT_QML_END_NAMESPACE

