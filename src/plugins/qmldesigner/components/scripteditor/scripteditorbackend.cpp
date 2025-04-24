// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scripteditorbackend.h"
#include "scripteditorevaluator.h"
#include "scripteditorutils.h"

#include <abstractview.h>
#include <functional.h>
#include <indentingtexteditormodifier.h>
#include <modelnodeoperations.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <selectioncontext.h>

#include <QMessageBox>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>

namespace {
const char defaultCondition[] = "condition";

QString getSourceFromProperty(const QmlDesigner::AbstractProperty &property)
{
    QTC_ASSERT(property.isValid(), return {});

    if (!property.exists())
        return {};

    if (property.isSignalHandlerProperty())
        return property.toSignalHandlerProperty().source();
    if (property.isBindingProperty())
        return property.toBindingProperty().expression();

    return {};
}
} // namespace

namespace QmlDesigner {
static ScriptEditorStatements::MatchedCondition emptyCondition;

ConditionListModel::ConditionListModel(AbstractView *view)
    : m_view(view)
    , m_condition(emptyCondition)
{}

int ConditionListModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_tokens.size();
}

QHash<int, QByteArray> ConditionListModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{Qt::UserRole + 1, "type"}, {Qt::UserRole + 2, "value"}};
    return roleNames;
}

QVariant ConditionListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {
        if (role == Qt::UserRole + 1) {
            return m_tokens.at(index.row()).type;
        } else if (role == Qt::UserRole + 2) {
            return m_tokens.at(index.row()).value;
        }

        qCWarning(ScriptEditorLog) << __FUNCTION__ << "invalid role";
    } else {
        qCWarning(ScriptEditorLog) << __FUNCTION__ << "invalid index";
    }

    return QVariant();
}

void ConditionListModel::setup()
{
    m_tokens.clear();

    internalSetup();

    emit validChanged();
    emit emptyChanged();

    beginResetModel();
    endResetModel();
}

void ConditionListModel::setCondition(ScriptEditorStatements::MatchedCondition &condition)
{
    m_condition = condition;
    setup();
}

ScriptEditorStatements::MatchedCondition &ConditionListModel::condition()
{
    return m_condition;
}

ConditionListModel::ConditionToken ConditionListModel::tokenFromConditionToken(
    const ScriptEditorStatements::ConditionToken &token)
{
    ConditionToken ret;
    ret.type = Operator;
    ret.value = ScriptEditorStatements::toJavascript(token);

    return ret;
}

ConditionListModel::ConditionToken ConditionListModel::tokenFromComparativeStatement(
    const ScriptEditorStatements::ComparativeStatement &token)
{
    ConditionToken ret;

    if (auto *variable = std::get_if<ScriptEditorStatements::Variable>(&token)) {
        ret.type = Variable;
        ret.value = variable->expression();
        return ret;
    } else if (auto *literal = std::get_if<QString>(&token)) {
        ret.type = Literal;
        ret.value = "\"" + *literal + "\"";
        return ret;
    } else if (auto *literal = std::get_if<bool>(&token)) {
        ret.type = Literal;
        if (*literal)
            ret.value = "true";
        else
            ret.value = "false";
        return ret;
    } else if (auto *literal = std::get_if<double>(&token)) {
        ret.type = Literal;
        ret.value = QString::number(*literal);
        return ret;
    }

    ret.type = Invalid;
    ret.value = "invalid";
    return {};
}

void ConditionListModel::insertToken(int index, const QString &value)
{
    beginInsertRows({}, index, index);

    m_tokens.insert(index, valueToToken(value));
    validateAndRebuildTokens();

    endInsertRows();
}

void ConditionListModel::updateToken(int index, const QString &value)
{
    m_tokens[index] = valueToToken(value);
    validateAndRebuildTokens();

    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
}

void ConditionListModel::appendToken(const QString &value)
{
    beginInsertRows({}, rowCount() - 1, rowCount() - 1);

    insertToken(rowCount(), value);
    validateAndRebuildTokens();

    endInsertRows();
}

void ConditionListModel::removeToken(int index)
{
    QTC_ASSERT(index < m_tokens.count(), return);
    beginRemoveRows({}, index, index);

    m_tokens.remove(index, 1);
    validateAndRebuildTokens();

    endRemoveRows();
}

void ConditionListModel::insertIntermediateToken(int index, const QString &value)
{
    beginInsertRows({}, index, index);

    ConditionToken token;
    token.type = Intermediate;
    token.value = value;

    m_tokens.insert(index, token);

    endInsertRows();
}

void ConditionListModel::insertShadowToken(int index, const QString &value)
{
    beginInsertRows({}, index, index);

    ConditionToken token;
    token.type = Shadow;
    token.value = value;

    m_tokens.insert(index, token);

    endInsertRows();
}

