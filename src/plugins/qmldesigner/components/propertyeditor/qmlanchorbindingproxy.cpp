/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "abstractview.h"
#include <qmlanchors.h>
#include <nodeabstractproperty.h>
#include <nodeinstance.h>
#include <QDebug>

namespace QmlDesigner {

class ModelNode;
class NodeState;

const QString auxDataString = QLatin1String("anchors_");

namespace Internal {

QmlAnchorBindingProxy::QmlAnchorBindingProxy(QObject *parent) :
        QObject(parent), m_locked(false)
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

    emit itemNodeChanged();
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

void QmlAnchorBindingProxy::invalidate(const QmlItemNode &fxItemNode)
{
    if (m_locked)
        return;

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

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_topTarget = newTarget;
    calcTopMargin();

    emit topTargetChanged();
}


void QmlAnchorBindingProxy::setBottomTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_bottomTarget)
        return;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_bottomTarget = newTarget;
    calcBottomMargin();

    emit bottomTargetChanged();
}

void QmlAnchorBindingProxy::setLeftTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_leftTarget)
        return;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_leftTarget = newTarget;
    calcLeftMargin();

    emit leftTargetChanged();
}

void QmlAnchorBindingProxy::setRightTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_rightTarget)
        return;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_rightTarget = newTarget;
    calcRightMargin();

    emit rightTargetChanged();
}

void QmlAnchorBindingProxy::setVerticalTarget(const QVariant &target)
{
     QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_verticalTarget)
        return;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_verticalTarget = newTarget;
    m_fxItemNode.anchors().setAnchor(AnchorLine::VerticalCenter, m_verticalTarget, AnchorLine::VerticalCenter);

    emit verticalTargetChanged();
}

void QmlAnchorBindingProxy::setHorizontalTarget(const QVariant &target)
{
    QmlItemNode newTarget(target.value<ModelNode>());

    if (newTarget == m_horizontalTarget)
        return;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_horizontalTarget = newTarget;
    m_fxItemNode.anchors().setAnchor(AnchorLine::HorizontalCenter, m_horizontalTarget, AnchorLine::HorizontalCenter);

    emit horizontalTargetChanged();
}

void QmlAnchorBindingProxy::resetLayout() {
    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

        m_fxItemNode.anchors().removeAnchors();
        m_fxItemNode.anchors().removeMargins();

        if (qFuzzyCompare(m_fxItemNode.instancePosition().x(), 0.0))
            m_fxItemNode.setVariantProperty("x", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "x"));
        if (qFuzzyCompare(m_fxItemNode.instancePosition().y(), 0.0))
            m_fxItemNode.setVariantProperty("y", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "y"));
        if (qFuzzyCompare(m_fxItemNode.instanceSize().width(), 0.0))
            m_fxItemNode.setVariantProperty("width", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "width"));
        if (qFuzzyCompare(m_fxItemNode.instanceSize().height(), 0.0))
            m_fxItemNode.setVariantProperty("height", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "height"));

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

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    if (!anchor) {
        removeBottomAnchor();
    } else {
        calcBottomMargin();
        m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "height", m_fxItemNode.instanceSize().height());
        m_fxItemNode.removeVariantProperty("height");
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

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    if (!anchor) {
        removeLeftAnchor();
    } else {
        calcLeftMargin();
        m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "x", m_fxItemNode.instancePosition().x());
        m_fxItemNode.removeVariantProperty("x");
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

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    if (!anchor) {
        removeRightAnchor();
    } else {
        calcRightMargin();
        m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "width", m_fxItemNode.instanceSize().width());
        m_fxItemNode.removeVariantProperty("width");
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
    m_locked = true;

    if (m_topTarget.modelNode() == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal topMargin = transformedBoundingBox().top() - parentBoundingBox().top();
        m_fxItemNode.anchors().setMargin( AnchorLine::Top, topMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Top, m_topTarget, AnchorLine::Top);
    } else {
        qreal topMargin = boundingBox(m_fxItemNode).top() - boundingBox(m_topTarget).bottom();
        m_fxItemNode.anchors().setMargin( AnchorLine::Top, topMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Top, m_topTarget, AnchorLine::Bottom);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::calcBottomMargin()
{
    m_locked = true;

    if (m_bottomTarget.modelNode() == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal bottomMargin =  parentBoundingBox().bottom() - transformedBoundingBox().bottom();
        m_fxItemNode.anchors().setMargin( AnchorLine::Bottom, bottomMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Bottom, m_bottomTarget, AnchorLine::Bottom);
    } else {
        qreal bottomMargin = boundingBox(m_fxItemNode).bottom() - boundingBox(m_rightTarget).top();
        m_fxItemNode.anchors().setMargin( AnchorLine::Bottom, bottomMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Bottom, m_bottomTarget, AnchorLine::Top);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::calcLeftMargin()
{
    m_locked = true;

    if (m_leftTarget.modelNode() == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal leftMargin = transformedBoundingBox().left() - parentBoundingBox().left();
        m_fxItemNode.anchors().setMargin(AnchorLine::Left, leftMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Left, m_leftTarget, AnchorLine::Left);
    } else {
        qreal leftMargin = boundingBox(m_fxItemNode).left() - boundingBox(m_leftTarget).right();
        m_fxItemNode.anchors().setMargin( AnchorLine::Left, leftMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Left, m_leftTarget, AnchorLine::Right);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::calcRightMargin()
{
    m_locked = true;

    if (m_rightTarget.modelNode() == m_fxItemNode.modelNode().parentProperty().parentModelNode()) {
        qreal rightMargin = parentBoundingBox().right() - transformedBoundingBox().right();
        m_fxItemNode.anchors().setMargin( AnchorLine::Right, rightMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Right, m_rightTarget, AnchorLine::Right);
    } else {
        qreal rightMargin = boundingBox(m_rightTarget).left() - boundingBox(m_fxItemNode).right();
        m_fxItemNode.anchors().setMargin( AnchorLine::Right, rightMargin);
        m_fxItemNode.anchors().setAnchor(AnchorLine::Right, m_rightTarget, AnchorLine::Left);
    }

    m_locked = false;
}

void QmlAnchorBindingProxy::setTopAnchor(bool anchor)
{
    if (!m_fxItemNode.hasNodeParent())
        return ;

    if (topAnchored() == anchor)
        return;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    if (!anchor) {
        removeTopAnchor();
    } else {
        calcTopMargin();
        m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "y", m_fxItemNode.instancePosition().y());
        m_fxItemNode.removeVariantProperty("y");
    }
    emit topAnchorChanged();
    if (hasAnchors() != anchor)
        emit anchorsChanged();
}

void QmlAnchorBindingProxy::removeTopAnchor() {
    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_fxItemNode.anchors().removeAnchor(AnchorLine::Top);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Top);

    if (qFuzzyCompare(m_fxItemNode.instancePosition().y(), 0.0) && m_fxItemNode.modelNode().hasAuxiliaryData(auxDataString + "y"))
        m_fxItemNode.setVariantProperty("y", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "y"));

}

void QmlAnchorBindingProxy::removeBottomAnchor() {
    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_fxItemNode.anchors().removeAnchor(AnchorLine::Bottom);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Bottom);

    if (qFuzzyCompare(m_fxItemNode.instanceSize().height(), 0.0) && m_fxItemNode.modelNode().hasAuxiliaryData(auxDataString + "height"))
        m_fxItemNode.setVariantProperty("height", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "height"));
}

