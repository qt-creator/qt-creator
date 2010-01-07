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

#ifndef RESIZEHANDLEITEM_H
#define RESIZEHANDLEITEM_H

#include <QGraphicsPixmapItem>

#include "resizecontroller.h"

namespace QmlDesigner {

class ResizeHandleItem : public QGraphicsPixmapItem
{
public:
    enum
    {
        Type = 0xEAEA
    };


    ResizeHandleItem(QGraphicsItem *parent, const ResizeController &resizeController);

    void setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition);

    int type() const;
    QRectF boundingRect() const;
    QPainterPath shape() const;

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
    QWeakPointer<ResizeControllerData> m_resizeControllerData;
    QPointF m_itemSpacePosition;
};

inline int ResizeHandleItem::type() const
{
    return Type;
}

}

#endif // RESIZEHANDLEITEM_H
