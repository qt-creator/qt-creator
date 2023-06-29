// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QGraphicsView>

namespace QmlDesigner {

class FormEditorGraphicsView : public QGraphicsView
{
    Q_OBJECT

signals:
    void zoomChanged(double zoom);
    void zoomIn();
    void zoomOut();

public:
    explicit FormEditorGraphicsView(QWidget *parent = nullptr);

    void setRootItemRect(const QRectF &rect);
    QRectF rootItemRect() const;

    void activateCheckboardBackground();
    void activateColoredBackground(const QColor &color);
    void drawBackground(QPainter *painter, const QRectF &rect) override;

    void setZoomFactor(double zoom);
    void frame(const QRectF &bbox);

    void setBackgoundImage(const QImage &image);
    QImage backgroundImage() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    enum Panning { NotStarted, MouseWheelStarted, SpaceKeyStarted };

    void startPanning(QEvent *event);
    void stopPanning(QEvent *event);

    Panning m_isPanning = Panning::NotStarted;
    QPoint m_panningStartPosition;
    QRectF m_rootItemRect;
    QImage m_backgroundImage;
};

} // namespace QmlDesigner
