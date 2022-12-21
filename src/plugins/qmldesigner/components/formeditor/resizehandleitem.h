// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "resizecontroller.h"

#include <qmldesignercomponents_global.h>

#include <QGraphicsItem>

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT ResizeHandleItem : public QGraphicsItem
{
public:
    enum
    {
        Type = 0xEAEA
    };

    ResizeHandleItem(QGraphicsItem *parent, const ResizeController &resizeController);
    ~ResizeHandleItem() override;
    void setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition);

    int type() const override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    ResizeController resizeController() const;

    static ResizeHandleItem* fromGraphicsItem(QGraphicsItem *item);

    bool isTopLeftHandle() const;
    bool isTopRightHandle() const;
    bool isBottomLeftHandle() const;
    bool isBottomRightHandle() const;

    bool isTopHandle() const;
    bool isLeftHandle() const;
    bool isRightHandle() const;
    bool isBottomHandle() const;

    QPointF itemSpacePosition() const;

private:
    WeakResizeController m_weakResizeController;
    QPointF m_itemSpacePosition;
};

inline int ResizeHandleItem::type() const
{
    return Type;
}

} // namespace QmlDesigner
