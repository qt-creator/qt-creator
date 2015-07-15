/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlanchorbindingproxy.h"
#include "abstractview.h"
#include <qmlanchors.h>
#include <nodeabstractproperty.h>
#include <variantproperty.h>

#include <QtQml>
#include <QDebug>

namespace QmlDesigner {

class ModelNode;
class NodeState;

const PropertyName auxDataString("anchors_");

static inline void backupPropertyAndRemove(ModelNode node, const PropertyName &propertyName)
{
    if (node.hasVariantProperty(propertyName)) {
        node.setAuxiliaryData(auxDataString + propertyName, node.variantProperty(propertyName).value());
        node.removeProperty(propertyName);

    }
    if (node.hasBindingProperty(propertyName)) {
        node.setAuxiliaryData(auxDataString + propertyName, QmlItemNode(node).instanceValue(propertyName));
        node.removeProperty(propertyName);
    }
}


static inline void restoreProperty(const ModelNode &node, const PropertyName &propertyName)
{
    if (node.hasAuxiliaryData(auxDataString + propertyName))
        node.variantProperty(propertyName).setValue(node.auxiliaryData(auxDataString + propertyName));
}

namespace Internal {

QmlAnchorBindingProxy::QmlAnchorBindingProxy(QObject *parent) :
    QObject(parent),
    m_relativeTopTarget(SameEdge), m_relativeBottomTarget(SameEdge),
    m_relativeLeftTarget(SameEdge), m_relativeRightTarget(SameEdge),
    m_relativeVerticalTarget(Center), m_relativeHorizontalTarget(Center),
    m_locked(false), m_ignoreQml(false)
{
}

QmlAnchorBindingProxy::~QmlAnchorBindingProxy()
{
}

void QmlAnchorBindingProxy::setup(const QmlItemNode &fxItemNode)
{
    m_qmlItemNode = fxItemNode;

    m_ignoreQml = true;

    setupAnchorTargets();

    emit itemNodeChanged();
    emit parentChanged();

    emit emitAnchorSignals();

    if (m_qmlItemNode.hasNodeParent()) {
        emit topTargetChanged();
        emit bottomTargetChanged();
        emit leftTargetChanged();
        emit rightTargetChanged();
        emit verticalTargetChanged();
        emit horizontalTargetChanged();
    }

    emit invalidated();

    m_ignoreQml = false;
}

void QmlAnchorBindingProxy::invalidate(const QmlItemNode &fxItemNode)
{
    if (m_locked)
        return;

    m_qmlItemNode = fxItemNode;

    m_ignoreQml = true;

    m_verticalTarget =
            m_horizontalTarget =
            m_topTarget =
            m_bottomTarget =
            m_leftTarget =
            m_rightTarget =
            m_qmlItemNode.modelNode().parentProperty().parentModelNode();

    setupAnchorTargets();

    emitAnchorSignals();

    if (m_qmlItemNode.hasNodeParent()) {
        emit itemNodeChanged();
        emit topTargetChanged();
        emit bottomTargetChanged();
        emit leftTargetChanged();
        emit rightTargetChanged();
        emit verticalTargetChanged();
        emit horizontalTargetChanged();
    }

    emit invalidated();

    m_ignoreQml = false;
}

void QmlAnchorBindingProxy::setupAnchorTargets()
{
    if (m_qmlItemNode.modelNode().hasParentProperty())
        setDefaultAnchorTarget(m_qmlItemNode.modelNode().parentProperty().parentModelNode());
    else
        setDefaultAnchorTarget(ModelNode());

    if (topAnchored()) {
        AnchorLine topAnchor = m_qmlItemNode.anchors().instanceAnchor(AnchorLineTop);
        ModelNode targetNode = topAnchor.qmlItemNode();
        if (targetNode.isValid()) {
            m_topTarget = targetNode;
            if (topAnchor.type() == AnchorLineTop) {
                m_relativeTopTarget = SameEdge;
            } else if (topAnchor.type() == AnchorLineBottom) {
                m_relativeTopTarget = OppositeEdge;
            } else if (topAnchor.type() == AnchorLineVerticalCenter) {
                m_relativeTopTarget = Center;
            } else {
                qWarning() << __FUNCTION__ << "invalid anchor line";
            }
        } else {
            m_relativeTopTarget = SameEdge;
        }
    }

    if (bottomAnchored()) {
        AnchorLine bottomAnchor = m_qmlItemNode.anchors().instanceAnchor(AnchorLineBottom);
        ModelNode targetNode = bottomAnchor.qmlItemNode();
        if (targetNode.isValid()) {
            m_bottomTarget = targetNode;
            if (bottomAnchor.type() == AnchorLineBottom) {
                m_relativeBottomTarget = SameEdge;
            } else if (bottomAnchor.type() == AnchorLineTop) {
                m_relativeBottomTarget = OppositeEdge;
            } else if (bottomAnchor.type() == AnchorLineVerticalCenter) {
                m_relativeBottomTarget = Center;
            } else {
                qWarning() << __FUNCTION__ << "invalid anchor line";
            }
        } else {
            m_relativeBottomTarget = SameEdge;
        }
    }

    if (leftAnchored()) {
        AnchorLine leftAnchor = m_qmlItemNode.anchors().instanceAnchor(AnchorLineLeft);
        ModelNode targetNode = leftAnchor.qmlItemNode();
        if (targetNode.isValid()) {
            m_leftTarget = targetNode;
            if (leftAnchor.type() == AnchorLineLeft) {
                m_relativeLeftTarget = SameEdge;
            } else if (leftAnchor.type() == AnchorLineRight) {
                m_relativeLeftTarget = OppositeEdge;
            } else if (leftAnchor.type() == AnchorLineHorizontalCenter) {
                m_relativeLeftTarget = Center;
            } else {
                qWarning() << __FUNCTION__ << "invalid anchor line";
            }
        } else {
            m_relativeLeftTarget = SameEdge;
        }
    }

    if (rightAnchored()) {
        AnchorLine rightAnchor = m_qmlItemNode.anchors().instanceAnchor(AnchorLineRight);
        ModelNode targetNode = rightAnchor.qmlItemNode();
        if (targetNode.isValid()) {
            m_rightTarget = targetNode;
            if (rightAnchor.type() == AnchorLineRight) {
                m_relativeRightTarget = SameEdge;
            } else if (rightAnchor.type() == AnchorLineLeft) {
                m_relativeRightTarget = OppositeEdge;
            } else if (rightAnchor.type() == AnchorLineHorizontalCenter) {
                m_relativeRightTarget = Center;
            } else {
                qWarning() << __FUNCTION__ << "invalid anchor line";
            }
        } else {
            m_relativeRightTarget = SameEdge;
        }
    }

    if (verticalCentered()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineVerticalCenter).qmlItemNode();
        if (targetNode.isValid())
            m_verticalTarget = targetNode;
    }

