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

#include "graphicsview.h"
#include "curveeditormodel.h"
#include "curveitem.h"
#include "utils.h"

#include <QAction>
#include <QMenu>
#include <QResizeEvent>
#include <QScrollBar>

#include <cmath>

namespace DesignTools {

GraphicsView::GraphicsView(CurveEditorModel *model, QWidget *parent)
    : QGraphicsView(parent)
    , m_zoomX(0.0)
    , m_zoomY(0.0)
    , m_transform()
    , m_scene()
    , m_model(model)
    , m_playhead(this)
    , m_selector()
    , m_style(model->style())
    , m_dialog(m_style)
{
    model->setGraphicsView(this);

    setScene(&m_scene);
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setRenderHint(QPainter::Antialiasing, true);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    connect(&m_dialog, &CurveEditorStyleDialog::styleChanged, this, &GraphicsView::setStyle);

    auto itemSlot = [this](unsigned int id, const AnimationCurve &curve) {
        applyZoom(m_zoomX, m_zoomY);
        m_model->setCurve(id, curve);
    };

    connect(&m_scene, &GraphicsScene::curveChanged, itemSlot);

    applyZoom(m_zoomX, m_zoomY);
    update();
}

CurveEditorModel *GraphicsView::model() const
{
    return m_model;
}

CurveEditorStyle GraphicsView::editorStyle() const
{
    return m_style;
}

bool GraphicsView::hasActiveItem() const
{
    return m_scene.hasActiveItem();
}

bool GraphicsView::hasActiveHandle() const
{
    return m_scene.hasActiveHandle();
}

double GraphicsView::minimumTime() const
{
    bool check = m_model->minimumTime() < m_scene.minimumTime();
    return check ? m_model->minimumTime() : m_scene.minimumTime();
}

double GraphicsView::maximumTime() const
{
    bool check = m_model->maximumTime() > m_scene.maximumTime();
    return check ? m_model->maximumTime() : m_scene.maximumTime();
}

double GraphicsView::minimumValue() const
{
    return m_scene.empty() ? -1.0 : m_scene.minimumValue();
}

double GraphicsView::maximumValue() const
{
    return m_scene.empty() ? 1.0 : m_scene.maximumValue();
}

double GraphicsView::zoomX() const
{
    return m_zoomX;
}

double GraphicsView::zoomY() const
{
    return m_zoomY;
}

QRectF GraphicsView::canvasRect() const
{
    QRect r = viewport()->rect().adjusted(
        m_style.valueAxisWidth + m_style.canvasMargin,
        m_style.timeAxisHeight + m_style.canvasMargin,
        -m_style.canvasMargin,
        -m_style.canvasMargin);

    return mapToScene(r).boundingRect();
}

QRectF GraphicsView::timeScaleRect() const
{
    QRect vp(viewport()->rect());
    QPoint tl = vp.topLeft() + QPoint(m_style.valueAxisWidth, 0);
    QPoint br = vp.topRight() + QPoint(0, m_style.timeAxisHeight);
    return mapToScene(QRect(tl, br)).boundingRect();
}

QRectF GraphicsView::valueScaleRect() const
{
    QRect vp(viewport()->rect());
    QPoint tl = vp.topLeft() + QPoint(0, m_style.timeAxisHeight);
    QPoint br = vp.bottomLeft() + QPoint(m_style.valueAxisWidth, 0);
    return mapToScene(QRect(tl, br)).boundingRect();
}

QRectF GraphicsView::defaultRasterRect() const
{
    QPointF topLeft(mapTimeToX(minimumTime()), mapValueToY(maximumValue()));
    QPointF bottomRight(mapTimeToX(maximumTime()), mapValueToY(minimumValue()));
    return QRectF(topLeft, bottomRight);
}

void GraphicsView::setStyle(const CurveEditorStyle &style)
{
    m_style = style;

    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(item))
            curveItem->setStyle(style);
    }

    applyZoom(m_zoomX, m_zoomY);
    viewport()->update();
}

void GraphicsView::setZoomX(double zoom, const QPoint &pivot)
{
    applyZoom(zoom, m_zoomY, pivot);
    viewport()->update();
}

void GraphicsView::setZoomY(double zoom, const QPoint &pivot)
{
    applyZoom(m_zoomX, zoom, pivot);
    viewport()->update();
}

void GraphicsView::setCurrentFrame(int frame, bool notify)
{
    int clampedFrame = clamp(frame, m_model->minimumTime(), m_model->maximumTime());
    m_playhead.moveToFrame(clampedFrame, this);
    viewport()->update();

    if (notify)
        notifyFrameChanged(frame);
}

void GraphicsView::scrollContent(double x, double y)
{
    QScrollBar *hs = horizontalScrollBar();
    QScrollBar *vs = verticalScrollBar();
    hs->setValue(hs->value() + x);
    vs->setValue(vs->value() + y);
}

void GraphicsView::reset(const std::vector<CurveItem *> &items)
{
    m_scene.clear();
    for (auto *item : items)
        m_scene.addCurveItem(item);

    applyZoom(m_zoomX, m_zoomY);
    viewport()->update();
}