void ConditionListModel::setShadowToken(int index, const QString &value)
{
    m_tokens[index].type = Shadow;
    m_tokens[index].value = value;

    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
}

bool ConditionListModel::valid() const
{
    return m_valid;
}

bool ConditionListModel::empty() const
{
    return m_tokens.isEmpty();
}

void ConditionListModel::command(const QString &string)
{
    //TODO remove from prodcution code
    QStringList list = string.split("%", Qt::SkipEmptyParts);

    if (list.size() < 2)
        return;

    if (list.size() == 2) {
        if (list.first() == "A") {
            appendToken(list.last());
        } else if (list.first() == "R") {
            bool ok = true;
            int index = list.last().toInt(&ok);

            if (ok)
                removeToken(index);
        }
    }

    if (list.size() == 3) {
        if (list.first() == "U") {
            bool ok = true;
            int index = list.at(1).toInt(&ok);

            if (ok)
                updateToken(index, list.last());
        } else if (list.first() == "I") {
            bool ok = true;
            int index = list.at(1).toInt(&ok);

            if (ok)
                insertToken(index, list.last());
        }
    }
}

void ConditionListModel::setInvalid(const QString &errorMessage, int index)
{
    m_valid = false;
    m_errorMessage = errorMessage;

    emit errorChanged();
    emit validChanged();

    if (index != -1) {
        m_errorIndex = index;
        emit errorIndexChanged();
    }
}

void ConditionListModel::setValid()
{
    m_valid = true;
    m_errorMessage.clear();
    m_errorIndex = -1;

    emit errorChanged();
    emit validChanged();
    emit errorIndexChanged();
}

QString ConditionListModel::error() const
{
    return m_errorMessage;
}

int ConditionListModel::errorIndex() const
{
    return m_errorIndex;
}

bool ConditionListModel::operatorAllowed(int cursorPosition)
{
    if (m_tokens.empty())
        return false;

    int tokenIdx = cursorPosition - 1;

    if (tokenIdx >= 0 && tokenIdx < m_tokens.length() && m_tokens[tokenIdx].type != Operator)
        return true;

    return false;
}

void ConditionListModel::internalSetup()
{
    setInvalid(tr("No Valid Condition"));
    if (!m_condition.statements.size() && !m_condition.tokens.size())
        return;

    if (m_condition.statements.size() != m_condition.tokens.size() + 1)
        return;

    if (m_condition.statements.size() == 1 && m_condition.tokens.isEmpty()) {
        auto token = tokenFromComparativeStatement(m_condition.statements.first());
        if (token.value == defaultCondition)
            return;
    }

    auto s_it = m_condition.statements.begin();
    auto o_it = m_condition.tokens.begin();

    while (o_it != m_condition.tokens.end()) {
        m_tokens.append(tokenFromComparativeStatement(*s_it));
        m_tokens.append(tokenFromConditionToken(*o_it));

        s_it++;
        o_it++;
    }
    m_tokens.append(tokenFromComparativeStatement(*s_it));

    setValid();
}

ConditionListModel::ConditionToken ConditionListModel::valueToToken(const QString &value)
{
    const QStringList operators = {"&&", "||", "===", "!==", ">", ">=", "<", "<="};

    if (operators.contains(value)) {
        ConditionToken token;
        token.type = Operator;
        token.value = value;
        return token;
    }

    bool ok = false;
    value.toDouble(&ok);

    if (value == "true" || value == "false" || ok || (value.startsWith("\"") && value.endsWith("\""))) {
        ConditionToken token;
        token.type = Literal;
        token.value = value;
        return token;
    }

    static QRegularExpression regexp("^[a-z_]\\w*|^[A-Z]\\w*\\.{1}([a-z_]\\w*\\.?)+");
    QRegularExpressionMatch match = regexp.match(value);

    if (match.hasMatch()) { //variable
        ConditionToken token;
        token.type = Variable;
        token.value = value;
        return token;
    }

    ConditionToken token;
    token.type = Invalid;
    token.value = value;

    return token;
}

void ConditionListModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

int ConditionListModel::checkOrder() const
{
    auto it = m_tokens.begin();

    bool wasOperator = true;

    int ret = 0;
    while (it != m_tokens.end()) {
        if (wasOperator && it->type == Operator)
            return ret;
        if (!wasOperator && it->type == Literal)
            return ret;
        if (!wasOperator && it->type == Variable)
            return ret;
        wasOperator = it->type == Operator;
        it++;
        ret++;
    }

    if (wasOperator)
        return ret;

    return -1;
}

