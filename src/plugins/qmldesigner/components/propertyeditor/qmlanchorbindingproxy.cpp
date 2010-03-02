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

#include "qmlanchorbindingproxy.h"
#include <qmlanchors.h>
#include <nodeabstractproperty.h>
#include <nodeinstance.h>
#include <QDebug>

namespace QmlDesigner {

class ModelNode;
class NodeState;

namespace Internal {

QmlAnchorBindingProxy::QmlAnchorBindingProxy(QObject *parent) :
        QObject(parent)
{
}

QmlAnchorBindingProxy::~QmlAnchorBindingProxy()
{
}

void QmlAnchorBindingProxy::setup(const QmlItemNode &fxItemNode)
{
    m_fxItemNode = fxItemNode;

    m_verticalTarget = m_horizontalTarget = m_topTarget = m_bottomTarget = m_leftTarget = m_rightTarget = m_fxItemNode.modelNode().parentProperty().parentModelNode();

    if (topAnchored())
        m_topTarget = m_fxItemNode.anchors().instanceAnchor(AnchorLine::Top).qmlItemNode();

    if (bottomAnchored())
        m_bottomTarget = m_fxItemNode.anchors().instanceAnchor(AnchorLine::Bottom).qmlItemNode();

    if (leftAnchored())
        m_leftTarget = m_fxItemNode.anchors().instanceAnchor(AnchorLine::Left).qmlItemNode();

    if (rightAnchored())
        m_rightTarget = m_fxItemNode.anchors().instanceAnchor(AnchorLine::Right).qmlItemNode();

    if (verticalCentered())
        m_verticalTarget = m_fxItemNode.anchors().instanceAnchor(AnchorLine::VerticalCenter).qmlItemNode();

    if (horizontalCentered())
        m_horizontalTarget = m_fxItemNode.anchors().instanceAnchor(AnchorLine::HorizontalCenter).qmlItemNode();


    emit parentChanged();
    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit centeredHChanged();
    emit centeredVChanged();
    emit anchorsChanged();

    if (m_fxItemNode.hasNodeParent()) {
        emit itemNodeChanged();
        emit topTargetChanged();
        emit bottomTargetChanged();
        emit leftTargetChanged();
        emit rightTargetChanged();
        emit verticalTargetChanged();
        emit horizontalTargetChanged();
    }
}

bool QmlAnchorBindingProxy::hasParent()
{
    return m_fxItemNode.isValid() && m_fxItemNode.hasNodeParent();
}

bool QmlAnchorBindingProxy::topAnchored()
{
    return m_fxItemNode.isValid() && m_fxItemNode.anchors().instanceHasAnchor(AnchorLine::Top);
}

bool QmlAnchorBindingProxy::bottomAnchored()
{
    return m_fxItemNode.isValid() && m_fxItemNode.anchors().instanceHasAnchor(AnchorLine::Bottom);
}

bool QmlAnchorBindingProxy::leftAnchored()
{
    return m_fxItemNode.isValid() && m_fxItemNode.anchors().instanceHasAnchor(AnchorLine::Left);
}

bool QmlAnchorBindingProxy::rightAnchored()
{
    return m_fxItemNode.isValid() && m_fxItemNode.anchors().instanceHasAnchor(AnchorLine::Right);
}

bool QmlAnchorBindingProxy::hasAnchors()
{
    return m_fxItemNode.isValid() && m_fxItemNode.anchors().instanceHasAnchors();
}


void QmlAnchorBindingProxy::setTopTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_topTarget)
        return;

    m_topTarget = newTarget;
    calcTopMargin();

    emit topTargetChanged();
}


void QmlAnchorBindingProxy::setBottomTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_bottomTarget)
        return;

    m_bottomTarget = newTarget;
    calcBottomMargin();

    emit bottomTargetChanged();
}

void QmlAnchorBindingProxy::setLeftTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_leftTarget)
        return;

    m_leftTarget = newTarget;
    calcLeftMargin();

    emit leftTargetChanged();
}

void QmlAnchorBindingProxy::setRightTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_rightTarget)
        return;

    m_rightTarget = newTarget;
    calcRightMargin();

    emit rightTargetChanged();
}

