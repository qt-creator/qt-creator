/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlanchorbindingproxy.h"
#include "abstractview.h"
#include <qmlanchors.h>
#include <nodeabstractproperty.h>
#include <variantproperty.h>
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


static inline void restoreProperty(ModelNode node, const PropertyName &propertyName)
{
    if (node.hasAuxiliaryData(auxDataString + propertyName))
        node.variantProperty(propertyName).setValue(node.auxiliaryData(auxDataString + propertyName));
}

namespace Internal {

QmlAnchorBindingProxy::QmlAnchorBindingProxy(QObject *parent) :
    QObject(parent), m_locked(false), m_ignoreQml(false)
{
}

QmlAnchorBindingProxy::~QmlAnchorBindingProxy()
{
}

void QmlAnchorBindingProxy::setup(const QmlItemNode &fxItemNode)
{
    m_qmlItemNode = fxItemNode;

    m_ignoreQml = true;

    if (m_qmlItemNode.modelNode().hasParentProperty())
        setDefaultAnchorTarget(m_qmlItemNode.modelNode().parentProperty().parentModelNode());
    else
        setDefaultAnchorTarget(ModelNode());

    if (topAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineTop).qmlItemNode();
        if (targetNode.isValid())
            m_topTarget = targetNode;
    }

    if (bottomAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineBottom).qmlItemNode();
        if (targetNode.isValid())
            m_bottomTarget = targetNode;
    }

    if (leftAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineLeft).qmlItemNode();
        if (targetNode.isValid())
            m_leftTarget = targetNode;
    }

    if (rightAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineRight).qmlItemNode();
        if (targetNode.isValid())
            m_rightTarget = targetNode;
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

    emit itemNodeChanged();
    emit parentChanged();
    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit centeredHChanged();
    emit centeredVChanged();
    emit anchorsChanged();

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

void QmlAnchorBindingProxy::invalidate(const QmlItemNode &fxItemNode)
{
    if (m_locked)
        return;

    m_qmlItemNode = fxItemNode;

    m_ignoreQml = true;

    m_verticalTarget = m_horizontalTarget = m_topTarget = m_bottomTarget = m_leftTarget = m_rightTarget = m_qmlItemNode.modelNode().parentProperty().parentModelNode();

    if (topAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineTop).qmlItemNode();
        if (targetNode.isValid())
            m_topTarget = targetNode;
    }

    if (bottomAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineBottom).qmlItemNode();
        if (targetNode.isValid())
            m_bottomTarget = targetNode;
    }

    if (leftAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineLeft).qmlItemNode();
        if (targetNode.isValid())
            m_leftTarget = targetNode;
    }

    if (rightAnchored()) {
        ModelNode targetNode = m_qmlItemNode.anchors().instanceAnchor(AnchorLineRight).qmlItemNode();
        if (targetNode.isValid())
            m_rightTarget = targetNode;
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

    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit centeredHChanged();
    emit centeredVChanged();
    emit anchorsChanged();

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

bool QmlAnchorBindingProxy::hasParent()
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.hasNodeParent();
}

bool QmlAnchorBindingProxy::isFilled()
{
    return m_qmlItemNode.isValid() && hasAnchors() && topAnchored() && bottomAnchored() && leftAnchored() && rightAnchored()
            && (m_qmlItemNode.instanceValue("anchors.topMargin").toInt() == 0)
            && (m_qmlItemNode.instanceValue("anchors.bottomMargin").toInt() == 0)
            && (m_qmlItemNode.instanceValue("anchors.leftMargin").toInt() == 0)
            && (m_qmlItemNode.instanceValue("anchors.rightMargin").toInt() == 0);
}

bool QmlAnchorBindingProxy::topAnchored()
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineTop);
}

bool QmlAnchorBindingProxy::bottomAnchored()
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineBottom);
}

bool QmlAnchorBindingProxy::leftAnchored()
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineLeft);
}

bool QmlAnchorBindingProxy::rightAnchored()
{
    return m_qmlItemNode.isValid() && m_qmlItemNode.anchors().instanceHasAnchor(AnchorLineRight);
}

bool QmlAnchorBindingProxy::hasAnchors()
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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setTopTarget"));

    m_topTarget = newTarget;
    calcTopMargin();

    emit topTargetChanged();
}


