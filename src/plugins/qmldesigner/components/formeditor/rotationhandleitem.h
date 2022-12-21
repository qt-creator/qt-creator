// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "rotationcontroller.h"

#include <qmldesignercomponents_global.h>

#include <QGraphicsItem>

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT RotationHandleItem : public QGraphicsItem
{
public:
    enum
    {
        Type = 0xEBEB
    };

    RotationHandleItem(QGraphicsItem *parent, const RotationController &rotationController);
    ~RotationHandleItem() override;
    void setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition, const qreal rotation);

    int type() const override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    RotationController rotationController() const;

    static RotationHandleItem* fromGraphicsItem(QGraphicsItem *item);

    bool isTopLeftHandle() const;
    bool isTopRightHandle() const;
    bool isBottomLeftHandle() const;
    bool isBottomRightHandle() const;

    QPointF itemSpacePosition() const;

private:
    WeakRotationController m_weakRotationController;
    QPointF m_itemSpacePosition;
};

inline int RotationHandleItem::type() const
{
    return Type;
}

} // namespace QmlDesigner
