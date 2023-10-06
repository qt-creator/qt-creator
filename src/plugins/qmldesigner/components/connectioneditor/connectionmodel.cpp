// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectionmodel.h"
#include "connectionview.h"
#include "utils/algorithm.h"

#include <bindingproperty.h>
#include <connectioneditorevaluator.h>
#include <exception.h>
#include <model/modelutils.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <plaintexteditmodifier.h>
#include <rewritertransaction.h>
#include <rewriterview.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

namespace {

const char defaultCondition[] = "condition";

QStringList propertyNameListToStringList(const QmlDesigner::PropertyNameList &propertyNameList)
{
    QStringList stringList;
    for (const QmlDesigner::PropertyName &propertyName : propertyNameList)
        stringList << QString::fromUtf8(propertyName);

    stringList.removeDuplicates();
    return stringList;
}

bool isConnection(const QmlDesigner::ModelNode &modelNode)
{
    return (modelNode.metaInfo().simplifiedTypeName() == "Connections");
}

} //namespace

namespace QmlDesigner {

ConnectionModel::ConnectionModel(ConnectionView *parent)
    : QStandardItemModel(parent), m_connectionView(parent),
      m_delegate(new ConnectionModelBackendDelegate(this))
{
    connect(this, &QStandardItemModel::dataChanged, this, &ConnectionModel::handleDataChanged);
}

Qt::ItemFlags ConnectionModel::flags(const QModelIndex &modelIndex) const
{
    if (!modelIndex.isValid())
        return Qt::ItemIsEnabled;

    if (!m_connectionView || !m_connectionView->model())
        return Qt::ItemIsEnabled;

    const int internalId = data(index(modelIndex.row(), TargetModelNodeRow), UserRoles::InternalIdRole).toInt();
    ModelNode modelNode = m_connectionView->modelNodeForInternalId(internalId);

    if (modelNode.isValid() && ModelUtils::isThisOrAncestorLocked(modelNode))
        return Qt::ItemIsEnabled;

    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void ConnectionModel::resetModel()
{
    beginResetModel();
    clear();
    setHorizontalHeaderLabels(QStringList({ tr("Target"), tr("Signal Handler"), tr("Action") }));

    if (connectionView()->isAttached()) {
        for (const ModelNode &modelNode : connectionView()->allModelNodes())
            addModelNode(modelNode);
    }
    endResetModel();
    m_delegate->update();
}

SignalHandlerProperty ConnectionModel::signalHandlerPropertyForRow(int rowNumber) const
{
    const int internalId = data(index(rowNumber, TargetModelNodeRow), UserRoles::InternalIdRole).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), UserRoles::TargetPropertyNameRole).toString();

    ModelNode modelNode = connectionView()->modelNodeForInternalId(internalId);
    if (modelNode.isValid())
        return modelNode.signalHandlerProperty(targetPropertyName.toUtf8());

    return SignalHandlerProperty();
}

void ConnectionModel::addModelNode(const ModelNode &modelNode)
{
    if (isConnection(modelNode))
        addConnection(modelNode);
}

void ConnectionModel::addConnection(const ModelNode &modelNode)
{
    for (const AbstractProperty &property : modelNode.properties()) {
        if (property.isSignalHandlerProperty() && property.name() != "target") {
            addSignalHandler(property.toSignalHandlerProperty());
        }
    }
}

void ConnectionModel::addSignalHandler(const SignalHandlerProperty &signalHandlerProperty)
{
    QStandardItem *targetItem;
    QStandardItem *signalItem;
    QStandardItem *actionItem;

    QString idLabel;

    ModelNode connectionsModelNode = signalHandlerProperty.parentModelNode();

    if (connectionsModelNode.bindingProperty("target").isValid()) {
        idLabel = connectionsModelNode.bindingProperty("target").expression();
    }

    targetItem = new QStandardItem(idLabel);
    updateCustomData(targetItem, signalHandlerProperty);
    const QString propertyName = QString::fromUtf8(signalHandlerProperty.name());
    const QString source = signalHandlerProperty.source();

    signalItem = new QStandardItem(propertyName);
    QList<QStandardItem*> items;

    items.append(targetItem);
    items.append(signalItem);

    actionItem = new QStandardItem(source);

    items.append(actionItem);

    appendRow(items);
}

void ConnectionModel::removeModelNode(const ModelNode &modelNode)
{
    if (isConnection(modelNode))
        removeConnection(modelNode);
}

void ConnectionModel::removeConnection(const ModelNode & /*modelNode*/)
{
    Q_ASSERT_X(false, "not implemented", Q_FUNC_INFO);
}

void ConnectionModel::updateSource(int row)
{
    SignalHandlerProperty signalHandlerProperty = signalHandlerPropertyForRow(row);

    const QString sourceString = data(index(row, SourceRow)).toString();

    RewriterTransaction transaction =
        connectionView()->beginRewriterTransaction(QByteArrayLiteral("ConnectionModel::updateSource"));

    try {
        signalHandlerProperty.setSource(sourceString);
        transaction.commit();
    }
    catch (Exception &e) {
        m_exceptionError = e.description();
        QTimer::singleShot(200, this, &ConnectionModel::handleException);
    }
}

void ConnectionModel::updateSignalName(int rowNumber)
{
    SignalHandlerProperty signalHandlerProperty = signalHandlerPropertyForRow(rowNumber);
    ModelNode connectionNode = signalHandlerProperty.parentModelNode();

    const PropertyName newName = data(index(rowNumber, TargetPropertyNameRow)).toString().toUtf8();
    if (!newName.isEmpty()) {
        connectionView()->executeInTransaction("ConnectionModel::updateSignalName", [=, &connectionNode](){

            const QString source = signalHandlerProperty.source();

            connectionNode.signalHandlerProperty(newName).setSource(source);
            connectionNode.removeProperty(signalHandlerProperty.name());
        });

        QStandardItem* idItem = item(rowNumber, 0);
        SignalHandlerProperty newSignalHandlerProperty = connectionNode.signalHandlerProperty(newName);
        updateCustomData(idItem, newSignalHandlerProperty);
    } else {
        qWarning() << "BindingModel::updatePropertyName invalid property name";
    }
}

