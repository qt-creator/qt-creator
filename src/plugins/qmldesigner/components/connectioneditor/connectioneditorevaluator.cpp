// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectioneditorevaluator.h"
#include "qmljs/parser/qmljsast_p.h"
#include "qmljs/qmljsdocument.h"

#include <utils/ranges.h>

#include <QList>

using namespace QmlDesigner;

using QmlJS::AST::Node;
using Kind = Node::Kind;
using ConnectionEditorStatements::ConditionalStatement;
using ConnectionEditorStatements::ConditionToken;
using ConnectionEditorStatements::MatchedCondition;
using ConnectionEditorStatements::MatchedStatement;

namespace {
enum class TrackingArea { No, Condition, Ok, Ko };

template<typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
Overload(Ts...) -> Overload<Ts...>;

inline ConditionToken operator2ConditionToken(const int &op)
{
    using QSOperator::Op;
    switch (op) {
    case Op::Le:
        return ConditionToken::SmallerEqualsThan;
    case Op::Lt:
        return ConditionToken::SmallerThan;
    case Op::Ge:
        return ConditionToken::LargerEqualsThan;
    case Op::Gt:
        return ConditionToken::LargerThan;
    case Op::Equal:
        return ConditionToken::Equals;
    case Op::NotEqual:
        return ConditionToken::Not;
    case Op::And:
        return ConditionToken::And;
    case Op::Or:
        return ConditionToken::Or;
    case Op::StrictEqual:
        return ConditionToken::Equals;
    case Op::StrictNotEqual:
        return ConditionToken::Not;
    default:
        return ConditionToken::Unknown;
    }
}

inline bool isStatementNode(const int &kind)
{
    return kind == Kind::Kind_CallExpression || kind == Kind::Kind_FieldMemberExpression
           || kind == Kind::Kind_IdentifierExpression;
}

inline bool isAcceptedIfBinaryOperator(const int &operation)
{
    switch (operation) {
    case QSOperator::Le:
    case QSOperator::Lt:
    case QSOperator::Ge:
    case QSOperator::Gt:
    case QSOperator::Equal:
    case QSOperator::NotEqual:
    case QSOperator::And:
    case QSOperator::Or:
    case QSOperator::StrictEqual:
    case QSOperator::StrictNotEqual:
    case QSOperator::InplaceXor:
        return true;
    default:
        return false;
    }
}

class NodeStatus
{
public:
    NodeStatus()
        : m_kind(Kind::Kind_Undefined)
        , m_children(0)
        , m_isStatementNode(false)
    {}

    NodeStatus(Node *node)
        : m_kind(Kind(node->kind))
        , m_children(0)
        , m_isStatementNode(::isStatementNode(m_kind))
    {}

    int childId() const { return m_children - 1; }

    int children() const { return m_children; }

    operator Kind() { return m_kind; }

    bool operator==(int kind) const { return m_kind == kind; }

    bool operator==(Kind kind) const { return m_kind == kind; }

    bool operator!=(Kind kind) const { return m_kind != kind; }

    int increaseChildNo() { return m_children++; }

    bool isStatementNode() const { return m_isStatementNode; }

private:
    Kind m_kind;
    int m_children = 0;
    bool m_isStatementNode = false;
};

class BoolCondition : public QmlJS::AST::Visitor
{
public:
    MatchedCondition matchedCondition() const { return m_condition; }

    void reset()
    {
        m_failed = false;
        m_depth = 0;
        m_fields.clear();
        m_identifier.clear();
    }

    bool isValid() { return !m_failed; }

    const QString &errorString() const { return m_failureString; }

protected:
    bool preVisit(QmlJS::AST::Node *node) override
    {
        if (m_failed)
            return false;

        switch (node->kind) {
        case Kind::Kind_BinaryExpression:
        case Kind::Kind_FieldMemberExpression:
        case Kind::Kind_IdentifierExpression:
        case Kind::Kind_StringLiteral:
        case Kind::Kind_NumericLiteral:
        case Kind::Kind_TrueLiteral:
        case Kind::Kind_FalseLiteral:
            return true;
        default: {
            checkValidityAndReturn(false, "Invalid node type.");
            return false;
        }
        }
    }

