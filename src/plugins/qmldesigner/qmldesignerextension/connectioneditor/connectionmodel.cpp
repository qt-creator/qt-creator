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

#include <QMessageBox>
#include <QTableView>
#include <QTimer>
#include <QItemEditorFactory>
#include <QStyleFactory>

namespace {

QStringList prependOnForSignalHandler(const QStringList &signalNames)
{
    QStringList signalHandlerNames;
    foreach (const QString &signalName, signalNames) {
        QString signalHandlerName = signalName;
        if (!signalHandlerName.isEmpty()) {
            QChar firstChar = signalHandlerName.at(0).toUpper();
            signalHandlerName[0] = firstChar;
            signalHandlerName.prepend(QLatin1String("on"));
            signalHandlerNames.append(signalHandlerName);
        }
    }
    return signalHandlerNames;
}

QStringList propertyNameListToStringList(const QmlDesigner::PropertyNameList &propertyNameList)
{
    QStringList stringList;
    foreach (QmlDesigner::PropertyName propertyName, propertyNameList) {
        stringList << QString::fromLatin1(propertyName);
    }
    return stringList;
}

bool isConnection(const QmlDesigner::ModelNode &modelNode)
{
    return (modelNode.type() == "Connections"
            || modelNode.type() == "QtQuick.Connections"
            || modelNode.type() == "Qt.Connections");

}

enum ColumnRoles {
    TargetModelNodeRow = 0,
    TargetPropertyNameRow = 1,
    SourceRow = 2
};

} //namespace

namespace QmlDesigner {

namespace Internal {

ConnectionModel::ConnectionModel(ConnectionView *parent) : QStandardItemModel(parent), m_connectionView(parent), m_lock(false)
{
    connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(handleDataChanged(QModelIndex,QModelIndex)));
}

void ConnectionModel::resetModel()
{
    beginResetModel();
    clear();

    QStringList labels;

    labels << tr("Target");
    labels << tr("Signal Handler");
    labels <<tr("Action");

    setHorizontalHeaderLabels(labels);

    if (connectionView()->isAttached()) {
        foreach (const ModelNode modelNode, connectionView()->allModelNodes()) {
            addModelNode(modelNode);
        }
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
        return modelNode.signalHandlerProperty(targetPropertyName.toLatin1());

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
    const QString propertyName = QString::fromLatin1(signalHandlerProperty.name());
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
        QTimer::singleShot(200, this, SLOT(handleException()));
    }

}

void ConnectionModel::updateSignalName(int rowNumber)
{
    SignalHandlerProperty signalHandlerProperty = signalHandlerPropertyForRow(rowNumber);

    const PropertyName newName = data(index(rowNumber, TargetPropertyNameRow)).toString().toLatin1();
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

ConnectionDelegate::ConnectionDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
    static QItemEditorFactory *factory = 0;
    if (factory == 0) {
        factory = new QItemEditorFactory;
        QItemEditorCreatorBase *creator
                = new QItemEditorCreator<ConnectionComboBox>("text");
        factory->registerEditor(QVariant::String, creator);
    }

    setItemEditorFactory(factory);
}

QWidget *ConnectionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

    QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);

    const ConnectionModel *connectionModel = qobject_cast<const ConnectionModel*>(index.model());

    //model->connectionView()->allModelNodes();

    ConnectionComboBox *bindingComboBox = qobject_cast<ConnectionComboBox*>(widget);

    if (!connectionModel) {
        qWarning() << "ConnectionDelegate::createEditor no model";
        return widget;
    }

    if (!connectionModel->connectionView()) {
        qWarning() << "ConnectionDelegate::createEditor no connection view";
        return widget;
    }

    if (!bindingComboBox) {
        qWarning() << "ConnectionDelegate::createEditor no bindingComboBox";
        return widget;
    }

    switch (index.column()) {
    case TargetModelNodeRow: {
        foreach (const ModelNode &modelNode, connectionModel->connectionView()->allModelNodes()) {
            if (!modelNode.id().isEmpty()) {
                bindingComboBox->addItem(modelNode.id());
            }
        }
    } break;
    case TargetPropertyNameRow: {
        bindingComboBox->addItems(prependOnForSignalHandler(connectionModel->getSignalsForRow(index.row())));
    } break;
    case SourceRow: {
        ModelNode rootModelNode = connectionModel->connectionView()->rootModelNode();
        if (QmlItemNode::isValidQmlItemNode(rootModelNode) && !rootModelNode.id().isEmpty()) {

            QString itemText = tr("Change to default state");
            QString source = QString::fromLatin1("{ %1.state = \"\" }").arg(rootModelNode.id());
            bindingComboBox->addItem(itemText, source);

            foreach (const QmlModelState &state, QmlItemNode(rootModelNode).states().allStates()) {
                QString itemText = tr("Change state to %1").arg(state.name());
                QString source = QString::fromLatin1("{ %1.state = \"%2\" }").arg(rootModelNode.id()).arg(state.name());
                bindingComboBox->addItem(itemText, source);
            }
        }
    } break;

    default: qWarning() << "ConnectionDelegate::createEditor column" << index.column();
    }

    connect(bindingComboBox, SIGNAL(activated(QString)), this, SLOT(emitCommitData(QString)));

    return widget;
}

void ConnectionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;
    QStyledItemDelegate::paint(painter, opt, index);
}

void ConnectionDelegate::emitCommitData(const QString & /*text*/)
{
    ConnectionComboBox *bindingComboBox = qobject_cast<ConnectionComboBox*>(sender());

    emit commitData(bindingComboBox);
}

ConnectionComboBox::ConnectionComboBox(QWidget *parent) : QComboBox(parent)
{
    static QScopedPointer<QStyle> style(QStyleFactory::create(QLatin1String("windows")));
    setEditable(true);
    if (style)
        setStyle(style.data());
}

QString ConnectionComboBox::text() const
{
    int index = findText(currentText());
    if (index > -1) {
        QVariant variantData = itemData(index);
        if (variantData.isValid())
            return variantData.toString();
    }

    return currentText();
}

void ConnectionComboBox::setText(const QString &text)
{
    setEditText(text);
}

} // namespace Internal

} // namespace QmlDesigner