void ConnectionModel::updateTargetNode(int rowNumber)
{
    SignalHandlerProperty signalHandlerProperty = signalHandlerPropertyForRow(rowNumber);
    const QString newTarget = data(index(rowNumber, TargetModelNodeRow)).toString();
    ModelNode connectionNode = signalHandlerProperty.parentModelNode();

    const bool isAlias = newTarget.contains(".");
    bool isSingleton = false;

    if (RewriterView* rv = connectionView()->rewriterView()) {
        for (const QmlTypeData &data : rv->getQMLTypes()) {
            if (!data.typeName.isEmpty()) {
                if (data.typeName == newTarget) {
                    if (connectionView()->model()->metaInfo(data.typeName.toUtf8()).isValid()) {
                        isSingleton = true;
                        break;
                    }
                } else if (isAlias) {
                    if (data.typeName == newTarget.split(".").constFirst()) {
                        if (connectionView()->model()->metaInfo(data.typeName.toUtf8()).isValid()) {
                            isSingleton = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (!newTarget.isEmpty()) {
        //if it's a singleton, then let's reparent connections to rootNode,
        //if it's an alias, then reparent to alias property owner:
        const ModelNode parent = connectionView()->modelNodeForId((isSingleton || (isSingleton && isAlias))
                                                                  ? connectionView()->rootModelNode().id()
                                                                  : isAlias
                                                                  ? newTarget.split(".").constFirst()
                                                                  : newTarget);

        if (parent.isValid() && QmlItemNode::isValidQmlItemNode(parent))
            parent.nodeListProperty("data").reparentHere(connectionNode);

        connectionView()->executeInTransaction("ConnectionModel::updateTargetNode", [= ,&connectionNode](){
            connectionNode.bindingProperty("target").setExpression(newTarget);
        });

    } else {
        qWarning() << "BindingModel::updatePropertyName invalid target id";
    }
}

void ConnectionModel::updateCustomData(QStandardItem *item, const SignalHandlerProperty &signalHandlerProperty)
{
    item->setData(signalHandlerProperty.parentModelNode().internalId(), UserRoles::InternalIdRole);
    item->setData(signalHandlerProperty.name(), UserRoles::TargetPropertyNameRole);
    item->setData(signalHandlerProperty.parentModelNode()
                      .bindingProperty("target")
                      .resolveToModelNode()
                      .id(),
                  UserRoles::TargetNameRole);

    // TODO signalHandlerProperty.source() contains a statement that defines the type.
    // foo.bar() <- function call
    // foo.state = "literal" //state change
    //anything else is assignment
    // e.g. foo.bal = foo2.bula ; foo.bal = "literal" ; goo.gal = true

    item->setData(tr(ConnectionEditorEvaluator::getDisplayStringForType(
                         signalHandlerProperty.source())
                         .toLatin1()),
                  UserRoles::ActionTypeRole);
}

ModelNode ConnectionModel::getTargetNodeForConnection(const ModelNode &connection) const
{
    ModelNode result;

    const BindingProperty bindingProperty = connection.bindingProperty("target");
    const QString bindExpression = bindingProperty.expression();

    if (bindingProperty.isValid()) {
        if (bindExpression.contains(".")) {
            QStringList substr = bindExpression.split(".");
            const QString itemId = substr.constFirst();
            if (substr.size() > 1) {
                const ModelNode aliasParent = connectionView()->modelNodeForId(itemId);
                substr.removeFirst(); //remove id, only alias pieces left
                const QString aliasBody = substr.join(".");
                if (aliasParent.isValid() && aliasParent.hasBindingProperty(aliasBody.toUtf8())) {
                    const BindingProperty binding = aliasParent.bindingProperty(aliasBody.toUtf8());
                    if (binding.isValid() && connectionView()->hasId(binding.expression())) {
                        result = connectionView()->modelNodeForId(binding.expression());
                    }
                }
            }
        } else {
            result = connectionView()->modelNodeForId(bindExpression);
        }
    }

    return result;
}

static QString addOnToSignalName(const QString &signal)
{
    QString ret = signal;
    ret[0] = ret.at(0).toUpper();
    ret.prepend("on");
    return ret;
}

static PropertyName getFirstSignalForTarget(const NodeMetaInfo &target)
{
    PropertyName ret = "clicked";

    if (!target.isValid())
        return ret;

    const auto signalNames = target.signalNames();
    if (signalNames.isEmpty())
        return ret;

    const PropertyNameList priorityList = {"clicked",
                                           "toggled",
                                           "started",
                                           "stopped",
                                           "moved",
                                           "valueChanged",
                                           "visualPostionChanged",
                                           "accepted",
                                           "currentIndexChanged",
                                           "activeFocusChanged"};

    for (const auto &signal : priorityList) {
        if (signalNames.contains(signal))
            return signal;
    }

    ret = target.signalNames().first();

    return ret;
}

void ConnectionModel::addConnection()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_CONNECTION_ADDED);

    ModelNode rootModelNode = connectionView()->rootModelNode();

    if (rootModelNode.isValid() && rootModelNode.metaInfo().isValid()) {

        NodeMetaInfo nodeMetaInfo = connectionView()->model()->qtQuickConnectionsMetaInfo();

        if (nodeMetaInfo.isValid()) {
            ModelNode selectedNode = connectionView()->selectedModelNodes().constFirst();
            const PropertyName signalHandlerName = addOnToSignalName(
                                                       QString::fromUtf8(getFirstSignalForTarget(
                                                           selectedNode.metaInfo())))
                                                       .toUtf8();

            connectionView()
                ->executeInTransaction("ConnectionModel::addConnection", [=, &rootModelNode]() {
                    ModelNode newNode = connectionView()
                                            ->createModelNode("QtQuick.Connections",
                                                              nodeMetaInfo.majorVersion(),
                                                              nodeMetaInfo.minorVersion());
                    QString source = "console.log(\"clicked\")";

                    if (connectionView()->selectedModelNodes().size() == 1) {
                        ModelNode selectedNode = connectionView()->selectedModelNodes().constFirst();
                        if (QmlItemNode::isValidQmlItemNode(selectedNode))
                            selectedNode.nodeAbstractProperty("data").reparentHere(newNode);
                        else
                            rootModelNode
                                .nodeAbstractProperty(rootModelNode.metaInfo().defaultPropertyName())
                                .reparentHere(newNode);

                        if (QmlItemNode(selectedNode).isFlowActionArea()
                            || QmlVisualNode(selectedNode).isFlowTransition())
                            source = selectedNode.validId() + ".trigger()";

                        if (!connectionView()->selectedModelNodes().constFirst().id().isEmpty())
                            newNode.bindingProperty("target").setExpression(selectedNode.validId());
                    } else {
                        rootModelNode
                            .nodeAbstractProperty(rootModelNode.metaInfo().defaultPropertyName())
                            .reparentHere(newNode);
                        newNode.bindingProperty("target").setExpression(rootModelNode.validId());
                    }

                    newNode.signalHandlerProperty(signalHandlerName).setSource(source);

                    selectProperty(newNode.signalHandlerProperty(signalHandlerName));
                });
        }
    }
}

void ConnectionModel::bindingPropertyChanged(const BindingProperty &bindingProperty)
{
    if (isConnection(bindingProperty.parentModelNode()))
        resetModel();
}

void ConnectionModel::variantPropertyChanged(const VariantProperty &variantProperty)
{
    if (isConnection(variantProperty.parentModelNode()))
        resetModel();
}

void ConnectionModel::abstractPropertyChanged(const AbstractProperty &abstractProperty)
{
    if (isConnection(abstractProperty.parentModelNode()))
        resetModel();
}

void ConnectionModel::deleteConnectionByRow(int currentRow)
{
    SignalHandlerProperty targetSignal = signalHandlerPropertyForRow(currentRow);
    SignalHandlerProperty selectedSignal = signalHandlerPropertyForRow(currentIndex());

    const bool targetEqualsSelected = targetSignal == selectedSignal;

    QTC_ASSERT(targetSignal.isValid(), return);
    ModelNode node = targetSignal.parentModelNode();
    QTC_ASSERT(node.isValid(), return );

    QList<SignalHandlerProperty> allSignals = node.signalProperties();
    if (allSignals.size() > 1) {
        if (allSignals.contains(targetSignal))
            node.removeProperty(targetSignal.name());
    } else {
        node.destroy();
    }

    if (!targetEqualsSelected)
        selectProperty(selectedSignal);
}

void ConnectionModel::removeRowFromTable(const SignalHandlerProperty &property)
{
    for (int currentRow = 0; currentRow < rowCount(); currentRow++) {
        SignalHandlerProperty targetSignal = signalHandlerPropertyForRow(currentRow);

        if (targetSignal == property) {
            removeRow(currentRow);
            break;
        }
    }
}

void ConnectionModel::add()
{
    addConnection();
}

void ConnectionModel::remove(int row)
{
    deleteConnectionByRow(row);
}

void ConnectionModel::setCurrentIndex(int i)
{
    if (m_currentIndex != i) {
        m_currentIndex = i;
        emit currentIndexChanged();
    }
    m_delegate->setCurrentRow(i);
}

int ConnectionModel::currentIndex() const
{
    return m_currentIndex;
}

void ConnectionModel::selectProperty(const SignalHandlerProperty &property)
{
    for (int i = 0; i < rowCount(); i++) {
        auto otherProperty = signalHandlerPropertyForRow(i);

        if (property == otherProperty) {
            setCurrentIndex(i);
            return;
        }
    }
}

void ConnectionModel::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    SignalHandlerProperty selectedSignal = signalHandlerPropertyForRow(currentIndex());
    if (selectedSignal.isValid()) {
        ModelNode targetNode = getTargetNodeForConnection(selectedSignal.parentModelNode());
        if (targetNode == removedNode) {
            emit m_delegate->popupShouldClose();
        }
    }
}

void ConnectionModel::handleException()
{
    QMessageBox::warning(nullptr, tr("Error"), m_exceptionError);
    resetModel();
}

ConnectionModelBackendDelegate *ConnectionModel::delegate() const
{
    return m_delegate;
}

void ConnectionModel::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (topLeft != bottomRight) {
        qWarning() << "ConnectionModel::handleDataChanged multi edit?";
        return;
    }

    m_lock = true;

    int currentColumn = topLeft.column();
    int currentRow = topLeft.row();

    switch (currentColumn) {
    case TargetModelNodeRow: {
        updateTargetNode(currentRow);
    } break;
    case TargetPropertyNameRow: {
        updateSignalName(currentRow);
    } break;
    case SourceRow: {
        updateSource(currentRow);
    } break;

    default: qWarning() << "ConnectionModel::handleDataChanged column" << currentColumn;
    }

    m_lock = false;
}

ConnectionView *ConnectionModel::connectionView() const
{
    return m_connectionView;
}

QStringList ConnectionModel::getSignalsForRow(int row) const
{
    QStringList stringList;
    SignalHandlerProperty signalHandlerProperty = signalHandlerPropertyForRow(row);

    if (signalHandlerProperty.isValid())
        stringList.append(getPossibleSignalsForConnection(signalHandlerProperty.parentModelNode()));

    return stringList;
}

QStringList ConnectionModel::getflowActionTriggerForRow(int row) const
{
    QStringList stringList;
    SignalHandlerProperty signalHandlerProperty = signalHandlerPropertyForRow(row);

    if (signalHandlerProperty.isValid()) {
        const ModelNode parentModelNode = signalHandlerProperty.parentModelNode();
        ModelNode targetNode = getTargetNodeForConnection(parentModelNode);
        if (!targetNode.isValid() && !parentModelNode.isRootNode())
             targetNode = parentModelNode.parentProperty().parentModelNode();
         if (targetNode.isValid()) {
             for (auto &node : targetNode.allSubModelNodesAndThisNode()) {
                 if (QmlItemNode(node).isFlowActionArea() && node.hasId())
                     stringList.append(node.id() + ".trigger()");
             }
         }
     }
     return stringList;
}

QStringList ConnectionModel::getPossibleSignalsForConnection(const ModelNode &connection) const
{
    QStringList stringList;

    auto getAliasMetaSignals = [&](QString aliasPart, NodeMetaInfo metaInfo) {
        if (metaInfo.isValid() && metaInfo.hasProperty(aliasPart.toUtf8())) {
            NodeMetaInfo propertyMetaInfo = metaInfo.property(aliasPart.toUtf8()).propertyType();
            if (propertyMetaInfo.isValid()) {
                return propertyNameListToStringList(propertyMetaInfo.signalNames());
            }
        }
        return QStringList();
    };

    if (connection.isValid()) {
        //separate check for singletons
        if (connection.hasBindingProperty("target")) {
            const BindingProperty bp = connection.bindingProperty("target");

            if (bp.isValid()) {
                const QString bindExpression = bp.expression();

                if (const RewriterView * const rv = connectionView()->rewriterView()) {
                    for (const QmlTypeData &data : rv->getQMLTypes()) {
                        if (!data.typeName.isEmpty()) {
                            if (data.typeName == bindExpression) {
                                NodeMetaInfo metaInfo = connectionView()->model()->metaInfo(data.typeName.toUtf8());
                                if (metaInfo.isValid()) {
                                    stringList << propertyNameListToStringList(metaInfo.signalNames());
                                    break;
                                }
                            } else if (bindExpression.contains(".")) {
                                //if it doesn't fit the same name, maybe it's an alias?
                                QStringList expression = bindExpression.split(".");
                                if ((expression.size() > 1) && (expression.constFirst() == data.typeName)) {
                                    expression.removeFirst();

                                    stringList << getAliasMetaSignals(
                                                      expression.join("."),
                                                      connectionView()->model()->metaInfo(data.typeName.toUtf8()));
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        ModelNode targetNode = getTargetNodeForConnection(connection);
        if (targetNode.isValid() && targetNode.metaInfo().isValid()) {
            stringList.append(propertyNameListToStringList(targetNode.metaInfo().signalNames()));
        } else {
            //most likely it's component's internal alias:

            if (connection.hasBindingProperty("target")) {
                const BindingProperty bp = connection.bindingProperty("target");

                if (bp.isValid()) {
                    QStringList expression = bp.expression().split(".");
                    if (expression.size() > 1) {
                        const QString itemId = expression.constFirst();
                        if (connectionView()->hasId(itemId)) {
                            ModelNode parentItem = connectionView()->modelNodeForId(itemId);
                            if (parentItem.isValid()
                                    && parentItem.hasMetaInfo()
                                    && parentItem.metaInfo().isValid()) {
                                expression.removeFirst();
                                stringList << getAliasMetaSignals(expression.join("."),
                                                                  parentItem.metaInfo());
                            }
                        }
                    }
                }
            }
        }
    }

    return stringList;
}

QHash<int, QByteArray> ConnectionModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{TargetPropertyNameRole, "signal"},
                                            {TargetNameRole, "target"},
                                            {ActionTypeRole, "action"}};
    return roleNames;
}

ConnectionModelBackendDelegate::ConnectionModelBackendDelegate(ConnectionModel *parent)
    : QObject(parent), m_signalDelegate(parent->connectionView()), m_okStatementDelegate(parent),
      m_koStatementDelegate(parent), m_conditionListModel(parent),
      m_propertyTreeModel(parent->connectionView()), m_propertyListProxyModel(&m_propertyTreeModel)

{
    connect(&m_signalDelegate, &PropertyTreeModelDelegate::commitData, this, [this]() {
        handleTargetChanged();
    });

    connect(&m_okStatementDelegate,
            &ConnectionModelStatementDelegate::statementChanged,
            this,
            [this]() { handleOkStatementChanged(); });

    connect(&m_koStatementDelegate,
            &ConnectionModelStatementDelegate::statementChanged,
            this,
            [this]() { handleKOStatementChanged(); });

    connect(&m_conditionListModel, &ConditionListModel::conditionChanged, this, [this]() {
        handleConditionChanged();
    });

    m_signalDelegate.setPropertyType(PropertyTreeModel::SignalType);
}

QString generateDefaultStatement(ConnectionModelBackendDelegate::ActionType actionType,
                                 const QString &rootId)
{
    switch (actionType) {
    case ConnectionModelStatementDelegate::CallFunction:
        return "Qt.quit()";
    case ConnectionModelStatementDelegate::Assign:
        return QString("%1.visible = %1.visible").arg(rootId);
    case ConnectionModelStatementDelegate::ChangeState:
        return QString("%1.state = \"\"").arg(rootId);
    case ConnectionModelStatementDelegate::SetProperty:
        return QString("%1.visible = true").arg(rootId);
    case ConnectionModelStatementDelegate::PrintMessage:
        return QString("console.log(\"test\")").arg(rootId);
    case ConnectionModelStatementDelegate::Custom:
        return {};
    };

    //Qt.quit()
    //console.log("test")
    //root.state = ""
    //root.visible = root.visible
    //root.visible = true

    return {};
}

void ConnectionModelBackendDelegate::changeActionType(ActionType actionType)
{
    QTC_ASSERT(actionType != ConnectionModelStatementDelegate::Custom, return );

    ConnectionModel *model = qobject_cast<ConnectionModel *>(parent());

    QTC_ASSERT(model, return );
    QTC_ASSERT(model->connectionView()->isAttached(), return );

    const QString validId = model->connectionView()->rootModelNode().validId();

    SignalHandlerProperty signalHandlerProperty = model->signalHandlerPropertyForRow(currentRow());

    // Do not take ko into account for now

    model->connectionView()
        ->executeInTransaction("ConnectionModelBackendDelegate::removeCondition", [&]() {
            ConnectionEditorStatements::MatchedStatement &okStatement
                = ConnectionEditorStatements::okStatement(m_handler);

            ConnectionEditorStatements::MatchedStatement &koStatement
                = ConnectionEditorStatements::koStatement(m_handler);

            koStatement = ConnectionEditorStatements::EmptyBlock();

            //We expect a valid id on the root node
            const QString validId = model->connectionView()->rootModelNode().validId();
            QString statementSource = generateDefaultStatement(actionType, validId);

            auto tempHandler = ConnectionEditorEvaluator::parseStatement(statementSource);

            auto newOkStatement = ConnectionEditorStatements::okStatement(tempHandler);

            QTC_ASSERT(!ConnectionEditorStatements::isEmptyStatement(newOkStatement), return );

            okStatement = newOkStatement;

            QString newSource = ConnectionEditorStatements::toJavascript(m_handler);

            signalHandlerProperty.setSource(newSource);
        });

    setSource(signalHandlerProperty.source());

    setupHandlerAndStatements();
    setupCondition();
}

void ConnectionModelBackendDelegate::addCondition()
{
    ConnectionEditorStatements::MatchedStatement okStatement
        = ConnectionEditorStatements::okStatement(m_handler);

    ConnectionEditorStatements::MatchedCondition newCondition;

    ConnectionEditorStatements::Variable variable;
    variable.nodeId = defaultCondition;
    newCondition.statements.append(variable);

    ConnectionEditorStatements::ConditionalStatement conditionalStatement;

    conditionalStatement.ok = okStatement;
    conditionalStatement.condition = newCondition;

    m_handler = conditionalStatement;

    QString newSource = ConnectionEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);

    setupHandlerAndStatements();
    setupCondition();
}

void ConnectionModelBackendDelegate::removeCondition()
{
    ConnectionEditorStatements::MatchedStatement okStatement
        = ConnectionEditorStatements::okStatement(m_handler);

    m_handler = okStatement;

    QString newSource = ConnectionEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);

    setupHandlerAndStatements();
    setupCondition();
}

void ConnectionModelBackendDelegate::addElse()
{
    ConnectionEditorStatements::MatchedStatement okStatement
        = ConnectionEditorStatements::okStatement(m_handler);

    auto &condition = ConnectionEditorStatements::conditionalStatement(m_handler);
    condition.ko = condition.ok;

    QString newSource = ConnectionEditorStatements::toJavascript(m_handler);


    commitNewSource(newSource);
    setupHandlerAndStatements();
}

void ConnectionModelBackendDelegate::removeElse()
{
    ConnectionEditorStatements::MatchedStatement okStatement
        = ConnectionEditorStatements::okStatement(m_handler);

    auto &condition = ConnectionEditorStatements::conditionalStatement(m_handler);
    condition.ko = ConnectionEditorStatements::EmptyBlock();

    QString newSource = ConnectionEditorStatements::toJavascript(m_handler);


    commitNewSource(newSource);
    setupHandlerAndStatements();
}

void ConnectionModelBackendDelegate::setNewSource(const QString &newSource)
{
    setSource(newSource);
    commitNewSource(newSource);
    setupHandlerAndStatements();
    setupCondition();
}

int ConnectionModelBackendDelegate::currentRow() const
{
    return m_currentRow;
}

QString removeOnFromSignalName(const QString &signal)
{
    if (signal.isEmpty())
        return {};
    QString ret = signal;
    ret.remove(0, 2);
    ret[0] = ret.at(0).toLower();
    return ret;
}

void ConnectionModelBackendDelegate::setCurrentRow(int i)
{
    if (m_currentRow == i)
        return;

    m_currentRow = i;

    update();
}

void ConnectionModelBackendDelegate::update()
{
    if (m_blockReflection)
        return;

    if (m_currentRow == -1)
        return;

    m_propertyTreeModel.resetModel();
    m_propertyListProxyModel.setRowAndInternalId(0, internalRootIndex);

    ConnectionModel *model = qobject_cast<ConnectionModel *>(parent());

    QTC_ASSERT(model, return );
    if (!model->connectionView()->isAttached())
        return;

    SignalHandlerProperty signalHandlerProperty = model->signalHandlerPropertyForRow(currentRow());

    QStringList targetNodes;

    for (const ModelNode &modelNode : model->connectionView()->allModelNodes()) {
        if (!modelNode.id().isEmpty())
            targetNodes.append(modelNode.id());
    }

    std::sort(targetNodes.begin(), targetNodes.end());

    const QString targetNodeName = signalHandlerProperty.parentModelNode()
                                       .bindingProperty("target")
                                       .resolveToModelNode()
                                       .id();

    if (!targetNodes.contains(targetNodeName))
        targetNodes.append(targetNodeName);

    setSource(signalHandlerProperty.source());

    m_signalDelegate.setup(targetNodeName,
                           removeOnFromSignalName(QString::fromUtf8(signalHandlerProperty.name())));

    setupHandlerAndStatements();

    setupCondition();

    QTC_ASSERT(model, return );
}

void ConnectionModelBackendDelegate::handleException()
{
    QMessageBox::warning(nullptr, tr("Error"), m_exceptionError);
}

bool ConnectionModelBackendDelegate::hasCondition() const
{
    return m_hasCondition;
}

bool ConnectionModelBackendDelegate::hasElse() const
{
    return m_hasElse;
}

void ConnectionModelBackendDelegate::setHasCondition(bool b)
{
    if (b == m_hasCondition)
        return;

    m_hasCondition = b;
    emit hasConditionChanged();
}

void ConnectionModelBackendDelegate::setHasElse(bool b)
{
    if (b == m_hasElse)
        return;

    m_hasElse = b;
    emit hasElseChanged();
}

ConnectionModelBackendDelegate::ActionType ConnectionModelBackendDelegate::actionType() const
{
    return m_actionType;
}

PropertyTreeModelDelegate *ConnectionModelBackendDelegate::signal()
{
    return &m_signalDelegate;
}

ConnectionModelStatementDelegate *ConnectionModelBackendDelegate::okStatement()
{
    return &m_okStatementDelegate;
}

ConnectionModelStatementDelegate *ConnectionModelBackendDelegate::koStatement()
{
    return &m_koStatementDelegate;
}

ConditionListModel *ConnectionModelBackendDelegate::conditionListModel()
{
    return &m_conditionListModel;
}

QString ConnectionModelBackendDelegate::indentedSource() const
{
    if (m_source.isEmpty())
        return {};

    QTextDocument doc(m_source);
    QTextCursor cursor(&doc);
    IndentingTextEditModifier mod(&doc, cursor);

    mod.indent(0, m_source.length() - 1);
    return mod.text();
}

QString ConnectionModelBackendDelegate::source() const
{
    return m_source;
}

void ConnectionModelBackendDelegate::setSource(const QString &source)
{
    if (source == m_source)
        return;

    m_source = source;
    emit sourceChanged();
}

PropertyTreeModel *ConnectionModelBackendDelegate::propertyTreeModel()
{
    return &m_propertyTreeModel;
}

PropertyListProxyModel *ConnectionModelBackendDelegate::propertyListProxyModel()
{
    return &m_propertyListProxyModel;
}

void ConnectionModelBackendDelegate::setupCondition()
{
    auto &condition = ConnectionEditorStatements::matchedCondition(m_handler);
    m_conditionListModel.setCondition(ConnectionEditorStatements::matchedCondition(m_handler));
    setHasCondition(!condition.statements.isEmpty());
}

void ConnectionModelBackendDelegate::setupHandlerAndStatements()
{
    ConnectionModel *model = qobject_cast<ConnectionModel *>(parent());
    QTC_ASSERT(model, return );
    SignalHandlerProperty signalHandlerProperty = model->signalHandlerPropertyForRow(currentRow());

    if (signalHandlerProperty.source().isEmpty()) {
        m_actionType = ConnectionModelStatementDelegate::Custom;
        m_handler = ConnectionEditorStatements::EmptyBlock();
    } else {
        m_handler = ConnectionEditorEvaluator::parseStatement(signalHandlerProperty.source());

        const QString statementType = QmlDesigner::ConnectionEditorStatements::toDisplayName(
            m_handler);

        if (statementType == ConnectionEditorStatements::EMPTY_DISPLAY_NAME) {
            m_actionType = ConnectionModelStatementDelegate::Custom;
        } else if (statementType == ConnectionEditorStatements::ASSIGNMENT_DISPLAY_NAME) {
            m_actionType = ConnectionModelStatementDelegate::Assign;
            //setupAssignment();
        } else if (statementType == ConnectionEditorStatements::SETPROPERTY_DISPLAY_NAME) {
            m_actionType = ConnectionModelStatementDelegate::SetProperty;
            //setupSetProperty();
        } else if (statementType == ConnectionEditorStatements::FUNCTION_DISPLAY_NAME) {
            m_actionType = ConnectionModelStatementDelegate::CallFunction;
            //setupCallFunction();
        } else if (statementType == ConnectionEditorStatements::SETSTATE_DISPLAY_NAME) {
            m_actionType = ConnectionModelStatementDelegate::ChangeState;
            //setupChangeState();
        } else if (statementType == ConnectionEditorStatements::LOG_DISPLAY_NAME) {
            m_actionType = ConnectionModelStatementDelegate::PrintMessage;
            //setupPrintMessage();
        } else {
            m_actionType = ConnectionModelStatementDelegate::Custom;
        }
    }

    ConnectionEditorStatements::MatchedStatement &okStatement
        = ConnectionEditorStatements::okStatement(m_handler);
    m_okStatementDelegate.setStatement(okStatement);
    m_okStatementDelegate.setActionType(m_actionType);

    ConnectionEditorStatements::MatchedStatement &koStatement
        = ConnectionEditorStatements::koStatement(m_handler);

    if (!ConnectionEditorStatements::isEmptyStatement(koStatement)) {
        m_koStatementDelegate.setStatement(koStatement);
        m_koStatementDelegate.setActionType(m_actionType);
    }

    ConnectionEditorStatements::isEmptyStatement(koStatement);
    setHasElse(!ConnectionEditorStatements::isEmptyStatement(koStatement));

    emit actionTypeChanged();
}

void ConnectionModelBackendDelegate::handleTargetChanged()
{
    ConnectionModel *model = qobject_cast<ConnectionModel *>(parent());

    QTC_ASSERT(model, return );

    QTC_ASSERT(model->connectionView()->isAttached(), return );

    SignalHandlerProperty signalHandlerProperty = model->signalHandlerPropertyForRow(currentRow());

    const PropertyName handlerName = addOnToSignalName(m_signalDelegate.name()).toUtf8();

    const auto parentModelNode = signalHandlerProperty.parentModelNode();

    QTC_ASSERT(parentModelNode.isValid(), return );

    const auto newId = m_signalDelegate.id();

    const int internalId = signalHandlerProperty.parentModelNode().internalId();

    model->connectionView()
        ->executeInTransaction("ConnectionModelBackendDelegate::handleTargetChanged", [&]() {
            const auto oldTargetNodeName
                = parentModelNode.bindingProperty("target").resolveToModelNode().id();

            if (signalHandlerProperty.name() != handlerName) {
                const auto expression = signalHandlerProperty.source();
                parentModelNode.removeProperty(signalHandlerProperty.name());
                parentModelNode.signalHandlerProperty(handlerName).setSource(expression);
            }

            if (oldTargetNodeName != newId) {
                parentModelNode.bindingProperty("target").setExpression(newId);

                const ModelNode parent = parentModelNode.view()->modelNodeForId(newId);

                if (parent.isValid() && QmlItemNode::isValidQmlVisualNode(parent))
                    parent.nodeListProperty("data").reparentHere(parentModelNode);
                else
                    model->connectionView()->rootModelNode().nodeListProperty("data").reparentHere(
                        parentModelNode);
            }
        });

    model->selectProperty(model->connectionView()
                              ->modelNodeForInternalId(internalId)
                              .signalHandlerProperty(handlerName));
}

void ConnectionModelBackendDelegate::handleOkStatementChanged()
{
    ConnectionEditorStatements::MatchedStatement &okStatement
        = ConnectionEditorStatements::okStatement(m_handler);

    okStatement = m_okStatementDelegate.statement(); //TODO why?

    QString newSource = ConnectionEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
}

void ConnectionModelBackendDelegate::handleKOStatementChanged()
{
    ConnectionEditorStatements::MatchedStatement &koStatement
        = ConnectionEditorStatements::koStatement(m_handler);

    koStatement = m_koStatementDelegate.statement(); //TODO why?

    QString newSource = ConnectionEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
}

void ConnectionModelBackendDelegate::handleConditionChanged()
{

    ConnectionModel *model = qobject_cast<ConnectionModel *>(parent());
    QTC_ASSERT(model, return );
    QTC_ASSERT(model->connectionView()->isAttached(), return );

    ConnectionEditorStatements::MatchedCondition &condition
        = ConnectionEditorStatements::matchedCondition(m_handler);
    condition = m_conditionListModel.condition(); //why?
    QString newSource = ConnectionEditorStatements::toJavascript(m_handler);

    commitNewSource(newSource);
}

void ConnectionModelBackendDelegate::commitNewSource(const QString &source)
{
    ConnectionModel *model = qobject_cast<ConnectionModel *>(parent());

    QTC_ASSERT(model, return );

    QTC_ASSERT(model->connectionView()->isAttached(), return );

    SignalHandlerProperty signalHandlerProperty = model->signalHandlerPropertyForRow(currentRow());

    m_blockReflection = true;
    model->connectionView()->executeInTransaction("ConnectionModelBackendDelegate::commitNewSource",
                                                  [&]() {
                                                      signalHandlerProperty.setSource(source);
                                                  });

    setSource(signalHandlerProperty.source());
    m_blockReflection = false;
}

static ConnectionEditorStatements::MatchedStatement emptyStatement;

ConnectionModelStatementDelegate::ConnectionModelStatementDelegate(ConnectionModel *parent)
    : QObject(parent), m_functionDelegate(parent->connectionView()),
      m_lhsDelegate(parent->connectionView()), m_rhsAssignmentDelegate(parent->connectionView()),
      m_statement(emptyStatement), m_model(parent)
{
    m_functionDelegate.setPropertyType(PropertyTreeModel::SlotType);

    connect(&m_functionDelegate, &PropertyTreeModelDelegate::commitData, this, [this]() {
        handleFunctionChanged();
    });

    connect(&m_rhsAssignmentDelegate, &PropertyTreeModelDelegate::commitData, this, [this]() {
        handleRhsAssignmentChanged();
    });

    connect(&m_lhsDelegate, &PropertyTreeModelDelegate::commitData, this, [this]() {
        handleLhsChanged();
    });

    connect(&m_stringArgument, &StudioQmlTextBackend::activated, this, [this]() {
        handleStringArgumentChanged();
    });

    connect(&m_states, &StudioQmlComboBoxBackend::activated, this, [this]() {
        handleStateChanged();
    });

    connect(&m_stateTargets, &StudioQmlComboBoxBackend::activated, this, [this]() {
        handleStateTargetsChanged();
    });
}

void ConnectionModelStatementDelegate::setActionType(ActionType type)
{
    if (m_actionType == type)
        return;

    m_actionType = type;
    emit actionTypeChanged();
    setup();
}

void ConnectionModelStatementDelegate::setup()
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

void ConnectionModelStatementDelegate::setStatement(
    ConnectionEditorStatements::MatchedStatement &statement)
{
    m_statement = statement;
    setup();
}

ConnectionEditorStatements::MatchedStatement &ConnectionModelStatementDelegate::statement()
{
    return m_statement;
}

ConnectionModelStatementDelegate::ActionType ConnectionModelStatementDelegate::actionType() const
{
    return m_actionType;
}

PropertyTreeModelDelegate *ConnectionModelStatementDelegate::function()
{
    return &m_functionDelegate;
}

PropertyTreeModelDelegate *ConnectionModelStatementDelegate::lhs()
{
    return &m_lhsDelegate;
}

PropertyTreeModelDelegate *ConnectionModelStatementDelegate::rhsAssignment()
{
    return &m_rhsAssignmentDelegate;
}

StudioQmlTextBackend *ConnectionModelStatementDelegate::stringArgument()
{
    return &m_stringArgument;
}

StudioQmlComboBoxBackend *ConnectionModelStatementDelegate::stateTargets()
{
    return &m_stateTargets;
}

StudioQmlComboBoxBackend *ConnectionModelStatementDelegate::states()
{
    return &m_states;
}

void ConnectionModelStatementDelegate::handleFunctionChanged()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::MatchedFunction>(m_statement),
               return );

    ConnectionEditorStatements::MatchedFunction &functionStatement
        = std::get<ConnectionEditorStatements::MatchedFunction>(m_statement);

    functionStatement.functionName = m_functionDelegate.name();
    functionStatement.nodeId = m_functionDelegate.id();

    emit statementChanged();
}

void ConnectionModelStatementDelegate::handleLhsChanged()
{
    if (m_actionType == Assign) {
        QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::Assignment>(m_statement),
                   return );

        ConnectionEditorStatements::Assignment &assignmentStatement
            = std::get<ConnectionEditorStatements::Assignment>(m_statement);

        assignmentStatement.lhs.nodeId = m_lhsDelegate.id();
        assignmentStatement.lhs.propertyName = m_lhsDelegate.name();

    } else if (m_actionType == SetProperty) {
        QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::PropertySet>(m_statement),
                   return );

        ConnectionEditorStatements::PropertySet &setPropertyStatement
            = std::get<ConnectionEditorStatements::PropertySet>(m_statement);

        setPropertyStatement.lhs.nodeId = m_lhsDelegate.id();
        setPropertyStatement.lhs.propertyName = m_lhsDelegate.name();
    } else {
        QTC_ASSERT(false, return );
    }

    emit statementChanged();
}

void ConnectionModelStatementDelegate::handleRhsAssignmentChanged()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::Assignment>(m_statement), return );

    ConnectionEditorStatements::Assignment &assignmentStatement
        = std::get<ConnectionEditorStatements::Assignment>(m_statement);

    assignmentStatement.rhs.nodeId = m_rhsAssignmentDelegate.id();
    assignmentStatement.rhs.propertyName = m_rhsAssignmentDelegate.name();

    setupPropertyType();

    emit statementChanged();
}

