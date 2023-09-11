// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "connectioneditorstatements.h"

#include <QHash>

using namespace QmlDesigner;
using namespace ConnectionEditorStatements;

namespace {
template<typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
Overload(Ts...) -> Overload<Ts...>;

inline constexpr QStringView trueString{u"true"};
inline constexpr QStringView falseString{u"false"};

struct StringVisitor
{
    QString operator()(const bool &bVal) const
    {
        return (bVal ? trueString : falseString).toString();
    }

    QString operator()(const double &dVal) const { return QString::number(dVal); }

    QString operator()(const QString &str) const { return "\"" + str + "\""; }

    QString operator()(const Variable &var)
    {
        QString propertyName;
        if (var.propertyName.size())
            propertyName = ".";
        propertyName.append(var.propertyName);
        return "Variable{" + var.nodeId + propertyName + "}";
    }

    QString operator()(const ConnectionEditorStatements::MatchedFunction &func)
    {
        return "MatchedFunction{" + func.nodeId + "." + func.functionName + "}";
    }

    QString operator()(const ConnectionEditorStatements::Assignment &assignment)
    {
        return "Assignment{" + assignment.lhs.expression() + " = " + StringVisitor()(assignment.rhs)
               + "}";
    }

    QString operator()(const ConnectionEditorStatements::PropertySet &propertySet)
    {
        return "PropertySet{" + propertySet.lhs.expression() + " = "
               + std::visit(StringVisitor{}, propertySet.rhs) + "}";
    }

    QString operator()(const ConnectionEditorStatements::StateSet &stateSet)
    {
        return "StateSet{" + stateSet.nodeId + ".state = " + stateSet.stateName + "}";
    }

    QString operator()(const ConnectionEditorStatements::EmptyBlock &) { return "EmptyBlock{}"; }

    QString operator()(const ConnectionEditorStatements::ConsoleLog &consoleLog)
    {
        return "ConsoleLog{" + std::visit(StringVisitor{}, consoleLog.argument) + "}";
    }

    QString operator()(const ConditionToken &token)
    {
        switch (token) {
        case ConditionToken::Not:
            return "Not";
        case ConditionToken::And:
            return "And";
        case ConditionToken::Or:
            return "Or";
        case ConditionToken::LargerThan:
            return "LargerThan";
        case ConditionToken::LargerEqualsThan:
            return "LargerEuqalsThan";
        case ConditionToken::SmallerThan:
            return "SmallerThan";
        case ConditionToken::SmallerEqualsThan:
            return "SmallerEqualsThan";
        case ConditionToken::Equals:
            return "Equals";
        default:
            return {};
        }
    }

    QString operator()(const ConnectionEditorStatements::MatchedCondition &matched)
    {
        if (!matched.statements.size() && !matched.tokens.size())
            return "MatchedCondition{}";

        if (matched.statements.size() != matched.tokens.size() + 1)
            return "MatchedCondition{Invalid}";

        QString value = "MatchedCondition{";
        int i = 0;
        for (i = 0; i < matched.tokens.size(); i++) {
            const ComparativeStatement &statement = matched.statements[i];
            const ConditionToken &token = matched.tokens[i];
            value += std::visit(StringVisitor{}, statement) + " ";
            value += StringVisitor()(token) + " ";
        }
        value += std::visit(StringVisitor{}, matched.statements[i]);
        value += "}";
        return value;
    }

    QString operator()(const ConnectionEditorStatements::ConditionalStatement &conditional)
    {
        QString value;
        value.reserve(200);
        value = "IF (";
        value += StringVisitor()(conditional.condition);
        value += ") {\n";
        value += std::visit(StringVisitor{}, conditional.ok);
        if (!isEmptyStatement(conditional.ko)) {
            value += "\n} ELSE {\n";
            value += std::visit(StringVisitor{}, conditional.ko);
        }
        value += "\n}";

        return value;
    }

    QString operator()(const ConnectionEditorStatements::MatchedStatement &conditional)
    {
        return std::visit(StringVisitor{}, conditional);
    }
};

struct JSOverload
{
    QString operator()(const bool &bVal) const
    {
        return (bVal ? trueString : falseString).toString();
    }

    QString operator()(const double &dVal) const { return QString::number(dVal); }

    QString operator()(const QString &str) const { return "\"" + str + "\""; }

    QString operator()(const Variable &var)
    {
        QString propertyName;
        if (var.propertyName.size())
            propertyName = ".";
        propertyName.append(var.propertyName);
        return var.nodeId + propertyName;
    }

    QString operator()(const ConnectionEditorStatements::MatchedFunction &func)
    {
        QString funcName;
        if (func.functionName.size())
            funcName = ".";
        funcName.append(func.functionName);
        return func.nodeId + funcName + "()";
    }

    QString operator()(const ConnectionEditorStatements::Assignment &assignment)
    {
        return JSOverload()(assignment.lhs) + " = " + JSOverload()(assignment.rhs);
    }

    QString operator()(const ConnectionEditorStatements::PropertySet &propertySet)
    {
        return JSOverload()(propertySet.lhs) + " = " + std::visit(JSOverload{}, propertySet.rhs);
    }

    QString operator()(const ConnectionEditorStatements::StateSet &stateSet)
    {
        return stateSet.nodeId + ".state = " + stateSet.stateName;
    }

    QString operator()(const ConnectionEditorStatements::EmptyBlock &) { return "{}"; }

    QString operator()(const ConnectionEditorStatements::ConsoleLog &consoleLog)
    {
        return "console.log(" + std::visit(JSOverload{}, consoleLog.argument) + ")";
    }

    QString operator()(const ConditionToken &token) { return toJavascript(token); }