    bool visit(QmlJS::AST::BinaryExpression *binaryExpression) override
    {
        if (m_failed)
            return false;

        if (binaryExpression->op == QSOperator::Equal)
            return checkValidityAndReturn(false, "Use \"===\" for comparing two expressions.");

        if (binaryExpression->op == QSOperator::NotEqual)
            return checkValidityAndReturn(false, "Use \"!==\" for comparing two field member expressions.");

        if (!isAcceptedIfBinaryOperator(binaryExpression->op))
            return checkValidityAndReturn(false, "Invalid binary operator");

        if (binaryExpression->left->kind == Kind::Kind_StringLiteral)
            return checkValidityAndReturn(false, "Left hand string literal");

        if (binaryExpression->left->kind == Kind::Kind_NumericLiteral)
            return checkValidityAndReturn(false, "Left hand numeric literal");

        acceptBoolOperand(binaryExpression->left);
        m_condition.tokens.append(operator2ConditionToken(binaryExpression->op));
        acceptBoolOperand(binaryExpression->right);

        return false;
    }

    bool visit([[maybe_unused]] QmlJS::AST::IdentifierExpression *identifier) override
    {
        if (m_failed)
            return false;

        ++m_depth;
        return false;
    }

    bool visit([[maybe_unused]] QmlJS::AST::FieldMemberExpression *field) override
    {
        if (m_failed)
            return false;

        ++m_depth;
        return true;
    }

    void endVisit(QmlJS::AST::FieldMemberExpression *fieldExpression) override
    {
        if (m_failed)
            return;

        m_fields << fieldExpression->name.toString();
        checkAndResetVariable();
    }

    void endVisit(QmlJS::AST::IdentifierExpression *identifier) override
    {
        if (m_failed)
            return;

        m_identifier = identifier->name.toString();
        checkAndResetVariable();
    }

    void endVisit(QmlJS::AST::StringLiteral *stringLiteral) override
    {
        if (m_failed)
            return;
        m_condition.statements.push_back(stringLiteral->value.toString());
    }

    void endVisit(QmlJS::AST::NumericLiteral *numericLiteral) override
    {
        if (m_failed)
            return;
        m_condition.statements.push_back(numericLiteral->value);
    }

    void endVisit([[maybe_unused]] QmlJS::AST::TrueLiteral *trueLiteral) override
    {
        if (m_failed)
            return;
        m_condition.statements.push_back(true);
    }

    void endVisit([[maybe_unused]] QmlJS::AST::FalseLiteral *falseLiteral) override
    {
        if (m_failed)
            return;
        m_condition.statements.push_back(false);
    }

    void throwRecursionDepthError() override
    {
        checkValidityAndReturn(false, "Recursion depth problem");
        qDebug() << Q_FUNC_INFO << this;
    }

    void checkAndResetVariable()
    {
        if (--m_depth == 0) {
            m_condition.statements.push_back(
                ConnectionEditorStatements::Variable{m_identifier, m_fields.join(".")});
            m_identifier.clear();
            m_fields.clear();
        }
    }

    void acceptBoolOperand(Node *operand)
    {
        BoolCondition boolEvaluator;
        operand->accept(&boolEvaluator);
        if (!checkValidityAndReturn(boolEvaluator.isValid(), boolEvaluator.errorString()))
            return;

        m_condition.statements.append(boolEvaluator.m_condition.statements);
        m_condition.tokens.append(boolEvaluator.m_condition.tokens);
    }

private:
    bool m_failed = false;
    int m_depth = 0;
    QString m_identifier;
    QStringList m_fields;
    QString m_failureString;
    MatchedCondition m_condition;

    bool checkValidityAndReturn(bool valid, const QString &error)
    {
        if (!m_failed && !valid) {
            m_failed = true;
            m_failureString = error;
        }
        return !m_failed;
    }
};

class RightHandVisitor : public QmlJS::AST::Visitor
{
public:
    ConnectionEditorStatements::RightHandSide rhs() const { return m_rhs; }