static ConnectionEditorStatements::Literal parseTextArgument(const QString &text)
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

static ConnectionEditorStatements::ComparativeStatement parseTextArgumentComparativeStatement(
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

static ConnectionEditorStatements::RightHandSide parseLogTextArgument(const QString &text)
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

void ConnectionModelStatementDelegate::handleStringArgumentChanged()
{
    if (m_actionType == SetProperty) {
        QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::PropertySet>(m_statement),
                   return );

        ConnectionEditorStatements::PropertySet &propertySet
            = std::get<ConnectionEditorStatements::PropertySet>(m_statement);

        propertySet.rhs = parseTextArgument(m_stringArgument.text());

    } else if (m_actionType == PrintMessage) {
        QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::ConsoleLog>(m_statement),
                   return );

        ConnectionEditorStatements::ConsoleLog &consoleLog
            = std::get<ConnectionEditorStatements::ConsoleLog>(m_statement);

        consoleLog.argument = parseLogTextArgument(m_stringArgument.text());
    } else {
        QTC_ASSERT(false, return );
    }

    emit statementChanged();
}

void ConnectionModelStatementDelegate::handleStateChanged()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::StateSet>(m_statement), return );

    ConnectionEditorStatements::StateSet &stateSet = std::get<ConnectionEditorStatements::StateSet>(
        m_statement);

    QString stateName = m_states.currentText();
    if (stateName == baseStateName())
        stateName = "";
    stateSet.stateName = "\"" + stateName + "\"";

    emit statementChanged();
}

