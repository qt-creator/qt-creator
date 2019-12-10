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

#include "qmlitemnode.h"
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

bool QmlItemNode::isItemOrWindow(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Item"))
        return true;

    if (modelNode.metaInfo().isGraphicalItem()  && modelNode.isRootNode())
        return true;

    return false;
}

QmlItemNode QmlItemNode::createQmlItemNode(AbstractView *view,
                                           const ItemLibraryEntry &itemLibraryEntry,
                                           const QPointF &position,
                                           QmlItemNode parentQmlItemNode)
{
    return QmlItemNode(createQmlObjectNode(view, itemLibraryEntry, position, parentQmlItemNode));
}

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view, const QString &imageName, const QPointF &position, QmlItemNode parentQmlItemNode)
{
    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = QmlItemNode(view->rootModelNode());

    NodeAbstractProperty parentProperty = parentQmlItemNode.defaultNodeAbstractProperty();

    return QmlItemNode::createQmlItemNodeFromImage(view, imageName, position, parentProperty);
}

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view, const QString &imageName, const QPointF &position, NodeAbstractProperty parentproperty)
{
    QmlItemNode newQmlItemNode;

    if (parentproperty.isValid() && view->model()->hasNodeMetaInfo("QtQuick.Image")) {
        view->executeInTransaction("QmlItemNode::createQmlItemNodeFromImage", [=, &newQmlItemNode, &parentproperty](){
            NodeMetaInfo metaInfo = view->model()->metaInfo("QtQuick.Image");
            QList<QPair<PropertyName, QVariant> > propertyPairList;
            propertyPairList.append({PropertyName("x"), QVariant(qRound(position.x()))});
            propertyPairList.append({PropertyName("y"), QVariant(qRound(position.y()))});

            QString relativeImageName = imageName;

            //use relative path
            if (QFileInfo::exists(view->model()->fileUrl().toLocalFile())) {
                QDir fileDir(QFileInfo(view->model()->fileUrl().toLocalFile()).absolutePath());
                relativeImageName = fileDir.relativeFilePath(imageName);
                propertyPairList.append({PropertyName("source"), QVariant(relativeImageName)});
            }

            newQmlItemNode = QmlItemNode(view->createModelNode("QtQuick.Image", metaInfo.majorVersion(), metaInfo.minorVersion(), propertyPairList));
            parentproperty.reparentHere(newQmlItemNode);

            newQmlItemNode.setId(view->generateNewId(QLatin1String("image")));

            newQmlItemNode.modelNode().variantProperty("fillMode").setEnumeration("Image.PreserveAspectFit");

            Q_ASSERT(newQmlItemNode.isValid());
        });
    }

    return newQmlItemNode;
}

bool QmlItemNode::isValid() const
{
    return isValidQmlItemNode(modelNode());
}

bool QmlItemNode::isValidQmlItemNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isValid() && isItemOrWindow(modelNode);
}

QList<QmlItemNode> QmlItemNode::children() const
{
    QList<ModelNode> childrenList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("children"))
                childrenList.append(modelNode().nodeListProperty("children").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            foreach (const ModelNode &node, modelNode().nodeListProperty("data").toModelNodeList()) {
                if (QmlItemNode::isValidQmlItemNode(node))
                    childrenList.append(node);
            }
        }
    }

    return toQmlItemNodeList(childrenList);
}

QList<QmlObjectNode> QmlItemNode::resources() const
{
    QList<ModelNode> resourcesList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("resources"))
                resourcesList.append(modelNode().nodeListProperty("resources").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            foreach (const ModelNode &node, modelNode().nodeListProperty("data").toModelNodeList()) {
                if (!QmlItemNode::isValidQmlItemNode(node))
                    resourcesList.append(node);
            }
        }
    }

    return toQmlObjectNodeList(resourcesList);
}

QList<QmlObjectNode> QmlItemNode::allDirectSubNodes() const
{
    return toQmlObjectNodeList(modelNode().directSubModelNodes());
}