void QmlAnchorBindingProxy::setVerticalTarget(const QVariant &target)
{
     QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_verticalTarget)
        return;

    m_verticalTarget = newTarget;
    m_fxItemNode.anchors().setAnchor(AnchorLine::VerticalCenter, m_verticalTarget, AnchorLine::VerticalCenter);

    emit verticalTargetChanged();
}

void QmlAnchorBindingProxy::setHorizontalTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_horizontalTarget)
        return;

    m_horizontalTarget = newTarget;
    m_fxItemNode.anchors().setAnchor(AnchorLine::HorizontalCenter, m_horizontalTarget, AnchorLine::HorizontalCenter);

    emit horizontalTargetChanged();
}

void QmlAnchorBindingProxy::resetLayout() {
        m_fxItemNode.anchors().removeAnchors();
        m_fxItemNode.anchors().removeMargins();

        emit topAnchorChanged();
        emit bottomAnchorChanged();
        emit leftAnchorChanged();
        emit rightAnchorChanged();
        emit anchorsChanged();
    }

void QmlAnchorBindingProxy::setBottomAnchor(bool anchor)
{
    if (!m_fxItemNode.hasNodeParent())
        return;

    if (bottomAnchored() == anchor)
        return;

    if (!anchor) {
        removeBottomAnchor();
    } else {
        calcBottomMargin();
    }
    emit bottomAnchorChanged();

    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::setLeftAnchor(bool anchor)
{
    if (!m_fxItemNode.hasNodeParent())
        return;

    if (leftAnchored() == anchor)
        return;

    if (!anchor) {
        removeLeftAnchor();
    } else {
        calcLeftMargin();
    }
    emit leftAnchorChanged();
    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::setRightAnchor(bool anchor)
{
    if (!m_fxItemNode.hasNodeParent())
        return;

    if (rightAnchored() == anchor)
        return;

    if (!anchor) {
        removeRightAnchor();
    } else {
        calcRightMargin();
    }
    emit rightAnchorChanged();
    if (hasAnchors() != anchor)
        emit anchorsChanged();
}
 QRectF QmlAnchorBindingProxy::parentBoundingBox()
{
    if (m_fxItemNode.hasInstanceParent())
        return m_fxItemNode.instanceParent().toQmlItemNode().instanceBoundingRect();

    return QRect();
}

QRectF QmlAnchorBindingProxy::boundingBox(QmlItemNode node)
{
    if (node.isValid())
        return node.instanceTransform().mapRect(node.instanceBoundingRect());

    return QRect();
}

QRectF QmlAnchorBindingProxy::transformedBoundingBox()
{
    return m_fxItemNode.instanceTransform().mapRect(m_fxItemNode.instanceBoundingRect());
}

void QmlAnchorBindingProxy::calcTopMargin()
{
    if (m_topTarget == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal topMargin = transformedBoundingBox().top() - parentBoundingBox().top();
        m_fxItemNode.anchors().setMargin( AnchorLine::Top, topMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Top, m_topTarget, AnchorLine::Top);
    } else {
        qDebug() << boundingBox(m_fxItemNode).top();
        qDebug() << boundingBox(m_topTarget).bottom();
        qreal topMargin = boundingBox(m_fxItemNode).top() - boundingBox(m_topTarget).bottom();
        m_fxItemNode.anchors().setMargin( AnchorLine::Top, topMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Top, m_topTarget, AnchorLine::Bottom);
    }
}

void QmlAnchorBindingProxy::calcBottomMargin()
{
    if (m_bottomTarget == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal bottomMargin =  parentBoundingBox().bottom() - transformedBoundingBox().bottom();
        m_fxItemNode.anchors().setMargin( AnchorLine::Bottom, bottomMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Bottom, m_bottomTarget, AnchorLine::Bottom);
    } else {
        qreal bottomMargin = boundingBox(m_fxItemNode).bottom() - boundingBox(m_rightTarget).top();
        m_fxItemNode.anchors().setMargin( AnchorLine::Bottom, bottomMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Bottom, m_bottomTarget, AnchorLine::Top);
    }
}

void QmlAnchorBindingProxy::calcLeftMargin()
{
    if (m_leftTarget == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal leftMargin = transformedBoundingBox().left() - parentBoundingBox().left();
        m_fxItemNode.anchors().setMargin(AnchorLine::Left, leftMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Left, m_leftTarget, AnchorLine::Left);
    } else {
        qreal leftMargin = boundingBox(m_fxItemNode).left() - boundingBox(m_leftTarget).right();
        m_fxItemNode.anchors().setMargin( AnchorLine::Left, leftMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Left, m_leftTarget, AnchorLine::Right);
    }
}

void QmlAnchorBindingProxy::calcRightMargin()
{
    if (m_rightTarget == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal rightMargin = parentBoundingBox().right() - transformedBoundingBox().right();
        m_fxItemNode.anchors().setMargin( AnchorLine::Right, rightMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Right, m_rightTarget, AnchorLine::Right);
    } else {
        qreal rightMargin = boundingBox(m_rightTarget).left() - boundingBox(m_fxItemNode).right();
        m_fxItemNode.anchors().setMargin( AnchorLine::Right, rightMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Right, m_rightTarget, AnchorLine::Left);
    }
}

void QmlAnchorBindingProxy::setTopAnchor(bool anchor)
{
    if (!m_fxItemNode.hasNodeParent())
        return ;

    if (topAnchored() == anchor)
        return;

    if (!anchor) {
        removeTopAnchor();
    } else {
        calcTopMargin();
    }
    emit topAnchorChanged();
    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::removeTopAnchor() {
    m_fxItemNode.anchors().removeAnchor(AnchorLine::Top);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Top);
}

void QmlAnchorBindingProxy::removeBottomAnchor() {
    m_fxItemNode.anchors().removeAnchor(AnchorLine::Bottom);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Bottom);
}

void QmlAnchorBindingProxy::removeLeftAnchor() {
    m_fxItemNode.anchors().removeAnchor(AnchorLine::Left);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Left);
}

