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

#include "rectangularselectionitem.h"

#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QBrush>
#include <QPen>
#include <qmath.h>


namespace qmt {

class RectangularSelectionItem::GraphicsHandleItem :
        public QGraphicsRectItem
{
public:

    GraphicsHandleItem(Handle handle, RectangularSelectionItem *parent)
        : QGraphicsRectItem(parent),
          m_owner(parent),
          m_handle(handle),
          m_secondarySelected(false)
    {
        setPen(QPen(Qt::black));
        setBrush(QBrush(Qt::black));
    }

    void setSecondarySelected(bool secondary_selected)
    {
        if (secondary_selected != m_secondarySelected) {
            m_secondarySelected = secondary_selected;
            if (secondary_selected) {
                setPen(QPen(Qt::lightGray));
                setBrush(Qt::NoBrush);

            } else {
                setPen(QPen(Qt::black));
                setBrush(QBrush(Qt::black));
            }
        }
    }

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        m_startPos = mapToScene(event->pos());
        if (!m_secondarySelected) {
            m_owner->moveHandle(m_handle, QPointF(0.0, 0.0), PRESS, NONE);
        }
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF pos = mapToScene(event->pos()) - m_startPos;
        if (!m_secondarySelected) {
            m_owner->moveHandle(m_handle, pos, MOVE, NONE);
        }

    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF pos = mapToScene(event->pos()) - m_startPos;
        if (!m_secondarySelected) {
            m_owner->moveHandle(m_handle, pos, RELEASE, NONE);
        }
    }

private:

    RectangularSelectionItem *m_owner;

    Handle m_handle;

    bool m_secondarySelected;

    QPointF m_startPos;
};