    void reset()
    {
        m_failed = false;
        m_specified = false;
        m_depth = 0;
        m_fields.clear();
        m_identifier.clear();
    }

    bool isValid() const { return !m_failed && m_specified; }

    bool isLiteralType() const
    {
        if (!isValid())
            return false;

        return ConnectionEditorStatements::isLiteralType(rhs());
    }

    bool couldBeLHS() const
    {
        if (!isValid())
            return false;
        return std::holds_alternative<ConnectionEditorStatements::Variable>(m_rhs);
    }

    inline bool couldBeVariable() const { return couldBeLHS(); }

    ConnectionEditorStatements::Literal literal() const
    {
        if (!isLiteralType())
            return {};

        return std::visit(
            Overload{[](const bool &var) -> ConnectionEditorStatements::Literal { return var; },
                     [](const double &var) -> ConnectionEditorStatements::Literal { return var; },
                     [](const QString &var) -> ConnectionEditorStatements::Literal { return var; },
                     [](const auto &) -> ConnectionEditorStatements::Literal { return false; }},
            m_rhs);
    }

    ConnectionEditorStatements::Variable lhs() const
    {
        if (!isValid())
            return {};

        if (auto rhs = std::get_if<ConnectionEditorStatements::Variable>(&m_rhs))
            return *rhs;

        return {};
    }

    ConnectionEditorStatements::Variable variable() const
    {
        if (!isValid())
            return {};

        if (auto rhs = std::get_if<ConnectionEditorStatements::Variable>(&m_rhs))
            return *rhs;

        return {};
    }

protected:
    bool preVisit(QmlJS::AST::Node *node) override
    {
        if (skipIt())
            setFailed();

        if (m_failed)
            return false;

        switch (node->kind) {
        case Kind::Kind_CallExpression:
        case Kind::Kind_FieldMemberExpression:
        case Kind::Kind_IdentifierExpression:
        case Kind::Kind_StringLiteral:
        case Kind::Kind_NumericLiteral:
        case Kind::Kind_TrueLiteral:
        case Kind::Kind_FalseLiteral:
            return true;
        default: {
            setFailed();
            return false;
        }
        }
    }

    bool visit([[maybe_unused]] QmlJS::AST::CallExpression *call) override
    {
        if (skipIt())
            return false;

        ++m_depth;
        return true;
    }

    bool visit([[maybe_unused]] QmlJS::AST::IdentifierExpression *identifier) override
    {
        if (skipIt())
            return false;

        ++m_depth;
        return false;
    }

    bool visit([[maybe_unused]] QmlJS::AST::FieldMemberExpression *field) override
    {
        if (skipIt())
            return false;

        ++m_depth;
        return true;
    }

    void endVisit([[maybe_unused]] QmlJS::AST::CallExpression *call) override
    {
        if (skipIt())
            return;

        checkAndResetCal();
    }

    void endVisit(QmlJS::AST::IdentifierExpression *identifier) override
    {
        if (skipIt())
            return;

        m_identifier = identifier->name.toString();
        checkAndResetNonCal();
    }

    void endVisit(QmlJS::AST::FieldMemberExpression *fieldExpression) override
    {
        if (skipIt())
            return;

        m_fields << fieldExpression->name.toString();
        checkAndResetNonCal();
    }

    void endVisit(QmlJS::AST::StringLiteral *stringLiteral) override
    {
        if (skipIt())
            return;
        m_rhs = stringLiteral->value.toString();
        m_specified = true;
    }

    void endVisit(QmlJS::AST::NumericLiteral *numericLiteral) override
    {
        if (skipIt())
            return;
        m_rhs = numericLiteral->value;
        m_specified = true;
    }

    void endVisit([[maybe_unused]] QmlJS::AST::TrueLiteral *trueLiteral) override
    {
        if (skipIt())
            return;
        m_rhs = true;
        m_specified = true;
    }

    void endVisit([[maybe_unused]] QmlJS::AST::FalseLiteral *falseLiteral) override
    {
        if (skipIt())
            return;
        m_rhs = false;
        m_specified = true;
    }

