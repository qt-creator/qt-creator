// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveeditorstyle.h"
#include "curveeditorstyledialog.h"
#include "graphicsscene.h"
#include "playhead.h"
#include "selector.h"

#include <QGraphicsView>

namespace QmlDesigner {

class CurveItem;
class CurveEditorModel;
class Playhead;
class TreeItem;

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

    friend Playhead;

signals:
    void currentFrameChanged(int frame, bool notify);

    void zoomChanged(double x, double y);

public:
    GraphicsView(CurveEditorModel *model, QWidget *parent = nullptr);

    ~GraphicsView() override;

    CurveEditorModel *model() const;

    CurveEditorStyle editorStyle() const;

    bool dragging() const;

    int mapTimeToX(double time) const;

    int mapValueToY(double value) const;

    double mapXtoTime(int x) const;

    double mapYtoValue(int y) const;

    QPointF globalToScene(const QPoint &point) const;

    QPointF globalToRaster(const QPoint &point) const;

    double minimumTime() const;

    double maximumTime() const;

    double minimumValue() const;

    double maximumValue() const;

    double zoomX() const;

    double zoomY() const;

    QRectF canvasRect() const;

    QRectF timeScaleRect() const;

    QRectF valueScaleRect() const;

    QRectF defaultRasterRect() const;

    void setIsMcu(bool isMcu);

    void setLocked(TreeItem *item);

    void setPinned(TreeItem *item);

    void setStyle(const CurveEditorStyle &style);

    void setZoomX(double zoom, const QPoint &pivot = QPoint());

    void setZoomY(double zoom, const QPoint &pivot = QPoint());

    void setCurrentFrame(int frame, bool notify = true);

    void scrollContent(double x, double y);

    void reset(const std::vector<CurveItem *> &items);

    void updateSelection();

    void setInterpolation(Keyframe::Interpolation interpol);

    void toggleUnified();

protected:
    void resizeEvent(QResizeEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

    void drawForeground(QPainter *painter, const QRectF &rect) override;

    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    void applyZoom(double x, double y, const QPoint &pivot = QPoint());

    void drawGrid(QPainter *painter);

#if 0
    void drawExtremaX(QPainter *painter, const QRectF &rect);

    void drawExtremaY(QPainter *painter, const QRectF &rect);
#endif

    void drawTimeScale(QPainter *painter, const QRectF &rect);

    void drawValueScale(QPainter *painter, const QRectF &rect);

    void drawRangeBar(QPainter *painter, const QRectF &rect);

    double timeLabelInterval(QPainter *painter, double maxTime);

    QRectF rangeMinHandle(const QRectF &rect);

    QRectF rangeMaxHandle(const QRectF &rect);

    QPoint viewportCenter() const;

private:
    bool m_dragging;

    double m_zoomX;

    double m_zoomY;

    QTransform m_transform;

    GraphicsScene *m_scene;

    CurveEditorModel *m_model;

    Playhead m_playhead;

    Selector m_selector;

    CurveEditorStyle m_style;

    CurveEditorStyleDialog m_dialog;
};

} // End namespace QmlDesigner.