void QmlAnchorBindingProxy::removeRightAnchor() {
    m_fxItemNode.anchors().removeAnchor(AnchorLine::Right);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Right);
}

void QmlAnchorBindingProxy::setVerticalCentered(bool centered)
{
    if (!m_fxItemNode.hasNodeParent())
        return ;

    if (verticalCentered() == centered)
        return;

    if (!centered) {
        m_fxItemNode.anchors().removeAnchor(AnchorLine::VerticalCenter);
        m_fxItemNode.anchors().removeMargin(AnchorLine::VerticalCenter);
    } else {
        m_fxItemNode.anchors().setAnchor(AnchorLine::VerticalCenter, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::VerticalCenter);
    }
    emit centeredVChanged();
}

void QmlAnchorBindingProxy::setHorizontalCentered(bool centered)
{
    if (!m_fxItemNode.hasNodeParent())
        return ;

    if (horizontalCentered() == centered)
        return;

    if (!centered) {
        m_fxItemNode.anchors().removeAnchor(AnchorLine::HorizontalCenter);
        m_fxItemNode.anchors().removeMargin(AnchorLine::HorizontalCenter);
    } else {
        m_fxItemNode.anchors().setAnchor(AnchorLine::HorizontalCenter, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::HorizontalCenter);
    }
    emit centeredHChanged();
}

bool QmlAnchorBindingProxy::verticalCentered()
{
    return m_fxItemNode.isValid() && m_fxItemNode.anchors().instanceHasAnchor(AnchorLine::VerticalCenter);
}

bool QmlAnchorBindingProxy::horizontalCentered()
{
    return m_fxItemNode.isValid() && m_fxItemNode.anchors().instanceHasAnchor(AnchorLine::HorizontalCenter);
}

void QmlAnchorBindingProxy::fill()
{
    m_fxItemNode.anchors().fill();

    setHorizontalCentered(false);
    setVerticalCentered(false);

    m_fxItemNode.anchors().setMargin(AnchorLine::Right, 0);
    m_fxItemNode.anchors().setMargin(AnchorLine::Left, 0);
    m_fxItemNode.anchors().setMargin(AnchorLine::Top, 0);
    m_fxItemNode.anchors().setMargin(AnchorLine::Bottom, 0);

    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit anchorsChanged();
}

} // namespace Internal
} // namespace QmlDesigner