    void throwRecursionDepthError() override
    {
        setFailed();
        qDebug() << Q_FUNC_INFO << this;
    }

    void checkAndResetCal()
    {
        if (--m_depth == 0) {
            m_rhs = ConnectionEditorStatements::MatchedFunction{m_identifier, m_fields.join(".")};
            m_specified = true;

            m_identifier.clear();
            m_fields.clear();
        }
    }

    void checkAndResetNonCal()
    {
        if (--m_depth == 0) {
            m_rhs = ConnectionEditorStatements::Variable{m_identifier, m_fields.join(".")};
            m_specified = true;

            m_identifier.clear();
            m_fields.clear();
        }
    }

    bool skipIt() { return m_failed || m_specified; }

    void setFailed() { m_failed = true; }

private:
    bool m_failed = false;
    bool m_specified = false;
    int m_depth = 0;
    QString m_identifier;
    QStringList m_fields;
    ConnectionEditorStatements::RightHandSide m_rhs;
};

MatchedStatement checkForStateSet(const MatchedStatement &currentState)
{
    using namespace ConnectionEditorStatements;
    return std::visit(
        Overload{[](const PropertySet &propertySet) -> MatchedStatement {
                     if (propertySet.lhs.nodeId.size() && propertySet.lhs.propertyName == u"state"
                         && std::holds_alternative<QString>(propertySet.rhs))
                         return StateSet{propertySet.lhs.nodeId,
                                         ConnectionEditorStatements::toString(propertySet.rhs)};
                     return propertySet;
                 },
                 [](const auto &pSet) -> MatchedStatement { return pSet; }},
        currentState);
}

class ConsoleLogEvaluator : public QmlJS::AST::Visitor
{
public:
    bool isValid() { return m_completed; }

    ConnectionEditorStatements::ConsoleLog expression()
    {
        return ConnectionEditorStatements::ConsoleLog{m_arg};
    }

protected:
    bool preVisit(QmlJS::AST::Node *node) override
    {
        if (m_failed)
            return false;

        switch (m_stepId) {
        case 0: {
            if (node->kind != Kind::Kind_CallExpression)
                m_failed = true;
        } break;
        case 1: {
            if (node->kind != Kind::Kind_FieldMemberExpression)
                m_failed = true;
        } break;
        case 2: {
            if (node->kind != Kind::Kind_IdentifierExpression)
                m_failed = true;
        } break;
        case 3: {
            if (node->kind != Kind::Kind_ArgumentList)
                m_failed = true;
        } break;
        }

        m_stepId++;
        if (m_failed || m_completed)
            return false;

        return true;
    }

    bool visit(QmlJS::AST::IdentifierExpression *identifier) override
    {
        if (m_completed)
            return true;

        const QStringView identifierName = identifier->name;
        if (identifierName != u"console") {
            m_failed = true;
            return false;
        }
        return true;
    }

    bool visit(QmlJS::AST::FieldMemberExpression *fieldExpression) override
    {
        if (m_completed)
            return true;

        const QStringView fieldName = fieldExpression->name;
        if (fieldName != u"log") {
            m_failed = true;
            return false;
        }
        return true;
    }

    bool visit(QmlJS::AST::ArgumentList *arguments) override
    {
        if (m_completed)
            return true;

        if (arguments->next) {
            m_failed = true;
            return false;
        }
        m_completed = true;
        RightHandVisitor rVis;
        arguments->expression->accept(&rVis);
        m_arg = rVis.rhs();
        return true;
    }

    void throwRecursionDepthError() override
    {
        m_failed = true;
        qDebug() << Q_FUNC_INFO << this;
    }

private:
    bool m_failed = false;
    bool m_completed = false;
    int m_stepId = 0;
    ConnectionEditorStatements::RightHandSide m_arg;
};

struct StatementReply
{
    const TrackingArea area;

    MatchedStatement *operator()(MatchedStatement &handler) const { return &handler; }

