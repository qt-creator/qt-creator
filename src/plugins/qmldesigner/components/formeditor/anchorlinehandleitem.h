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

#ifndef ANCHORLINEHANDLEITEM_H
#define ANCHORLINEHANDLEITEM_H

#include <QGraphicsPathItem>

#include "anchorlinecontroller.h"

namespace QmlDesigner {

class AnchorLineHandleItem : public QGraphicsPathItem
{
public:
    enum
    {
        Type = 0xEAEB
    };


    AnchorLineHandleItem(QGraphicsItem *parent, const AnchorLineController &AnchorLineController);

    void setHandlePath(const QPainterPath & path);

    int type() const;
    QRectF boundingRect() const;
    QPainterPath shape() const;

    AnchorLineController anchorLineController() const;
    AnchorLine::Type anchorLine() const;


    static AnchorLineHandleItem* fromGraphicsItem(QGraphicsItem *item);


    bool isTopHandle() const;
    bool isLeftHandle() const;
    bool isRightHandle() const;
    bool isBottomHandle() const;

    QPointF itemSpacePosition() const;

    AnchorLine::Type anchorLineType() const;

    void setHiglighted(bool highlight);


private:
    QWeakPointer<AnchorLineControllerData> m_anchorLineControllerData;
};

inline int AnchorLineHandleItem::type() const
{
    return Type;
}

} // namespace QmlDesigner

#endif // ANCHORLINEHANDLEITEM_H