void GraphicsView::setInterpolation(Keyframe::Interpolation interpol)
{
    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *citem = qgraphicsitem_cast<CurveItem *>(item))
            if (citem->hasSelection())
                citem->setInterpolation(interpol);
    }

    viewport()->update();
}

void GraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    applyZoom(m_zoomX, m_zoomY);
}

void GraphicsView::keyPressEvent(QKeyEvent *event)
{
    Shortcut shortcut(event->modifiers(), static_cast<Qt::Key>(event->key()));
    if (shortcut == m_style.shortcuts.frameAll)
        applyZoom(0.0, 0.0);
    else if (shortcut == m_style.shortcuts.deleteKeyframe)
        deleteSelectedKeyframes();
}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (m_playhead.mousePress(globalToScene(event->globalPos())))
        return;

    Shortcut shortcut(event);
    if (shortcut == m_style.shortcuts.insertKeyframe) {
        insertKeyframe(globalToRaster(event->globalPos()).x());
        return;
    }

    if (shortcut == Shortcut(Qt::LeftButton)) {
        QPointF pos = mapToScene(event->pos());
        if (timeScaleRect().contains(pos)) {
            setCurrentFrame(std::round(mapXtoTime(pos.x())));
            m_playhead.setMoving(true);
            event->accept();
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);

    m_selector.mousePress(event, this);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_playhead.mouseMove(globalToScene(event->globalPos()), this))
        return;

    QGraphicsView::mouseMoveEvent(event);

    m_selector.mouseMove(event, this, m_playhead);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);

    m_playhead.mouseRelease(this);
    m_selector.mouseRelease(event, this);
    this->viewport()->update();
}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::AltModifier))
        return;

    QGraphicsView::wheelEvent(event);
}

void GraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    if (event->modifiers() != Qt::NoModifier)
        return;

    auto openStyleEditor = [this]() { m_dialog.show(); };

    QMenu menu;

    if (qEnvironmentVariableIsSet("QTC_STYLE_CURVE_EDITOR")) {
        QAction *openEditorAction = menu.addAction(tr("Open Style Editor"));
        connect(openEditorAction, &QAction::triggered, openStyleEditor);
    }

    menu.addSeparator();
    auto insertKeyframes = [this, event]() {
        insertKeyframe(globalToRaster(event->globalPos()).x(), true);
    };
    QAction *insertKeyframeAction = menu.addAction(tr("Insert Keyframe"));
    connect(insertKeyframeAction, &QAction::triggered, insertKeyframes);

    menu.exec(event->globalPos());
}

void GraphicsView::drawForeground(QPainter *painter, const QRectF &rect)
{
    QRectF abscissa = timeScaleRect();
    if (abscissa.isValid())
        drawTimeScale(painter, abscissa);

    auto ordinate = valueScaleRect();
    if (ordinate.isValid())
        drawValueScale(painter, ordinate);

    m_playhead.paint(painter, this);

    painter->fillRect(QRectF(rect.topLeft(), abscissa.bottomLeft()),
                      m_style.backgroundAlternateBrush);

    m_selector.paint(painter);
}

void GraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, m_style.backgroundBrush);
    painter->fillRect(scene()->sceneRect(), m_style.backgroundAlternateBrush);

    drawGrid(painter, rect);
}

int GraphicsView::mapTimeToX(double time) const
{
    return std::round(time * scaleX(m_transform));
}

int GraphicsView::mapValueToY(double y) const
{
    return std::round(y * scaleY(m_transform));
}

double GraphicsView::mapXtoTime(int x) const
{
    return static_cast<double>(x) / scaleX(m_transform);
}

double GraphicsView::mapYtoValue(int y) const
{
    return static_cast<double>(y) / scaleY(m_transform);
}

QPointF GraphicsView::globalToScene(const QPoint &point) const
{
    return mapToScene(viewport()->mapFromGlobal(point));
}

QPointF GraphicsView::globalToRaster(const QPoint &point) const
{
    QPointF scene = globalToScene(point);
    return QPointF(mapXtoTime(scene.x()), mapYtoValue(scene.y()));
}

void GraphicsView::applyZoom(double x, double y, const QPoint &pivot)
{
    QPointF pivotRaster(globalToRaster(pivot));

    m_zoomX = clamp(x, 0.0, 1.0);
    m_zoomY = clamp(y, 0.0, 1.0);

    double minTime = minimumTime();
    double maxTime = maximumTime();

    double minValue = minimumValue();
    double maxValue = maximumValue();

    QRectF canvas = canvasRect();

    double xZoomedOut = canvas.width() / (maxTime - minTime);
    double xZoomedIn = m_style.zoomInWidth;
    double scaleX = lerp(clamp(m_zoomX, 0.0, 1.0), xZoomedOut, xZoomedIn);

    double yZoomedOut = canvas.height() / (maxValue - minValue);
    double yZoomedIn = m_style.zoomInHeight;
    double scaleY = lerp(clamp(m_zoomY, 0.0, 1.0), -yZoomedOut, -yZoomedIn);

    m_transform = QTransform::fromScale(scaleX, scaleY);
    m_scene.setComponentTransform(m_transform);

    QRectF sr = m_scene.sceneRect().adjusted(
        -m_style.valueAxisWidth - m_style.canvasMargin,
        -m_style.timeAxisHeight - m_style.canvasMargin,
        m_style.canvasMargin,
        m_style.canvasMargin);

    setSceneRect(sr);

    m_playhead.resize(this);

    if (!pivot.isNull()) {
        QPointF deltaTransformed = pivotRaster - globalToRaster(pivot);
        scrollContent(mapTimeToX(deltaTransformed.x()), mapValueToY(deltaTransformed.y()));
    }
}

