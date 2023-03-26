// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsWidget>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace QmlDesigner {

class TimelineToolButton : public QGraphicsWidget
{
    Q_OBJECT

public:
    explicit TimelineToolButton(QAction *action, QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;

    bool isCheckable() const;
    bool isChecked() const;
    bool isDisabled() const;
    void setChecked(bool b);
    void setDisabled(bool b);

signals:
    void clicked();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    enum State { Pressed, Hovered, Normal };

    State m_state = Normal;
    QAction *m_action;
};

} // namespace QmlDesigner