RectangularSelectionItem::RectangularSelectionItem(IResizable *item_resizer, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_itemResizer(item_resizer),
      m_pointSize(QSizeF(9.0, 9.0)),
      m_points(8),
      m_showBorder(false),
      m_borderItem(0),
      m_freedom(FREEDOM_ANY),
      m_secondarySelected(false)
{
}

RectangularSelectionItem::~RectangularSelectionItem()
{
}

QRectF RectangularSelectionItem::boundingRect() const
{
    return childrenBoundingRect();
}

void RectangularSelectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void RectangularSelectionItem::setRect(const QRectF &rectangle)
{
    if (rectangle != m_rect) {
        m_rect = rectangle;
        update();
    }
}

void RectangularSelectionItem::setRect(qreal x, qreal y, qreal width, qreal height)
{
    setRect(QRectF(x, y, width, height));
}

void RectangularSelectionItem::setPointSize(const QSizeF &size)
{
    if (size != m_pointSize) {
        m_pointSize = size;
        update();
    }
}

void RectangularSelectionItem::setShowBorder(bool show_border)
{
    m_showBorder = show_border;
}

void RectangularSelectionItem::setFreedom(Freedom freedom)
{
    m_freedom = freedom;
}

void RectangularSelectionItem::setSecondarySelected(bool secondary_selected)
{
    if (secondary_selected != m_secondarySelected) {
        m_secondarySelected = secondary_selected;
        update();
    }
}

void RectangularSelectionItem::update()
{
    QMT_CHECK(HANDLE_FIRST == 0 && HANDLE_LAST == 7);
    prepareGeometryChange();

    for (int i = 0; i <= 7; ++i) {
        if (!m_points[i]) {
            m_points[i] = new GraphicsHandleItem((Handle) i, this);
        }
        m_points[i]->setRect(-m_pointSize.width() / 2.0, -m_pointSize.height() / 2.0, m_pointSize.width(), m_pointSize.height());
        bool visible = false;
        switch ((Handle) i) {
        case HANDLE_TOP_LEFT:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
            break;
        case HANDLE_TOP:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_VERTICAL_ONLY;
            break;
        case HANDLE_TOP_RIGHT:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
            break;
        case HANDLE_LEFT:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_HORIZONTAL_ONLY;
            break;
        case HANDLE_RIGHT:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_HORIZONTAL_ONLY;
            break;
        case HANDLE_BOTTOM_LEFT:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
            break;
        case HANDLE_BOTTOM:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_VERTICAL_ONLY;
            break;
        case HANDLE_BOTTOM_RIGHT:
            visible = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
            break;
        }
        m_points[i]->setSecondarySelected(m_secondarySelected);
        m_points[i]->setVisible(visible);
    }
    double horiz_center = (m_rect.left() + m_rect.right()) * 0.5;
    double vert_center = (m_rect.top() + m_rect.bottom()) * 0.5;
    m_points[0]->setPos(pos() + m_rect.topLeft());
    m_points[1]->setPos(pos() + QPointF(horiz_center, m_rect.top()));
    m_points[2]->setPos(pos() + m_rect.topRight());
    m_points[3]->setPos(pos() + QPointF(m_rect.left(), vert_center));
    m_points[4]->setPos(pos() + QPointF(m_rect.right(), vert_center));
    m_points[5]->setPos(pos() + m_rect.bottomLeft());
    m_points[6]->setPos(pos() + QPointF(horiz_center, m_rect.bottom()));
    m_points[7]->setPos(pos() + m_rect.bottomRight());

    if (m_showBorder) {
        if (!m_borderItem) {
            m_borderItem = new QGraphicsRectItem(this);
        }
        m_borderItem->setRect(m_rect);
        if (m_secondarySelected) {
            m_borderItem->setPen(QPen(QBrush(Qt::lightGray), 0.0, Qt::DashDotLine));
        } else {
            m_borderItem->setPen(QPen(QBrush(Qt::black), 0.0, Qt::DashDotLine));
        }
    } else if (m_borderItem) {
        if (m_borderItem->scene()) {
            m_borderItem->scene()->removeItem(m_borderItem);
        }
        delete m_borderItem;
        m_borderItem = 0;
    }
}

void RectangularSelectionItem::moveHandle(Handle handle, const QPointF &delta_move, HandleStatus handle_status, HandleQualifier handle_qualifier)
{
    Q_UNUSED(handle_qualifier);

    if (handle_status == PRESS) {
        m_originalResizePos = m_itemResizer->getPos();
        m_originalResizeRect = m_itemResizer->getRect();
    }

    bool moveable = false;
    switch (handle) {
    case HANDLE_TOP_LEFT:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
        break;
    case HANDLE_TOP:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_VERTICAL_ONLY;
        break;
    case HANDLE_TOP_RIGHT:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
        break;
    case HANDLE_LEFT:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_HORIZONTAL_ONLY;
        break;
    case HANDLE_RIGHT:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_HORIZONTAL_ONLY;
        break;
    case HANDLE_BOTTOM_LEFT:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
        break;
    case HANDLE_BOTTOM:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_VERTICAL_ONLY;
        break;
    case HANDLE_BOTTOM_RIGHT:
        moveable = m_freedom == FREEDOM_ANY || m_freedom == FREEDOM_KEEP_RATIO;
        break;
    }
    if (!moveable) {
        return;
    }

    QSizeF minimum_size = m_itemResizer->getMinimumSize();

    QPointF top_left_delta;
    QPointF bottom_right_delta;

    // calculate movements of corners
    if (m_freedom == FREEDOM_KEEP_RATIO) {
        qreal minimum_length = qSqrt(minimum_size.width() * minimum_size.width() + minimum_size.height() * minimum_size.height());
        QPointF v(minimum_size.width() / minimum_length, minimum_size.height() / minimum_length);
        qreal delta_length = qSqrt(delta_move.x() * delta_move.x() + delta_move.y() * delta_move.y());
        switch (handle) {
        case HANDLE_TOP_LEFT:
            if (delta_move.x() > 0 && delta_move.y() > 0) {
                top_left_delta = v * delta_length;
            } else if (delta_move.x() < 0 && delta_move.y() < 0) {
                top_left_delta = -(v * delta_length);
            }
            break;
        case HANDLE_TOP:
            QMT_CHECK(false);
            break;
        case HANDLE_TOP_RIGHT:
            if (delta_move.x() > 0 && delta_move.y() < 0) {
                top_left_delta = QPointF(0.0, -(v.x() * delta_length));
                bottom_right_delta = QPointF(v.y() * delta_length, 0.0);
            } else if (delta_move.x() < 0 && delta_move.y() > 0) {
                top_left_delta = QPointF(0.0, v.x() * delta_length);
                bottom_right_delta = QPointF(-(v.y() * delta_length), 0.0);
            }
            break;
        case HANDLE_LEFT:
            QMT_CHECK(false);
            break;
        case HANDLE_RIGHT:
            QMT_CHECK(false);
            break;
        case HANDLE_BOTTOM_LEFT:
            if (delta_move.x() < 0 && delta_move.y() > 0) {
                top_left_delta = QPointF(-(v.x() * delta_length), 0.0);
                bottom_right_delta = QPointF(0.0, v.y() * delta_length);
            } else if (delta_move.x() > 0 && delta_move.y() < 0) {
                top_left_delta = QPointF(v.x() * delta_length, 0.0);
                bottom_right_delta = QPointF(0.0, -(v.y() * delta_length));
            }
            break;
        case HANDLE_BOTTOM:
            QMT_CHECK(false);
            break;
        case HANDLE_BOTTOM_RIGHT:
            if (delta_move.x() > 0 && delta_move.y() > 0) {
                bottom_right_delta = v * delta_length;
            } else if (delta_move.x() < 0 && delta_move.y() < 0) {
                bottom_right_delta = -(v * delta_length);
            }
            break;
        }
    } else {
        switch (handle) {
        case HANDLE_TOP_LEFT:
            top_left_delta = delta_move;
            break;
        case HANDLE_TOP:
            top_left_delta = QPointF(0.0, delta_move.y());
            break;
        case HANDLE_TOP_RIGHT:
            top_left_delta = QPointF(0, delta_move.y());
            bottom_right_delta = QPointF(delta_move.x(), 0.0);
            break;
        case HANDLE_LEFT:
            top_left_delta = QPointF(delta_move.x(), 0.0);
            break;
        case HANDLE_RIGHT:
            bottom_right_delta = QPointF(delta_move.x(), 0.0);
            break;
        case HANDLE_BOTTOM_LEFT:
            top_left_delta = QPointF(delta_move.x(), 0.0);
            bottom_right_delta = QPointF(0.0, delta_move.y());
            break;
        case HANDLE_BOTTOM:
            bottom_right_delta = QPointF(0.0, delta_move.y());
            break;
        case HANDLE_BOTTOM_RIGHT:
            bottom_right_delta = delta_move;
            break;
        }
    }

    QRectF new_rect = m_originalResizeRect.adjusted(top_left_delta.x(), top_left_delta.y(), bottom_right_delta.x(), bottom_right_delta.y());
    QSizeF size_delta = minimum_size - new_rect.size();

    // correct horizontal resize against minimum width
    switch (handle) {
    case HANDLE_TOP_LEFT:
    case HANDLE_LEFT:
    case HANDLE_BOTTOM_LEFT:
        if (size_delta.width() > 0.0) {
            top_left_delta.setX(top_left_delta.x() - size_delta.width());
        }
        break;
    case HANDLE_TOP:
    case HANDLE_BOTTOM:
        break;
    case HANDLE_TOP_RIGHT:
    case HANDLE_RIGHT:
    case HANDLE_BOTTOM_RIGHT:
        if (size_delta.width() > 0.0) {
            bottom_right_delta.setX(bottom_right_delta.x() + size_delta.width());
        }
        break;
    }

    // correct vertical resize against minimum height
    switch (handle) {
    case HANDLE_TOP_LEFT:
    case HANDLE_TOP:
    case HANDLE_TOP_RIGHT:
        if (size_delta.height() > 0.0) {
            top_left_delta.setY(top_left_delta.y() - size_delta.height());
        }
        break;
    case HANDLE_LEFT:
    case HANDLE_RIGHT:
        break;
    case HANDLE_BOTTOM_LEFT:
    case HANDLE_BOTTOM:
    case HANDLE_BOTTOM_RIGHT:
        if (size_delta.height() > 0.0) {
            bottom_right_delta.setY(bottom_right_delta.y() + size_delta.height());
        }
        break;
    }

    m_itemResizer->setPosAndRect(m_originalResizePos, m_originalResizeRect, top_left_delta, bottom_right_delta);

    if (handle_status == RELEASE) {
        IResizable::Side horiz = IResizable::SIDE_NONE;
        IResizable::Side vert = IResizable::SIDE_NONE;
        switch (handle) {
        case HANDLE_TOP_LEFT:
        case HANDLE_LEFT:
        case HANDLE_BOTTOM_LEFT:
            horiz = IResizable::SIDE_LEFT_OR_TOP;
            break;
        case HANDLE_TOP_RIGHT:
        case HANDLE_RIGHT:
        case HANDLE_BOTTOM_RIGHT:
            horiz = IResizable::SIDE_RIGHT_OR_BOTTOM;
            break;
        case HANDLE_TOP:
        case HANDLE_BOTTOM:
            // nothing
            break;
        }
        switch (handle) {
        case HANDLE_TOP_LEFT:
        case HANDLE_TOP:
        case HANDLE_TOP_RIGHT:
            vert = IResizable::SIDE_LEFT_OR_TOP;
            break;
        case HANDLE_BOTTOM_LEFT:
        case HANDLE_BOTTOM:
        case HANDLE_BOTTOM_RIGHT:
            vert = IResizable::SIDE_RIGHT_OR_BOTTOM;
            break;
        case HANDLE_LEFT:
        case HANDLE_RIGHT:
            // nothing
            break;
        }
        m_itemResizer->alignItemSizeToRaster(horiz, vert, 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
    }
}

}
