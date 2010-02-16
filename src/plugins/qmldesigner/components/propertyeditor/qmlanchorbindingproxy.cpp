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
    emit parentChanged();
    emit topAnchorChanged();
    emit bottomAnchorChanged();
    emit leftAnchorChanged();
    emit rightAnchorChanged();
    emit centeredHChanged();
    emit centeredVChanged();
    emit anchorsChanged();
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

        qreal bottomMargin =  parentBoundingBox().bottom() - transformedBoundingBox().bottom();

        m_fxItemNode.anchors().setAnchor(AnchorLine::Bottom, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::Bottom);
        m_fxItemNode.anchors().setMargin(AnchorLine::Bottom, bottomMargin);
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
        qreal leftMargin = transformedBoundingBox().left() - parentBoundingBox().left();

        m_fxItemNode.anchors().setAnchor(AnchorLine::Left, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::Left);
        m_fxItemNode.anchors().setMargin(AnchorLine::Left, leftMargin);
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

        qreal rightMargin =  parentBoundingBox().right() - transformedBoundingBox().right();

        m_fxItemNode.anchors().setAnchor(AnchorLine::Right, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::Right);
        m_fxItemNode.anchors().setMargin(AnchorLine::Right, rightMargin);
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

QRectF QmlAnchorBindingProxy::transformedBoundingBox()
{
    return m_fxItemNode.instanceTransform().mapRect(m_fxItemNode.instanceBoundingRect());
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

        qreal topMargin = transformedBoundingBox().top() - parentBoundingBox().top();

        m_fxItemNode.anchors().setAnchor(AnchorLine::Top, m_fxItemNode.modelNode().parentProperty().parentModelNode(), AnchorLine::Top);
        m_fxItemNode.anchors().setMargin( AnchorLine::Top, topMargin);
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