    QString operator()(const ConnectionEditorStatements::MatchedCondition &matched)
    {
        if (!matched.statements.size() && !matched.tokens.size())
            return {};

        if (matched.statements.size() != matched.tokens.size() + 1)
            return {};

        QString value;
        int i = 0;
        for (i = 0; i < matched.tokens.size(); i++) {
            const ComparativeStatement &statement = matched.statements[i];
            const ConditionToken &token = matched.tokens[i];
            value += std::visit(JSOverload{}, statement) + " ";
            value += JSOverload()(token) + " ";
        }
        value += std::visit(JSOverload{}, matched.statements[i]);
        return value;
    }

    QString operator()(const ConnectionEditorStatements::MatchedStatement &statement)
    {
        if (isEmptyStatement(statement))
            return {};

        return std::visit(JSOverload{}, statement);
    }

    QString operator()(const ConnectionEditorStatements::ConditionalStatement &conditional)
    {
        QString value;
        value.reserve(200);
        value = "if (";
        value += JSOverload()(conditional.condition);
        value += ") {\n";

        if (!isEmptyStatement(conditional.ok))
            value += JSOverload()(conditional.ok);

        if (!isEmptyStatement(conditional.ko)) {
            value += "\n} else {\n";
            value += JSOverload()(conditional.ko);
        }
        value += "\n}";

        return value;
    }
};

} // namespace

bool ConnectionEditorStatements::isEmptyStatement(const MatchedStatement &stat)
{
    return std::holds_alternative<EmptyBlock>(stat);
}

QString ConnectionEditorStatements::toString(const ComparativeStatement &stat)
{
    return std::visit(StringVisitor{}, stat);
}

QString ConnectionEditorStatements::toString(const RightHandSide &rhs)
{
    return std::visit(StringVisitor{}, rhs);
}

QString ConnectionEditorStatements::toString(const Literal &literal)
{
    return std::visit(StringVisitor{}, literal);
}

QString ConnectionEditorStatements::toString(const MatchedStatement &statement)
{
    return std::visit(StringVisitor{}, statement);
}

QString ConnectionEditorStatements::toString(const Handler &handler)
{
    return std::visit(StringVisitor{}, handler);
}

QString ConnectionEditorStatements::toJavascript(const Handler &handler)
{
    return std::visit(JSOverload{}, handler);
}

bool ConnectionEditorStatements::isConsoleLog(const MatchedStatement &curState)
{
    return std::holds_alternative<ConsoleLog>(curState);
}

bool ConnectionEditorStatements::isLiteralType(const RightHandSide &var)
{
    return std::visit(Overload{[](const double &) { return true; },
                               [](const bool &) { return true; },
                               [](const QString &) { return true; },
                               [](const auto &) { return false; }},
                      var);
}

QString ConnectionEditorStatements::toDisplayName(const MatchedStatement &statement)
{
    const char *displayName = std::visit(
        Overload{[](const MatchedFunction &) { return FUNCTION_DISPLAY_NAME; },
                 [](const Assignment &) { return ASSIGNMENT_DISPLAY_NAME; },
                 [](const PropertySet &) { return SETPROPERTY_DISPLAY_NAME; },
                 [](const StateSet &) { return SETSTATE_DISPLAY_NAME; },
                 [](const ConsoleLog &) { return LOG_DISPLAY_NAME; },
                 [](const EmptyBlock &) { return EMPTY_DISPLAY_NAME; }},
        statement);

    return QString::fromLatin1(displayName);
}

QString ConnectionEditorStatements::toDisplayName(const Handler &handler)
{
    const MatchedStatement &statement = std::visit(
        Overload{[](const MatchedStatement &statement) { return statement; },
                 [](const ConditionalStatement &statement) { return statement.ok; }},
        handler);
    return toDisplayName(statement);
}

MatchedStatement &ConnectionEditorStatements::okStatement(
    ConnectionEditorStatements::Handler &handler)
{
    MatchedStatement statement;

    return std::visit(Overload{[](ConnectionEditorStatements::MatchedStatement &var)
                                   -> MatchedStatement & { return var; },
                               [](ConnectionEditorStatements::ConditionalStatement &statement)
                                   -> MatchedStatement & { return statement.ok; }},
                      handler);
}

MatchedStatement &ConnectionEditorStatements::koStatement(
    ConnectionEditorStatements::Handler &handler)
{
    static MatchedStatement block;

    if (auto *statement = std::get_if<ConnectionEditorStatements::ConditionalStatement>(&handler))
        return statement->ko;

    return block;
}

MatchedCondition &ConnectionEditorStatements::matchedCondition(Handler &handler)
{
    static MatchedCondition block;

    if (auto *statement = std::get_if<ConnectionEditorStatements::ConditionalStatement>(&handler))
        return statement->condition;

    return block;
}

ConditionalStatement &ConnectionEditorStatements::conditionalStatement(
    ConnectionEditorStatements::Handler &handler)
{
    static ConditionalStatement block;

    if (auto *statement = std::get_if<ConnectionEditorStatements::ConditionalStatement>(&handler))
        return *statement;

    return block;
}

QString ConnectionEditorStatements::toJavascript(const ConditionToken &token)
{
    switch (token) {
    case ConditionToken::Not:
        return "!==";
    case ConditionToken::And:
        return "&&";
    case ConditionToken::Or:
        return "||";
    case ConditionToken::LargerThan:
        return ">";
    case ConditionToken::LargerEqualsThan:
        return ">=";
    case ConditionToken::SmallerThan:
        return "<";
    case ConditionToken::SmallerEqualsThan:
        return "<=";
    case ConditionToken::Equals:
        return "===";
    default:
        return {};
    };
}