void ConditionListModel::validateAndRebuildTokens()
{
    /// NEW
    auto it = m_tokens.begin();

    while (it != m_tokens.end()) {
        if (it->type == Intermediate)
            *it = valueToToken(it->value);

        it++;
    }
    // NEW

    QString invalidValue;
    const bool invalidToken = Utils::contains(m_tokens, [&invalidValue](const ConditionToken &token) {
        if (token.type == Invalid)
            invalidValue = token.value;
        return token.type == Invalid;
    });

    if (invalidToken) {
        setInvalid(tr("Invalid token %1").arg(invalidValue));
        return;
    }

    if (int firstError = checkOrder() != -1) {
        setInvalid(tr("Invalid order at %1").arg(firstError), firstError);
        return;
    }

    setValid();

    rebuildTokens();
}

ScriptEditorStatements::ConditionToken ConditionListModel::toOperatorStatement(const ConditionToken &token)
{
    if (token.value == "&&")
        return ScriptEditorStatements::ConditionToken::And;

    if (token.value == "||")
        return ScriptEditorStatements::ConditionToken::Or;

    if (token.value == "===")
        return ScriptEditorStatements::ConditionToken::Equals;

    if (token.value == "!==")
        return ScriptEditorStatements::ConditionToken::Not;

    if (token.value == ">")
        return ScriptEditorStatements::ConditionToken::LargerThan;

    if (token.value == ">=")
        return ScriptEditorStatements::ConditionToken::LargerEqualsThan;

    if (token.value == "<")
        return ScriptEditorStatements::ConditionToken::SmallerThan;

    if (token.value == "<=")
        return ScriptEditorStatements::ConditionToken::SmallerEqualsThan;

    return ScriptEditorStatements::ConditionToken::Unknown;
}

static ScriptEditorStatements::ComparativeStatement parseTextArgumentComparativeStatement(
    const QString &text)
{
    if (text.startsWith("\"") && text.endsWith("\"")) {
        QString ret = text;
        ret.remove(0, 1);
        ret.chop(1);
        return ret;
    }

    if (text == "true")
        return true;

    if (text == "false")
        return false;

    bool ok = true;
    double d = text.toDouble(&ok);
    if (ok)
        return d;

    return text;
}

ScriptEditorStatements::ComparativeStatement ConditionListModel::toStatement(const ConditionToken &token)
{
    if (token.type == Variable) {
        QStringList list = token.value.split(".");
        ScriptEditorStatements::Variable variable;

        variable.nodeId = list.first();
        if (list.count() > 1)
            variable.propertyName = list.last();
        return variable;
    } else if (token.type == Literal) {
        return parseTextArgumentComparativeStatement(token.value);
    }

    return {};
}

void ConditionListModel::rebuildTokens()
{
    QTC_ASSERT(m_valid, return);

    m_condition.statements.clear();
    m_condition.tokens.clear();

    auto it = m_tokens.begin();

    while (it != m_tokens.end()) {
        QTC_ASSERT(it->type != Invalid, return);
        if (it->type == Operator)
            m_condition.tokens.append(toOperatorStatement(*it));
        else if (it->type == Literal || it->type == Variable)
            m_condition.statements.append(toStatement(*it));

        it++;
    }

    emit conditionChanged();
}

static ScriptEditorStatements::MatchedStatement emptyStatement;

StatementDelegate::StatementDelegate(AbstractView *view)
    : m_functionDelegate(view)
    , m_lhsDelegate(view)
    , m_rhsAssignmentDelegate(view)
    , m_statement(emptyStatement)
    , m_view(view)
{
    m_functionDelegate.setPropertyType(PropertyTreeModel::SlotType);

    connect(&m_functionDelegate, &PropertyTreeModelDelegate::commitData, this, [this] {
        handleFunctionChanged();
    });

    connect(&m_rhsAssignmentDelegate, &PropertyTreeModelDelegate::commitData, this, [this] {
        handleRhsAssignmentChanged();
    });

    connect(&m_lhsDelegate, &PropertyTreeModelDelegate::commitData, this, [this] {
        handleLhsChanged();
    });

    connect(&m_stringArgument, &StudioQmlTextBackend::activated, this, [this] {
        handleStringArgumentChanged();
    });

    connect(&m_states, &StudioQmlComboBoxBackend::activated, this, [this] { handleStateChanged(); });

    connect(&m_stateTargets, &StudioQmlComboBoxBackend::activated, this, [this] {
        handleStateTargetsChanged();
    });
}

void StatementDelegate::setActionType(ActionType type)
{
    if (m_actionType == type)
        return;

    m_actionType = type;
    emit actionTypeChanged();
    setup();
}

void StatementDelegate::setup()
{
    switch (m_actionType) {
    case CallFunction:
        setupCallFunction();
        break;
    case Assign:
        setupAssignment();
        break;
    case ChangeState:
        setupChangeState();
        break;
    case SetProperty:
        setupSetProperty();
        break;
    case PrintMessage:
        setupPrintMessage();
        break;
    case Custom:
        break;
    };
}