void GraphicsView::insertKeyframe(double time, bool allVisibleCurves)
{
    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(item)) {
            if (allVisibleCurves)
                curveItem->insertKeyframeByTime(std::round(time));
            else if (curveItem->isUnderMouse())
                curveItem->insertKeyframeByTime(std::round(time));
        }
    }
}

void GraphicsView::deleteSelectedKeyframes()
{
    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(item))
            curveItem->deleteSelectedKeyframes();
    }
}

void GraphicsView::drawGrid(QPainter *painter, const QRectF &rect)
{
    QRectF gridRect = rect.adjusted(m_style.valueAxisWidth + m_style.canvasMargin,
                                    m_style.timeAxisHeight + m_style.canvasMargin,
                                    -m_style.canvasMargin,
                                    -m_style.canvasMargin);

    if (!gridRect.isValid())
        return;

    auto drawVerticalLine = [painter, gridRect](double position) {
        painter->drawLine(position, gridRect.top(), position, gridRect.bottom());
    };

    painter->save();
    painter->setPen(m_style.gridColor);

    double timeIncrement = timeLabelInterval(painter, m_model->maximumTime());
    for (double i = minimumTime(); i <= maximumTime(); i += timeIncrement)
        drawVerticalLine(mapTimeToX(i));

    painter->restore();
}

#if 0
void GraphicsView::drawExtremaX(QPainter *painter, const QRectF &rect)
{
    auto drawVerticalLine = [rect, painter](double position) {
        painter->drawLine(position, rect.top(), position, rect.bottom());
    };

    painter->save();
    painter->setPen(Qt::red);
    drawVerticalLine(mapTimeToX(m_model->minimumTime()));
    drawVerticalLine(mapTimeToX(m_model->maximumTime()));
    painter->restore();
}

void GraphicsView::drawExtremaY(QPainter *painter, const QRectF &rect)
{
    if (m_scene.empty())
        return;

    auto drawHorizontalLine = [rect, painter](double position) {
        painter->drawLine(rect.left(), position, rect.right(), position);
    };

    painter->save();
    painter->setPen(Qt::blue);
    drawHorizontalLine(mapValueToY(m_scene.minimumValue()));
    drawHorizontalLine(mapValueToY(m_scene.maximumValue()));

    painter->restore();
}
#endif

void GraphicsView::drawTimeScale(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->setPen(m_style.fontColor);
    painter->fillRect(rect, m_style.backgroundAlternateBrush);

    QFontMetrics fm(painter->font());

    auto paintLabeledTick = [this, painter, rect, fm](double time) {
        QString timeText = QString("%1").arg(time);

        int position = mapTimeToX(time);

        QRect textRect = fm.boundingRect(timeText);
        textRect.moveCenter(QPoint(position, rect.center().y()));

        painter->drawText(textRect, Qt::AlignCenter, timeText);
        painter->drawLine(position, rect.bottom() - 2, position, textRect.bottom() + 2);
    };

    double timeIncrement = timeLabelInterval(painter, maximumTime());
    for (double i = minimumTime(); i <= maximumTime(); i += timeIncrement)
        paintLabeledTick(i);

    painter->restore();
}

void GraphicsView::drawValueScale(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->setPen(m_style.fontColor);
    painter->fillRect(rect, m_style.backgroundAlternateBrush);

    QFontMetrics fm(painter->font());
    auto paintLabeledTick = [this, painter, rect, fm](double value) {
        QString valueText = QString("%1").arg(value);

        int position = mapValueToY(value);

        QRect textRect = fm.boundingRect(valueText);
        textRect.moveCenter(QPoint(rect.center().x(), position));
        painter->drawText(textRect, Qt::AlignCenter, valueText);
    };

    paintLabeledTick(minimumValue());
    paintLabeledTick(maximumValue());
    painter->restore();
}

double GraphicsView::timeLabelInterval(QPainter *painter, double maxTime)
{
    QFontMetrics fm(painter->font());
    int minTextSpacing = fm.horizontalAdvance(QString("X%1X").arg(maxTime));

    int deltaTime = 1;
    int nextFactor = 5;

    double tickDistance = mapTimeToX(deltaTime);

    while (true) {
        if (tickDistance == 0 && deltaTime >= maxTime)
            return maxTime;

        if (tickDistance > minTextSpacing)
            break;

        deltaTime *= nextFactor;
        tickDistance = mapTimeToX(deltaTime);

        if (nextFactor == 5)
            nextFactor = 2;
        else
            nextFactor = 5;
    }

    return deltaTime;
}

} // End namespace DesignTools.
