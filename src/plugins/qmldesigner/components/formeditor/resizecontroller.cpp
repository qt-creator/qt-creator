/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "resizecontroller.h"

#include "formeditoritem.h"
#include "layeritem.h"

#include <resizehandleitem.h>
#include <QCursor>
#include <QGraphicsScene>

namespace QmlDesigner {



ResizeControllerData::ResizeControllerData(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : layerItem(layerItem),
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

ResizeControllerData::ResizeControllerData(const ResizeControllerData &other)
    : layerItem(other.layerItem),
    formEditorItem(other.formEditorItem),
    topLeftItem(other.topLeftItem),
    topRightItem(other.topRightItem),
    bottomLeftItem(other.bottomLeftItem),
    bottomRightItem(other.bottomRightItem),
    topItem(other.topItem),
    leftItem(other.leftItem),
    rightItem(other.rightItem),
    bottomItem(other.bottomItem)
{}

ResizeControllerData::~ResizeControllerData()
{
    if (layerItem) {
        layerItem->scene()->removeItem(topLeftItem);
        layerItem->scene()->removeItem(topRightItem);
        layerItem->scene()->removeItem(bottomLeftItem);
        layerItem->scene()->removeItem(bottomRightItem);
        layerItem->scene()->removeItem(topItem);
        layerItem->scene()->removeItem(leftItem);
        layerItem->scene()->removeItem(rightItem);
        layerItem->scene()->removeItem(bottomItem);
    }
}


ResizeController::ResizeController()
   : m_data(new ResizeControllerData(0, 0))
{

}

ResizeController::ResizeController(const QSharedPointer<ResizeControllerData> &data)
    : m_data(data)
{

}

ResizeController::ResizeController(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : m_data(new ResizeControllerData(layerItem, formEditorItem))
{
    m_data->topLeftItem = new ResizeHandleItem(layerItem, *this);
    m_data->topLeftItem->setZValue(302);
    m_data->topLeftItem->setCursor(Qt::SizeFDiagCursor);

    m_data->topRightItem = new ResizeHandleItem(layerItem, *this);
    m_data->topRightItem->setZValue(301);
    m_data->topRightItem->setCursor(Qt::SizeBDiagCursor);

    m_data->bottomLeftItem = new ResizeHandleItem(layerItem, *this);
    m_data->bottomLeftItem->setZValue(301);
    m_data->bottomLeftItem->setCursor(Qt::SizeBDiagCursor);

    m_data->bottomRightItem = new ResizeHandleItem(layerItem, *this);
    m_data->bottomRightItem->setZValue(305);
    m_data->bottomRightItem->setCursor(Qt::SizeFDiagCursor);

    m_data->topItem = new ResizeHandleItem(layerItem, *this);
    m_data->topItem->setZValue(300);
    m_data->topItem->setCursor(Qt::SizeVerCursor);

    m_data->leftItem = new ResizeHandleItem(layerItem, *this);
    m_data->leftItem->setZValue(300);
    m_data->leftItem->setCursor(Qt::SizeHorCursor);

    m_data->rightItem = new ResizeHandleItem(layerItem, *this);
    m_data->rightItem->setZValue(300);
    m_data->rightItem->setCursor(Qt::SizeHorCursor);

    m_data->bottomItem = new ResizeHandleItem(layerItem, *this);
    m_data->bottomItem->setZValue(300);
    m_data->bottomItem->setCursor(Qt::SizeVerCursor);

    updatePosition();
}


bool ResizeController::isValid() const
{
    return m_data->formEditorItem && m_data->formEditorItem->qmlItemNode().isValid();
}

void ResizeController::show()
{
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
    if (isValid()) {

        QRectF boundingRect = m_data->formEditorItem->qmlItemNode().instanceBoundingRect();
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
}


FormEditorItem* ResizeController::formEditorItem() const
{
    return m_data->formEditorItem.data();
}

QWeakPointer<ResizeControllerData> ResizeController::weakPointer() const
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