    MatchedStatement *operator()(ConditionalStatement &handler) const
    {
        switch (area) {
        case TrackingArea::Ko:
            return &handler.ko;
        case TrackingArea::Ok:
            return &handler.ok;
        case TrackingArea::Condition:
        default:
            return nullptr;
        }
    }
};

} // namespace

class QmlDesigner::ConnectionEditorEvaluatorPrivate
{
    friend class ConnectionEditorEvaluator;
    using Status = ConnectionEditorEvaluator::Status;

public:
    bool checkValidityAndReturn(bool valid, const QString &parseError = {});

    void setStatus(Status status) { m_checkStatus = status; }

    NodeStatus parentNodeStatus() const { return nodeStatus(1); }

    NodeStatus nodeStatus(int reverseLevel = 0) const;

    TrackingArea trackingArea() const { return m_trackingArea; }

    void setTrackingArea(bool ifFound, int ifCurrentChildren)
    {
        if (!ifFound) {
            m_trackingArea = TrackingArea::No;
            return;
        }
        switch (ifCurrentChildren) {
        case 1:
            m_trackingArea = TrackingArea::Condition;
            break;
        case 2:
            m_trackingArea = TrackingArea::Ok;
            break;
        case 3:
            m_trackingArea = TrackingArea::Ko;
            break;
        default:
            m_trackingArea = TrackingArea::No;
        }
    }

    int childLevelOfTheClosestItem(Kind kind)
    {
        for (const NodeStatus &status : m_nodeHierarchy | Utils::views::reverse) {
            if (status == kind)
                return status.childId();
        }
        return -1;
    }

    bool isInIfCondition() const { return m_trackingArea == TrackingArea::Condition; }

    MatchedStatement *currentStatement()
    {
        return std::visit(StatementReply{m_trackingArea}, m_handler);
    }

    MatchedCondition *currentCondition()
    {
        if (auto handler = std::get_if<ConditionalStatement>(&m_handler))
            return &handler->condition;
        return nullptr;
    }

    void addVariableCondition(Node *node)
    {
        if (MatchedCondition *currentCondition = this->currentCondition()) {
            if (currentCondition->statements.isEmpty()) {
                RightHandVisitor varVisitor;
                node->accept(&varVisitor);
                if (varVisitor.couldBeVariable())
                    currentCondition->statements.append(varVisitor.variable());
            }
        }
    }

private:
    int m_ifStatement = 0;
    int m_consoleLogCount = 0;
    int m_consoleIdentifierCount = 0;
    bool m_acceptLogArgument = false;
    TrackingArea m_trackingArea = TrackingArea::No;
    QString m_errorString;
    Status m_checkStatus = Status::UnStarted;
    QList<NodeStatus> m_nodeHierarchy;
    ConnectionEditorStatements::Handler m_handler;
};

ConnectionEditorEvaluator::ConnectionEditorEvaluator()
    : d(std::make_unique<ConnectionEditorEvaluatorPrivate>())
{}

ConnectionEditorEvaluator::~ConnectionEditorEvaluator() {}

ConnectionEditorEvaluator::Status ConnectionEditorEvaluator::status() const
{
    return d->m_checkStatus;
}

ConnectionEditorStatements::Handler ConnectionEditorEvaluator::resultNode() const
{
    return (d->m_checkStatus == Succeeded) ? d->m_handler : ConnectionEditorStatements::EmptyBlock{};
}

QString ConnectionEditorEvaluator::getDisplayStringForType(const QString &statement)
{
    ConnectionEditorEvaluator evaluator;
    QmlJS::Document::MutablePtr newDoc = QmlJS::Document::create(Utils::FilePath::fromString(
                                                                     "<expression>"),
                                                                 QmlJS::Dialect::JavaScript);

    newDoc->setSource(statement);
    newDoc->parseJavaScript();

    if (!newDoc->isParsedCorrectly())
        return ConnectionEditorStatements::CUSTOM_DISPLAY_NAME;

    newDoc->ast()->accept(&evaluator);

    const bool valid = evaluator.status() == ConnectionEditorEvaluator::Succeeded;

    if (!valid)
        return ConnectionEditorStatements::CUSTOM_DISPLAY_NAME;

    auto result = evaluator.resultNode();

    return QmlDesigner::ConnectionEditorStatements::toDisplayName(result);
}