QmlAnchors QmlItemNode::anchors() const
{
    return QmlAnchors(*this);
}

bool QmlItemNode::hasChildren() const
{
    if (modelNode().hasNodeListProperty("children"))
        return true;

    return !children().isEmpty();
}

bool QmlItemNode::hasResources() const
{
    if (modelNode().hasNodeListProperty("resources"))
        return true;

    return !resources().isEmpty();
}

bool QmlItemNode::instanceHasAnchors() const
{
    return anchors().instanceHasAnchors();
}

bool QmlItemNode::instanceHasShowContent() const
{
    return nodeInstance().hasContent();
}

bool QmlItemNode::instanceCanReparent() const
{
    return QmlObjectNode::instanceCanReparent() && !anchors().instanceHasAnchors() && !instanceIsAnchoredBySibling();
}

bool QmlItemNode::instanceIsAnchoredBySibling() const
{
    return nodeInstance().isAnchoredBySibling();
}

bool QmlItemNode::instanceIsAnchoredByChildren() const
{
    return nodeInstance().isAnchoredByChildren();
}

bool QmlItemNode::instanceIsMovable() const
{
    return nodeInstance().isMovable();
}

bool QmlItemNode::instanceIsResizable() const
{
    return nodeInstance().isResizable();
}

bool QmlItemNode::instanceIsInLayoutable() const
{
     return nodeInstance().isInLayoutable();
}

bool QmlItemNode::instanceHasRotationTransform() const
{
    return nodeInstance().transform().type() > QTransform::TxScale;
}

bool itemIsMovable(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab"))
        return false;

    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    return NodeHints::fromModelNode(modelNode).isMovable();
}

bool itemIsResizable(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab"))
        return false;

    return NodeHints::fromModelNode(modelNode).isResizable();
}

bool QmlItemNode::modelIsMovable() const
{
    return !modelNode().hasBindingProperty("x")
            && !modelNode().hasBindingProperty("y")
            && itemIsMovable(modelNode())
            && !modelIsInLayout();
}

bool QmlItemNode::modelIsResizable() const
{
    return !modelNode().hasBindingProperty("width")
            && !modelNode().hasBindingProperty("height")
            && itemIsResizable(modelNode())
            && !modelIsInLayout();
}

bool QmlItemNode::modelIsInLayout() const
{
    if (modelNode().hasParentProperty()) {
        ModelNode parentModelNode = modelNode().parentProperty().parentModelNode();
        if (QmlItemNode::isValidQmlItemNode(parentModelNode)
                && parentModelNode.metaInfo().isLayoutable())
            return true;

        return NodeHints::fromModelNode(parentModelNode).doesLayoutChildren();
    }

    return false;
}

QRectF  QmlItemNode::instanceBoundingRect() const
{
    return QRectF(QPointF(0, 0), nodeInstance().size());
}

QRectF  QmlItemNode::instanceSceneBoundingRect() const
{
    return QRectF(instanceScenePosition(), nodeInstance().size());
}

QRectF QmlItemNode::instancePaintedBoundingRect() const
{
    return nodeInstance().boundingRect();
}

QRectF QmlItemNode::instanceContentItemBoundingRect() const
{
    return nodeInstance().contentItemBoundingRect();
}

QTransform  QmlItemNode::instanceTransform() const
{
    return nodeInstance().transform();
}

QTransform QmlItemNode::instanceTransformWithContentTransform() const
{
    return nodeInstance().transform() * nodeInstance().contentTransform();
}

QTransform QmlItemNode::instanceTransformWithContentItemTransform() const
{
    return nodeInstance().transform() * nodeInstance().contentItemTransform();
}

QTransform QmlItemNode::instanceSceneTransform() const
{
    return nodeInstance().sceneTransform();
}

QTransform QmlItemNode::instanceSceneContentItemTransform() const
{
    return nodeInstance().sceneTransform() * nodeInstance().contentItemTransform();
}