void StatementDelegate::setStatement(ScriptEditorStatements::MatchedStatement &statement)
{
    m_statement = statement;
    setup();
}

ScriptEditorStatements::MatchedStatement &StatementDelegate::statement()
{
    return m_statement;
}

StatementDelegate::ActionType StatementDelegate::actionType() const
{
    return m_actionType;
}

PropertyTreeModelDelegate *StatementDelegate::function()
{
    return &m_functionDelegate;
}

PropertyTreeModelDelegate *StatementDelegate::lhs()
{
    return &m_lhsDelegate;
}

PropertyTreeModelDelegate *StatementDelegate::rhsAssignment()
{
    return &m_rhsAssignmentDelegate;
}

StudioQmlTextBackend *StatementDelegate::stringArgument()
{
    return &m_stringArgument;
}

StudioQmlComboBoxBackend *StatementDelegate::stateTargets()
{
    return &m_stateTargets;
}

StudioQmlComboBoxBackend *StatementDelegate::states()
{
    return &m_states;
}

void StatementDelegate::handleFunctionChanged()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::MatchedFunction>(m_statement), return);

    ScriptEditorStatements::MatchedFunction &functionStatement = std::get<ScriptEditorStatements::MatchedFunction>(
        m_statement);

    functionStatement.functionName = m_functionDelegate.name();
    functionStatement.nodeId = m_functionDelegate.id();

    emit statementChanged();
}

void StatementDelegate::handleLhsChanged()
{
    if (m_actionType == Assign) {
        QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::Assignment>(m_statement), return);

        ScriptEditorStatements::Assignment &assignmentStatement = std::get<ScriptEditorStatements::Assignment>(
            m_statement);

        assignmentStatement.lhs.nodeId = m_lhsDelegate.id();
        assignmentStatement.lhs.propertyName = m_lhsDelegate.name();

    } else if (m_actionType == SetProperty) {
        QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::PropertySet>(m_statement), return);

        ScriptEditorStatements::PropertySet &setPropertyStatement = std::get<ScriptEditorStatements::PropertySet>(
            m_statement);

        setPropertyStatement.lhs.nodeId = m_lhsDelegate.id();
        setPropertyStatement.lhs.propertyName = m_lhsDelegate.name();
    } else {
        QTC_ASSERT(false, return);
    }

    emit statementChanged();
}

void StatementDelegate::handleRhsAssignmentChanged()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::Assignment>(m_statement), return);

    ScriptEditorStatements::Assignment &assignmentStatement = std::get<ScriptEditorStatements::Assignment>(
        m_statement);

    assignmentStatement.rhs.nodeId = m_rhsAssignmentDelegate.id();
    assignmentStatement.rhs.propertyName = m_rhsAssignmentDelegate.name();

    setupPropertyType();

    emit statementChanged();
}

static ScriptEditorStatements::Literal parseTextArgument(const QString &text)
{
    if (text.startsWith("\"") && text.endsWith("\"")) {
        QString ret = text;
        ret.remove(0, 1);
        ret.chop(1);
        return ret;
    }

    if (text == "true")
        return true;

    if (text == "false")
        return false;

    bool ok = true;
    double d = text.toDouble(&ok);
    if (ok)
        return d;

    return text;
}

static ScriptEditorStatements::RightHandSide parseLogTextArgument(const QString &text)
{
    if (text.startsWith("\"") && text.endsWith("\"")) {
        QString ret = text;
        ret.remove(0, 1);
        ret.chop(1);
        return ret;
    }

    if (text == "true")
        return true;

    if (text == "false")
        return true;

    bool ok = true;
    double d = text.toDouble(&ok);
    if (ok)
        return d;

    //TODO variables and function calls
    return text;
}

void StatementDelegate::handleStringArgumentChanged()
{
    if (m_actionType == SetProperty) {
        QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::PropertySet>(m_statement), return);

        ScriptEditorStatements::PropertySet &propertySet = std::get<ScriptEditorStatements::PropertySet>(
            m_statement);

        propertySet.rhs = parseTextArgument(m_stringArgument.text());

    } else if (m_actionType == PrintMessage) {
        QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::ConsoleLog>(m_statement), return);

        ScriptEditorStatements::ConsoleLog &consoleLog = std::get<ScriptEditorStatements::ConsoleLog>(
            m_statement);

        consoleLog.argument = parseLogTextArgument(m_stringArgument.text());
    } else {
        QTC_ASSERT(false, return);
    }

    emit statementChanged();
}

