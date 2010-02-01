/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlitemnode.h"
#include <metainfo.h>
#include "qmlview.h"
#include "qmlchangeset.h"
#include "variantproperty.h"
#include "nodelistproperty.h"
#include "nodeinstance.h"
#include "qmlanchors.h"
#include "invalidmodelnodeexception.h"
#include "rewritertransaction.h"
#include "qmlmodelview.h"
#include "mathutils.h"

namespace QmlDesigner {


bool QmlItemNode::isValid() const
{
    return QmlModelNodeFacade::isValid() && modelNode().metaInfo().isSubclassOf("Qt/Item", 4, 6);
}

bool QmlItemNode::isRootNode() const
{
    return modelNode().isValid() && modelNode().isRootNode();
}

QStringList QmlModelStateGroup::names() const
{
    QStringList returnList;

    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        foreach (const ModelNode &node, modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState(node).isValid())
                returnList.append(QmlModelState(node).name());
        }
    }
    return returnList;
}

/**
  \brief Returns list of states (without 'base state').
  The list contains all states defined by this item.
  */
QmlModelStateGroup QmlItemNode::states() const
{
    if (isValid())
        return QmlModelStateGroup(modelNode());
    else
        return QmlModelStateGroup();
}

QList<QmlItemNode> QmlItemNode::children() const
{
    QList<QmlItemNode> returnList;

    if (isValid()) {

        QList<ModelNode> modelNodeList;

        if (modelNode().hasProperty("children")) {
            Q_ASSERT(modelNode().property("children").isNodeListProperty());
            modelNodeList.append(modelNode().nodeListProperty("children").toModelNodeList());
        }

        if (modelNode().hasProperty("data")) {
            Q_ASSERT(modelNode().property("data").isNodeListProperty());
            modelNodeList.append(modelNode().nodeListProperty("data").toModelNodeList());
        }

        foreach (const ModelNode &modelNode, modelNodeList) {
            if (QmlItemNode(modelNode).isValid())  //if ModelNode is FxItem
                returnList.append(modelNode);
        }
    }
    return returnList;
}

QList<QmlObjectNode> QmlItemNode::resources() const
{
    QList<QmlObjectNode> returnList;

    if (isValid()) {
        QList<ModelNode> modelNodeList;
        if (modelNode().hasProperty("resources")) {
            Q_ASSERT(modelNode().property("resources").isNodeListProperty());
            modelNodeList.append(modelNode().nodeListProperty("resources").toModelNodeList());
        }

        if (modelNode().hasProperty("data")) {
            Q_ASSERT(modelNode().property("data").isNodeListProperty());
            modelNodeList.append(modelNode().nodeListProperty("data").toModelNodeList());
        }

        foreach (const ModelNode &node, modelNodeList) {
            if (!QmlItemNode(node).isValid()) //if ModelNode is no FxItem
                returnList.append(node);
        }
    }
    return returnList;
}

QmlAnchors QmlItemNode::anchors() const
{
    return QmlAnchors(*this);
}

bool QmlItemNode::hasChildren() const
{
    return !children().isEmpty();
}

bool QmlItemNode::hasResources() const
{
    return !resources().isEmpty();
}

bool QmlItemNode::instanceHasAnchors() const
{
    return anchors().instanceHasAnchors();
}

bool QmlItemNode::hasShowContent() const
{
    return nodeInstance().hasContent();
}

bool QmlItemNode::canReparent() const
{
    return QmlObjectNode::canReparent() && !anchors().instanceHasAnchors() && !instanceIsAnchoredBy();
}

bool QmlItemNode::instanceIsAnchoredBy() const
{
    return nodeInstance().isAnchoredBy();
}

QRectF  QmlItemNode::instanceBoundingRect() const
{
    return nodeInstance().boundingRect();
}

QTransform  QmlItemNode::instanceTransform() const
{
    return nodeInstance().transform();
}

QTransform QmlItemNode::instanceSceneTransform() const
{
    return nodeInstance().sceneTransform();
}

QPointF QmlItemNode::instancePosition() const
{
    return nodeInstance().position();
}

QSizeF QmlItemNode::instanceSize() const
{
    return nodeInstance().size();
}

void QmlItemNode::paintInstance(QPainter *painter) const
{
    if (nodeInstance().isValid())
        nodeInstance().paint(painter);
}

void QmlItemNode::setSize(const QSizeF &size)
{
    RewriterTransaction transaction = qmlModelView()->beginRewriterTransaction();
    setVariantProperty("width", qRound(size.width()) - (nodeInstance().penWidth()));
    setVariantProperty("height", qRound(size.height()) - (nodeInstance().penWidth()));
}

void QmlItemNode::setPosition(const QPointF &position)
{
    RewriterTransaction transaction = qmlModelView()->beginRewriterTransaction();
    setVariantProperty("x", qRound(position.x()));
    setVariantProperty("y", qRound(position.y()));
}

void QmlItemNode::setPositionWithBorder(const QPointF &position)
{
    RewriterTransaction transaction = qmlModelView()->beginRewriterTransaction();
    setVariantProperty("x", qRound(position.x()) + (nodeInstance().penWidth() / 2));
    setVariantProperty("y", qRound(position.y()) + (nodeInstance().penWidth() / 2));
}

void QmlItemNode::selectNode()
{
    modelNode().selectNode();
}

void QmlItemNode::deselectNode()
{
    modelNode().deselectNode();
}

bool QmlItemNode::isSelected() const
{
    return modelNode().isSelected();
}

QList<QmlModelState> QmlModelStateGroup::allStates() const
{
    QList<QmlModelState> returnList;

    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        foreach (const ModelNode &node, modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState(node).isValid())
                returnList.append(QmlModelState(node));
        }
    }
    return returnList;
}

QString QmlItemNode::simplfiedTypeName() const
{
    return modelNode().simplifiedTypeName();
}

uint qHash(const QmlItemNode &node)
{
    return qHash(node.modelNode());
}

QmlModelState QmlModelStateGroup::addState(const QString &name)
{
    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);


    PropertyListType propertyList;
    propertyList.append(qMakePair(QString("name"), QVariant(name)));

    ModelNode newState = modelNode().view()->createModelNode("Qt/State", 4, 6, propertyList);
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
        foreach (const ModelNode &node, modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState(node).name() == name)
                return node;
        }
    }
    return QmlModelState();
}

QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &qmlItemNodeList)
{
    QList<ModelNode> modelNodeList;

    foreach (const QmlItemNode &qmlItemNode, qmlItemNodeList)
        modelNodeList.append(qmlItemNode.modelNode());

    return modelNodeList;
}

QList<QmlItemNode> toQmlItemNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlItemNode> qmlItemNodeList;

    foreach (const ModelNode &modelNode, modelNodeList) {
        QmlItemNode itemNode(modelNode);
        if (itemNode.isValid())
            qmlItemNodeList.append(itemNode);
    }

    return qmlItemNodeList;
}

const QList<QmlItemNode> QmlItemNode::allDirectSubModelNodes() const
{
    return toQmlItemNodeList(modelNode().allDirectSubModelNodes());
}

const QList<QmlItemNode> QmlItemNode::allSubModelNodes() const
{
    return toQmlItemNodeList(modelNode().allSubModelNodes());
}

bool QmlItemNode::hasAnySubModelNodes() const
{
    return modelNode().hasAnySubModelNodes();
}

} //QmlDesigner
