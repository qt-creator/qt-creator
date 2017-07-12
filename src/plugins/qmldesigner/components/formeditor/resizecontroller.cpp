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

#include "resizecontroller.h"

#include "formeditoritem.h"
#include "layeritem.h"

#include <resizehandleitem.h>
#include <QCursor>
#include <QGraphicsScene>

namespace QmlDesigner {

class ResizeControllerData
{
public:
    ResizeControllerData(LayerItem *layerItem,
                         FormEditorItem *formEditorItem);
    ResizeControllerData(const ResizeControllerData &other);
    ~ResizeControllerData();


    QPointer<LayerItem> layerItem;
    FormEditorItem *formEditorItem = nullptr;
    QSharedPointer<ResizeHandleItem> topLeftItem;
    QSharedPointer<ResizeHandleItem> topRightItem;
    QSharedPointer<ResizeHandleItem> bottomLeftItem;
    QSharedPointer<ResizeHandleItem> bottomRightItem;
    QSharedPointer<ResizeHandleItem> topItem;
    QSharedPointer<ResizeHandleItem> leftItem;
    QSharedPointer<ResizeHandleItem> rightItem;
    QSharedPointer<ResizeHandleItem> bottomItem;
};

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
        QGraphicsScene *scene = layerItem->scene();
        scene->removeItem(topLeftItem.data());
        scene->removeItem(topRightItem.data());
        scene->removeItem(bottomLeftItem.data());
        scene->removeItem(bottomRightItem.data());
        scene->removeItem(topItem.data());
        scene->removeItem(leftItem.data());
        scene->removeItem(rightItem.data());
        scene->removeItem(bottomItem.data());
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
    m_data->topLeftItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->topLeftItem->setZValue(302);
    m_data->topLeftItem->setCursor(Qt::SizeFDiagCursor);

    m_data->topRightItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->topRightItem->setZValue(301);
    m_data->topRightItem->setCursor(Qt::SizeBDiagCursor);

    m_data->bottomLeftItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->bottomLeftItem->setZValue(301);
    m_data->bottomLeftItem->setCursor(Qt::SizeBDiagCursor);

    m_data->bottomRightItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->bottomRightItem->setZValue(305);
    m_data->bottomRightItem->setCursor(Qt::SizeFDiagCursor);

    m_data->topItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->topItem->setZValue(300);
    m_data->topItem->setCursor(Qt::SizeVerCursor);

    m_data->leftItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->leftItem->setZValue(300);
    m_data->leftItem->setCursor(Qt::SizeHorCursor);

    m_data->rightItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->rightItem->setZValue(300);
    m_data->rightItem->setCursor(Qt::SizeHorCursor);

    m_data->bottomItem = QSharedPointer<ResizeHandleItem>(new ResizeHandleItem(layerItem, *this));
    m_data->bottomItem->setZValue(300);
    m_data->bottomItem->setCursor(Qt::SizeVerCursor);

    updatePosition();
}

ResizeController::ResizeController(const ResizeController &other)
    : m_data(other.m_data)
{

}

ResizeController::ResizeController(const WeakResizeController &resizeController)
    : m_data(resizeController.m_data.toStrongRef())
{
}

ResizeController::~ResizeController()
{
}

ResizeController &ResizeController::operator =(const ResizeController &other)
{
    if (this != &other)
        m_data = other.m_data;
    return *this;
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
    return m_data->formEditorItem;
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

WeakResizeController ResizeController::toWeakResizeController() const
{
    return WeakResizeController(*this);
}

WeakResizeController::WeakResizeController()
{

}

WeakResizeController::WeakResizeController(const WeakResizeController &resizeController)
    : m_data(resizeController.m_data)
{
}

WeakResizeController::WeakResizeController(const ResizeController &resizeController)
    : m_data(resizeController.m_data.toWeakRef())
{
}

WeakResizeController::~WeakResizeController()
{
}

WeakResizeController &WeakResizeController::operator =(const WeakResizeController &other)
{
    if (m_data != other.m_data)
        m_data = other.m_data;

    return *this;
}

ResizeController WeakResizeController::toResizeController() const
{
    return ResizeController(*this);
}

}