void StatementDelegate::handleStateChanged()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::StateSet>(m_statement), return);

    ScriptEditorStatements::StateSet &stateSet = std::get<ScriptEditorStatements::StateSet>(
        m_statement);

    QString stateName = m_states.currentText();
    if (stateName == baseStateName())
        stateName = "";
    stateSet.stateName = "\"" + stateName + "\"";

    emit statementChanged();
}

void StatementDelegate::handleStateTargetsChanged()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::StateSet>(m_statement), return);

    ScriptEditorStatements::StateSet &stateSet = std::get<ScriptEditorStatements::StateSet>(
        m_statement);

    stateSet.nodeId = m_stateTargets.currentText();
    stateSet.stateName = "\"\"";

    setupStates();

    emit statementChanged();
}

void StatementDelegate::setupAssignment()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::Assignment>(m_statement), return);

    const auto assignment = std::get<ScriptEditorStatements::Assignment>(m_statement);
    m_lhsDelegate.setup(assignment.lhs.nodeId, assignment.lhs.propertyName);
    m_rhsAssignmentDelegate.setup(assignment.rhs.nodeId, assignment.rhs.propertyName);
    setupPropertyType();
}

void StatementDelegate::setupSetProperty()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::PropertySet>(m_statement), return);

    const auto propertySet = std::get<ScriptEditorStatements::PropertySet>(m_statement);
    m_lhsDelegate.setup(propertySet.lhs.nodeId, propertySet.lhs.propertyName);
    m_stringArgument.setText(ScriptEditorStatements::toString(propertySet.rhs));
}

void StatementDelegate::setupCallFunction()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::MatchedFunction>(m_statement), return);

    const auto functionStatement = std::get<ScriptEditorStatements::MatchedFunction>(m_statement);
    m_functionDelegate.setup(functionStatement.nodeId, functionStatement.functionName);
}

void StatementDelegate::setupChangeState()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::StateSet>(m_statement), return);

    QTC_ASSERT(m_view->isAttached(), return);

    auto model = m_view->model();
    const auto items = Utils::filtered(m_view->allModelNodesOfType(model->qtQuickItemMetaInfo()),
                                       [](const ModelNode &node) {
                                           QmlItemNode item(node);
                                           return node.hasId() && item.isValid()
                                                  && !item.allStateNames().isEmpty();
                                       });

    using SL = ModelTracing::SourceLocation;
    QStringList itemIds = Utils::transform(items, bind_back(&ModelNode::id, SL{}));
    const auto groups = m_view->allModelNodesOfType(model->qtQuickStateGroupMetaInfo());

    const auto rootId = m_view->rootModelNode().id();
    itemIds.removeAll(rootId);

    QStringList groupIds = Utils::transform(groups, bind_back(&ModelNode::id, SL{}));

    Utils::sort(itemIds);
    Utils::sort(groupIds);

    if (!rootId.isEmpty())
        groupIds.prepend(rootId);

    const QStringList stateGroupModel = groupIds + itemIds;
    m_stateTargets.setModel(stateGroupModel);

    const auto stateSet = std::get<ScriptEditorStatements::StateSet>(m_statement);

    m_stateTargets.setCurrentText(stateSet.nodeId);
    setupStates();
}

static QString stripQuotesFromState(const QString &input)
{
    if (input.startsWith("\"") && input.endsWith("\"")) {
        QString ret = input;
        ret.remove(0, 1);
        ret.chop(1);
        return ret;
    }
    return input;
}

void StatementDelegate::setupStates()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::StateSet>(m_statement), return);
    QTC_ASSERT(m_view->isAttached(), return);

    const auto stateSet = std::get<ScriptEditorStatements::StateSet>(m_statement);

    const QString nodeId = m_stateTargets.currentText();

    const ModelNode node = m_view->modelNodeForId(nodeId);

    QStringList states;
    if (node.metaInfo().isQtQuickItem()) {
        QmlItemNode item(node);
        QTC_ASSERT(item.isValid(), return);
        if (item.isRootNode())
            states = item.states().names(); //model
        else
            states = item.allStateNames(); //instances
    } else {
        QmlModelStateGroup group(node);
        states = group.names(); //model
    }

    const QString stateName = stripQuotesFromState(stateSet.stateName);

    states.prepend(baseStateName());
    m_states.setModel(states);
    if (stateName.isEmpty())
        m_states.setCurrentText(baseStateName());
    else
        m_states.setCurrentText(stateName);
}

void StatementDelegate::setupPrintMessage()
{
    QTC_ASSERT(std::holds_alternative<ScriptEditorStatements::ConsoleLog>(m_statement), return);

    const auto consoleLog = std::get<ScriptEditorStatements::ConsoleLog>(m_statement);
    m_stringArgument.setText(ScriptEditorStatements::toString(consoleLog.argument));
}

