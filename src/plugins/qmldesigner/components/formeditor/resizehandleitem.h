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
