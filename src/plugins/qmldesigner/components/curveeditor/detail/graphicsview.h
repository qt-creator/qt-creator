/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "curveeditorstyle.h"
#include "curveeditorstyledialog.h"
#include "graphicsscene.h"
#include "playhead.h"
#include "selector.h"

#include <QGraphicsView>

namespace DesignTools {

class CurveItem;
class CurveEditorModel;
class Playhead;
class PropertyTreeItem;

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

    friend class Playhead;

signals:
    void notifyFrameChanged(int frame);

public:
    GraphicsView(CurveEditorModel *model, QWidget *parent = nullptr);

    ~GraphicsView() override;

    CurveEditorModel *model() const;

    CurveEditorStyle editorStyle() const;

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

    void setLocked(PropertyTreeItem *item);

    void setStyle(const CurveEditorStyle &style);

    void setZoomX(double zoom, const QPoint &pivot = QPoint());

    void setZoomY(double zoom, const QPoint &pivot = QPoint());

    void setCurrentFrame(int frame, bool notify = true);

    void scrollContent(double x, double y);

    void reset(const std::vector<CurveItem *> &items);

    void updateSelection(const std::vector<CurveItem *> &items);

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

    void drawGrid(QPainter *painter, const QRectF &rect);

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

private:
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

} // End namespace DesignTools.