void StatementDelegate::setupPropertyType()
{
    PropertyTreeModel::PropertyTypes type = PropertyTreeModel::AllTypes;

    const NodeMetaInfo metaInfo = m_rhsAssignmentDelegate.propertyMetaInfo();

    if (metaInfo.isBool())
        type = PropertyTreeModel::BoolType;
    else if (metaInfo.isNumber())
        type = PropertyTreeModel::NumberType;
    else if (metaInfo.isColor())
        type = PropertyTreeModel::ColorType;
    else if (metaInfo.isString())
        type = PropertyTreeModel::StringType;
    else if (metaInfo.isUrl())
        type = PropertyTreeModel::UrlType;

    m_lhsDelegate.setPropertyType(type);
}

QString StatementDelegate::baseStateName() const
{
    return tr("Base State");
}

ScriptEditorBackend::ScriptEditorBackend(AbstractView *view)
    : m_okStatementDelegate(view)
    , m_koStatementDelegate(view)
    , m_conditionListModel(view)
    , m_propertyTreeModel(view)
    , m_propertyListProxyModel(&m_propertyTreeModel)
    , m_view(view)
{
    connect(&m_okStatementDelegate, &StatementDelegate::statementChanged, this, [this] {
        handleOkStatementChanged();
    });

    connect(&m_koStatementDelegate, &StatementDelegate::statementChanged, this, [this] {
        handleKOStatementChanged();
    });

    connect(&m_conditionListModel, &ConditionListModel::conditionChanged, this, [this] {
        handleConditionChanged();
    });
}

static QString generateDefaultStatement(ScriptEditorBackend::ActionType actionType,
                                        const QString &rootId)
{
    switch (actionType) {
    case StatementDelegate::CallFunction:
        return "Qt.quit()";
    case StatementDelegate::Assign:
        return QString("%1.visible = %1.visible").arg(rootId);
    case StatementDelegate::ChangeState:
        return QString("%1.state = \"\"").arg(rootId);
    case StatementDelegate::SetProperty:
        return QString("%1.visible = true").arg(rootId);
    case StatementDelegate::PrintMessage:
        return QString("console.log(\"test\")").arg(rootId);
    case StatementDelegate::Custom:
        return {};
    };

    return {};
}

void ScriptEditorBackend::changeActionType(ActionType actionType)
{
    QTC_ASSERT(actionType != StatementDelegate::Custom, return);

    AbstractView *view = m_view;

    QTC_ASSERT(view, return);
    QTC_ASSERT(view->isAttached(), return);

    view->executeInTransaction("ScriptEditorBackend::removeCondition", [&]() {
        ScriptEditorStatements::MatchedStatement &okStatement = ScriptEditorStatements::okStatement(
            m_handler);

        ScriptEditorStatements::MatchedStatement &koStatement = ScriptEditorStatements::koStatement(
            m_handler);

        koStatement = ScriptEditorStatements::EmptyBlock();

        //We expect a valid id on the root node
        const QString validId = view->rootModelNode().validId();
        QString statementSource = generateDefaultStatement(actionType, validId);

        auto tempHandler = ScriptEditorEvaluator::parseStatement(statementSource); //what's that?

        auto newOkStatement = ScriptEditorStatements::okStatement(tempHandler);

        QTC_ASSERT(!ScriptEditorStatements::isEmptyStatement(newOkStatement), return);

        okStatement = newOkStatement;

        QString newSource = ScriptEditorStatements::toJavascript(m_handler);

        setPropertySource(newSource);
    });

    setSource(getSourceFromProperty(getSourceProperty()));

    setupHandlerAndStatements();
    setupCondition();
}

void ScriptEditorBackend::addCondition()
{
    ScriptEditorStatements::MatchedStatement okStatement = ScriptEditorStatements::okStatement(
        m_handler);

    ScriptEditorStatements::MatchedCondition newCondition;

    ScriptEditorStatements::Variable variable;
    variable.nodeId = defaultCondition;
    newCondition.statements.append(variable);

    ScriptEditorStatements::ConditionalStatement conditionalStatement;

    conditionalStatement.ok = okStatement;
    conditionalStatement.condition = newCondition;

    m_handler = conditionalStatement;

    QString newSource = ScriptEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);

    setupHandlerAndStatements();
    setupCondition();
}

void ScriptEditorBackend::removeCondition()
{
    ScriptEditorStatements::MatchedStatement okStatement = ScriptEditorStatements::okStatement(
        m_handler);

    m_handler = okStatement;

    QString newSource = ScriptEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);

    setupHandlerAndStatements();
    setupCondition();
}

void ScriptEditorBackend::addElse()
{
    ScriptEditorStatements::MatchedStatement okStatement = ScriptEditorStatements::okStatement(
        m_handler);

    auto &condition = ScriptEditorStatements::conditionalStatement(m_handler);
    condition.ko = condition.ok;

    QString newSource = ScriptEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
    setupHandlerAndStatements();
}

