// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectionmodel.h"
#include "connectioneditorlogging.h"
#include "connectionview.h"

#include <modelutils.h>
#include <nodelistproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <rewritertransaction.h>
#include <rewriterview.h>
#include <scripteditorevaluator.h>
#include <scripteditorutils.h>
#include <variantproperty.h>

#include <QMessageBox>

namespace {
QStringList propertyNameListToStringList(const QmlDesigner::PropertyNameList &propertyNameList)
{
    QStringList stringList;
    stringList.reserve(propertyNameList.size());
    for (const QmlDesigner::PropertyName &propertyName : propertyNameList)
        stringList.push_back(QString::fromUtf8(propertyName));

    return stringList;
}

bool isConnection(const QmlDesigner::ModelNode &modelNode)
{
    return modelNode.metaInfo().isQtQmlConnections();
}

} //namespace

namespace QmlDesigner {

ConnectionModel::ConnectionModel(ConnectionView *view)
    : m_connectionView(view)
    , m_delegate{this}
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
    clear();
    beginResetModel();
    setHorizontalHeaderLabels(QStringList({ tr("Target"), tr("Signal Handler"), tr("Action") }));

    if (connectionView()->isAttached()) {
        for (const ModelNode &modelNode : connectionView()->allModelNodes())
            addModelNode(modelNode);
    }
    endResetModel();
    m_delegate.update();
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
        qCWarning(ConnectionEditorLog) << __FUNCTION__ << "invalid property name";
    }
}

namespace {
#ifdef QDS_USE_PROJECTSTORAGE
bool isSingleton(AbstractView *view, bool isAlias, QStringView newTarget)
{
    using Storage::Info::ExportedTypeName;

    auto model = view->model();

    if (!model)
        return false;

    const auto sourceId = model->fileUrlSourceId();
    const Utils::SmallString name = newTarget;
    const Utils::SmallString aliasName = newTarget.split(u'.').front();
    auto hasTargetExportedTypeName = [&](const auto &metaInfo) {
        const auto exportedTypeNames = metaInfo.exportedTypeNamesForSourceId(sourceId);
        if (std::ranges::find(exportedTypeNames, name, &ExportedTypeName::name)
            != exportedTypeNames.end())
            return true;
        if (isAlias) {
            if (std::ranges::find(exportedTypeNames, aliasName, &ExportedTypeName::name)
                != exportedTypeNames.end())
                return true;
        }
        return false;
    };

    auto singletonMetaInfos = model->singletonMetaInfos();
    return std::ranges::find_if(singletonMetaInfos, hasTargetExportedTypeName)
           != singletonMetaInfos.end();
}
#else
bool isSingleton(AbstractView *view, bool isAlias, const QString &newTarget)
{
    if (RewriterView *rv = view->rewriterView()) {
        for (const QmlTypeData &data : rv->getQMLTypes()) {
            if (!data.typeName.isEmpty()) {
                if (data.typeName == newTarget) {
                    if (view->model()->metaInfo(data.typeName.toUtf8()).isValid())
                        return true;

                } else if (isAlias) {
                    if (data.typeName == newTarget.split(".").constFirst()) {
                        if (view->model()->metaInfo(data.typeName.toUtf8()).isValid())
                            return true;
                    }
                }
            }
        }
    }

    return false;
}
#endif
} // namespace

void ConnectionModel::updateTargetNode(int rowNumber)
{
    SignalHandlerProperty signalHandlerProperty = signalHandlerPropertyForRow(rowNumber);
    const QString newTarget = data(index(rowNumber, TargetModelNodeRow)).toString();
    ModelNode connectionNode = signalHandlerProperty.parentModelNode();

    const bool isAlias = newTarget.contains(".");
    bool isSingleton = QmlDesigner::isSingleton(m_connectionView, isAlias, newTarget);

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
        qCWarning(ConnectionEditorLog) << __FUNCTION__ << "invalid target id";
    }
}

