// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