void ScriptEditorBackend::removeElse()
{
    ScriptEditorStatements::MatchedStatement okStatement = ScriptEditorStatements::okStatement(
        m_handler);

    auto &condition = ScriptEditorStatements::conditionalStatement(m_handler);
    condition.ko = ScriptEditorStatements::EmptyBlock();

    QString newSource = ScriptEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
    setupHandlerAndStatements();
}

void ScriptEditorBackend::setNewSource(const QString &newSource)
{
    setSource(newSource);
    commitNewSource(newSource);
    setupHandlerAndStatements();
    setupCondition();
}

void ScriptEditorBackend::update()
{
    if (m_blockReflection)
        return;

    m_propertyTreeModel.resetModel();
    m_propertyListProxyModel.setRowAndInternalId(0, internalRootIndex);

    AbstractView *view = m_view;

    QTC_ASSERT(view, return);
    if (!view->isAttached())
        return;

    // setup print message as default action if property does not exists
    auto sourceProperty = getSourceProperty();
    if (sourceProperty.exists())
        setSource(getSourceFromProperty(sourceProperty));
    else
        setSource(QStringLiteral("console.log(\"%1\")").arg(sourceProperty.parentModelNode().id()));

    setupHandlerAndStatements();

    setupCondition();
}

void ScriptEditorBackend::jumpToCode()
{
    AbstractView *view = m_view;

    QTC_ASSERT(view, return);
    QTC_ASSERT(view->isAttached(), return);
    auto sourceProperty = getSourceProperty();

    ModelNodeOperations::jumpToCode(sourceProperty.parentModelNode());
}

void ScriptEditorBackend::registerDeclarativeType()
{
    qmlRegisterType<StatementDelegate>("ScriptEditorBackend", 1, 0, "StatementDelegate");
    qmlRegisterType<PropertyTreeModel>("ScriptEditorBackend", 1, 0, "PropertyTreeModel");
    qmlRegisterType<ConditionListModel>("ScriptEditorBackend", 1, 0, "ConditionListModel");
    qmlRegisterType<PropertyListProxyModel>("ScriptEditorBackend", 1, 0, "PropertyListProxyModel");

    QTC_ASSERT(!QmlDesignerPlugin::viewManager().views().isEmpty(), return);
    AbstractView *view = QmlDesignerPlugin::viewManager().views().first();

    qmlRegisterSingletonType<ScriptEditorBackend>("ScriptEditorBackend",
                                                  1,
                                                  0,
                                                  "ScriptEditorBackend",
                                                  [view](QQmlEngine *, QJSEngine *) {
                                                      return new ScriptEditorBackend(view);
                                                  });
}

bool ScriptEditorBackend::blockReflection() const
{
    return m_blockReflection;
}

BindingProperty ScriptEditorBackend::getBindingProperty() const
{
    AbstractView *view = m_view;

    QTC_ASSERT(view, return {});
    QTC_ASSERT(view->isAttached(), return {});

    return SelectionContext{view}.currentSingleSelectedNode().bindingProperty("script");
}

void ScriptEditorBackend::handleException()
{
    QMessageBox::warning(nullptr, tr("Error"), m_exceptionError);
}

bool ScriptEditorBackend::hasCondition() const
{
    return m_hasCondition;
}

bool ScriptEditorBackend::hasElse() const
{
    return m_hasElse;
}

void ScriptEditorBackend::setHasCondition(bool b)
{
    if (b == m_hasCondition)
        return;

    m_hasCondition = b;
    emit hasConditionChanged();
}

void ScriptEditorBackend::setHasElse(bool b)
{
    if (b == m_hasElse)
        return;

    m_hasElse = b;
    emit hasElseChanged();
}

ScriptEditorBackend::ActionType ScriptEditorBackend::actionType() const
{
    return m_actionType;
}

AbstractProperty ScriptEditorBackend::getSourceProperty() const
{
    return getBindingProperty();
}

StatementDelegate *ScriptEditorBackend::okStatement()
{
    return &m_okStatementDelegate;
}

StatementDelegate *ScriptEditorBackend::koStatement()
{
    return &m_koStatementDelegate;
}

ConditionListModel *ScriptEditorBackend::conditionListModel()
{
    return &m_conditionListModel;
}

QString ScriptEditorBackend::indentedSource() const
{
    if (m_source.isEmpty())
        return {};

    QTextDocument doc(m_source);
    IndentingTextEditModifier mod(&doc);

    mod.indent(0, m_source.length() - 1);
    return mod.text();
}

QString ScriptEditorBackend::source() const
{
    return m_source;
}

void ScriptEditorBackend::setSource(const QString &source)
{
    if (source == m_source)
        return;

    m_source = source;
    emit sourceChanged();
}