void ConnectionModelStatementDelegate::handleStateTargetsChanged()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::StateSet>(m_statement), return );

    ConnectionEditorStatements::StateSet &stateSet = std::get<ConnectionEditorStatements::StateSet>(
        m_statement);

    stateSet.nodeId = m_stateTargets.currentText();
    stateSet.stateName = "\"\"";

    setupStates();

    emit statementChanged();
}

void ConnectionModelStatementDelegate::setupAssignment()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::Assignment>(m_statement), return );

    const auto assignment = std::get<ConnectionEditorStatements::Assignment>(m_statement);
    m_lhsDelegate.setup(assignment.lhs.nodeId, assignment.lhs.propertyName);
    m_rhsAssignmentDelegate.setup(assignment.rhs.nodeId, assignment.rhs.propertyName);
    setupPropertyType();
}

void ConnectionModelStatementDelegate::setupSetProperty()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::PropertySet>(m_statement),
               return );

    const auto propertySet = std::get<ConnectionEditorStatements::PropertySet>(m_statement);
    m_lhsDelegate.setup(propertySet.lhs.nodeId, propertySet.lhs.propertyName);
    m_stringArgument.setText(ConnectionEditorStatements::toString(propertySet.rhs));
}

void ConnectionModelStatementDelegate::setupCallFunction()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::MatchedFunction>(m_statement),
               return );

    const auto functionStatement = std::get<ConnectionEditorStatements::MatchedFunction>(
        m_statement);
    m_functionDelegate.setup(functionStatement.nodeId, functionStatement.functionName);
}

