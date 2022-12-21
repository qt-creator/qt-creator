// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsView>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QGraphicsView;
class QMouseEvent;
QT_END_NAMESPACE

namespace ScxmlEditor {

namespace PluginInterface { class GraphicsScene; }

namespace Common {

class GraphicsView;

class Magnifier : public QWidget
{
    Q_OBJECT

public:
    Magnifier(QWidget *parent = nullptr);

    void setCurrentView(GraphicsView *mainView);
    void setCurrentScene(PluginInterface::GraphicsScene *scene);
    void setTopLeft(const QPoint &topLeft);

signals:
    void clicked(double zoomLevel);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    QPoint m_topLeft;
    QPointer<GraphicsView> m_mainView;
    QRadialGradient m_gradientBrush;
    QGraphicsView *m_graphicsView;
};

} // namespace Common
} // namespace ScxmlEditor
