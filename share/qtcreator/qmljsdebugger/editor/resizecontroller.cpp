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

#include "resizecontroller.h"
#include "layeritem.h"

#include <resizehandleitem.h>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include <QDebug>

namespace QmlViewer {

ResizeControllerData::ResizeControllerData(LayerItem *layerItem, QGraphicsObject *formEditorItem)
    :
    layerItem(layerItem),
    formEditorItem(formEditorItem),
    topLeftItem(0),
    topRightItem(0),
    bottomLeftItem(0),
    bottomRightItem(0),
    topItem(0),
    leftItem(0),
    rightItem(0),
    bottomItem(0)
{

}

ResizeControllerData::~ResizeControllerData()
{
    clear();
}

void ResizeControllerData::clear()
{
    if (!formEditorItem.isNull() && topLeftItem) {
        formEditorItem.data()->scene()->removeItem(topLeftItem);
        formEditorItem.data()->scene()->removeItem(topRightItem);
        formEditorItem.data()->scene()->removeItem(bottomLeftItem);
        formEditorItem.data()->scene()->removeItem(bottomRightItem);
        formEditorItem.data()->scene()->removeItem(topItem);
        formEditorItem.data()->scene()->removeItem(leftItem);
        formEditorItem.data()->scene()->removeItem(rightItem);
        formEditorItem.data()->scene()->removeItem(bottomItem);
        formEditorItem.clear();
        layerItem.clear();
        topLeftItem = 0;
        topRightItem = 0;
        bottomLeftItem = 0;
        bottomRightItem = 0;
        topItem = 0;
        leftItem = 0;
        rightItem = 0;
        bottomItem = 0;
    }
}

ResizeController::ResizeController()
   : m_data(new ResizeControllerData(0, 0))
{

}

ResizeController::~ResizeController()
{
    delete m_data;
    m_data = 0;
}

void ResizeController::setItem(LayerItem *layerItem, QGraphicsObject *item)
{
    createFor(layerItem, item);
}

ResizeController::ResizeController(LayerItem *layerItem, QGraphicsObject *formEditorItem) :
    m_data(new ResizeControllerData(layerItem, formEditorItem))
{
    createFor(layerItem, formEditorItem);
}

void ResizeController::createFor(LayerItem *layerItem, QGraphicsObject *formEditorItem)
{
    if (m_data)
        m_data->clear();
    else
        m_data = new ResizeControllerData(layerItem, formEditorItem);

    m_data->formEditorItem = formEditorItem;
    m_data->layerItem = layerItem;

    m_data->topLeftItem = new ResizeHandleItem(layerItem, this);
    m_data->topLeftItem->setZValue(3020);
    m_data->topLeftItem->setCustomCursor(Qt::SizeFDiagCursor);

    m_data->topRightItem = new ResizeHandleItem(layerItem, this);
    m_data->topRightItem->setZValue(3010);
    m_data->topRightItem->setCustomCursor(Qt::SizeBDiagCursor);

    m_data->bottomLeftItem = new ResizeHandleItem(layerItem, this);
    m_data->bottomLeftItem->setZValue(3010);
    m_data->bottomLeftItem->setCustomCursor(Qt::SizeBDiagCursor);

    m_data->bottomRightItem = new ResizeHandleItem(layerItem, this);
    m_data->bottomRightItem->setZValue(3050);
    m_data->bottomRightItem->setCustomCursor(Qt::SizeFDiagCursor);

    m_data->topItem = new ResizeHandleItem(layerItem, this);
    m_data->topItem->setZValue(3000);
    m_data->topItem->setCustomCursor(Qt::SizeVerCursor);

    m_data->leftItem = new ResizeHandleItem(layerItem, this);
    m_data->leftItem->setZValue(3000);
    m_data->leftItem->setCustomCursor(Qt::SizeHorCursor);

    m_data->rightItem = new ResizeHandleItem(layerItem, this);
    m_data->rightItem->setZValue(3000);
    m_data->rightItem->setCustomCursor(Qt::SizeHorCursor);

    m_data->bottomItem = new ResizeHandleItem(layerItem, this);
    m_data->bottomItem->setZValue(3000);
    m_data->bottomItem->setCustomCursor(Qt::SizeVerCursor);

    updatePosition();
}

bool ResizeController::isValid() const
{
    return !m_data->formEditorItem.isNull();
}

void ResizeController::show()
{
    updatePosition();

    m_data->topLeftItem->show();
    m_data->topRightItem->show();
    m_data->bottomLeftItem->show();
    m_data->bottomRightItem->show();
    m_data->topItem->show();
    m_data->leftItem->show();
    m_data->rightItem->show();
    m_data->bottomItem->show();
}
void ResizeController::hide()
{
    m_data->topLeftItem->hide();
    m_data->topRightItem->hide();
    m_data->bottomLeftItem->hide();
    m_data->bottomRightItem->hide();
    m_data->topItem->hide();
    m_data->leftItem->hide();
    m_data->rightItem->hide();
    m_data->bottomItem->hide();
}


static QPointF topCenter(const QRectF &rect)
{
    return QPointF(rect.center().x(), rect.top());
}

static QPointF leftCenter(const QRectF &rect)
{
    return QPointF(rect.left(), rect.center().y());
}

static QPointF rightCenter(const QRectF &rect)
{
    return QPointF(rect.right(), rect.center().y());
}

static QPointF bottomCenter(const QRectF &rect)
{
    return QPointF(rect.center().x(), rect.bottom());
}


void ResizeController::updatePosition()
{
    QGraphicsItem *formEditorItem = m_data->formEditorItem.data();
    QRectF boundingRect =formEditorItem->boundingRect();
    QPointF topLeftPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                       boundingRect.topLeft()));
    QPointF topRightPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                        boundingRect.topRight()));
    QPointF bottomLeftPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                          boundingRect.bottomLeft()));
    QPointF bottomRightPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                           boundingRect.bottomRight()));

    QPointF topPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                   topCenter(boundingRect)));
    QPointF leftPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                    leftCenter(boundingRect)));

    QPointF rightPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                     rightCenter(boundingRect)));
    QPointF bottomPointInLayerSpace(m_data->formEditorItem->mapToItem(m_data->layerItem.data(),
                                                                      bottomCenter(boundingRect)));


    m_data->topRightItem->setHandlePosition(topRightPointInLayerSpace, boundingRect.topRight());
    m_data->topLeftItem->setHandlePosition(topLeftPointInLayerSpace, boundingRect.topLeft());
    m_data->bottomLeftItem->setHandlePosition(bottomLeftPointInLayerSpace, boundingRect.bottomLeft());
    m_data->bottomRightItem->setHandlePosition(bottomRightPointInLayerSpace, boundingRect.bottomRight());
    m_data->topItem->setHandlePosition(topPointInLayerSpace, topCenter(boundingRect));
    m_data->leftItem->setHandlePosition(leftPointInLayerSpace, leftCenter(boundingRect));
    m_data->rightItem->setHandlePosition(rightPointInLayerSpace, rightCenter(boundingRect));
    m_data->bottomItem->setHandlePosition(bottomPointInLayerSpace, bottomCenter(boundingRect));

}


QGraphicsObject* ResizeController::formEditorItem() const
{
    return m_data->formEditorItem.data();
}

ResizeControllerData *ResizeController::data() const
{
    return m_data;
}

bool ResizeController::isTopLeftHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->topLeftItem;
}

bool ResizeController::isTopRightHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->topRightItem;
}

bool ResizeController::isBottomLeftHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->bottomLeftItem;
}

bool ResizeController::isBottomRightHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->bottomRightItem;
}

bool ResizeController::isTopHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->topItem;
}

bool ResizeController::isLeftHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->leftItem;
}

bool ResizeController::isRightHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->rightItem;
}

bool ResizeController::isBottomHandle(const ResizeHandleItem *handle) const
{
    return handle == m_data->bottomItem;
}

}
