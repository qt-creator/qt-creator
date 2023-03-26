// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tabviewindexmodel.h"

#include <metainfo.h>
#include <variantproperty.h>
#include <nodelistproperty.h>

TabViewIndexModel::TabViewIndexModel(QObject *parent) :
    QObject(parent)
{
}

void TabViewIndexModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    auto modelNodeBackendObject = modelNodeBackend.value<QObject*>();

    if (modelNodeBackendObject)
        setModelNode(modelNodeBackendObject->property("modelNode").value<QmlDesigner::ModelNode>());

    setupModel();
    emit modelNodeBackendChanged();
}

void TabViewIndexModel::setModelNode(const QmlDesigner::ModelNode &modelNode)
{
    m_modelNode = modelNode;
}

QStringList TabViewIndexModel::tabViewIndexModel() const
{
    return m_tabViewIndexModel;
}

void TabViewIndexModel::setupModel()
{
    m_tabViewIndexModel.clear();
    if (m_modelNode.metaInfo().isQtQuickControlsTabView()) {
        const QList<QmlDesigner::ModelNode> childModelNodes
            = m_modelNode.defaultNodeAbstractProperty().directSubNodes();
        for (const QmlDesigner::ModelNode &childModelNode : childModelNodes) {
            if (childModelNode.metaInfo().isQtQuickControlsTab()) {
                QmlDesigner::QmlItemNode itemNode(childModelNode);
                if (itemNode.isValid()) {
                    m_tabViewIndexModel.append(itemNode.instanceValue("title").toString());
                }
            }
        }
    }
}

void TabViewIndexModel::registerDeclarativeType()
{
    qmlRegisterType<TabViewIndexModel>("HelperWidgets",2,0,"TabViewIndexModel");
}

QVariant TabViewIndexModel::modelNodeBackend() const
{
    return QVariant::fromValue(m_modelNode);
}