void QmlAnchorBindingProxy::setBottomTarget(const QString &target)
{
    if (m_ignoreQml)
        return;

    if (m_ignoreQml)
        return;

    QmlItemNode newTarget(targetIdToNode(target));

    if (newTarget == m_bottomTarget)
        return;

    if (!newTarget.isValid())
        return;

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setBottomTarget"));

    m_bottomTarget = newTarget;
    calcBottomMargin();

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setLeftTarget"));

    m_leftTarget = newTarget;
    calcLeftMargin();

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setRightTarget"));

    m_rightTarget = newTarget;
    calcRightMargin();

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setVerticalTarget"));

    m_verticalTarget = newTarget;
    m_qmlItemNode.anchors().setAnchor(AnchorLineVerticalCenter, m_verticalTarget, AnchorLineVerticalCenter);

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setHorizontalTarget"));

    m_horizontalTarget = newTarget;
    m_qmlItemNode.anchors().setAnchor(AnchorLineHorizontalCenter, m_horizontalTarget, AnchorLineHorizontalCenter);

    emit horizontalTargetChanged();
}

QStringList QmlAnchorBindingProxy::possibleTargetItems() const
{
    QStringList stringList;
    if (!m_qmlItemNode.isValid())
        return stringList;

    QList<QmlItemNode> itemList;

    if (m_qmlItemNode.instanceParent().modelNode().isValid())
        itemList = toQmlItemNodeList(m_qmlItemNode.instanceParent().modelNode().allDirectSubModelNodes());
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
        stringList.append(QLatin1String("parent"));

    return stringList;
}

int QmlAnchorBindingProxy::indexOfPossibleTargetItem(const QString &targetName) const
{
    return possibleTargetItems().indexOf(targetName);
}