void ConnectionModel::updateCustomData(QStandardItem *item, const SignalHandlerProperty &signalHandlerProperty)
{
    item->setData(signalHandlerProperty.parentModelNode().internalId(), UserRoles::InternalIdRole);
    item->setData(signalHandlerProperty.name().toByteArray(), UserRoles::TargetPropertyNameRole);
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

    item->setData(tr(ScriptEditorEvaluator::getDisplayStringForType(signalHandlerProperty.source())
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

void ConnectionModel::addConnection(const PropertyName &signalName)
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_CONNECTION_ADDED);

    ModelNode rootModelNode = connectionView()->rootModelNode();

    if (rootModelNode.isValid() && rootModelNode.metaInfo().isValid()) {
#ifndef QDS_USE_PROJECTSTORAGE
        NodeMetaInfo nodeMetaInfo = connectionView()->model()->qtQmlConnectionsMetaInfo();

        if (nodeMetaInfo.isValid()) {
#endif
            ModelNode selectedNode = connectionView()->firstSelectedModelNode();
            if (!selectedNode)
                selectedNode = connectionView()->rootModelNode();

            PropertyName signalHandlerName = signalName;
            if (signalHandlerName.isEmpty())
                signalHandlerName = getFirstSignalForTarget(selectedNode.metaInfo());

            signalHandlerName = addOnToSignalName(QString::fromUtf8(signalHandlerName)).toUtf8();

            connectionView()
                ->executeInTransaction("ConnectionModel::addConnection", [&] {
#ifdef QDS_USE_PROJECTSTORAGE
                    ModelNode newNode = connectionView()->createModelNode("Connections");
#else
                    ModelNode newNode = connectionView()->createModelNode("QtQuick.Connections",
                                                                          nodeMetaInfo.majorVersion(),
                                                                          nodeMetaInfo.minorVersion());
#endif
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
#ifndef QDS_USE_PROJECTSTORAGE
        }
#endif
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
    m_delegate.setCurrentRow(i);
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
            emit m_delegate.popupShouldClose();
        }
    }
}

void ConnectionModel::handleException()
{
    QMessageBox::warning(nullptr, tr("Error"), m_exceptionError);
    resetModel();
}

ConnectionModelBackendDelegate *ConnectionModel::delegate()
{
    return &m_delegate;
}

void ConnectionModel::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (topLeft != bottomRight) {
        qCWarning(ConnectionEditorLog) << __FUNCTION__ << "multi edit?";
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
    default:
        qCWarning(ConnectionEditorLog) << __FUNCTION__ << "column" << currentColumn;
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

namespace {}

QStringList ConnectionModel::getPossibleSignalsForConnection(const ModelNode &connection) const
{
    if (!connection)
        return {};

    QStringList stringList;

    auto getAliasMetaSignals = [&](PropertyNameView aliasPart, NodeMetaInfo metaInfo) -> QStringList {
        if (NodeMetaInfo propertyMetaInfo = metaInfo.property(aliasPart).propertyType())
            return propertyNameListToStringList(propertyMetaInfo.signalNames());

        return {};
    };

    //separate check for singletons
    if (const BindingProperty bp = connection.bindingProperty("target")) {
#ifdef QDS_USE_PROJECTSTORAGE
        if (auto model = m_connectionView->model()) {
            const Utils::SmallString bindExpression = bp.expression();
            using Storage::Info::ExportedTypeName;
            const auto sourceId = model->fileUrlSourceId();
            for (const auto &metaInfo : model->singletonMetaInfos()) {
                const auto exportedTypeNames = metaInfo.exportedTypeNamesForSourceId(sourceId);
                if (std::ranges::find(exportedTypeNames, bindExpression, &ExportedTypeName::name)
                    != exportedTypeNames.end()) {
                    stringList.append(propertyNameListToStringList(metaInfo.signalNames()));
                } else {
                    std::string_view expression = bindExpression;
                    auto index = expression.find('.');
                    if (index == std::string_view::npos)
                        continue;

                    expression.remove_prefix(index);

                    stringList.append(getAliasMetaSignals(expression, metaInfo));
                }
            }
        }
#else
        const QString bindExpression = bp.expression();

        if (const RewriterView *const rv = connectionView()->rewriterView()) {
            for (const QmlTypeData &data : rv->getQMLTypes()) {
                if (!data.typeName.isEmpty()) {
                    if (data.typeName == bindExpression) {
                        NodeMetaInfo metaInfo = connectionView()->model()->metaInfo(
                            data.typeName.toUtf8());
                        if (metaInfo.isValid()) {
                            stringList << propertyNameListToStringList(metaInfo.signalNames());
                            break;
                        }
                    } else if (bindExpression.contains(".")) {
                        //if it doesn't fit the same name, maybe it's an alias?
                        QStringList expression = bindExpression.split(".");
                        if ((expression.size() > 1) && (expression.constFirst() == data.typeName)) {
                            expression.removeFirst();

                            stringList << getAliasMetaSignals(expression.join(".").toUtf8(),
                                                              connectionView()->model()->metaInfo(
                                                                  data.typeName.toUtf8()));
                            break;
                        }
                    }
                }
            }
        }
#endif
        std::ranges::sort(stringList);
        stringList.erase(std::ranges::unique(stringList).begin(), stringList.end());
    }

    ModelNode targetNode = getTargetNodeForConnection(connection);
    if (auto metaInfo = targetNode.metaInfo()) {
        stringList.append(propertyNameListToStringList(metaInfo.signalNames()));
    } else {
        //most likely it's component's internal alias:

        if (const BindingProperty bp = connection.bindingProperty("target")) {
            QStringList expression = bp.expression().split(".");
            if (expression.size() > 1) {
                const QString itemId = expression.constFirst();
                if (connectionView()->hasId(itemId)) {
                    ModelNode parentItem = connectionView()->modelNodeForId(itemId);
                    if (parentItem.isValid() && parentItem.hasMetaInfo()
                        && parentItem.metaInfo().isValid()) {
                        expression.removeFirst();
                        stringList << getAliasMetaSignals(expression.join(".").toUtf8(),
                                                          parentItem.metaInfo());
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

int ConnectionModelBackendDelegate::currentRow() const
{
    return m_currentRow;
}

ConnectionModelBackendDelegate::ConnectionModelBackendDelegate(ConnectionModel *model)
    : ScriptEditorBackend(model->connectionView())
    , m_signalDelegate(model->connectionView())
    , m_model(model)
{
    connect(&m_signalDelegate,
            &PropertyTreeModelDelegate::commitData,
            this,
            &ConnectionModelBackendDelegate::handleTargetChanged);

    m_signalDelegate.setPropertyType(PropertyTreeModel::SignalType);
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
    if (m_currentRow == -1)
        return;

    if (blockReflection()) {
        return;
    }

    ScriptEditorBackend::update();

    ConnectionModel *model = m_model;

    QTC_ASSERT(model, return);
    if (!model->connectionView()->isAttached())
        return;

    auto signalHandlerProperty = model->signalHandlerPropertyForRow(currentRow());

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

    m_signalDelegate.setup(targetNodeName,
                           removeOnFromSignalName(QString::fromUtf8(signalHandlerProperty.name())));
}

PropertyTreeModelDelegate *ConnectionModelBackendDelegate::signal()
{
    return &m_signalDelegate;
}

void ConnectionModelBackendDelegate::handleTargetChanged()
{
    ConnectionModel *model = m_model;

    QTC_ASSERT(model, return);

    QTC_ASSERT(model->connectionView()->isAttached(), return);

    SignalHandlerProperty signalHandlerProperty = getSignalHandlerProperty();

    const PropertyName handlerName = addOnToSignalName(m_signalDelegate.name()).toUtf8();

    const auto parentModelNode = signalHandlerProperty.parentModelNode();

    QTC_ASSERT(parentModelNode.isValid(), return);

    const auto newId = m_signalDelegate.id();

    const int internalId = signalHandlerProperty.parentModelNode().internalId();

    model->connectionView()
        ->executeInTransaction("ConnectionModelBackendDelegate::handleTargetChanged", [&]() {
            const auto oldTargetNodeName = parentModelNode.bindingProperty("target")
                                               .resolveToModelNode()
                                               .id();

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

    model->selectProperty(
        model->connectionView()->modelNodeForInternalId(internalId).signalHandlerProperty(handlerName));
}

AbstractProperty ConnectionModelBackendDelegate::getSourceProperty() const
{
    return getSignalHandlerProperty();
}

void ConnectionModelBackendDelegate::setPropertySource(const QString &source)
{
    auto property = getSourceProperty();

    QTC_ASSERT(property.isValid(), return);

    if (source.isEmpty())
        return;

    auto normalizedSource = QmlDesigner::SignalHandlerProperty::normalizedSourceWithBraces(source);
    if (property.exists())
        property.toSignalHandlerProperty().setSource(normalizedSource);
    else
        property.parentModelNode().signalHandlerProperty(property.name()).setSource(normalizedSource);
}

SignalHandlerProperty ConnectionModelBackendDelegate::getSignalHandlerProperty() const
{
    ConnectionModel *model = m_model;
    QTC_ASSERT(model, return {});
    QTC_ASSERT(model->connectionView()->isAttached(), return {});

    return model->signalHandlerPropertyForRow(currentRow());
}

void QmlDesigner::ConnectionModel::modelAboutToBeDetached()
{
    emit m_delegate.popupShouldClose();
}

void ConnectionModel::showPopup()
{
    emit m_delegate.popupShouldOpen();
}

} // namespace QmlDesigner
