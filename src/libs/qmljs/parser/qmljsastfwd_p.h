/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmljsglobal_p.h"

#include <QtCore/qglobal.h>

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

class SourceLocation
{
public:
    explicit SourceLocation(quint32 offset = 0, quint32 length = 0, quint32 line = 0, quint32 column = 0)
        : offset(offset), length(length),
          startLine(line), startColumn(column)
    { }

    bool isValid() const { return length != 0; }

    quint32 begin() const { return offset; }
    quint32 end() const { return offset + length; }

// attributes
    // ### encode
    quint32 offset;
    quint32 length;
    quint32 startLine;
    quint32 startColumn;
};

class Visitor;
class Node;
class ExpressionNode;
class Statement;
class ThisExpression;
class IdentifierExpression;
class NullExpression;
class TrueLiteral;
class FalseLiteral;
class NumericLiteral;
class StringLiteral;
class RegExpLiteral;
class ArrayLiteral;
class ObjectLiteral;
class ElementList;
class Elision;
class PropertyAssignmentList;
class PropertyGetterSetter;
class PropertyNameAndValue;
class PropertyName;
class IdentifierPropertyName;
class StringLiteralPropertyName;
class NumericLiteralPropertyName;
class ArrayMemberExpression;
class FieldMemberExpression;
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
class Block;
class StatementList;
class VariableStatement;
class VariableDeclarationList;
class VariableDeclaration;
class EmptyStatement;
class ExpressionStatement;
class IfStatement;
class DoWhileStatement;
class WhileStatement;
class ForStatement;
class LocalForStatement;
class ForEachStatement;
class LocalForEachStatement;
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
class FunctionBody;
class Program;
class SourceElements;
class SourceElement;
class FunctionSourceElement;
class StatementSourceElement;
class DebuggerStatement;
class NestedExpression;

// ui elements
class UiProgram;
class UiPragma;
class UiImport;
class UiPublicMember;
class UiParameterList;
class UiObjectDefinition;
class UiObjectInitializer;
class UiObjectBinding;
class UiScriptBinding;
class UiSourceElement;
class UiArrayBinding;
class UiObjectMember;
class UiObjectMemberList;
class UiArrayMemberList;
class UiQualifiedId;
class UiQualifiedPragmaId;
class UiHeaderItemList;
class UiEnumDeclaration;
class UiEnumMemberList;

} } // namespace AST

QT_QML_END_NAMESPACE

