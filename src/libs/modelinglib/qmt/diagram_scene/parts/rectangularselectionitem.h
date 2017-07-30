/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <QGraphicsItem>
#include <QVector>

QT_BEGIN_NAMESPACE
class QGraphicsRectItem;
QT_END_NAMESPACE

namespace qmt {

class IResizable;

class RectangularSelectionItem : public QGraphicsItem
{
    class GraphicsHandleItem;
    friend class GraphicsHandleItem;

    enum HandleStatus {
        Press,
        Move,
        Release,
        Cancel
    };

    enum HandleQualifier {
        None,
        KeepPos
    };

public:
    enum Handle {
        HandleNone = -1,
        HandleFirst = 0,
        HandleTopLeft = HandleFirst,
        HandleTop,
        HandleTopRight,
        HandleLeft,
        HandleRight,
        HandleBottomLeft,
        HandleBottom,
        HandleBottomRight,
        HandleLast = HandleBottomRight
    };

    enum Freedom {
        FreedomAny,
        FreedomVerticalOnly,
        FreedomHorizontalOnly,
        FreedomKeepRatio
    };

    explicit RectangularSelectionItem(IResizable *itemResizer, QGraphicsItem *parent = nullptr);
    ~RectangularSelectionItem() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF rect() const { return m_rect; }
    void setRect(const QRectF &rectangle);
    void setRect(qreal x, qreal y, qreal width, qreal height);
    QSizeF pointSize() const { return m_pointSize; }
    void setPointSize(const QSizeF &size);
    bool showBorder() const { return m_showBorder; }
    void setShowBorder(bool showBorder);
    Freedom freedom() const { return m_freedom; }
    void setFreedom(Freedom freedom);
    bool isSecondarySelected() const { return m_isSecondarySelected; }
    void setSecondarySelected(bool secondarySelected);
    Handle activeHandle() const { return m_activeHandle; }

private:
    void update();
    void moveHandle(Handle handle, const QPointF &deltaMove, HandleStatus handleStatus,
                    HandleQualifier handleQualifier);

    IResizable *m_itemResizer = nullptr;
    QRectF m_rect;
    QSizeF m_pointSize;
    QVector<GraphicsHandleItem *> m_points;
    QPointF m_originalResizePos;
    QRectF m_originalResizeRect;
    bool m_showBorder = false;
    QGraphicsRectItem *m_borderItem = nullptr;
    Freedom m_freedom = FreedomAny;
    bool m_isSecondarySelected = false;
    Handle m_activeHandle = HandleNone;
};

} // namespace qmt