PropertyTreeModel *ScriptEditorBackend::propertyTreeModel()
{
    return &m_propertyTreeModel;
}

PropertyListProxyModel *ScriptEditorBackend::propertyListProxyModel()
{
    return &m_propertyListProxyModel;
}

void ScriptEditorBackend::setupCondition()
{
    auto &condition = ScriptEditorStatements::matchedCondition(m_handler);
    m_conditionListModel.setCondition(ScriptEditorStatements::matchedCondition(m_handler));
    setHasCondition(!condition.statements.isEmpty());
}

void ScriptEditorBackend::setupHandlerAndStatements()
{
    AbstractView *view = m_view;
    QTC_ASSERT(view, return);

    if (m_source.isEmpty()) {
        m_actionType = StatementDelegate::Custom;
        m_handler = ScriptEditorStatements::EmptyBlock();
    } else {
        m_handler = ScriptEditorEvaluator::parseStatement(m_source);
        const QString statementType = QmlDesigner::ScriptEditorStatements::toDisplayName(m_handler);
        if (statementType == ScriptEditorStatements::EMPTY_DISPLAY_NAME) {
            m_actionType = StatementDelegate::Custom;
        } else if (statementType == ScriptEditorStatements::ASSIGNMENT_DISPLAY_NAME) {
            m_actionType = StatementDelegate::Assign;
        } else if (statementType == ScriptEditorStatements::SETPROPERTY_DISPLAY_NAME) {
            m_actionType = StatementDelegate::SetProperty;
        } else if (statementType == ScriptEditorStatements::FUNCTION_DISPLAY_NAME) {
            m_actionType = StatementDelegate::CallFunction;
        } else if (statementType == ScriptEditorStatements::SETSTATE_DISPLAY_NAME) {
            m_actionType = StatementDelegate::ChangeState;
        } else if (statementType == ScriptEditorStatements::LOG_DISPLAY_NAME) {
            m_actionType = StatementDelegate::PrintMessage;
        } else {
            m_actionType = StatementDelegate::Custom;
        }
    }

    ScriptEditorStatements::MatchedStatement &okStatement = ScriptEditorStatements::okStatement(
        m_handler);
    m_okStatementDelegate.setStatement(okStatement);
    m_okStatementDelegate.setActionType(m_actionType);

    ScriptEditorStatements::MatchedStatement &koStatement = ScriptEditorStatements::koStatement(
        m_handler);

    if (!ScriptEditorStatements::isEmptyStatement(koStatement)) {
        m_koStatementDelegate.setStatement(koStatement);
        m_koStatementDelegate.setActionType(m_actionType);
    }

    setHasElse(!ScriptEditorStatements::isEmptyStatement(koStatement));

    emit actionTypeChanged();
}

void ScriptEditorBackend::handleOkStatementChanged()
{
    ScriptEditorStatements::MatchedStatement &okStatement = ScriptEditorStatements::okStatement(
        m_handler);

    okStatement = m_okStatementDelegate.statement(); //TODO why?

    QString newSource = ScriptEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
}

void ScriptEditorBackend::handleKOStatementChanged()
{
    ScriptEditorStatements::MatchedStatement &koStatement = ScriptEditorStatements::koStatement(
        m_handler);

    koStatement = m_koStatementDelegate.statement(); //TODO why?

    QString newSource = ScriptEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
}

void ScriptEditorBackend::handleConditionChanged()
{
    AbstractView *view = m_view;

    QTC_ASSERT(view, return);
    QTC_ASSERT(view->isAttached(), return);

    ScriptEditorStatements::MatchedCondition &condition = ScriptEditorStatements::matchedCondition(
        m_handler);
    condition = m_conditionListModel.condition(); //why?
    QString newSource = ScriptEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
}

void ScriptEditorBackend::setPropertySource(const QString &source)
{
    auto property = getSourceProperty();

    QTC_ASSERT(property.isValid(), return);

    if (source.isEmpty())
        return;

    auto normalizedSource = QmlDesigner::SignalHandlerProperty::normalizedSourceWithBraces(source);
    if (property.exists())
        property.toBindingProperty().setExpression(normalizedSource);
    else
        property.parentModelNode().bindingProperty(property.name()).setExpression(normalizedSource);
}

void ScriptEditorBackend::commitNewSource(const QString &source)
{
    AbstractView *view = m_view;

    QTC_ASSERT(view, return);
    QTC_ASSERT(view->isAttached(), return);

    m_blockReflection = true;
    view->executeInTransaction("ScriptEditorBackend::commitNewSource",
                               [&]() { setPropertySource(source); });
    setSource(getSourceFromProperty(getSourceProperty()));
    m_blockReflection = false;
}

} // namespace QmlDesigner
