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
#pragma once

#include "resizecontroller.h"

#include <qmldesignercorelib_global.h>

#include <QGraphicsItem>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ResizeHandleItem : public QGraphicsItem
{
public:
    enum
    {
        Type = 0xEAEA
    };

    ResizeHandleItem(QGraphicsItem *parent, const ResizeController &resizeController);
    ~ResizeHandleItem();
    void setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition);

    int type() const;
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

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
