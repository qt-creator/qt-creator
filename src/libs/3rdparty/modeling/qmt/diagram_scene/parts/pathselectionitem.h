/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_GRAPHICSPATHSELECTIONITEM_H
#define QMT_GRAPHICSPATHSELECTIONITEM_H

#include <QGraphicsItem>


namespace qmt {

class IWindable;

class PathSelectionItem :
        public QGraphicsItem
{
    class GraphicsHandleItem;

    friend class GraphicsHandleItem;

    enum HandleStatus {
        PRESS,
        MOVE,
        RELEASE,
        CANCEL
    };

    enum HandleQualifier {
        NONE,
        DELETE_HANDLE
    };

public:

    PathSelectionItem(IWindable *windable, QGraphicsItem *parent = 0);

    ~PathSelectionItem();

public:

    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QPainterPath shape() const;

public:

    QSizeF getPointSize() const { return _point_size; }

    void setPointSize(const QSizeF &size);

    QList<QPointF> getPoints() const;

    void setPoints(const QList<QPointF> &points);

    void setSecondarySelected(bool secondary_selected);

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:

    void update();

    void moveHandle(int point_index, const QPointF &delta_move, HandleStatus handle_status, HandleQualifier handle_qualifier);

private:

    IWindable *_windable;

    QSizeF _point_size;

    bool _secondary_selected;

    QList<GraphicsHandleItem *> _handles;

    QPointF _original_handle_pos;
};

}

#endif // QMT_GRAPHICSPATHSELECTIONITEM_H
