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
    QObject* modelNodeBackendObject = modelNodeBackend.value<QObject*>();

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
    if (m_modelNode.isValid()
            && m_modelNode.metaInfo().isValid()
            && m_modelNode.metaInfo().isSubclassOf("QtQuick.Controls.TabView")) {

        foreach (const QmlDesigner::ModelNode &childModelNode, m_modelNode.defaultNodeAbstractProperty().directSubNodes()) {
            if (childModelNode.metaInfo().isValid()
                    && childModelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab")) {
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