ConnectionEditorStatements::Handler ConnectionEditorEvaluator::parseStatement(const QString &statement)
{
    ConnectionEditorEvaluator evaluator;
    QmlJS::Document::MutablePtr newDoc = QmlJS::Document::create(Utils::FilePath::fromString(
                                                                     "<expression>"),
                                                                 QmlJS::Dialect::JavaScript);

    newDoc->setSource(statement);
    newDoc->parseJavaScript();

    if (!newDoc->isParsedCorrectly())
        return ConnectionEditorStatements::EmptyBlock{};

    newDoc->ast()->accept(&evaluator);

    const bool valid = evaluator.status() == ConnectionEditorEvaluator::Succeeded;

    if (!valid)
        return ConnectionEditorStatements::EmptyBlock{};

    return evaluator.resultNode();
}

bool ConnectionEditorEvaluator::preVisit(Node *node)
{
    if (d->m_nodeHierarchy.size()) {
        NodeStatus &parentNode = d->m_nodeHierarchy.last();
        parentNode.increaseChildNo();
        if (parentNode == Kind::Kind_IfStatement)
            d->setTrackingArea(true, parentNode.children());
    }

    d->m_nodeHierarchy.append(node);

    switch (node->kind) {
    case Kind::Kind_Program:
        return true;
    case Kind::Kind_StatementList:
    case Kind::Kind_IfStatement:
    case Kind::Kind_Block:
    case Kind::Kind_IdentifierExpression:
    case Kind::Kind_BinaryExpression:
    case Kind::Kind_FieldMemberExpression:
    case Kind::Kind_ExpressionStatement:
    case Kind::Kind_Expression:
    case Kind::Kind_CallExpression:
    case Kind::Kind_TrueLiteral:
    case Kind::Kind_FalseLiteral:
    case Kind::Kind_StringLiteral:
    case Kind::Kind_NumericLiteral:
    case Kind::Kind_ArgumentList:
        return status() == UnFinished;
    default:
        return false;
    }
}

void ConnectionEditorEvaluator::postVisit(QmlJS::AST::Node *node)
{
    if (d->m_nodeHierarchy.isEmpty()) {
        d->checkValidityAndReturn(false, "Unexpected post visiting");
        return;
    }
    if (d->m_nodeHierarchy.last() == node->kind) {
        d->m_nodeHierarchy.removeLast();
    } else {
        d->checkValidityAndReturn(false, "Post visiting kind does not match");
        return;
    }

    if (node->kind == Kind::Kind_IfStatement) {
        bool ifFound = false;
        int closestIfChildren = 0;
        for (const NodeStatus &nodeStatus : d->m_nodeHierarchy | Utils::views::reverse) {
            if (nodeStatus == Kind::Kind_IfStatement) {
                ifFound = true;
                closestIfChildren = nodeStatus.children();
                break;
            }
        }
        d->setTrackingArea(ifFound, closestIfChildren);
    }
}

bool ConnectionEditorEvaluator::visit([[maybe_unused]] QmlJS::AST::Program *program)
{
    d->setStatus(UnFinished);
    d->setTrackingArea(false, 0);
    d->m_ifStatement = 0;
    d->m_consoleLogCount = 0;
    d->m_consoleIdentifierCount = 0;
    d->m_handler = ConnectionEditorStatements::EmptyBlock{};
    return true;
}

bool ConnectionEditorEvaluator::visit([[maybe_unused]] QmlJS::AST::IfStatement *ifStatement)
{
    if (d->m_ifStatement++)
        return d->checkValidityAndReturn(false, "Nested if conditions are not supported");

    if (ifStatement->ok->kind != Kind::Kind_Block)
        return d->checkValidityAndReturn(false, "True block should be in a curly bracket.");

    if (ifStatement->ko && ifStatement->ko->kind != Kind::Kind_Block)
        return d->checkValidityAndReturn(false, "False block should be in a curly bracket.");

    d->m_handler = ConditionalStatement{};
    return d->checkValidityAndReturn(true);
}