void ConnectionModelStatementDelegate::setupChangeState()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::StateSet>(m_statement), return );

    QTC_ASSERT(m_model->connectionView()->isAttached(), return );

    auto model = m_model->connectionView()->model();
    const auto items = Utils::filtered(m_model->connectionView()->allModelNodesOfType(
                                           model->qtQuickItemMetaInfo()),
                                       [](const ModelNode &node) {
                                           QmlItemNode item(node);
                                           return node.hasId() && item.isValid()
                                                  && !item.allStateNames().isEmpty();
                                       });

    QStringList itemIds = Utils::transform(items, [](const ModelNode &node) { return node.id(); });
    const auto groups = m_model->connectionView()->allModelNodesOfType(
        model->qtQuickStateGroupMetaInfo());

    const auto rootId = m_model->connectionView()->rootModelNode().id();
    itemIds.removeAll(rootId);

    QStringList groupIds = Utils::transform(groups, [](const ModelNode &node) { return node.id(); });

    Utils::sort(itemIds);
    Utils::sort(groupIds);

    if (!rootId.isEmpty())
        groupIds.prepend(rootId);

    const QStringList stateGroupModel = groupIds + itemIds;
    m_stateTargets.setModel(stateGroupModel);

    const auto stateSet = std::get<ConnectionEditorStatements::StateSet>(m_statement);

    m_stateTargets.setCurrentText(stateSet.nodeId);
    setupStates();
}
QString stripQuotesFromState(const QString &input)
{
    if (input.startsWith("\"") && input.endsWith("\"")) {
        QString ret = input;
        ret.remove(0, 1);
        ret.chop(1);
        return ret;
    }
    return input;
}
void ConnectionModelStatementDelegate::setupStates()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::StateSet>(m_statement), return );
    QTC_ASSERT(m_model->connectionView()->isAttached(), return );

    const auto stateSet = std::get<ConnectionEditorStatements::StateSet>(m_statement);

    const QString nodeId = m_stateTargets.currentText();

    const ModelNode node = m_model->connectionView()->modelNodeForId(nodeId);

    QStringList states;
    if (node.metaInfo().isQtQuickItem()) {
        QmlItemNode item(node);
        QTC_ASSERT(item.isValid(), return );
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

void ConnectionModelStatementDelegate::setupPrintMessage()
{
    QTC_ASSERT(std::holds_alternative<ConnectionEditorStatements::ConsoleLog>(m_statement), return );

    const auto consoleLog = std::get<ConnectionEditorStatements::ConsoleLog>(m_statement);
    m_stringArgument.setText(ConnectionEditorStatements::toString(consoleLog.argument));
}

void ConnectionModelStatementDelegate::setupPropertyType()
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

QString ConnectionModelStatementDelegate::baseStateName() const
{
    return tr("Base State");
}

static ConnectionEditorStatements::MatchedCondition emptyCondition;

ConditionListModel::ConditionListModel(ConnectionModel *parent)
    : m_connectionModel(parent), m_condition(emptyCondition)
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

        qWarning() << Q_FUNC_INFO << "invalid role";
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
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

void ConditionListModel::setCondition(ConnectionEditorStatements::MatchedCondition &condition)
{
    m_condition = condition;
    setup();
}

ConnectionEditorStatements::MatchedCondition &ConditionListModel::condition()
{
    return m_condition;
}

ConditionListModel::ConditionToken ConditionListModel::tokenFromConditionToken(
    const ConnectionEditorStatements::ConditionToken &token)
{
    ConditionToken ret;
    ret.type = Operator;
    ret.value = ConnectionEditorStatements::toJavascript(token);

    return ret;
}

ConditionListModel::ConditionToken ConditionListModel::tokenFromComparativeStatement(
    const ConnectionEditorStatements::ComparativeStatement &token)
{
    ConditionToken ret;

    if (auto *variable = std::get_if<ConnectionEditorStatements::Variable>(&token)) {
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
    //resetModel();
}

void ConditionListModel::updateToken(int index, const QString &value)
{
    m_tokens[index] = valueToToken(value);
    validateAndRebuildTokens();

    dataChanged(createIndex(index, 0), createIndex(index, 0));
    //resetModel();
}

void ConditionListModel::appendToken(const QString &value)
{
    beginInsertRows({}, rowCount() - 1, rowCount() - 1);

    insertToken(rowCount(), value);
    validateAndRebuildTokens();

    endInsertRows();
    //resetModel();
}

void ConditionListModel::removeToken(int index)
{
    QTC_ASSERT(index < m_tokens.count(), return );
    beginRemoveRows({}, index, index);

    m_tokens.remove(index, 1);
    validateAndRebuildTokens();

    endRemoveRows();

    //resetModel();
}

void ConditionListModel::insertIntermediateToken(int index, const QString &value)
{
    beginInsertRows({}, index, index);

    ConditionToken token;
    token.type = Intermediate;
    token.value = value;

    m_tokens.insert(index, token);

    endInsertRows();
    //resetModel();
}

void ConditionListModel::insertShadowToken(int index, const QString &value)
{
    beginInsertRows({}, index, index);

    ConditionToken token;
    token.type = Shadow;
    token.value = value;

    m_tokens.insert(index, token);

    endInsertRows();

    //resetModel();
}

void ConditionListModel::setShadowToken(int index, const QString &value)
{
    m_tokens[index].type = Shadow;
    m_tokens[index].value = value;

    dataChanged(createIndex(index, 0), createIndex(index, 0));
    //resetModel();
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

    if (value == "true" || value == "false" || ok
        || (value.startsWith("\"") && value.endsWith("\""))) {
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
    const bool invalidToken = Utils::contains(m_tokens,
                                              [&invalidValue](const ConditionToken &token) {
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

void ConditionListModel::rebuildTokens()
{
    QTC_ASSERT(m_valid, return );

    m_condition.statements.clear();
    m_condition.tokens.clear();

    auto it = m_tokens.begin();

    while (it != m_tokens.end()) {
        QTC_ASSERT(it->type != Invalid, return );
        if (it->type == Operator)
            m_condition.tokens.append(toOperatorStatement(*it));
        else if (it->type == Literal || it->type == Variable)
            m_condition.statements.append(toStatement(*it));

        it++;
    }

    emit conditionChanged();
}

ConnectionEditorStatements::ConditionToken ConditionListModel::toOperatorStatement(
    const ConditionToken &token)
{
    if (token.value == "&&")
        return ConnectionEditorStatements::ConditionToken::And;

    if (token.value == "||")
        return ConnectionEditorStatements::ConditionToken::Or;

    if (token.value == "===")
        return ConnectionEditorStatements::ConditionToken::Equals;

    if (token.value == "!==")
        return ConnectionEditorStatements::ConditionToken::Not;

    if (token.value == ">")
        return ConnectionEditorStatements::ConditionToken::LargerThan;

    if (token.value == ">=")
        return ConnectionEditorStatements::ConditionToken::LargerEqualsThan;

    if (token.value == "<")
        return ConnectionEditorStatements::ConditionToken::SmallerThan;

    if (token.value == "<=")
        return ConnectionEditorStatements::ConditionToken::SmallerEqualsThan;

    return ConnectionEditorStatements::ConditionToken::Unknown;
}

ConnectionEditorStatements::ComparativeStatement ConditionListModel::toStatement(
    const ConditionToken &token)
{
    if (token.type == Variable) {
        QStringList list = token.value.split(".");
        ConnectionEditorStatements::Variable variable;

        variable.nodeId = list.first();
        if (list.count() > 1)
            variable.propertyName = list.last();
        return variable;
    } else if (token.type == Literal) {
        return parseTextArgumentComparativeStatement(token.value);
    }

    return {};
}

void QmlDesigner::ConnectionModel::modelAboutToBeDetached()
{
    emit m_delegate->popupShouldClose();
}

} // namespace QmlDesigner
