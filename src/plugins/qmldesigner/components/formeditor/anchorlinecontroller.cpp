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

#include "anchorlinecontroller.h"

#include "formeditoritem.h"
#include "layeritem.h"
#include <QGraphicsScene>

#include "anchorlinehandleitem.h"

namespace QmlDesigner {


AnchorLineControllerData::AnchorLineControllerData(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : layerItem(layerItem),
    formEditorItem(formEditorItem),
    topItem(0),
    leftItem(0),
    rightItem(0),
    bottomItem(0)
{
}

AnchorLineControllerData::AnchorLineControllerData(const AnchorLineControllerData &other)
    : layerItem(other.layerItem),
    formEditorItem(other.formEditorItem),
    topItem(other.topItem),
    leftItem(other.leftItem),
    rightItem(other.rightItem),
    bottomItem(other.bottomItem)
{}

AnchorLineControllerData::~AnchorLineControllerData()
{
    if (layerItem) {
        layerItem->scene()->removeItem(topItem);
        layerItem->scene()->removeItem(leftItem);
        layerItem->scene()->removeItem(rightItem);
        layerItem->scene()->removeItem(bottomItem);
    }
}


AnchorLineController::AnchorLineController()
   : m_data(new AnchorLineControllerData(0, 0))
{

}

AnchorLineController::AnchorLineController(const QSharedPointer<AnchorLineControllerData> &data)
    : m_data(data)
{

}

AnchorLineController::AnchorLineController(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : m_data(new AnchorLineControllerData(layerItem, formEditorItem))
{
    m_data->topItem = new AnchorLineHandleItem(layerItem, *this);
    m_data->topItem->setZValue(300);

    m_data->leftItem = new AnchorLineHandleItem(layerItem, *this);
    m_data->leftItem->setZValue(300);

    m_data->rightItem = new AnchorLineHandleItem(layerItem, *this);
    m_data->rightItem->setZValue(300);

    m_data->bottomItem = new AnchorLineHandleItem(layerItem, *this);
    m_data->bottomItem->setZValue(300);

    updatePosition();
}


bool AnchorLineController::isValid() const
{
    return m_data->formEditorItem != 0;
}

void AnchorLineController::show(AnchorLine::Type anchorLineMask)
{
    if (anchorLineMask & AnchorLine::Top)
        m_data->topItem->show();
    else
        m_data->topItem->hide();

    if (anchorLineMask & AnchorLine::Left)
        m_data->leftItem->show();
    else
        m_data->leftItem->hide();

    if (anchorLineMask & AnchorLine::Right)
        m_data->rightItem->show();
    else
        m_data->rightItem->hide();

    if (anchorLineMask & AnchorLine::Bottom)
        m_data->bottomItem->show();
    else
        m_data->bottomItem->hide();
}

void AnchorLineController::hide()
{
    m_data->topItem->hide();
    m_data->leftItem->hide();
    m_data->rightItem->hide();
    m_data->bottomItem->hide();
}

static QPainterPath rectToPath(const QRectF &rect)
{
    QPainterPath path;
    path.addRoundedRect(rect, 4, 4);

    return path;
}

void AnchorLineController::updatePosition()
{
    QRectF boundingRect = m_data->formEditorItem->qmlItemNode().instanceBoundingRect();

    QRectF topBoundingRect(boundingRect);
    QRectF leftBoundingRect(boundingRect);
    QRectF bottomBoundingRect(boundingRect);
    QRectF rightBoundingRect(boundingRect);


    if (formEditorItem()->isContainer()) {
        topBoundingRect.setBottom(boundingRect.top() + 6);
        topBoundingRect.adjust(7, -5, -7, 0);

        leftBoundingRect.setRight(boundingRect.left() + 6);
        leftBoundingRect.adjust(-5, 7, 0, -7);

        bottomBoundingRect.setTop(boundingRect.bottom() - 6);
        bottomBoundingRect.adjust(7, 0, -7, 5);

        rightBoundingRect.setLeft(boundingRect.right() - 6);
        rightBoundingRect.adjust(0, 7, 5, -7);

    } else {
        double height = qMin(boundingRect.height() / 4., 10.);
        double width = qMin(boundingRect.width() / 4., 10.);

        topBoundingRect.setHeight(height);
        topBoundingRect.adjust(width, -4, -width, -1);

        leftBoundingRect.setWidth(width);
        leftBoundingRect.adjust(-4, height, -1, -height);

        bottomBoundingRect.setTop(boundingRect.bottom() - height);
        bottomBoundingRect.adjust(width, 1, -width, 4);

        rightBoundingRect.setLeft(boundingRect.right() - width);
        rightBoundingRect.adjust(1, height, 4, -height);
    }

    m_data->topItem->setHandlePath(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                        rectToPath(topBoundingRect)));
    m_data->leftItem->setHandlePath(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                         rectToPath(leftBoundingRect)));
    m_data->bottomItem->setHandlePath(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                           rectToPath(bottomBoundingRect)));
    m_data->rightItem->setHandlePath(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                          rectToPath(rightBoundingRect)));
}


FormEditorItem* AnchorLineController::formEditorItem() const
{
    return m_data->formEditorItem;
}

QWeakPointer<AnchorLineControllerData> AnchorLineController::weakPointer() const
{
    return m_data;
}


bool AnchorLineController::isTopHandle(const AnchorLineHandleItem *handle) const
{
    return handle == m_data->topItem;
}

bool AnchorLineController::isLeftHandle(const AnchorLineHandleItem *handle) const
{
    return handle == m_data->leftItem;
}

bool AnchorLineController::isRightHandle(const AnchorLineHandleItem *handle) const
{
    return handle == m_data->rightItem;
}

bool AnchorLineController::isBottomHandle(const AnchorLineHandleItem *handle) const
{
    return handle == m_data->bottomItem;
}

void AnchorLineController::clearHighlight()
{
    m_data->topItem->setHiglighted(false);
    m_data->leftItem->setHiglighted(false);
    m_data->rightItem->setHiglighted(false);
    m_data->bottomItem->setHiglighted(false);
}

} // namespace QmlDesigner
