// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>
#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner {
namespace ConnectionEditorStatements {

inline constexpr char FUNCTION_DISPLAY_NAME[] = QT_TRANSLATE_NOOP(
    "QmlDesigner::ConnectionEditorStatements", "Function");
inline constexpr char ASSIGNMENT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP(
    "QmlDesigner::ConnectionEditorStatements", "Assignment");
inline constexpr char SETPROPERTY_DISPLAY_NAME[] = QT_TRANSLATE_NOOP(
    "QmlDesigner::ConnectionEditorStatements", "Set Property");
inline constexpr char SETSTATE_DISPLAY_NAME[] = QT_TRANSLATE_NOOP(
    "QmlDesigner::ConnectionEditorStatements", "Set State");
inline constexpr char LOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP(
    "QmlDesigner::ConnectionEditorStatements", "Print");
inline constexpr char EMPTY_DISPLAY_NAME[] = QT_TRANSLATE_NOOP(
    "QmlDesigner::ConnectionEditorStatements", "Empty");
inline constexpr char UNKNOWN_DISPLAY_NAME[] = QT_TRANSLATE_NOOP(
    "QmlDesigner::ConnectionEditorStatements", "Unknown");

struct Variable;
struct MatchedFunction;
struct Assignment;
struct PropertySet;
struct StateSet;
struct ConsoleLog;
struct MatchedCondition;
struct ConditionalStatement;

using EmptyBlock = std::monostate;
using Literal = std::variant<bool, double, QString>;
using ComparativeStatement = std::variant<bool, double, QString, Variable>;
using RightHandSide = std::variant<bool, double, QString, Variable, MatchedFunction>;
using MatchedStatement = std::variant<EmptyBlock, MatchedFunction, Assignment, PropertySet, StateSet, ConsoleLog>;
using Handler = std::variant<MatchedStatement, ConditionalStatement>;

struct Variable
{ //Only one level for now (.)
    QString nodeId;
    QString propertyName;

    QString expression() const { return nodeId + "." + propertyName; }
};

struct MatchedFunction
{ //First item before "." is considered node
    QString nodeId;
    QString functionName;
};

struct Assignment
{
    Variable lhs; //There always should be a binding property on the left hand side. The first identifier is considered node
    Variable rhs; //Similar to function but no function call. Just regular FieldExpression/binding
};

struct PropertySet
{
    Variable lhs; //There always should be a binding property on the left hand side. The first identifier is considered node
    Literal rhs;
};

struct StateSet
{
    QString nodeId;
    QString stateName;
};

struct ConsoleLog
{
    RightHandSide argument;
};

enum class ConditionToken {
    Unknown,
    Not,
    And,
    Or,
    LargerThan,
    LargerEqualsThan,
    SmallerThan,
    SmallerEqualsThan,
    Equals
};

struct MatchedCondition
{
    QList<ConditionToken> tokens;
    QList<ComparativeStatement> statements;
};

struct ConditionalStatement
{
    MatchedStatement ok = EmptyBlock{};
    MatchedStatement ko = EmptyBlock{}; //else statement
    MatchedCondition condition;
};

QMLDESIGNERCORE_EXPORT bool isEmptyStatement(const MatchedStatement &stat);
QMLDESIGNERCORE_EXPORT QString toString(const ComparativeStatement &stat);
QMLDESIGNERCORE_EXPORT QString toString(const RightHandSide &rhs);
QMLDESIGNERCORE_EXPORT QString toString(const Literal &literal);
QMLDESIGNERCORE_EXPORT QString toString(const MatchedStatement &statement);

QMLDESIGNERCORE_EXPORT bool isConsoleLog(const MatchedStatement &curState);
QMLDESIGNERCORE_EXPORT bool isLiteralType(const RightHandSide &var);

QMLDESIGNERCORE_EXPORT QString toString(const Handler &handler);
QMLDESIGNERCORE_EXPORT QString toJavascript(const Handler &handler);
QMLDESIGNERCORE_EXPORT QString toDisplayName(const MatchedStatement &statement);
QMLDESIGNERCORE_EXPORT QString toDisplayName(const Handler &handler);

QMLDESIGNERCORE_EXPORT MatchedStatement okStatement(const ConnectionEditorStatements::Handler &handler);
QMLDESIGNERCORE_EXPORT MatchedStatement koStatement(const ConnectionEditorStatements::Handler &handler);

} // namespace ConnectionEditorStatements

} // namespace QmlDesigner
