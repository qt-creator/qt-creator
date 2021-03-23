/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "signallist.h"

#include "signallistdelegate.h"

#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>

#include <metainfo.h>

#include <variantproperty.h>
#include <bindingproperty.h>
#include <signalhandlerproperty.h>
#include <qmldesignerconstants.h>
#include <qmlitemnode.h>
#include <nodeabstractproperty.h>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QTableView>

namespace QmlDesigner {

SignalListModel::SignalListModel(QObject *parent)
    : QStandardItemModel(0, 3, parent)
{
    setHeaderData(TargetColumn, Qt::Horizontal, tr("Item ID"));
    setHeaderData(SignalColumn, Qt::Horizontal, tr("Signal"));
    setHeaderData(ButtonColumn, Qt::Horizontal, "");
}

void SignalListModel::setConnected(int row, bool connected)
{
    for (int col = 0; col < columnCount(); ++col) {
        QModelIndex idx = index(row, col);
        setData(idx, connected, ConnectedRole);
    }
}


SignalListFilterModel::SignalListFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool SignalListFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex targetIndex = sourceModel()->index(sourceRow, SignalListModel::TargetColumn, sourceParent);
    QModelIndex signalIndex = sourceModel()->index(sourceRow, SignalListModel::SignalColumn, sourceParent);

    return (sourceModel()->data(targetIndex).toString().contains(filterRegularExpression())
            || sourceModel()->data(signalIndex).toString().contains(filterRegularExpression()));
}




SignalList::SignalList(QObject *)
    : m_dialog(QPointer<SignalListDialog>())
    , m_model(new SignalListModel(this))
    , m_modelNode()
{
}

SignalList::~SignalList()
{
    hideWidget();
}

void SignalList::prepareDialog()
{
    m_dialog = new SignalListDialog(Core::ICore::dialogParent());
    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_dialog->initialize(m_model);
    m_dialog->setWindowTitle(::QmlDesigner::SignalList::tr("Signal List for %1")
                             .arg(m_modelNode.validId()));

    auto *delegate = static_cast<SignalListDelegate *>(m_dialog->tableView()->itemDelegate());
    connect(delegate, &SignalListDelegate::connectClicked, this, &SignalList::connectClicked);
}

void SignalList::showWidget()
{
    prepareDialog();
    m_dialog->show();
    m_dialog->raise();
}

void SignalList::hideWidget()
{
    if (m_dialog)
        m_dialog->close();

    m_dialog = nullptr;
}

SignalList* SignalList::showWidget(const ModelNode &modelNode)
{
    auto signalList = new SignalList();
    signalList->setModelNode(modelNode);
    signalList->prepareSignals();
    signalList->showWidget();

    connect(signalList->m_dialog, &QDialog::destroyed,
            [signalList]() { signalList->deleteLater(); } );

    return signalList;
}

void SignalList::setModelNode(const ModelNode &modelNode)
{
    if (modelNode.isValid())
        m_modelNode = modelNode;
}

void SignalList::prepareSignals()
{
    if (!m_modelNode.isValid())
        return;

    QList<QmlConnections> connections = QmlFlowViewNode::getAssociatedConnections(m_modelNode);

    for (ModelNode &node : m_modelNode.view()->allModelNodes()) {
        // Collect all items which contain at least one of the specified signals
        const PropertyNameList signalNames = node.metaInfo().signalNames();
        // Put the signals into a QSet to avoid duplicates
        auto signalNamesSet = QSet<PropertyName>(signalNames.begin(), signalNames.end());
        for (const PropertyName &signal : signalNamesSet) {
            if (QmlFlowViewNode::st_mouseSignals.contains(signal))
                appendSignalToModel(connections, node, signal);
        }

        // Gather valid properties and aliases from components
        for (const PropertyName &property : node.metaInfo().propertyNames()) {
            const TypeName propertyType = node.metaInfo().propertyTypeName(property);
            const NodeMetaInfo info = m_modelNode.model()->metaInfo(propertyType);
            // Collect all items which contain at least one of the specified signals
            const PropertyNameList signalNames = info.signalNames();
            // Put the signals into a QSet to avoid duplicates
            auto signalNamesSet = QSet<PropertyName>(signalNames.begin(), signalNames.end());
            for (const PropertyName &signal : signalNamesSet) {
                if (QmlFlowViewNode::st_mouseSignals.contains(signal))
                    appendSignalToModel(connections, node, signal, property);
            }
        }
    }
}

void SignalList::connectClicked(const QModelIndex &modelIndex)
{
    auto proxyModel = static_cast<const SignalListFilterModel *>(modelIndex.model());
    QModelIndex mappedModelIndex = proxyModel->mapToSource(modelIndex);
    bool connected = mappedModelIndex.data(SignalListModel::ConnectedRole).toBool();

    if (!connected)
        addConnection(mappedModelIndex);
    else
        removeConnection(mappedModelIndex);
}

