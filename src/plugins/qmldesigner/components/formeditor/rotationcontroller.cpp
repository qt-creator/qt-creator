// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rotationcontroller.h"

#include "formeditoritem.h"
#include "layeritem.h"

#include <rotationhandleitem.h>
#include <QCursor>
#include <QGraphicsScene>

#include <utils/stylehelper.h>
#include <theme.h>

namespace QmlDesigner {

class RotationControllerData
{
public:
    RotationControllerData(LayerItem *layerItem,
                         FormEditorItem *formEditorItem);
    RotationControllerData(const RotationControllerData &other);
    ~RotationControllerData();


    QPointer<LayerItem> layerItem;
    FormEditorItem *formEditorItem = nullptr;
    QSharedPointer<RotationHandleItem> topLeftItem;
    QSharedPointer<RotationHandleItem> topRightItem;
    QSharedPointer<RotationHandleItem> bottomLeftItem;
    QSharedPointer<RotationHandleItem> bottomRightItem;
};

RotationControllerData::RotationControllerData(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : layerItem(layerItem)
    , formEditorItem(formEditorItem)
{

}

RotationControllerData::RotationControllerData(const RotationControllerData &other) = default;

RotationControllerData::~RotationControllerData()
{
    if (layerItem) {
        QGraphicsScene *scene = layerItem->scene();
        scene->removeItem(topLeftItem.data());
        scene->removeItem(topRightItem.data());
        scene->removeItem(bottomLeftItem.data());
        scene->removeItem(bottomRightItem.data());
    }
}


RotationController::RotationController()
   : m_data(new RotationControllerData(nullptr, nullptr))
{

}

RotationController::RotationController(const QSharedPointer<RotationControllerData> &data)
    : m_data(data)
{

}

RotationController::RotationController(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : m_data(new RotationControllerData(layerItem, formEditorItem))
{
    QCursor rotationCursor = getRotationCursor();

    m_data->topLeftItem = QSharedPointer<RotationHandleItem>(new RotationHandleItem(layerItem, *this));
    m_data->topLeftItem->setZValue(302);
    m_data->topLeftItem->setCursor(rotationCursor);

    m_data->topRightItem = QSharedPointer<RotationHandleItem>(new RotationHandleItem(layerItem, *this));
    m_data->topRightItem->setZValue(301);
    m_data->topRightItem->setCursor(rotationCursor);

    m_data->bottomLeftItem = QSharedPointer<RotationHandleItem>(new RotationHandleItem(layerItem, *this));
    m_data->bottomLeftItem->setZValue(301);
    m_data->bottomLeftItem->setCursor(rotationCursor);

    m_data->bottomRightItem = QSharedPointer<RotationHandleItem>(new RotationHandleItem(layerItem, *this));
    m_data->bottomRightItem->setZValue(305);
    m_data->bottomRightItem->setCursor(rotationCursor);

    updatePosition();
}

RotationController::RotationController(const RotationController &other) = default;

RotationController::RotationController(const WeakRotationController &rotationController)
    : m_data(rotationController.m_data.toStrongRef())
{
}

RotationController::~RotationController() = default;

RotationController &RotationController::operator =(const RotationController &other)
{
    if (this != &other)
        m_data = other.m_data;
    return *this;
}


bool RotationController::isValid() const
{
    return m_data->formEditorItem && m_data->formEditorItem->qmlItemNode().isValid();
}

void RotationController::show()
{
    m_data->topLeftItem->show();
    m_data->topRightItem->show();
    m_data->bottomLeftItem->show();
    m_data->bottomRightItem->show();
}
void RotationController::hide()
{
    m_data->topLeftItem->hide();
    m_data->topRightItem->hide();
    m_data->bottomLeftItem->hide();
    m_data->bottomRightItem->hide();
}


void RotationController::updatePosition()
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

        const qreal rotation = m_data->formEditorItem->qmlItemNode().rotation();

        m_data->topRightItem->setHandlePosition(topRightPointInLayerSpace, boundingRect.topRight(), rotation);
        m_data->topLeftItem->setHandlePosition(topLeftPointInLayerSpace, boundingRect.topLeft(), rotation);
        m_data->bottomLeftItem->setHandlePosition(bottomLeftPointInLayerSpace, boundingRect.bottomLeft(), rotation);
        m_data->bottomRightItem->setHandlePosition(bottomRightPointInLayerSpace, boundingRect.bottomRight(), rotation);
    }
}


FormEditorItem* RotationController::formEditorItem() const
{
    return m_data->formEditorItem;
}

bool RotationController::isTopLeftHandle(const RotationHandleItem *handle) const
{
    return handle == m_data->topLeftItem;
}

bool RotationController::isTopRightHandle(const RotationHandleItem *handle) const
{
    return handle == m_data->topRightItem;
}

bool RotationController::isBottomLeftHandle(const RotationHandleItem *handle) const
{
    return handle == m_data->bottomLeftItem;
}

bool RotationController::isBottomRightHandle(const RotationHandleItem *handle) const
{
    return handle == m_data->bottomRightItem;
}

QCursor RotationController::getRotationCursor() const
{
    const QString fontName = "qtds_propertyIconFont.ttf";
    const int cursorSize = 32; //32 is cursor recommended size

    QIcon rotationIcon = Utils::StyleHelper::getCursorFromIconFont(
                fontName,
                Theme::getIconUnicode(Theme::rotationFill),
                Theme::getIconUnicode(Theme::rotationOutline),
                cursorSize, cursorSize);

    return QCursor(rotationIcon.pixmap(cursorSize, cursorSize));
}

WeakRotationController RotationController::toWeakRotationController() const
{
    return WeakRotationController(*this);
}

WeakRotationController::WeakRotationController() = default;

WeakRotationController::WeakRotationController(const WeakRotationController &rotationController) = default;

WeakRotationController::WeakRotationController(const RotationController &rotationController)
    : m_data(rotationController.m_data.toWeakRef())
{
}

WeakRotationController::~WeakRotationController() = default;

WeakRotationController &WeakRotationController::operator =(const WeakRotationController &other)
{
    if (m_data != other.m_data)
        m_data = other.m_data;

    return *this;
}

RotationController WeakRotationController::toRotationController() const
{
    return RotationController(*this);
}

}