    if (horizontalCentered()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineHorizontalCenter).qmlItemNode();
        if (targetNode.isValid())
            m_horizontalTarget = targetNode;
    }
}

void QmlAnchorBindingProxy::emitAnchorSignals()
{
    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit centeredHChanged();
    emit centeredVChanged();
    emit anchorsChanged();

    emit relativeAnchorTargetTopChanged();
    emit relativeAnchorTargetBottomChanged();
    emit relativeAnchorTargetLeftChanged();
    emit relativeAnchorTargetRightChanged();
}

void QmlAnchorBindingProxy::setDefaultRelativeTopTarget()
{
    if (m_topTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        m_relativeTopTarget = SameEdge;
    } else {
        m_relativeTopTarget = OppositeEdge;
    }
}

void QmlAnchorBindingProxy::setDefaultRelativeBottomTarget()
{
    if (m_bottomTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        m_relativeBottomTarget = SameEdge;
    } else {
        m_relativeBottomTarget = OppositeEdge;
    }
}

void QmlAnchorBindingProxy::setDefaultRelativeLeftTarget()
{
    if (m_leftTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        m_relativeLeftTarget = SameEdge;
    } else {
        m_relativeLeftTarget = OppositeEdge;
    }
}

void QmlAnchorBindingProxy::setDefaultRelativeRightTarget()
{
    if (m_rightTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        m_relativeRightTarget = SameEdge;
    } else {
        m_relativeRightTarget = OppositeEdge;
    }
}

