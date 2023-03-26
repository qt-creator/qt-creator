// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

QT_FORWARD_DECLARE_CLASS(QPainterPath)

namespace qmt {

class IWindable;

class PathSelectionItem : public QGraphicsItem
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
        DeleteHandle
    };

public:
    explicit PathSelectionItem(IWindable *windable, QGraphicsItem *parent = nullptr);
    ~PathSelectionItem() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;

    QSizeF pointSize() const { return m_pointSize; }
    void setPointSize(const QSizeF &size);
    QList<QPointF> points() const;
    void setPoints(const QList<QPointF> &points);
    void setSecondarySelected(bool secondarySelected);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    bool isEndHandle(int pointIndex) const;
    void update();
    void moveHandle(int pointIndex, const QPointF &deltaMove, HandleStatus handleStatus,
                    HandleQualifier handleQualifier);
    void keyPressed(int pointIndex, QKeyEvent *event, const QPointF &pos);

    IWindable *m_windable = nullptr;
    QSizeF m_pointSize;
    bool m_isSecondarySelected = false;
    QList<GraphicsHandleItem *> m_handles;
    GraphicsHandleItem *m_focusHandleItem = nullptr;
    QPointF m_originalHandlePos;
};

} // namespace qmt
