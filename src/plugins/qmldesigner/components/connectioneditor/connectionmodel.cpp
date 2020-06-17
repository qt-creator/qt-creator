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

#include "connectionmodel.h"
#include "connectionview.h"

#include <bindingproperty.h>
#include <variantproperty.h>
#include <signalhandlerproperty.h>
#include <rewritertransaction.h>
#include <nodeabstractproperty.h>
#include <exception.h>
#include <nodemetainfo.h>
#include <nodelistproperty.h>

#include <QStandardItemModel>
#include <QMessageBox>
#include <QTableView>
#include <QTimer>

namespace {

QStringList propertyNameListToStringList(const QmlDesigner::PropertyNameList &propertyNameList)
{
    QStringList stringList;
    for (const QmlDesigner::PropertyName propertyName : propertyNameList) {
        stringList << QString::fromUtf8(propertyName);
    }
    stringList.removeDuplicates();
    return stringList;
}

bool isConnection(const QmlDesigner::ModelNode &modelNode)
{
    return (modelNode.type() == "Connections"
            || modelNode.type() == "QtQuick.Connections"
            || modelNode.type() == "Qt.Connections"
            || modelNode.type() == "QtQml.Connections");
}

} //namespace

namespace QmlDesigner {

namespace Internal {

ConnectionModel::ConnectionModel(ConnectionView *parent)
    : QStandardItemModel(parent)
    , m_connectionView(parent)
{
    connect(this, &QStandardItemModel::dataChanged, this, &ConnectionModel::handleDataChanged);
}

void ConnectionModel::resetModel()
{
    beginResetModel();
    clear();
    setHorizontalHeaderLabels(QStringList({ tr("Target"), tr("Signal Handler"), tr("Action") }));

    if (connectionView()->isAttached()) {
        for (const ModelNode modelNode : connectionView()->allModelNodes())
            addModelNode(modelNode);
    }

    const int columnWidthTarget = connectionView()->connectionTableView()->columnWidth(0);
    connectionView()->connectionTableView()->setColumnWidth(0, columnWidthTarget - 80);

    endResetModel();
}

SignalHandlerProperty ConnectionModel::signalHandlerPropertyForRow(int rowNumber) const
{
    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2).toString();

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

    if (!newTarget.isEmpty()) {
        //if it's an alias, then let's reparent connections to alias property owner:
        const ModelNode parent = connectionView()->modelNodeForId(isAlias
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
    item->setData(signalHandlerProperty.parentModelNode().internalId(), Qt::UserRole + 1);
    item->setData(signalHandlerProperty.name(), Qt::UserRole + 2);
}

ModelNode ConnectionModel::getTargetNodeForConnection(const ModelNode &connection) const
{
    BindingProperty bindingProperty = connection.bindingProperty("target");

    if (bindingProperty.isValid()) {
        bool isAlias = bindingProperty.expression().contains(".");

        if (bindingProperty.expression() == QLatin1String("parent"))
            return connection.parentProperty().parentModelNode();
        else if (isAlias) {
            QStringList substr = bindingProperty.expression().split(".");
            if (substr.size() > 1) {
                ModelNode aliasParent = connectionView()->modelNodeForId(substr.constFirst());
                QString aliasBody = substr.at(1);
                if (aliasParent.hasProperty(aliasBody.toUtf8())) {
                    AbstractProperty abstractProp = aliasParent.property(aliasBody.toUtf8());
                    if (abstractProp.isBindingProperty()) {
                        BindingProperty binding = abstractProp.toBindingProperty();
                        if (connectionView()->hasId(binding.expression())) {
                            ModelNode resolve = connectionView()->modelNodeForId(binding.expression());
                            if (resolve.isValid())
                                return resolve;
                        }
                    }
                }
            }
        }
        return connectionView()->modelNodeForId(bindingProperty.expression());
    }

    return ModelNode();
}

void ConnectionModel::addConnection()
{
    ModelNode rootModelNode = connectionView()->rootModelNode();

    if (rootModelNode.isValid() && rootModelNode.metaInfo().isValid()) {

        NodeMetaInfo nodeMetaInfo = connectionView()->model()->metaInfo("QtQuick.Connections");

        if (nodeMetaInfo.isValid()) {
            connectionView()->executeInTransaction("ConnectionModel::addConnection", [=](){
                ModelNode newNode = connectionView()->createModelNode("QtQuick.Connections",
                                                                      nodeMetaInfo.majorVersion(),
                                                                      nodeMetaInfo.minorVersion());
                QString source = "print(\"clicked\")";

                if (connectionView()->selectedModelNodes().count() == 1) {
                    ModelNode selectedNode = connectionView()->selectedModelNodes().constFirst();
                    if (QmlItemNode::isValidQmlItemNode(selectedNode))
                        selectedNode.nodeAbstractProperty("data").reparentHere(newNode);
                    else
                        rootModelNode.nodeAbstractProperty(rootModelNode.metaInfo().defaultPropertyName()).reparentHere(newNode);

                    if (QmlItemNode(selectedNode).isFlowActionArea())
                        source = selectedNode.validId() + ".trigger()";

                    if (QmlVisualNode(selectedNode).isFlowTransition())
                        source = selectedNode.validId() + ".trigger()";

                    if (!connectionView()->selectedModelNodes().constFirst().id().isEmpty())
                    newNode.bindingProperty("target").setExpression(selectedNode.id());
                    else
                        newNode.bindingProperty("target").setExpression(QLatin1String("parent"));
                } else {
                    rootModelNode.nodeAbstractProperty(rootModelNode.metaInfo().defaultPropertyName()).reparentHere(newNode);
                    newNode.bindingProperty("target").setExpression(QLatin1String("parent"));
                }

                newNode.signalHandlerProperty("onClicked").setSource(source);
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
    QmlDesigner::ModelNode node = targetSignal.parentModelNode();
    QList<SignalHandlerProperty> allSignals = node.signalProperties();
    if (allSignals.size() > 1) {
        if (allSignals.contains(targetSignal))
            node.removeProperty(targetSignal.name());
    }
    else {
        node.destroy();
    }
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

void ConnectionModel::handleException()
{
    QMessageBox::warning(nullptr, tr("Error"), m_exceptionError);
    resetModel();
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

    if (connection.isValid()) {
        ModelNode targetNode = getTargetNodeForConnection(connection);
        if (targetNode.isValid() && targetNode.metaInfo().isValid()) {
            stringList.append(propertyNameListToStringList(targetNode.metaInfo().signalNames()));
        }
    }

    return stringList;
}

} // namespace Internal

} // namespace QmlDesigner