RewriterTransaction QmlAnchorBindingProxy::beginRewriterTransaction(const QByteArray &identifier)
{
     return m_qmlItemNode.modelNode().view()->beginRewriterTransaction(identifier);
}

bool QmlAnchorBindingProxy::hasParent() const
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.hasNodeParent();
}

bool QmlAnchorBindingProxy::isInLayout() const
{
    return false;
}

bool QmlAnchorBindingProxy::isFilled() const
{
    return m_qmlItemNode.isValid()
            && hasAnchors()
            && topAnchored()
            && bottomAnchored()
            && leftAnchored()
            && rightAnchored()
            && (m_qmlItemNode.instanceValue("anchors.topMargin").toInt() == 0)
            && (m_qmlItemNode.instanceValue("anchors.bottomMargin").toInt() == 0)
            && (m_qmlItemNode.instanceValue("anchors.leftMargin").toInt() == 0)
            && (m_qmlItemNode.instanceValue("anchors.rightMargin").toInt() == 0);
}

bool QmlAnchorBindingProxy::topAnchored() const
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineTop);
}

bool QmlAnchorBindingProxy::bottomAnchored() const
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineBottom);
}

bool QmlAnchorBindingProxy::leftAnchored() const
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineLeft);
}

bool QmlAnchorBindingProxy::rightAnchored() const
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineRight);
}

bool QmlAnchorBindingProxy::hasAnchors() const
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchors();
}


void QmlAnchorBindingProxy::setTopTarget(const QString &target)
{

    if (m_ignoreQml)
        return;

    QmlItemNode newTarget(targetIdToNode(target));

    if (newTarget == m_topTarget)
        return;

    if (!newTarget.isValid())
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setTopTarget"));

    m_topTarget = newTarget;

    setDefaultRelativeTopTarget();

    anchorTop();

    emit topTargetChanged();
}


void QmlAnchorBindingProxy::setBottomTarget(const QString &target)
{
    if (m_ignoreQml)
        return;

    QmlItemNode newTarget(targetIdToNode(target));

    if (newTarget == m_bottomTarget)
        return;

    if (!newTarget.isValid())
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setBottomTarget"));

    m_bottomTarget = newTarget;
    setDefaultRelativeBottomTarget();
    anchorBottom();

    emit bottomTargetChanged();
}

void QmlAnchorBindingProxy::setLeftTarget(const QString &target)
{
    if (m_ignoreQml)
        return;

    QmlItemNode newTarget(targetIdToNode(target));

    if (newTarget == m_leftTarget)
        return;

    if (!newTarget.isValid())
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setLeftTarget"));

    m_leftTarget = newTarget;
    setDefaultRelativeLeftTarget();
    anchorLeft();

    emit leftTargetChanged();
}

void QmlAnchorBindingProxy::setRightTarget(const QString &target)
{
    if (m_ignoreQml)
        return;

    QmlItemNode newTarget(targetIdToNode(target));

    if (newTarget == m_rightTarget)
        return;

    if (!newTarget.isValid())
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRightTarget"));

    m_rightTarget = newTarget;
    setDefaultRelativeRightTarget();
    anchorRight();

    emit rightTargetChanged();
}

void QmlAnchorBindingProxy::setVerticalTarget(const QString &target)
{
    if (m_ignoreQml)
        return;

    QmlItemNode newTarget(targetIdToNode(target));

    if (newTarget == m_verticalTarget)
        return;

    if (!newTarget.isValid())
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setVerticalTarget"));

    m_verticalTarget = newTarget;
    anchorVertical();

    emit verticalTargetChanged();
}

