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

#include "anchorhandleitem.h"

#include <formeditoritem.h>
#include <QPen>
#include <QPainterPathStroker>
#include <cmath>
#include <QtDebug>

namespace QmlDesigner {

AnchorHandleItem::AnchorHandleItem(QGraphicsItem *parent, const AnchorController &anchorController)
    : QGraphicsItemGroup(parent),
    m_anchorControllerData(anchorController.weakPointer()),
    m_sourceAnchorLinePathItem(new QGraphicsPathItem(this)),
    m_arrowPathItem(new QGraphicsPathItem(this)),
    m_targetAnchorLinePathItem(new QGraphicsPathItem(this)),
    m_targetNamePathItem(new QGraphicsPathItem(this))
{
    addToGroup(m_sourceAnchorLinePathItem);
    addToGroup(m_arrowPathItem);
    addToGroup(m_targetAnchorLinePathItem);
    addToGroup(m_targetNamePathItem);

    setFlag(QGraphicsItem::ItemIsMovable, true);
}

AnchorLine::Type AnchorHandleItem::sourceAnchorLine() const
{
    if (isTopHandle())
        return AnchorLine::Top;
    if (isBottomHandle())
        return AnchorLine::Bottom;
    if (isLeftHandle())
        return AnchorLine::Left;
    if (isRightHandle())
        return AnchorLine::Right;

    return AnchorLine::Invalid;
}

AnchorLine AnchorHandleItem::targetAnchorLine() const
{
    QmlAnchors anchors(anchorController().formEditorItem()->qmlItemNode().anchors());

    if (isTopHandle())
        return anchors.instanceAnchor(AnchorLine::Top);
    if (isBottomHandle())
        return anchors.instanceAnchor(AnchorLine::Bottom);
    if (isLeftHandle())
        return anchors.instanceAnchor(AnchorLine::Left);
    if (isRightHandle())
        return anchors.instanceAnchor(AnchorLine::Right);

    return AnchorLine();
}

static QString anchorLineToString(AnchorLine::Type anchorLineType)
{
    switch(anchorLineType) {
        case AnchorLine::Top: return "Top";
        case AnchorLine::Bottom: return "Bottom";
        case AnchorLine::Left: return "Left";
        case AnchorLine::Right: return "Right";
        default: break;
    }

    return QString();

}

QString AnchorHandleItem::toolTipString() const
{
    QString templateString("<p>Anchor Handle</p><p>%1</p><p>%2</p>");
    QmlItemNode fromNode(anchorController().formEditorItem()->qmlItemNode());
    QString fromString(QString("%3: %1(%2)").arg(fromNode.simplfiedTypeName(), fromNode.id(), anchorLineToString(sourceAnchorLine())));

    AnchorLine toAnchorLine(targetAnchorLine());
    QmlItemNode toNode(toAnchorLine.qmlItemNode());
    QString toString;
    if (toNode.isValid())
        toString = QString("%3: %1(%2)").arg(toNode.simplfiedTypeName(), toNode.id(), anchorLineToString(toAnchorLine.type()));

    return templateString.arg(fromString).arg(toString);
}

void AnchorHandleItem::setHandlePath(const AnchorHandlePathData  &pathData)
{
    m_beginArrowPoint = pathData.beginArrowPoint;
    m_endArrowPoint = pathData.endArrowPoint;
    m_arrowPathItem->setPath(pathData.arrowPath);
    m_sourceAnchorLinePathItem->setPath(pathData.sourceAnchorLinePath);
    m_targetAnchorLinePathItem->setPath(pathData.targetAnchorLinePath);
    m_targetNamePathItem->setPath(pathData.targetNamePath);

    setHighlighted(false);
}

AnchorController AnchorHandleItem::anchorController() const
{
    Q_ASSERT(!m_anchorControllerData.isNull());
    return AnchorController(m_anchorControllerData.toStrongRef());
}

AnchorHandleItem* AnchorHandleItem::fromGraphicsItem(QGraphicsItem *item)
{
    return qgraphicsitem_cast<AnchorHandleItem*>(item);
}

bool AnchorHandleItem::isTopHandle() const
{
    return anchorController().isTopHandle(this);
}

bool AnchorHandleItem::isLeftHandle() const
{
    return anchorController().isLeftHandle(this);
}

bool AnchorHandleItem::isRightHandle() const
{
    return anchorController().isRightHandle(this);
}

bool AnchorHandleItem::isBottomHandle() const
{
    return anchorController().isBottomHandle(this);
}

AnchorLine::Type AnchorHandleItem::anchorType() const
{
    if (isTopHandle())
        return AnchorLine::Top;

    if (isBottomHandle())
        return AnchorLine::Bottom;

    if (isLeftHandle())
        return AnchorLine::Left;

    if (isRightHandle())
        return AnchorLine::Right;


    return AnchorLine::Invalid;
}

void AnchorHandleItem::setHighlighted(bool highlight)
{
    QLinearGradient gradient(m_beginArrowPoint, m_endArrowPoint);
    gradient.setCoordinateMode(QGradient::LogicalMode);
    m_arrowPathItem->setPen(QPen(QBrush(Qt::gray), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
    m_targetAnchorLinePathItem->setPen(QColor(70, 0, 0, 90));
    m_targetAnchorLinePathItem->setBrush(QColor(255, 0, 0, 50));
    m_arrowPathItem->setPen(QPen(QBrush(Qt::gray), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
    m_targetNamePathItem->setPen(QColor(0, 0, 255, 90));
    m_targetNamePathItem->setBrush(QColor(0, 0, 255, 50));

    if (highlight) {
        gradient.setColorAt(0.0,  QColor(0, 0, 120, 255));
        gradient.setColorAt(1.0,  QColor(120, 0, 0, 255));
        m_arrowPathItem->setBrush(gradient);
        m_sourceAnchorLinePathItem->setPen(QColor(0, 0, 70, 255));
        m_sourceAnchorLinePathItem->setBrush(QColor(0, 0, 70, 255));
        m_targetAnchorLinePathItem->show();
        m_targetNamePathItem->show();

    } else {
        gradient.setColorAt(0.0,  QColor(0, 0, 255, 255));
        gradient.setColorAt(1.0,  QColor(255, 0, 0, 255));
        m_arrowPathItem->setBrush(gradient);
        m_sourceAnchorLinePathItem->setPen(QColor(0, 0, 100, 255));
        m_sourceAnchorLinePathItem->setBrush(QColor(0, 0, 100, 255));
        m_targetAnchorLinePathItem->hide();
        m_targetNamePathItem->hide();
    }
}

QPointF AnchorHandleItem::itemSpacePosition() const
{
    return parentItem()->mapToItem(anchorController().formEditorItem(), pos());
}

} // namespace QmlDesigner