void SignalList::appendSignalToModel(const QList<QmlConnections> &connections,
                                     ModelNode &node,
                                     const PropertyName &signal,
                                     const PropertyName &property)
{
    QStandardItem *idItem = new QStandardItem();
    QString id(node.validId());
    if (!property.isEmpty())
        id += "." + QString::fromLatin1(property);

    idItem->setData(id, Qt::DisplayRole);

    QStandardItem *signalItem = new QStandardItem();
    signalItem->setData(signal, Qt::DisplayRole);

    QStandardItem *buttonItem = new QStandardItem();

    idItem->setData(false, SignalListModel::ConnectedRole);
    signalItem->setData(false, SignalListModel::ConnectedRole);
    buttonItem->setData(false, SignalListModel::ConnectedRole);

    for (const QmlConnections &connection : connections) {
        if (connection.target() == id) {
            for (const SignalHandlerProperty &property : connection.signalProperties()) {
                auto signalWithoutPrefix = SignalHandlerProperty::prefixRemoved(property.name());
                if (signalWithoutPrefix == signal) {
                    buttonItem->setData(connection.modelNode().internalId(),
                                        SignalListModel::ConnectionsInternalIdRole);

                    idItem->setData(true, SignalListModel::ConnectedRole);
                    signalItem->setData(true, SignalListModel::ConnectedRole);
                    buttonItem->setData(true, SignalListModel::ConnectedRole);
                }
            }
        }
    }
    m_model->appendRow({idItem, signalItem, buttonItem});
}

void SignalList::addConnection(const QModelIndex &modelIndex)
{
    const QModelIndex targetModelIndex = modelIndex.siblingAtColumn(SignalListModel::TargetColumn);
    const QModelIndex signalModelIndex = modelIndex.siblingAtColumn(SignalListModel::SignalColumn);
    const QModelIndex buttonModelIndex = modelIndex.siblingAtColumn(SignalListModel::ButtonColumn);
    const PropertyName signalName = m_model->data(signalModelIndex,
                                                 Qt::DisplayRole).toByteArray();

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_CONNECTION_ADDED);

    AbstractView *view = m_modelNode.view();
    const ModelNode rootModelNode = view->rootModelNode();

    if (rootModelNode.isValid() && rootModelNode.metaInfo().isValid()) {
        NodeMetaInfo nodeMetaInfo = view->model()->metaInfo("QtQuick.Connections");
        if (nodeMetaInfo.isValid()) {
            view->executeInTransaction("ConnectionModel::addConnection", [=, &rootModelNode](){
                ModelNode newNode = view->createModelNode("QtQuick.Connections",
                                                          nodeMetaInfo.majorVersion(),
                                                          nodeMetaInfo.minorVersion());
                const QString source = m_modelNode.validId() + ".trigger()";

                if (QmlItemNode::isValidQmlItemNode(m_modelNode))
                    m_modelNode.nodeAbstractProperty("data").reparentHere(newNode);
                else
                    rootModelNode.nodeAbstractProperty(rootModelNode.metaInfo().defaultPropertyName()).reparentHere(newNode);

                const QString expression = m_model->data(targetModelIndex, Qt::DisplayRole).toString();
                newNode.bindingProperty("target").setExpression(expression);
                newNode.signalHandlerProperty(SignalHandlerProperty::prefixAdded(signalName)).setSource(source);

                m_model->setConnected(modelIndex.row(), true);
                m_model->setData(buttonModelIndex, newNode.internalId(), SignalListModel::ConnectionsInternalIdRole);
            });
        }
    }
}

void SignalList::removeConnection(const QModelIndex &modelIndex)
{
    const QModelIndex signalModelIndex = modelIndex.siblingAtColumn(SignalListModel::SignalColumn);
    const QModelIndex buttonModelIndex = modelIndex.siblingAtColumn(SignalListModel::ButtonColumn);
    const PropertyName signalName = m_model->data(signalModelIndex,
                                                 Qt::DisplayRole).toByteArray();
    const int connectionInternalId = m_model->data(buttonModelIndex,
                                                  SignalListModel::ConnectionsInternalIdRole).toInt();

    AbstractView *view = m_modelNode.view();
    const ModelNode connectionModelNode = view->modelNodeForInternalId(connectionInternalId);
    SignalHandlerProperty targetSignal;

    if (connectionModelNode.isValid())
        targetSignal = connectionModelNode.signalHandlerProperty(signalName);

    ModelNode node = targetSignal.parentModelNode();
    if (node.isValid()) {
        view->executeInTransaction("ConnectionModel::removeConnection", [=, &node](){
            QList<SignalHandlerProperty> allSignals = node.signalProperties();
            if (allSignals.size() > 1) {
                const auto targetSignalWithPrefix = SignalHandlerProperty::prefixAdded(targetSignal.name());
                for (const SignalHandlerProperty &signal : allSignals)
                    if (signal.name() == targetSignalWithPrefix)
                        node.removeProperty(targetSignalWithPrefix);
            } else {
                node.destroy();
            }
            m_model->setConnected(modelIndex.row(), false);
            m_model->setData(buttonModelIndex, QVariant(), SignalListModel::ConnectionsInternalIdRole);
        });
    }
}

} // QmlDesigner namespace