void QmlAnchorBindingProxy::setHorizontalTarget(const QString &target)
{
    if (m_ignoreQml)
        return;

    QmlItemNode newTarget(targetIdToNode(target));

    if (newTarget == m_horizontalTarget)
        return;

    if (!newTarget.isValid())
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setHorizontalTarget"));

    m_horizontalTarget = newTarget;
    anchorHorizontal();

    emit horizontalTargetChanged();
}

void QmlAnchorBindingProxy::setRelativeAnchorTargetTop(QmlAnchorBindingProxy::RelativeAnchorTarget target)
{
    if (m_ignoreQml)
        return;

    if (target == m_relativeTopTarget)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRelativeAnchorTargetTop"));

    m_relativeTopTarget = target;

    anchorTop();

    emit relativeAnchorTargetTopChanged();
}

void QmlAnchorBindingProxy::setRelativeAnchorTargetBottom(QmlAnchorBindingProxy::RelativeAnchorTarget target)
{
    if (m_ignoreQml)
        return;

    if (target == m_relativeBottomTarget)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRelativeAnchorTargetBottom"));

    m_relativeBottomTarget = target;

    anchorBottom();

    emit relativeAnchorTargetBottomChanged();
}

void QmlAnchorBindingProxy::setRelativeAnchorTargetLeft(QmlAnchorBindingProxy::RelativeAnchorTarget target)
{
    if (m_ignoreQml)
        return;

    if (target == m_relativeLeftTarget)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRelativeAnchorTargetLeft"));

    m_relativeLeftTarget = target;

    anchorLeft();

    emit relativeAnchorTargetLeftChanged();
}

void QmlAnchorBindingProxy::setRelativeAnchorTargetRight(QmlAnchorBindingProxy::RelativeAnchorTarget target)
{
    if (m_ignoreQml)
        return;

    if (target == m_relativeRightTarget)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRelativeAnchorTargetRight"));

    m_relativeRightTarget = target;

    anchorRight();

    emit relativeAnchorTargetRightChanged();

}

void QmlAnchorBindingProxy::setRelativeAnchorTargetVertical(QmlAnchorBindingProxy::RelativeAnchorTarget target)
{
    if (m_ignoreQml)
        return;

    if (target == m_relativeVerticalTarget)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRelativeAnchorTargetVertical"));

    m_relativeVerticalTarget = target;

    anchorVertical();

    emit relativeAnchorTargetVerticalChanged();
}

void QmlAnchorBindingProxy::setRelativeAnchorTargetHorizontal(QmlAnchorBindingProxy::RelativeAnchorTarget target)
{
    if (m_ignoreQml)
        return;

    if (target == m_relativeHorizontalTarget)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRelativeAnchorTargetHorizontal"));

    m_relativeHorizontalTarget = target;

    anchorHorizontal();

    emit relativeAnchorTargetHorizontalChanged();
}

QStringList QmlAnchorBindingProxy::possibleTargetItems() const
{
    QStringList stringList;
    if (!m_qmlItemNode.isValid())
        return stringList;

    QList<QmlItemNode> itemList;

    if (m_qmlItemNode.instanceParent().modelNode().isValid())
        itemList = toQmlItemNodeList(m_qmlItemNode.instanceParent().modelNode().directSubModelNodes());
    itemList.removeOne(m_qmlItemNode);
    //We currently have no instanceChildren().
    //So we double check here if the instanceParents are equal.
    foreach (const QmlItemNode &node, itemList)
        if (node.isValid() && (node.instanceParent().modelNode() != m_qmlItemNode.instanceParent().modelNode()))
            itemList.removeAll(node);

    foreach (const QmlItemNode &itemNode, itemList) {
        if (itemNode.isValid() && !itemNode.id().isEmpty())
            stringList.append(itemNode.id());
    }

    QmlItemNode parent(m_qmlItemNode.instanceParent().toQmlItemNode());

    if (parent.isValid())
        stringList.append(QStringLiteral("parent"));

    return stringList;
}

void QmlAnchorBindingProxy::registerDeclarativeType()
{
    qmlRegisterType<QmlAnchorBindingProxy>("HelperWidgets",2,0,"AnchorBindingProxy");
}

