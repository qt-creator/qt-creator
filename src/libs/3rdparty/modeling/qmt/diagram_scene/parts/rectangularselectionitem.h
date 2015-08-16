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

#ifndef QMT_RECTENGULARSELECTIONITEM_H
#define QMT_RECTENGULARSELECTIONITEM_H

#include <QGraphicsItem>
#include <QVector>

QT_BEGIN_NAMESPACE
class QGraphicsRectItem;
QT_END_NAMESPACE


namespace qmt {

class IResizable;

class RectangularSelectionItem :
        public QGraphicsItem
{
    class GraphicsHandleItem;

    friend class GraphicsHandleItem;

    enum Handle {
        HANDLE_FIRST = 0,
        HANDLE_TOP_LEFT = HANDLE_FIRST,
        HANDLE_TOP,
        HANDLE_TOP_RIGHT,
        HANDLE_LEFT,
        HANDLE_RIGHT,
        HANDLE_BOTTOM_LEFT,
        HANDLE_BOTTOM,
        HANDLE_BOTTOM_RIGHT,
        HANDLE_LAST = HANDLE_BOTTOM_RIGHT
    };

    enum HandleStatus {
        PRESS,
        MOVE,
        RELEASE,
        CANCEL
    };

    enum HandleQualifier {
        NONE,
        KEEP_POS
    };

public:

    enum Freedom {
        FREEDOM_ANY,
        FREEDOM_VERTICAL_ONLY,
        FREEDOM_HORIZONTAL_ONLY,
        FREEDOM_KEEP_RATIO
    };

public:

    RectangularSelectionItem(IResizable *item_resizer, QGraphicsItem *parent = 0);

    ~RectangularSelectionItem();

public:

    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public:

    QRectF getRect() const { return _rect; }

    void setRect(const QRectF &rectangle);

    void setRect(qreal x, qreal y, qreal width, qreal height);

    QSizeF getPointSize() const { return _point_size; }

    void setPointSize(const QSizeF &size);

    bool getShowBorder() const { return _show_border; }

    void setShowBorder(bool show_border);

    Freedom getFreedom() const { return _freedom; }

    void setFreedom(Freedom freedom);

    bool isSecondarySelected() const { return _secondary_selected; }

    void setSecondarySelected(bool secondary_selected);

private:

    void update();

    void moveHandle(Handle handle, const QPointF &delta_move, HandleStatus handle_status, HandleQualifier handle_qualifier);

private:

    IResizable *_item_resizer;

    QRectF _rect;

    QSizeF _point_size;

    QVector<GraphicsHandleItem *> _points;

    QPointF _original_resize_pos;

    QRectF _original_resize_rect;

    bool _show_border;

    QGraphicsRectItem *_border_item;

    Freedom _freedom;

    bool _secondary_selected;
};

}

#endif // QMT_RECTENGULARSELECTIONITEM_H
