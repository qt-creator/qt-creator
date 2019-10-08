/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qmlvisualnode.h"
#include <metainfo.h>
#include "qmlchangeset.h"
#include "nodelistproperty.h"
#include "nodehints.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"
#include "invalidmodelnodeexception.h"
#include "itemlibraryinfo.h"

#include "plaintexteditmodifier.h"
#include "rewriterview.h"
#include "modelmerger.h"
#include "rewritingexception.h"

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>

namespace QmlDesigner {

bool QmlVisualNode::isItemOr3DNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Item"))
        return true;

    if (modelNode.metaInfo().isSubclassOf("QtQuick3D.Node"))
        return true;

    return false;
}

bool QmlVisualNode::isValid() const
{
    return isValidQmlVisualNode(modelNode());
}

bool QmlVisualNode::isValidQmlVisualNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode)
            && modelNode.metaInfo().isValid()
            && isItemOr3DNode(modelNode);
}

bool QmlVisualNode::isRootNode() const
{
    return modelNode().isValid() && modelNode().isRootNode();
}


QList<QmlVisualNode> QmlVisualNode::children() const
{
    QList<ModelNode> childrenList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("children"))
                childrenList.append(modelNode().nodeListProperty("children").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            for (const ModelNode &node : modelNode().nodeListProperty("data").toModelNodeList()) {
                if (QmlVisualNode::isValidQmlVisualNode(node))
                    childrenList.append(node);
            }
        }
    }

    return toQmlVisualNodeList(childrenList);
}

QList<QmlObjectNode> QmlVisualNode::resources() const
{
    QList<ModelNode> resourcesList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("resources"))
                resourcesList.append(modelNode().nodeListProperty("resources").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            for (const ModelNode &node : modelNode().nodeListProperty("data").toModelNodeList()) {
                if (!QmlItemNode::isValidQmlItemNode(node))
                    resourcesList.append(node);
            }
        }
    }

    return toQmlObjectNodeList(resourcesList);
}

QList<QmlObjectNode> QmlVisualNode::allDirectSubNodes() const
{
    return toQmlObjectNodeList(modelNode().directSubModelNodes());
}

bool QmlVisualNode::hasChildren() const
{
    if (modelNode().hasNodeListProperty("children"))
        return true;

    return !children().isEmpty();
}

bool QmlVisualNode::hasResources() const
{
    if (modelNode().hasNodeListProperty("resources"))
        return true;

    return !resources().isEmpty();
}

const QList<QmlVisualNode> QmlVisualNode::allDirectSubModelNodes() const
{
    return toQmlVisualNodeList(modelNode().directSubModelNodes());
}

const QList<QmlVisualNode> QmlVisualNode::allSubModelNodes() const
{
    return toQmlVisualNodeList(modelNode().allSubModelNodes());
}

bool QmlVisualNode::hasAnySubModelNodes() const
{
    return modelNode().hasAnySubModelNodes();
}

QmlModelStateGroup QmlVisualNode::states() const
{
    if (isValid())
        return QmlModelStateGroup(modelNode());
    else
        return QmlModelStateGroup();
}

QList<ModelNode> toModelNodeList(const QList<QmlVisualNode> &qmlVisualNodeList)
{
    QList<ModelNode> modelNodeList;

    for (const QmlVisualNode &QmlVisualNode : qmlVisualNodeList)
        modelNodeList.append(QmlVisualNode.modelNode());

    return modelNodeList;
}

QList<QmlVisualNode> toQmlVisualNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlVisualNode> QmlVisualNodeList;

    for (const ModelNode &modelNode : modelNodeList) {
        if (QmlVisualNode::isValidQmlVisualNode(modelNode))
            QmlVisualNodeList.append(modelNode);
    }

    return QmlVisualNodeList;
}

QStringList QmlModelStateGroup::names() const
{
    QStringList returnList;

    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        for (const ModelNode &node : modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState::isValidQmlModelState(node))
                returnList.append(QmlModelState(node).name());
        }
    }
    return returnList;
}

QList<QmlModelState> QmlModelStateGroup::allStates() const
{
    QList<QmlModelState> returnList;

    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        for (const ModelNode &node : modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState::isValidQmlModelState(node))
                returnList.append(node);
        }
    }
    return returnList;
}

QmlModelState QmlModelStateGroup::addState(const QString &name)
{
    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    ModelNode newState = QmlModelState::createQmlState(
                modelNode().view(), {{PropertyName("name"), QVariant(name)}});
    modelNode().nodeListProperty("states").reparentHere(newState);

    return newState;
}

void QmlModelStateGroup::removeState(const QString &name)
{
    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (state(name).isValid())
        state(name).modelNode().destroy();
}

QmlModelState QmlModelStateGroup::state(const QString &name) const
{
    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        for (const ModelNode &node : modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState(node).name() == name)
                return node;
        }
    }
    return QmlModelState();
}

} //QmlDesigner