bool ConnectionEditorEvaluator::visit(QmlJS::AST::IdentifierExpression *identifier)
{
    if (d->parentNodeStatus() == Kind::Kind_FieldMemberExpression)
        if (d->m_consoleLogCount)
            d->m_consoleIdentifierCount++;

    d->addVariableCondition(identifier);

    return d->checkValidityAndReturn(true);
}

bool ConnectionEditorEvaluator::visit(QmlJS::AST::BinaryExpression *binaryExpression)
{
    if (d->isInIfCondition()) {
        if (binaryExpression->op == QSOperator::Assign)
            return d->checkValidityAndReturn(false, "Assignment as a condition.");

        if (!isAcceptedIfBinaryOperator(binaryExpression->op))
            return d->checkValidityAndReturn(false, "Operator is not supported.");

        MatchedCondition *matchedCondition = d->currentCondition();
        if (!matchedCondition)
            return d->checkValidityAndReturn(false, "Matched condition is not available.");

        BoolCondition conditionVisitor;
        binaryExpression->accept(&conditionVisitor);

        if (conditionVisitor.isValid()) {
            *matchedCondition = conditionVisitor.matchedCondition();
        } else {
            return d->checkValidityAndReturn(false,
                                             "Matched condition is not valid. \""
                                                 + conditionVisitor.errorString() + "\"");
        }

        return false;
    } else {
        MatchedStatement *currentStatement = d->currentStatement();
        if (currentStatement && ConnectionEditorStatements::isEmptyStatement(*currentStatement)
            && d->parentNodeStatus().childId() == 0) {
            if (binaryExpression->op == QSOperator::Assign) {
                RightHandVisitor variableVisitor;
                binaryExpression->left->accept(&variableVisitor);

                if (!variableVisitor.couldBeLHS())
                    return d->checkValidityAndReturn(false, "Invalid left hand.");

                ConnectionEditorStatements::Variable lhs = variableVisitor.lhs();

                variableVisitor.reset();
                binaryExpression->right->accept(&variableVisitor);

                if (variableVisitor.couldBeLHS()) {
                    ConnectionEditorStatements::Assignment assignment{lhs, variableVisitor.variable()};
                    *currentStatement = assignment;
                } else if (variableVisitor.isLiteralType()) {
                    ConnectionEditorStatements::PropertySet propSet{lhs, variableVisitor.literal()};
                    *currentStatement = propSet;
                } else {
                    return d->checkValidityAndReturn(false, "Invalid RHS");
                }

                *currentStatement = checkForStateSet(*currentStatement);
            }
        }
    }

    return d->checkValidityAndReturn(true);
}

bool ConnectionEditorEvaluator::visit(QmlJS::AST::FieldMemberExpression *fieldExpression)
{
    if (d->parentNodeStatus() == Kind::Kind_CallExpression && fieldExpression->name == u"log")
        d->m_consoleLogCount++;

    d->addVariableCondition(fieldExpression);

    return d->checkValidityAndReturn(true);
}

bool ConnectionEditorEvaluator::visit(QmlJS::AST::CallExpression *callExpression)
{
    if (d->isInIfCondition())
        return d->checkValidityAndReturn(false, "Functions are not allowd in the expressions");

    MatchedStatement *currentStatement = d->currentStatement();
    if (!currentStatement)
        return d->checkValidityAndReturn(false, "Invalid place to call an expression");

    if (ConnectionEditorStatements::isEmptyStatement(*currentStatement)) {
        if (d->parentNodeStatus().childId() == 0) {
            ConsoleLogEvaluator logEvaluator;
            callExpression->accept(&logEvaluator);
            if (logEvaluator.isValid()) {
                *currentStatement = logEvaluator.expression(); // Console Log
            } else {
                RightHandVisitor callVisitor;

                callExpression->accept(&callVisitor);
                if (callVisitor.isValid()) {
                    ConnectionEditorStatements::RightHandSide rhs = callVisitor.rhs();
                    if (auto rhs_ = std::get_if<ConnectionEditorStatements::MatchedFunction>(&rhs))
                        *currentStatement = *rhs_;
                    else
                        return d->checkValidityAndReturn(false, "Invalid Matched Function type.");
                } else {
                    return d->checkValidityAndReturn(false, "Invalid Matched Function");
                }
            }
        }
    }
    return d->checkValidityAndReturn(true);
}

