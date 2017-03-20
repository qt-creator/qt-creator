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

#include <QStandardItemModel>
#include <QMessageBox>
#include <QTableView>
#include <QTimer>

namespace {

QStringList propertyNameListToStringList(const QmlDesigner::PropertyNameList &propertyNameList)
{
    QStringList stringList;
    foreach (QmlDesigner::PropertyName propertyName, propertyNameList) {
        stringList << QString::fromUtf8(propertyName);
    }
    return stringList;
}

bool isConnection(const QmlDesigner::ModelNode &modelNode)
{
    return (modelNode.type() == "Connections"
            || modelNode.type() == "QtQuick.Connections"
            || modelNode.type() == "Qt.Connections");

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
        foreach (const ModelNode modelNode, connectionView()->allModelNodes())
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

    ModelNode  modelNode = connectionView()->modelNodeForInternalId(internalId);

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
    foreach (const AbstractProperty &property, modelNode.properties()) {
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
        idLabel =connectionsModelNode.bindingProperty("target").expression();
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

    const PropertyName newName = data(index(rowNumber, TargetPropertyNameRow)).toString().toUtf8();
    const QString source = signalHandlerProperty.source();
    ModelNode connectionNode = signalHandlerProperty.parentModelNode();

    if (!newName.isEmpty()) {
        RewriterTransaction transaction =
            connectionView()->beginRewriterTransaction(QByteArrayLiteral("ConnectionModel::updateSignalName"));
        try {
            connectionNode.signalHandlerProperty(newName).setSource(source);
            connectionNode.removeProperty(signalHandlerProperty.name());
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            QMessageBox::warning(0, tr("Error"), e.description());
        }

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

    if (!newTarget.isEmpty()) {
        RewriterTransaction transaction =
            connectionView()->beginRewriterTransaction(QByteArrayLiteral("ConnectionModel::updateTargetNode"));
        try {
            connectionNode.bindingProperty("target").setExpression(newTarget);
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            QMessageBox::warning(0, tr("Error"), e.description());
        }

        QStandardItem* idItem = item(rowNumber, 0);
        updateCustomData(idItem, signalHandlerProperty);

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
        if (bindingProperty.expression() == QLatin1String("parent"))
            return connection.parentProperty().parentModelNode();
        return connectionView()->modelNodeForId(bindingProperty.expression());
    }

    return ModelNode();
}

void ConnectionModel::addConnection()
{
    ModelNode rootModelNode = connectionView()->rootModelNode();

    if (rootModelNode.isValid() && rootModelNode.metaInfo().isValid()) {

        NodeMetaInfo nodeMetaInfo  = connectionView()->model()->metaInfo("QtQuick.Connections");

        if (nodeMetaInfo.isValid()) {
            RewriterTransaction transaction =
                connectionView()->beginRewriterTransaction(QByteArrayLiteral("ConnectionModel::addConnection"));
            try {
                ModelNode newNode = connectionView()->createModelNode("QtQuick.Connections",
                                                                      nodeMetaInfo.majorVersion(),
                                                                      nodeMetaInfo.minorVersion());

                rootModelNode.nodeAbstractProperty(rootModelNode.metaInfo().defaultPropertyName()).reparentHere(newNode);
                newNode.signalHandlerProperty("onClicked").setSource(QLatin1String("print(\"clicked\")"));

                if (connectionView()->selectedModelNodes().count() == 1
                        && !connectionView()->selectedModelNodes().first().id().isEmpty()) {
                    ModelNode selectedNode = connectionView()->selectedModelNodes().first();
                    newNode.bindingProperty("target").setExpression(selectedNode.id());
                } else {
                    newNode.bindingProperty("target").setExpression(QLatin1String("parent"));
                }
                transaction.commit();
            } catch (Exception &e) { //better save then sorry
                QMessageBox::warning(0, tr("Error"), e.description());
            }
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

void ConnectionModel::deleteConnectionByRow(int currentRow)
{
    signalHandlerPropertyForRow(currentRow).parentModelNode().destroy();
}

void ConnectionModel::handleException()
{
    QMessageBox::warning(0, tr("Error"), m_exceptionError);
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

    if (signalHandlerProperty.isValid()) {
        stringList.append(getPossibleSignalsForConnection(signalHandlerProperty.parentModelNode()));
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