void QmlAnchorBindingProxy::removeLeftAnchor() {
    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_fxItemNode.anchors().removeAnchor(AnchorLine::Left);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Left);

    if (qFuzzyCompare(m_fxItemNode.instancePosition().x(), 0.0) && m_fxItemNode.modelNode().hasAuxiliaryData(auxDataString + "x"))
        m_fxItemNode.setVariantProperty("x", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "x"));
}

void QmlAnchorBindingProxy::removeRightAnchor() {
    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_fxItemNode.anchors().removeAnchor(AnchorLine::Right);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Right);

    if (qFuzzyCompare(m_fxItemNode.instanceSize().width(), 0.0) && m_fxItemNode.modelNode().hasAuxiliaryData(auxDataString + "width"))
        m_fxItemNode.setVariantProperty("width", m_fxItemNode.modelNode().auxiliaryData(auxDataString + "width"));
}

void QmlAnchorBindingProxy::setVerticalCentered(bool centered)
{
    if (!m_fxItemNode.hasNodeParent())
        return ;

    if (verticalCentered() == centered)
        return;

    m_locked = true;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    if (!centered) {
        m_fxItemNode.anchors().removeAnchor(AnchorLine::VerticalCenter);
        m_fxItemNode.anchors().removeMargin(AnchorLine::VerticalCenter);
    } else {
        m_fxItemNode.anchors().setAnchor(AnchorLine::VerticalCenter, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::VerticalCenter);
    }

    m_locked = false;

    emit centeredVChanged();
}

void QmlAnchorBindingProxy::setHorizontalCentered(bool centered)
{
    if (!m_fxItemNode.hasNodeParent())
        return ;

    if (horizontalCentered() == centered)
        return;

    m_locked = true;

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    if (!centered) {
        m_fxItemNode.anchors().removeAnchor(AnchorLine::HorizontalCenter);
        m_fxItemNode.anchors().removeMargin(AnchorLine::HorizontalCenter);
    } else {
        m_fxItemNode.anchors().setAnchor(AnchorLine::HorizontalCenter, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::HorizontalCenter);
    }

    m_locked = false;

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

    RewriterTransaction transaction = m_fxItemNode.modelNode().view()->beginRewriterTransaction();

    m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "x", m_fxItemNode.instancePosition().x());
    m_fxItemNode.removeVariantProperty("x");
    m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "y", m_fxItemNode.instancePosition().y());
    m_fxItemNode.removeVariantProperty("y");
    m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "width", m_fxItemNode.instanceSize().width());
    m_fxItemNode.removeVariantProperty("width");
    m_fxItemNode.modelNode().setAuxiliaryData(auxDataString + "height", m_fxItemNode.instanceSize().height());
    m_fxItemNode.removeVariantProperty("height");

    m_fxItemNode.anchors().fill();

    setHorizontalCentered(false);
    setVerticalCentered(false);

    m_fxItemNode.anchors().removeMargin(AnchorLine::Right);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Left);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Top);
    m_fxItemNode.anchors().removeMargin(AnchorLine::Bottom);

    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit anchorsChanged();
}

} // namespace Internal
} // namespace QmlDesigner