int QmlAnchorBindingProxy::indexOfPossibleTargetItem(const QString &targetName) const
{
    return possibleTargetItems().indexOf(targetName);
}

void QmlAnchorBindingProxy::resetLayout() {
    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::resetLayout"));

        m_qmlItemNode.anchors().removeAnchors();
        m_qmlItemNode.anchors().removeMargins();

        restoreProperty(modelNode(), "x");
        restoreProperty(modelNode(), "y");
        restoreProperty(modelNode(), "width");
        restoreProperty(modelNode(), "height");

        emit topAnchorChanged();
        emit bottomAnchorChanged();
        emit leftAnchorChanged();
        emit rightAnchorChanged();
        emit anchorsChanged();
    }

void QmlAnchorBindingProxy::setBottomAnchor(bool anchor)
{
    if (!m_qmlItemNode.hasNodeParent())
        return;

    if (bottomAnchored() == anchor)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setBottomAnchor"));

    if (!anchor) {
        removeBottomAnchor();
    } else {
        setDefaultRelativeBottomTarget();
        anchorBottom();
        if (topAnchored())
            backupPropertyAndRemove(modelNode(), "height");
    }

    emit relativeAnchorTargetBottomChanged();
    emit bottomAnchorChanged();

    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::setLeftAnchor(bool anchor)
{
    if (!m_qmlItemNode.hasNodeParent())
        return;

    if (leftAnchored() == anchor)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setLeftAnchor"));

    if (!anchor) {
        removeLeftAnchor();
    } else {
        setDefaultRelativeLeftTarget();

        anchorLeft();
        backupPropertyAndRemove(modelNode(), "x");
        if (rightAnchored())
            backupPropertyAndRemove(modelNode(), "width");
    }

    emit relativeAnchorTargetLeftChanged();
    emit leftAnchorChanged();
    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::setRightAnchor(bool anchor)
{
    if (!m_qmlItemNode.hasNodeParent())
        return;

    if (rightAnchored() == anchor)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setRightAnchor"));

    if (!anchor) {
        removeRightAnchor();
    } else {
        setDefaultRelativeRightTarget();

        anchorRight();
        if (leftAnchored())
            backupPropertyAndRemove(modelNode(), "width");
    }

    emit relativeAnchorTargetRightChanged();
    emit rightAnchorChanged();

    if (hasAnchors() != anchor)
        emit anchorsChanged();
}
 QRectF QmlAnchorBindingProxy::parentBoundingBox()
{
    if (m_qmlItemNode.hasInstanceParent()) {
        if (m_qmlItemNode.instanceParentItem().instanceContentItemBoundingRect().isValid())
            return m_qmlItemNode.instanceParentItem().instanceContentItemBoundingRect();
        return m_qmlItemNode.instanceParentItem().instanceBoundingRect();
    }

    return QRect();
}

QRectF QmlAnchorBindingProxy::boundingBox(const QmlItemNode &node)
{
    if (node.isValid())
        return node.instanceTransformWithContentTransform().mapRect(node.instanceBoundingRect());

    return QRect();
}

QRectF QmlAnchorBindingProxy::transformedBoundingBox()
{
    return m_qmlItemNode.instanceTransformWithContentTransform().mapRect(m_qmlItemNode.instanceBoundingRect());
}

void QmlAnchorBindingProxy::anchorTop()
{
    m_locked = true;

    if (m_relativeTopTarget == SameEdge) {
        qreal topMargin = transformedBoundingBox().top() - parentBoundingBox().top();
        m_qmlItemNode.anchors().setMargin( AnchorLineTop, topMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineTop, m_topTarget, AnchorLineTop);
    } else if (m_relativeTopTarget == OppositeEdge) {
        qreal topMargin = boundingBox(m_qmlItemNode).top() - boundingBox(m_topTarget).bottom();
        m_qmlItemNode.anchors().setMargin( AnchorLineTop, topMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineTop, m_topTarget, AnchorLineBottom);
    } else if (m_relativeTopTarget == Center) {
        qreal topMargin = boundingBox(m_qmlItemNode).top() - boundingBox(m_topTarget).center().y();
        m_qmlItemNode.anchors().setMargin(AnchorLineTop, topMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineTop, m_topTarget, AnchorLineVerticalCenter);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::anchorBottom()
{
    m_locked = true;

    if (m_relativeBottomTarget == SameEdge) {
        qreal bottomMargin =  parentBoundingBox().bottom() - transformedBoundingBox().bottom();
        m_qmlItemNode.anchors().setMargin( AnchorLineBottom, bottomMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineBottom, m_bottomTarget, AnchorLineBottom);
    } else  if (m_relativeBottomTarget == OppositeEdge) {
        qreal bottomMargin = boundingBox(m_bottomTarget).top()- boundingBox(m_qmlItemNode).bottom();
        m_qmlItemNode.anchors().setMargin( AnchorLineBottom, bottomMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineBottom, m_bottomTarget, AnchorLineTop);
    } else if (m_relativeBottomTarget == Center) {
        qreal bottomMargin = boundingBox(m_qmlItemNode).top() - boundingBox(m_bottomTarget).center().y();
        m_qmlItemNode.anchors().setMargin(AnchorLineBottom, bottomMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineBottom, m_bottomTarget, AnchorLineVerticalCenter);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::anchorLeft()
{
    m_locked = true;

    if (m_relativeLeftTarget == SameEdge) {
        qreal leftMargin = transformedBoundingBox().left() - parentBoundingBox().left();
        m_qmlItemNode.anchors().setMargin(AnchorLineLeft, leftMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineLeft, m_leftTarget, AnchorLineLeft);
    } else if (m_relativeLeftTarget == OppositeEdge) {
        qreal leftMargin = boundingBox(m_qmlItemNode).left() - boundingBox(m_leftTarget).right();
        m_qmlItemNode.anchors().setMargin( AnchorLineLeft, leftMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineLeft, m_leftTarget, AnchorLineRight);
    } else if (m_relativeLeftTarget == Center) {
        qreal leftMargin = boundingBox(m_qmlItemNode).top() - boundingBox(m_leftTarget).center().x();
        m_qmlItemNode.anchors().setMargin(AnchorLineLeft, leftMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineLeft, m_leftTarget, AnchorLineHorizontalCenter);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::anchorRight()
{
    m_locked = true;

    if (m_relativeRightTarget == SameEdge) {
        qreal rightMargin = parentBoundingBox().right() - transformedBoundingBox().right();
        m_qmlItemNode.anchors().setMargin( AnchorLineRight, rightMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineRight, m_rightTarget, AnchorLineRight);
    } else if (m_relativeRightTarget == OppositeEdge) {
        qreal rightMargin = boundingBox(m_rightTarget).left() - boundingBox(m_qmlItemNode).right();
        m_qmlItemNode.anchors().setMargin( AnchorLineRight, rightMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineRight, m_rightTarget, AnchorLineLeft);
    } else if (m_relativeRightTarget == Center) {
        qreal rightMargin = boundingBox(m_qmlItemNode).top() - boundingBox(m_rightTarget).center().x();
        m_qmlItemNode.anchors().setMargin(AnchorLineRight, rightMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineRight, m_rightTarget, AnchorLineHorizontalCenter);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::anchorVertical()
{
    m_locked = true;
    if (m_relativeVerticalTarget == SameEdge) {
        m_qmlItemNode.anchors().setAnchor(AnchorLineVerticalCenter, m_verticalTarget, AnchorLineRight);
    } else if (m_relativeVerticalTarget == OppositeEdge) {
        m_qmlItemNode.anchors().setAnchor(AnchorLineVerticalCenter, m_verticalTarget, AnchorLineLeft);
    } else if (m_relativeVerticalTarget == Center) {
        m_qmlItemNode.anchors().setAnchor(AnchorLineVerticalCenter, m_verticalTarget, AnchorLineVerticalCenter);

    }
    m_locked = false;
}

void QmlAnchorBindingProxy::anchorHorizontal()
{
    m_locked = true;
    if (m_relativeHorizontalTarget == SameEdge) {
        m_qmlItemNode.anchors().setAnchor(AnchorLineHorizontalCenter, m_horizontalTarget, AnchorLineRight);
    } else if (m_relativeVerticalTarget == OppositeEdge) {
        m_qmlItemNode.anchors().setAnchor(AnchorLineHorizontalCenter, m_horizontalTarget, AnchorLineLeft);
    } else if (m_relativeVerticalTarget == Center) {
        m_qmlItemNode.anchors().setAnchor(AnchorLineHorizontalCenter, m_horizontalTarget, AnchorLineHorizontalCenter);
    }
    m_locked = false;
}

QmlItemNode QmlAnchorBindingProxy::targetIdToNode(const QString &id) const
{
    QmlItemNode itemNode;

    if (m_qmlItemNode.isValid() && m_qmlItemNode.view()) {

        itemNode = m_qmlItemNode.view()->modelNodeForId(id);

        if (id == QStringLiteral("parent"))
            itemNode = m_qmlItemNode.instanceParent().modelNode();
    }

    return itemNode;
}

QString QmlAnchorBindingProxy::idForNode(const QmlItemNode &qmlItemNode) const
{
    if (m_qmlItemNode.instanceParent().modelNode() == qmlItemNode)
        return QStringLiteral("parent");

    return qmlItemNode.id();
}

ModelNode QmlAnchorBindingProxy::modelNode() const
{
    return m_qmlItemNode.modelNode();
}

void QmlAnchorBindingProxy::setTopAnchor(bool anchor)
{
    if (!m_qmlItemNode.hasNodeParent())
        return ;

    if (topAnchored() == anchor)
        return;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setTopAnchor"));

    if (!anchor) {
        removeTopAnchor();
    } else {
        setDefaultRelativeTopTarget();

        anchorTop();
        backupPropertyAndRemove(modelNode(), "y");
        if (bottomAnchored())
            backupPropertyAndRemove(modelNode(), "height");
    }

    emit relativeAnchorTargetTopChanged();
    emit topAnchorChanged();
    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::removeTopAnchor() {
    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::removeTopAnchor"));

    m_qmlItemNode.anchors().removeAnchor(AnchorLineTop);
    m_qmlItemNode.anchors().removeMargin(AnchorLineTop);

    restoreProperty(modelNode(), "y");
    restoreProperty(modelNode(), "height");

}

void QmlAnchorBindingProxy::removeBottomAnchor() {
    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::removeBottomAnchor"));

    m_qmlItemNode.anchors().removeAnchor(AnchorLineBottom);
    m_qmlItemNode.anchors().removeMargin(AnchorLineBottom);


    restoreProperty(modelNode(), "height");
}

void QmlAnchorBindingProxy::removeLeftAnchor() {
    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::removeLeftAnchor"));

    m_qmlItemNode.anchors().removeAnchor(AnchorLineLeft);
    m_qmlItemNode.anchors().removeMargin(AnchorLineLeft);

    restoreProperty(modelNode(), "x");
    restoreProperty(modelNode(), "width");
}

void QmlAnchorBindingProxy::removeRightAnchor() {
    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::removeRightAnchor"));

    m_qmlItemNode.anchors().removeAnchor(AnchorLineRight);
    m_qmlItemNode.anchors().removeMargin(AnchorLineRight);

    restoreProperty(modelNode(), "width");
}

void QmlAnchorBindingProxy::setVerticalCentered(bool centered)
{
    if (!m_qmlItemNode.hasNodeParent())
        return ;

    if (verticalCentered() == centered)
        return;

    m_locked = true;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setVerticalCentered"));

    if (!centered) {
        m_qmlItemNode.anchors().removeAnchor(AnchorLineVerticalCenter);
        m_qmlItemNode.anchors().removeMargin(AnchorLineVerticalCenter);
    } else {
        m_relativeVerticalTarget = Center;

        anchorVertical();
    }

    m_locked = false;

    emit relativeAnchorTargetVerticalChanged();
    emit centeredVChanged();
}

void QmlAnchorBindingProxy::setHorizontalCentered(bool centered)
{
    if (!m_qmlItemNode.hasNodeParent())
        return ;

    if (horizontalCentered() == centered)
        return;

    m_locked = true;

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::setHorizontalCentered"));

    if (!centered) {
        m_qmlItemNode.anchors().removeAnchor(AnchorLineHorizontalCenter);
        m_qmlItemNode.anchors().removeMargin(AnchorLineHorizontalCenter);
    } else {
        m_relativeHorizontalTarget = Center;

        anchorHorizontal();
    }

    m_locked = false;

    emit relativeAnchorTargetHorizontalChanged();
    emit centeredHChanged();
}

bool QmlAnchorBindingProxy::verticalCentered()
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineVerticalCenter);
}

QString QmlAnchorBindingProxy::topTarget() const
{
    return idForNode(m_topTarget);
}

QString QmlAnchorBindingProxy::bottomTarget() const
{
    return idForNode(m_bottomTarget);
}

QString QmlAnchorBindingProxy::leftTarget() const
{
    return idForNode(m_leftTarget);
}

QString QmlAnchorBindingProxy::rightTarget() const
{
    return idForNode(m_rightTarget);
}

QmlAnchorBindingProxy::RelativeAnchorTarget QmlAnchorBindingProxy::relativeAnchorTargetTop() const
{
    return m_relativeTopTarget;
}

QmlAnchorBindingProxy::RelativeAnchorTarget QmlAnchorBindingProxy::relativeAnchorTargetBottom() const
{
    return m_relativeBottomTarget;
}

QmlAnchorBindingProxy::RelativeAnchorTarget QmlAnchorBindingProxy::relativeAnchorTargetLeft() const
{
    return m_relativeLeftTarget;
}

QmlAnchorBindingProxy::RelativeAnchorTarget QmlAnchorBindingProxy::relativeAnchorTargetRight() const
{
    return m_relativeRightTarget;
}

QmlAnchorBindingProxy::RelativeAnchorTarget QmlAnchorBindingProxy::relativeAnchorTargetVertical() const
{
    return m_relativeVerticalTarget;
}

QmlAnchorBindingProxy::RelativeAnchorTarget QmlAnchorBindingProxy::relativeAnchorTargetHorizontal() const
{
    return m_relativeHorizontalTarget;
}

QString QmlAnchorBindingProxy::verticalTarget() const
{
    return idForNode(m_verticalTarget);
}

QString QmlAnchorBindingProxy::horizontalTarget() const
{
    return idForNode(m_horizontalTarget);
}

bool QmlAnchorBindingProxy::horizontalCentered()
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineHorizontalCenter);
}

void QmlAnchorBindingProxy::fill()
{

    RewriterTransaction transaction = beginRewriterTransaction(
                QByteArrayLiteral("QmlAnchorBindingProxy::fill"));


    backupPropertyAndRemove(modelNode(), "x");
    backupPropertyAndRemove(modelNode(), "y");
    backupPropertyAndRemove(modelNode(), "width");
    backupPropertyAndRemove(modelNode(), "height");

    m_qmlItemNode.anchors().fill();

    setHorizontalCentered(false);
    setVerticalCentered(false);

    m_qmlItemNode.anchors().removeMargin(AnchorLineRight);
    m_qmlItemNode.anchors().removeMargin(AnchorLineLeft);
    m_qmlItemNode.anchors().removeMargin(AnchorLineTop);
    m_qmlItemNode.anchors().removeMargin(AnchorLineBottom);

    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit anchorsChanged();
}

void QmlAnchorBindingProxy::setDefaultAnchorTarget(const ModelNode &modelNode)
{
    m_verticalTarget = modelNode;
    m_horizontalTarget = modelNode;
    m_topTarget = modelNode;
    m_bottomTarget = modelNode;
    m_leftTarget = modelNode;
    m_rightTarget = modelNode;
}

} // namespace Internal
} // namespace QmlDesigner

