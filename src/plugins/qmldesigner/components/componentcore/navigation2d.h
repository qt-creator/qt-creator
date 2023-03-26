// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QGestureEvent)
QT_FORWARD_DECLARE_CLASS(QScrollBar)
QT_FORWARD_DECLARE_CLASS(QWheelEvent)

namespace QmlDesigner {

class Navigation2dFilter : public QObject
{
    Q_OBJECT

signals:
    void zoomChanged(double scale, const QPointF &pos);
    void panChanged(const QPointF &direction);

    void zoomIn();
    void zoomOut();

public:
    static void scroll(const QPointF &direction, QScrollBar *sbx, QScrollBar *sby);

    Navigation2dFilter(QWidget *parent);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    bool gestureEvent(QGestureEvent *event);
    bool wheelEvent(QWheelEvent *event);
};

} // namespace QmlDesigner