void QmlAnchorBindingProxy::resetLayout() {
    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::resetLayout"));

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setBottomAnchor"));

    if (!anchor) {
        removeBottomAnchor();
    } else {
        calcBottomMargin();
        if (topAnchored())
            backupPropertyAndRemove(modelNode(), "height");
    }
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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setLeftAnchor"));

    if (!anchor) {
        removeLeftAnchor();
    } else {
        calcLeftMargin();
        backupPropertyAndRemove(modelNode(), "x");
        if (rightAnchored())
            backupPropertyAndRemove(modelNode(), "width");
    }

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setRightAnchor"));

    if (!anchor) {
        removeRightAnchor();
    } else {
        calcRightMargin();
        if (leftAnchored())
            backupPropertyAndRemove(modelNode(), "width");
    }
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

QRectF QmlAnchorBindingProxy::boundingBox(QmlItemNode node)
{
    if (node.isValid())
        return node.instanceTransformWithContentTransform().mapRect(node.instanceBoundingRect());

    return QRect();
}

QRectF QmlAnchorBindingProxy::transformedBoundingBox()
{
    return m_qmlItemNode.instanceTransformWithContentTransform().mapRect(m_qmlItemNode.instanceBoundingRect());
}

void QmlAnchorBindingProxy::calcTopMargin()
{
    m_locked = true;

    if (m_topTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal topMargin = transformedBoundingBox().top() - parentBoundingBox().top();
        m_qmlItemNode.anchors().setMargin( AnchorLineTop, topMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineTop, m_topTarget, AnchorLineTop);
    } else {
        qreal topMargin = boundingBox(m_qmlItemNode).top() - boundingBox(m_topTarget).bottom();
        m_qmlItemNode.anchors().setMargin( AnchorLineTop, topMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineTop, m_topTarget, AnchorLineBottom);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::calcBottomMargin()
{
    m_locked = true;

    if (m_bottomTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal bottomMargin =  parentBoundingBox().bottom() - transformedBoundingBox().bottom();
        m_qmlItemNode.anchors().setMargin( AnchorLineBottom, bottomMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineBottom, m_bottomTarget, AnchorLineBottom);
    } else {
        qreal bottomMargin = boundingBox(m_bottomTarget).top()- boundingBox(m_qmlItemNode).bottom();
        m_qmlItemNode.anchors().setMargin( AnchorLineBottom, bottomMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineBottom, m_bottomTarget, AnchorLineTop);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::calcLeftMargin()
{
    m_locked = true;

    if (m_leftTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal leftMargin = transformedBoundingBox().left() - parentBoundingBox().left();
        m_qmlItemNode.anchors().setMargin(AnchorLineLeft, leftMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineLeft, m_leftTarget, AnchorLineLeft);
    } else {
        qreal leftMargin = boundingBox(m_qmlItemNode).left() - boundingBox(m_leftTarget).right();
        m_qmlItemNode.anchors().setMargin( AnchorLineLeft, leftMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineLeft, m_leftTarget, AnchorLineRight);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::calcRightMargin()
{
    m_locked = true;

    if (m_rightTarget.modelNode() == m_qmlItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal rightMargin = parentBoundingBox().right() - transformedBoundingBox().right();
        m_qmlItemNode.anchors().setMargin( AnchorLineRight, rightMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineRight, m_rightTarget, AnchorLineRight);
    } else {
        qreal rightMargin = boundingBox(m_rightTarget).left() - boundingBox(m_qmlItemNode).right();
        m_qmlItemNode.anchors().setMargin( AnchorLineRight, rightMargin);
        m_qmlItemNode.anchors().setAnchor(AnchorLineRight, m_rightTarget, AnchorLineLeft);
    }

    m_locked = false;
}

QmlItemNode QmlAnchorBindingProxy::targetIdToNode(const QString &id) const
{
    QmlItemNode itemNode;

    if (m_qmlItemNode.isValid() && m_qmlItemNode.view()) {

        itemNode = m_qmlItemNode.view()->modelNodeForId(id);

        if (id == QLatin1String("parent"))
            itemNode = m_qmlItemNode.instanceParent().modelNode();
    }

    return itemNode;
}

QString QmlAnchorBindingProxy::idForNode(const QmlItemNode &qmlItemNode) const
{
    if (m_qmlItemNode.instanceParent().modelNode() == qmlItemNode)
        return QLatin1String("parent");

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setTopAnchor"));

    if (!anchor) {
        removeTopAnchor();
    } else {
        calcTopMargin();
        backupPropertyAndRemove(modelNode(), "y");
        if (bottomAnchored())
            backupPropertyAndRemove(modelNode(), "height");
    }
    emit topAnchorChanged();
    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::removeTopAnchor() {
    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::removeTopAnchor"));

    m_qmlItemNode.anchors().removeAnchor(AnchorLineTop);
    m_qmlItemNode.anchors().removeMargin(AnchorLineTop);

    restoreProperty(modelNode(), "y");
    restoreProperty(modelNode(), "height");

}

void QmlAnchorBindingProxy::removeBottomAnchor() {
    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::removeBottomAnchor"));

    m_qmlItemNode.anchors().removeAnchor(AnchorLineBottom);
    m_qmlItemNode.anchors().removeMargin(AnchorLineBottom);


    restoreProperty(modelNode(), "height");
}

void QmlAnchorBindingProxy::removeLeftAnchor() {
    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::removeLeftAnchor"));

    m_qmlItemNode.anchors().removeAnchor(AnchorLineLeft);
    m_qmlItemNode.anchors().removeMargin(AnchorLineLeft);

    restoreProperty(modelNode(), "x");
    restoreProperty(modelNode(), "width");
}

void QmlAnchorBindingProxy::removeRightAnchor() {
    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::removeRightAnchor"));

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setVerticalCentered"));

    if (!centered) {
        m_qmlItemNode.anchors().removeAnchor(AnchorLineVerticalCenter);
        m_qmlItemNode.anchors().removeMargin(AnchorLineVerticalCenter);
    } else {
        m_qmlItemNode.anchors().setAnchor(AnchorLineVerticalCenter, m_qmlItemNode.modelNode().parentProperty().parentModelNode(), AnchorLineVerticalCenter);
    }

    m_locked = false;

    emit centeredVChanged();
}

void QmlAnchorBindingProxy::setHorizontalCentered(bool centered)
{
    if (!m_qmlItemNode.hasNodeParent())
        return ;

    if (horizontalCentered() == centered)
        return;

    m_locked = true;

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::setHorizontalCentered"));

    if (!centered) {
        m_qmlItemNode.anchors().removeAnchor(AnchorLineHorizontalCenter);
        m_qmlItemNode.anchors().removeMargin(AnchorLineHorizontalCenter);
    } else {
        m_qmlItemNode.anchors().setAnchor(AnchorLineHorizontalCenter, m_qmlItemNode.modelNode().parentProperty().parentModelNode(), AnchorLineHorizontalCenter);
    }

    m_locked = false;

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

    RewriterTransaction transaction = m_qmlItemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("QmlAnchorBindingProxy::fill"));


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