bool ConnectionEditorEvaluator::visit([[maybe_unused]] QmlJS::AST::Block *block)
{
    Kind parentKind = d->parentNodeStatus();

    if (parentKind == Kind::Kind_IfStatement) {
        return d->checkValidityAndReturn(true);
    } else if (d->parentNodeStatus() == Kind::Kind_StatementList) {
        if (d->nodeStatus(2) == Kind::Kind_Program)
            return d->checkValidityAndReturn(true);
    }

    return d->checkValidityAndReturn(false, "Block count ptoblem");
}

bool ConnectionEditorEvaluator::visit(QmlJS::AST::ArgumentList *arguments)
{
    if (d->trackingArea() == TrackingArea::Condition)
        return d->checkValidityAndReturn(false, "Arguments are not supported in if condition");

    auto currentStatement = d->currentStatement();
    if (!currentStatement)
        return d->checkValidityAndReturn(false, "No statement found for argument");

    if (!ConnectionEditorStatements::isConsoleLog(*currentStatement))
        return d->checkValidityAndReturn(false, "Arguments are only supported for console.log");

    if (d->m_acceptLogArgument && !arguments->next)
        return d->checkValidityAndReturn(true);

    return d->checkValidityAndReturn(false, "The only supported argument is in console.log");
}

void ConnectionEditorEvaluator::endVisit([[maybe_unused]] QmlJS::AST::Program *program)
{
    if (status() == UnFinished)
        d->setStatus(Succeeded);
}

void ConnectionEditorEvaluator::endVisit(QmlJS::AST::FieldMemberExpression *fieldExpression)
{
    if (status() != UnFinished)
        return;

    if (fieldExpression->name == u"log") {
        if (d->m_consoleIdentifierCount != d->m_consoleLogCount) {
            d->m_acceptLogArgument = false;
        } else {
            d->m_consoleIdentifierCount--;
            d->m_acceptLogArgument = true;
        }
        --(d->m_consoleLogCount);
    }
}

void ConnectionEditorEvaluator::endVisit([[maybe_unused]] QmlJS::AST::CallExpression *callExpression)
{
    d->m_acceptLogArgument = false;
}

void ConnectionEditorEvaluator::endVisit(QmlJS::AST::IfStatement * /*ifStatement*/)
{
    if (status() != UnFinished)
        return;

    if (auto statement = std::get_if<ConditionalStatement>(&d->m_handler)) {
        if (!isEmptyStatement(statement->ok) && !isEmptyStatement(statement->ko)) {
            if (statement->ok.index() != statement->ko.index())
                d->checkValidityAndReturn(false, "Matched statements types are mismatched");
        }
    }
}

void ConnectionEditorEvaluator::endVisit(QmlJS::AST::StatementList * /*statementList*/)
{
    if (status() != UnFinished)
        return;

    if (d->nodeStatus().children() > 1)
        d->checkValidityAndReturn(false, "More than one statements are available.");
}

void ConnectionEditorEvaluator::throwRecursionDepthError()
{
    d->checkValidityAndReturn(false, "Recursion depth problem");
    qDebug() << Q_FUNC_INFO << this;
}

bool ConnectionEditorEvaluatorPrivate::checkValidityAndReturn(bool valid, const QString &parseError)
{
    if (!valid) {
        if (m_checkStatus != Status::Failed) {
            setStatus(Status::Failed);
            m_errorString = parseError;
            qDebug() << Q_FUNC_INFO << "Error: " << parseError;
        }
    }

    return m_checkStatus;
}

NodeStatus ConnectionEditorEvaluatorPrivate::nodeStatus(int reverseLevel) const
{
    if (m_nodeHierarchy.size() > reverseLevel)
        return m_nodeHierarchy.at(m_nodeHierarchy.size() - reverseLevel - 1);
    return {};
}
