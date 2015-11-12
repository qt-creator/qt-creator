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
          m_isSecondarySelected(false)
    {
        setPen(QPen(Qt::black));
        setBrush(QBrush(Qt::black));
    }

    void setSecondarySelected(bool secondarySelected)
    {
        if (secondarySelected != m_isSecondarySelected) {
            m_isSecondarySelected = secondarySelected;
            if (secondarySelected) {
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
        if (!m_isSecondarySelected) {
            m_owner->moveHandle(m_handle, QPointF(0.0, 0.0), Press, None);
        }
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF pos = mapToScene(event->pos()) - m_startPos;
        if (!m_isSecondarySelected) {
            m_owner->moveHandle(m_handle, pos, Move, None);
        }

    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF pos = mapToScene(event->pos()) - m_startPos;
        if (!m_isSecondarySelected) {
            m_owner->moveHandle(m_handle, pos, Release, None);
        }
    }

private:

    RectangularSelectionItem *m_owner;

    Handle m_handle;

    bool m_isSecondarySelected;

    QPointF m_startPos;
};


RectangularSelectionItem::RectangularSelectionItem(IResizable *itemResizer, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_itemResizer(itemResizer),
      m_pointSize(QSizeF(9.0, 9.0)),
      m_points(8),
      m_showBorder(false),
      m_borderItem(0),
      m_freedom(FreedomAny),
      m_isSecondarySelected(false)
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

void RectangularSelectionItem::setShowBorder(bool showBorder)
{
    m_showBorder = showBorder;
}

void RectangularSelectionItem::setFreedom(Freedom freedom)
{
    m_freedom = freedom;
}

void RectangularSelectionItem::setSecondarySelected(bool secondarySelected)
{
    if (secondarySelected != m_isSecondarySelected) {
        m_isSecondarySelected = secondarySelected;
        update();
    }
}

void RectangularSelectionItem::update()
{
    QMT_CHECK(HandleFirst == 0 && HandleLast == 7);
    prepareGeometryChange();

    for (int i = 0; i <= 7; ++i) {
        if (!m_points[i]) {
            m_points[i] = new GraphicsHandleItem((Handle) i, this);
        }
        m_points[i]->setRect(-m_pointSize.width() / 2.0, -m_pointSize.height() / 2.0, m_pointSize.width(), m_pointSize.height());
        bool visible = false;
        switch ((Handle) i) {
        case HandleTopLeft:
            visible = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
            break;
        case HandleTop:
            visible = m_freedom == FreedomAny || m_freedom == FreedomVerticalOnly;
            break;
        case HandleTopRight:
            visible = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
            break;
        case HandleLeft:
            visible = m_freedom == FreedomAny || m_freedom == FreedomHorizontalOnly;
            break;
        case HandleRight:
            visible = m_freedom == FreedomAny || m_freedom == FreedomHorizontalOnly;
            break;
        case HandleBottomLeft:
            visible = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
            break;
        case HandleBottom:
            visible = m_freedom == FreedomAny || m_freedom == FreedomVerticalOnly;
            break;
        case HandleBottomRight:
            visible = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
            break;
        }
        m_points[i]->setSecondarySelected(m_isSecondarySelected);
        m_points[i]->setVisible(visible);
    }
    double horizCenter = (m_rect.left() + m_rect.right()) * 0.5;
    double vertCenter = (m_rect.top() + m_rect.bottom()) * 0.5;
    m_points[0]->setPos(pos() + m_rect.topLeft());
    m_points[1]->setPos(pos() + QPointF(horizCenter, m_rect.top()));
    m_points[2]->setPos(pos() + m_rect.topRight());
    m_points[3]->setPos(pos() + QPointF(m_rect.left(), vertCenter));
    m_points[4]->setPos(pos() + QPointF(m_rect.right(), vertCenter));
    m_points[5]->setPos(pos() + m_rect.bottomLeft());
    m_points[6]->setPos(pos() + QPointF(horizCenter, m_rect.bottom()));
    m_points[7]->setPos(pos() + m_rect.bottomRight());

    if (m_showBorder) {
        if (!m_borderItem) {
            m_borderItem = new QGraphicsRectItem(this);
        }
        m_borderItem->setRect(m_rect);
        if (m_isSecondarySelected) {
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

void RectangularSelectionItem::moveHandle(Handle handle, const QPointF &deltaMove, HandleStatus handleStatus, HandleQualifier handleQualifier)
{
    Q_UNUSED(handleQualifier);

    if (handleStatus == Press) {
        m_originalResizePos = m_itemResizer->pos();
        m_originalResizeRect = m_itemResizer->rect();
    }

    bool moveable = false;
    switch (handle) {
    case HandleTopLeft:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
        break;
    case HandleTop:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomVerticalOnly;
        break;
    case HandleTopRight:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
        break;
    case HandleLeft:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomHorizontalOnly;
        break;
    case HandleRight:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomHorizontalOnly;
        break;
    case HandleBottomLeft:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
        break;
    case HandleBottom:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomVerticalOnly;
        break;
    case HandleBottomRight:
        moveable = m_freedom == FreedomAny || m_freedom == FreedomKeepRatio;
        break;
    }
    if (!moveable) {
        return;
    }

    QSizeF minimumSize = m_itemResizer->minimumSize();

    QPointF topLeftDelta;
    QPointF bottomRightDelta;

    // calculate movements of corners
    if (m_freedom == FreedomKeepRatio) {
        qreal minimumLength = qSqrt(minimumSize.width() * minimumSize.width() + minimumSize.height() * minimumSize.height());
        QPointF v(minimumSize.width() / minimumLength, minimumSize.height() / minimumLength);
        qreal deltaLength = qSqrt(deltaMove.x() * deltaMove.x() + deltaMove.y() * deltaMove.y());
        switch (handle) {
        case HandleTopLeft:
            if (deltaMove.x() > 0 && deltaMove.y() > 0) {
                topLeftDelta = v * deltaLength;
            } else if (deltaMove.x() < 0 && deltaMove.y() < 0) {
                topLeftDelta = -(v * deltaLength);
            }
            break;
        case HandleTop:
            QMT_CHECK(false);
            break;
        case HandleTopRight:
            if (deltaMove.x() > 0 && deltaMove.y() < 0) {
                topLeftDelta = QPointF(0.0, -(v.x() * deltaLength));
                bottomRightDelta = QPointF(v.y() * deltaLength, 0.0);
            } else if (deltaMove.x() < 0 && deltaMove.y() > 0) {
                topLeftDelta = QPointF(0.0, v.x() * deltaLength);
                bottomRightDelta = QPointF(-(v.y() * deltaLength), 0.0);
            }
            break;
        case HandleLeft:
            QMT_CHECK(false);
            break;
        case HandleRight:
            QMT_CHECK(false);
            break;
        case HandleBottomLeft:
            if (deltaMove.x() < 0 && deltaMove.y() > 0) {
                topLeftDelta = QPointF(-(v.x() * deltaLength), 0.0);
                bottomRightDelta = QPointF(0.0, v.y() * deltaLength);
            } else if (deltaMove.x() > 0 && deltaMove.y() < 0) {
                topLeftDelta = QPointF(v.x() * deltaLength, 0.0);
                bottomRightDelta = QPointF(0.0, -(v.y() * deltaLength));
            }
            break;
        case HandleBottom:
            QMT_CHECK(false);
            break;
        case HandleBottomRight:
            if (deltaMove.x() > 0 && deltaMove.y() > 0) {
                bottomRightDelta = v * deltaLength;
            } else if (deltaMove.x() < 0 && deltaMove.y() < 0) {
                bottomRightDelta = -(v * deltaLength);
            }
            break;
        }
    } else {
        switch (handle) {
        case HandleTopLeft:
            topLeftDelta = deltaMove;
            break;
        case HandleTop:
            topLeftDelta = QPointF(0.0, deltaMove.y());
            break;
        case HandleTopRight:
            topLeftDelta = QPointF(0, deltaMove.y());
            bottomRightDelta = QPointF(deltaMove.x(), 0.0);
            break;
        case HandleLeft:
            topLeftDelta = QPointF(deltaMove.x(), 0.0);
            break;
        case HandleRight:
            bottomRightDelta = QPointF(deltaMove.x(), 0.0);
            break;
        case HandleBottomLeft:
            topLeftDelta = QPointF(deltaMove.x(), 0.0);
            bottomRightDelta = QPointF(0.0, deltaMove.y());
            break;
        case HandleBottom:
            bottomRightDelta = QPointF(0.0, deltaMove.y());
            break;
        case HandleBottomRight:
            bottomRightDelta = deltaMove;
            break;
        }
    }

    QRectF newRect = m_originalResizeRect.adjusted(topLeftDelta.x(), topLeftDelta.y(), bottomRightDelta.x(), bottomRightDelta.y());
    QSizeF sizeDelta = minimumSize - newRect.size();

    // correct horizontal resize against minimum width
    switch (handle) {
    case HandleTopLeft:
    case HandleLeft:
    case HandleBottomLeft:
        if (sizeDelta.width() > 0.0) {
            topLeftDelta.setX(topLeftDelta.x() - sizeDelta.width());
        }
        break;
    case HandleTop:
    case HandleBottom:
        break;
    case HandleTopRight:
    case HandleRight:
    case HandleBottomRight:
        if (sizeDelta.width() > 0.0) {
            bottomRightDelta.setX(bottomRightDelta.x() + sizeDelta.width());
        }
        break;
    }

    // correct vertical resize against minimum height
    switch (handle) {
    case HandleTopLeft:
    case HandleTop:
    case HandleTopRight:
        if (sizeDelta.height() > 0.0) {
            topLeftDelta.setY(topLeftDelta.y() - sizeDelta.height());
        }
        break;
    case HandleLeft:
    case HandleRight:
        break;
    case HandleBottomLeft:
    case HandleBottom:
    case HandleBottomRight:
        if (sizeDelta.height() > 0.0) {
            bottomRightDelta.setY(bottomRightDelta.y() + sizeDelta.height());
        }
        break;
    }

    m_itemResizer->setPosAndRect(m_originalResizePos, m_originalResizeRect, topLeftDelta, bottomRightDelta);

    if (handleStatus == Release) {
        IResizable::Side horiz = IResizable::SideNone;
        IResizable::Side vert = IResizable::SideNone;
        switch (handle) {
        case HandleTopLeft:
        case HandleLeft:
        case HandleBottomLeft:
            horiz = IResizable::SideLeftOrTop;
            break;
        case HandleTopRight:
        case HandleRight:
        case HandleBottomRight:
            horiz = IResizable::SideRightOrBottom;
            break;
        case HandleTop:
        case HandleBottom:
            // nothing
            break;
        }
        switch (handle) {
        case HandleTopLeft:
        case HandleTop:
        case HandleTopRight:
            vert = IResizable::SideLeftOrTop;
            break;
        case HandleBottomLeft:
        case HandleBottom:
        case HandleBottomRight:
            vert = IResizable::SideRightOrBottom;
            break;
        case HandleLeft:
        case HandleRight:
            // nothing
            break;
        }
        m_itemResizer->alignItemSizeToRaster(horiz, vert, 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
    }
}

}
