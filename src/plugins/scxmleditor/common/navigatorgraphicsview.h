// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsView>

namespace ScxmlEditor {

namespace PluginInterface { class GraphicsScene; }

namespace Common {

class NavigatorGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit NavigatorGraphicsView(QWidget *parent = nullptr);

    void setGraphicsScene(PluginInterface::GraphicsScene *scene);
    void updateView();
    void setMainViewPolygon(const QPolygonF &pol);

signals:
    void moveMainViewTo(const QPointF &point);
    void zoomIn();
    void zoomOut();

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPolygonF m_mainViewPolygon;
    bool m_mouseDown = false;
    double m_minZoomValue = 1.0;
};

} // namespace Common
} // namespace ScxmlEditor