QPointF QmlItemNode::instanceScenePosition() const
{
    if (hasInstanceParentItem())
        return instanceParentItem().instanceSceneTransform().map(nodeInstance().position());
     else if (modelNode().hasParentProperty() && QmlItemNode::isValidQmlItemNode(modelNode().parentProperty().parentModelNode()))
        return QmlItemNode(modelNode().parentProperty().parentModelNode()).instanceSceneTransform().map(nodeInstance().position());

    return {};
}

QPointF QmlItemNode::instancePosition() const
{
    return nodeInstance().position();
}

QSizeF QmlItemNode::instanceSize() const
{
    return nodeInstance().size();
}

int QmlItemNode::instancePenWidth() const
{
    return nodeInstance().penWidth();
}

bool QmlItemNode::instanceIsRenderPixmapNull() const
{
    return nodeInstance().renderPixmap().isNull();
}

QPixmap QmlItemNode::instanceRenderPixmap() const
{
    return nodeInstance().renderPixmap();
}

QPixmap QmlItemNode::instanceBlurredRenderPixmap() const
{
    return nodeInstance().blurredRenderPixmap();
}

uint qHash(const QmlItemNode &node)
{
    return qHash(node.modelNode());
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
        if (QmlItemNode::isValidQmlItemNode(modelNode))
            qmlItemNodeList.append(modelNode);
    }

    return qmlItemNodeList;
}

const QList<QmlItemNode> QmlItemNode::allDirectSubModelNodes() const
{
    return toQmlItemNodeList(modelNode().directSubModelNodes());
}

const QList<QmlItemNode> QmlItemNode::allSubModelNodes() const
{
    return toQmlItemNodeList(modelNode().allSubModelNodes());
}

bool QmlItemNode::hasAnySubModelNodes() const
{
    return modelNode().hasAnySubModelNodes();
}

void QmlItemNode::setPosition(const QPointF &position)
{
    if (!hasBindingProperty("x")
            && !anchors().instanceHasAnchor(AnchorLineLeft)
            && !anchors().instanceHasAnchor(AnchorLineHorizontalCenter))
        setVariantProperty("x", qRound(position.x()));

    if (!hasBindingProperty("y")
            && !anchors().instanceHasAnchor(AnchorLineTop)
            && !anchors().instanceHasAnchor(AnchorLineVerticalCenter))
        setVariantProperty("y", qRound(position.y()));
}

void QmlItemNode::setPostionInBaseState(const QPointF &position)
{
    modelNode().variantProperty("x").setValue(qRound(position.x()));
    modelNode().variantProperty("y").setValue(qRound(position.y()));
}

bool QmlItemNode::isInLayout() const
{
    if (isValid() && hasNodeParent()) {

        ModelNode parent = modelNode().parentProperty().parentModelNode();

        if (parent.isValid() && parent.metaInfo().isValid())
            return parent.metaInfo().isSubclassOf("QtQuick.Layouts.Layout");
    }

    return false;
}

bool QmlItemNode::canBereparentedTo(const ModelNode &potentialParent) const
{
    if (!NodeHints::fromModelNode(potentialParent).canBeContainerFor(modelNode()))
        return false;
    return NodeHints::fromModelNode(modelNode()).canBeReparentedTo(potentialParent);
}

bool QmlItemNode::isInStackedContainer() const
{
    if (hasInstanceParent())
        return NodeHints::fromModelNode(instanceParent()).isStackedContainer();
    return false;
}

void QmlItemNode::setSize(const QSizeF &size)
{
    if (!hasBindingProperty("width") && !(anchors().instanceHasAnchor(AnchorLineRight)
                                          && anchors().instanceHasAnchor(AnchorLineLeft)))
        setVariantProperty("width", qRound(size.width()));

    if (!hasBindingProperty("height") && !(anchors().instanceHasAnchor(AnchorLineBottom)
                                           && anchors().instanceHasAnchor(AnchorLineTop)))
        setVariantProperty("height", qRound(size.height()));
}

} //QmlDesigner
