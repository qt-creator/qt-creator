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

#include "textitem.h"
#include <QGraphicsObject>

namespace ScxmlEditor {

namespace PluginInterface {

/**
 * @brief The TagTextItem class provides the movable and editable text-item.
 */
class TagTextItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit TagTextItem(QGraphicsItem *parent = nullptr);

    int type() const override
    {
        return TagTextType;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF boundingRect() const override;

    void setText(const QString &text);
    void setDefaultTextColor(const QColor &col);

    void resetMovePoint(const QPointF &p = QPointF(0, 0));
    QPointF movePoint() const;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *e) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;

signals:
    void textChanged();
    void textReady(const QString &text);
    void selected(bool sel);
    void movePointChanged();

private:
    bool needIgnore(const QPointF sPos);

    QPointF m_movePoint;
    QPointF m_startPos;
    TextItem *m_textItem;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
