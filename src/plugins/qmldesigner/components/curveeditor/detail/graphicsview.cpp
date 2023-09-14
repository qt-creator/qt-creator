// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "graphicsview.h"
#include "axis.h"
#include "curveeditormodel.h"
#include "curveitem.h"
#include "navigation2d.h"
#include "treeitem.h"
#include "curveeditorutils.h"

#include <theme.h>
#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QAction>
#include <QMenu>
#include <QResizeEvent>
#include <QScrollBar>

#include <cmath>
#include <iomanip>
#include <sstream>

namespace QmlDesigner {

template< typename T >
T* nextParentOfType(QWidget* widget)
{
    auto* p = widget->parent();
    while (p) {
        if (T* w = qobject_cast<T*>(p))
            return w;
        p = p->parent();
    }
    return nullptr;
}

GraphicsView::GraphicsView(CurveEditorModel *model, QWidget *parent)
    : QGraphicsView(parent)
    , m_dragging(false)
    , m_zoomX(0.0)
    , m_zoomY(0.0)
    , m_transform()
    , m_scene(new GraphicsScene())
    , m_model(model)
    , m_playhead(this)
    , m_selector()
    , m_style(model->style())
    , m_dialog(m_style)
{
    model->setGraphicsView(this);

    setScene(m_scene);
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setRenderHint(QPainter::Antialiasing, true);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    connect(&m_dialog, &CurveEditorStyleDialog::styleChanged, this, &GraphicsView::setStyle);

    connect(m_scene, &GraphicsScene::curveMessage, m_model, &CurveEditorModel::setStatusLineMsg);

    auto itemSlot = [this](unsigned int id, const AnimationCurve &curve) {
        m_model->setCurve(id, curve);
        applyZoom(m_zoomX, m_zoomY);
    };

    connect(m_scene, &GraphicsScene::curveChanged, itemSlot);

    QmlDesigner::Navigation2dFilter *filter = new QmlDesigner::Navigation2dFilter(viewport());
    connect(filter, &Navigation2dFilter::panChanged, [this](const QPointF &direction) {
        QScrollBar* verticalBar = nullptr;
        if (QScrollArea* area = nextParentOfType< QScrollArea >(this))
            verticalBar = area->verticalScrollBar();
        Navigation2dFilter::scroll(direction, horizontalScrollBar(), verticalBar);
    });

    auto zoomChanged = &QmlDesigner::Navigation2dFilter::zoomChanged;
    connect(filter, zoomChanged, [this](double scale, const QPointF &pos) {
        applyZoom(m_zoomX + scale, m_zoomY, mapToGlobal(pos.toPoint()));
    });
    viewport()->installEventFilter(filter);

    applyZoom(m_zoomX, m_zoomY);
    update();
}

GraphicsView::~GraphicsView()
{
    if (m_scene) {
        delete m_scene;
        m_scene = nullptr;
    }
}

CurveEditorModel *GraphicsView::model() const
{
    return m_model;
}

CurveEditorStyle GraphicsView::editorStyle() const
{
    return m_style;
}

bool GraphicsView::dragging() const
{
    return m_dragging;
}

double GraphicsView::minimumTime() const
{
    bool check = m_model->minimumTime() < m_scene->minimumTime();
    return check ? m_model->minimumTime() : m_scene->minimumTime();
}

double GraphicsView::maximumTime() const
{
    bool check = m_model->maximumTime() > m_scene->maximumTime();
    return check ? m_model->maximumTime() : m_scene->maximumTime();
}

double GraphicsView::minimumValue() const
{
    return m_scene->empty() ? CurveEditorStyle::defaultValueMin : m_scene->minimumValue();
}

double GraphicsView::maximumValue() const
{
    return m_scene->empty() ? CurveEditorStyle::defaultValueMax : m_scene->maximumValue();
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
    QPoint br = vp.bottomLeft() + QPoint(m_style.valueAxisWidth, 0);
    return mapToScene(QRect(vp.topLeft(), br)).boundingRect();
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

    const auto curves = m_scene->curves();
    for (auto *curve : curves)
        curve->setStyle(style);

    applyZoom(m_zoomX, m_zoomY);
    viewport()->update();
}

void GraphicsView::setIsMcu(bool isMcu)
{
    m_scene->setIsMcu(isMcu);
}

void GraphicsView::setLocked(TreeItem *item)
{
    if (item->asNodeItem()) {
        for (auto *ci : item->children())
            setLocked(ci);
    } else if (item->asPropertyItem()) {
        if (CurveItem *curve = m_scene->findCurve(item->id())) {
            if (item->locked() || item->implicitlyLocked()) {
                curve->setLocked(true);
                m_scene->moveToBottom(curve);
            } else {
                curve->setLocked(false);
                m_scene->moveToTop(curve);
            }
        }
    }
}

void GraphicsView::setPinned(TreeItem *item)
{
    auto pin = [this](PropertyTreeItem *pitem, bool pinned) {
        if (pinned) {
            if (CurveItem *curve = m_scene->findCurve(pitem->id()))
                curve->setPinned(pinned);
            else if (CurveItem *citem = TreeModel::curveItem(pitem))
                m_scene->addCurveItem(citem);
        } else if (!pinned) {
            if (!m_model->isSelected(pitem) && !pitem->pinned())
                m_scene->removeCurveItem(pitem->id());
            else if (CurveItem *curve = m_scene->findCurve(pitem->id()))
                curve->setPinned(pinned);
        }
    };

    if (auto *pitem = item->asPropertyItem()) {
        pin(pitem, pitem->pinned() || pitem->implicitlyPinned());
    } else if (auto *nitem = item->asNodeItem()) {
        bool pinned = nitem->pinned();
        if (!pinned && m_model->isSelected(nitem)) {
            for (auto *i : nitem->children()) {
                if (CurveItem *curve = m_scene->findCurve(i->id()))
                    curve->setPinned(pinned);
            }
            return;
        }

        for (auto *i : nitem->children()) {
            if (auto *pitem = i->asPropertyItem())
                pin(pitem, pinned);
        }
    }

    applyZoom(m_zoomX, m_zoomY);
    viewport()->update();
}

void GraphicsView::setZoomX(double zoom, const QPoint &pivot)
{
    if (pivot.isNull())
        applyZoom(zoom, m_zoomY, viewportCenter());
    else
        applyZoom(zoom, m_zoomY, pivot);

    viewport()->update();
}

void GraphicsView::setZoomY(double zoom, const QPoint &pivot)
{
    if (pivot.isNull())
        applyZoom(zoom, m_zoomY, viewportCenter());
    else
        applyZoom(zoom, m_zoomY, pivot);

    viewport()->update();
}

void GraphicsView::setCurrentFrame(int frame, bool notify)
{
    int clampedFrame = clamp(frame, m_model->minimumTime(), m_model->maximumTime());

    m_playhead.moveToFrame(clampedFrame, this);
    viewport()->update();

    emit currentFrameChanged(clampedFrame, notify);
}

void GraphicsView::scrollContent(double x, double y)
{
    QScrollBar *hs = horizontalScrollBar();
    QScrollBar *vs = verticalScrollBar();
    hs->setValue(hs->value() + static_cast<int>(x));
    vs->setValue(vs->value() + static_cast<int>(y));
}

void GraphicsView::reset(const std::vector<CurveItem *> &items)
{
    m_scene->reset();
    for (auto *item : items)
        m_scene->addCurveItem(item);

    applyZoom(m_zoomX, m_zoomY);
    viewport()->update();
}

void GraphicsView::updateSelection()
{
    std::vector<CurveItem *> preservedItems = m_scene->takePinnedItems();
    std::vector<CurveItem *> deleteItems;
    for (auto *curve : m_model->selectedCurves()) {
        auto finder = [curve](CurveItem *item) { return curve->id() == item->id(); };
        auto iter = std::find_if(preservedItems.begin(), preservedItems.end(), finder);
        if (iter == preservedItems.end())
            preservedItems.push_back(curve);
        else
            deleteItems.push_back(curve);
    }
    freeClear(deleteItems);
    reset(preservedItems);
}

void GraphicsView::setInterpolation(Keyframe::Interpolation interpol)
{
    const auto selectedCurves = m_scene->selectedCurves();
    for (auto *curve : selectedCurves)
        curve->setInterpolation(interpol);

    viewport()->update();
}

void GraphicsView::toggleUnified()
{
    const auto selectedCurves = m_scene->selectedCurves();
    for (auto *curve : selectedCurves)
        curve->toggleUnified();

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
        m_scene->deleteSelectedKeyframes();
}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (m_playhead.mousePress(globalToScene(event->globalPosition().toPoint()))) {
        m_dragging = true;
        return;
    }

    Shortcut shortcut(event);
    if (shortcut == m_style.shortcuts.insertKeyframe) {
        m_scene->insertKeyframe(globalToRaster(event->globalPosition().toPoint()).x());
        return;
    }

    if (shortcut == Shortcut(Qt::LeftButton)) {
        QPointF pos = mapToScene(event->pos());
        if (timeScaleRect().contains(pos)) {
            m_dragging = true;
            double t = mapXtoTime(static_cast<int>(pos.x()));
            setCurrentFrame(roundToInt(t));
            m_playhead.setMoving(true);
            event->accept();
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);

    m_selector.mousePress(event, this, m_scene);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_playhead.mouseMove(globalToScene(event->globalPosition().toPoint()), this))
        return;

    QGraphicsView::mouseMoveEvent(event);

    m_selector.mouseMove(event, this, m_scene, m_playhead);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);

    m_playhead.mouseRelease(this);
    m_selector.mouseRelease(event, m_scene);
    this->viewport()->update();
    m_dragging = false;
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

    if (Utils::qtcEnvironmentVariableIsSet("QTC_STYLE_CURVE_EDITOR")) {
        QAction *openEditorAction = menu.addAction(tr("Open Style Editor"));
        connect(openEditorAction, &QAction::triggered, openStyleEditor);
    }

    QPointF rasterPos = globalToRaster(event->globalPos());

    menu.addSeparator();
    auto insertKeyframes = [this, rasterPos]() { m_scene->insertKeyframe(rasterPos.x(), true); };
    QAction *insertKeyframeAction = menu.addAction(tr("Insert Keyframe"));
    connect(insertKeyframeAction, &QAction::triggered, insertKeyframes);

    if (!m_scene->hasEditableSegment(rasterPos.x()))
        insertKeyframeAction->setEnabled(false);

    auto deleteKeyframes = [this] { m_scene->deleteSelectedKeyframes(); };
    QAction *deleteKeyframeAction = menu.addAction(tr("Delete Selected Keyframes"));
    connect(deleteKeyframeAction, &QAction::triggered, deleteKeyframes);

    if (!m_scene->hasSelectedKeyframe())
        deleteKeyframeAction->setEnabled(false);

    menu.exec(event->globalPos());
}

void GraphicsView::drawForeground(QPainter *painter, const QRectF &rect)
{
    QRectF abscissa = timeScaleRect();
    if (abscissa.isValid())
        drawTimeScale(painter, abscissa);

    painter->fillRect(QRectF(rect.topLeft(), abscissa.bottomLeft()), m_style.backgroundAlternateBrush);

    m_playhead.paint(painter, this);

    auto ordinate = valueScaleRect();
    if (ordinate.isValid())
        drawValueScale(painter, ordinate);

    m_selector.paint(painter);
}

void GraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, m_style.backgroundBrush);

    drawGrid(painter);
}

int GraphicsView::mapTimeToX(double time) const
{
    return roundToInt(time * scaleX(m_transform));
}

int GraphicsView::mapValueToY(double y) const
{
    return roundToInt(y * scaleY(m_transform));
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
    QPoint scene = globalToScene(point).toPoint();
    return QPointF(mapXtoTime(scene.x()), mapYtoValue(scene.y()));
}

void GraphicsView::applyZoom(double x, double y, const QPoint &pivot)
{
    m_scene->doNotMoveItems(true);

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
    m_scene->setComponentTransform(m_transform);

    QRectF sr = m_scene->rect();
    if (sr.isNull()) {
        sr.setLeft(m_scene->animationRangeMin());
        sr.setRight(m_scene->animationRangeMax());
        sr = m_transform.mapRect(sr);
    }

    sr = sr.adjusted(
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

    for (auto *curve : m_scene->curves()) {
        if (curve->valueType() == AnimationCurve::ValueType::Bool) {
            curve->remapValue(minValue, maxValue);
        }
    }

    m_scene->doNotMoveItems(false);

    viewport()->update();

    emit zoomChanged(m_zoomX, m_zoomY);
}

void GraphicsView::drawGrid(QPainter *painter)
{
    QRectF gridRect = scene()->sceneRect();

    if (!gridRect.isValid())
        return;

    auto drawVerticalLine = [painter, gridRect](double position) {
        QPointF p1(position, gridRect.top());
        QPointF p2(position, gridRect.bottom());
        painter->drawLine(p1, p2);
    };

    painter->save();
    painter->setPen(m_style.gridColor);

    painter->fillRect(gridRect, m_style.backgroundAlternateBrush);

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
    if (m_scene->empty())
        return;

    auto drawHorizontalLine = [rect, painter](double position) {
        painter->drawLine(rect.left(), position, rect.right(), position);
    };

    painter->save();
    painter->setPen(Qt::blue);
    drawHorizontalLine(mapValueToY(m_scene->minimumValue()));
    drawHorizontalLine(mapValueToY(m_scene->maximumValue()));

    painter->restore();
}
#endif

void GraphicsView::drawRangeBar(QPainter *painter, const QRectF &rect)
{
    painter->save();

    QFontMetrics fm(painter->font());
    QRectF labelRect = fm.boundingRect(QString("0"));
    labelRect.moveCenter(rect.center());

    qreal bTick = rect.bottom() - 2;
    qreal tTick = labelRect.bottom() + 2;
    QRectF activeRect = QRectF(QPointF(mapTimeToX(m_model->minimumTime()), tTick),
                               QPointF(mapTimeToX(m_model->maximumTime()), bTick));

    painter->fillRect(activeRect, m_style.rangeBarColor);

    QColor handleColor(m_style.rangeBarCapsColor);
    painter->setBrush(handleColor);
    painter->setPen(handleColor);

    const qreal radius = 5.;
    QRectF minHandle = rangeMinHandle(rect);
    painter->drawRoundedRect(minHandle, radius, radius);
    minHandle.setLeft(minHandle.center().x());
    painter->fillRect(minHandle, handleColor);

    QRectF maxHandle = rangeMaxHandle(rect);
    painter->drawRoundedRect(maxHandle, radius, radius);
    maxHandle.setRight(maxHandle.center().x());
    painter->fillRect(maxHandle, handleColor);

    painter->restore();
}

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

    drawRangeBar(painter, rect);

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
        std::stringstream sstr;
        sstr << std::fixed << std::setprecision(10) << value;
        sstr >> value;

        QString valueText = QString("%1").arg(value);
        int position = mapValueToY(value);
        QRect textRect = fm.boundingRect(valueText);
        textRect.moveCenter(QPoint(rect.center().x(), position));

        painter->drawText(textRect, Qt::AlignCenter, valueText);
    };

    double min = minimumValue();
    double max = maximumValue();
    if (std::isfinite(min) && std::isfinite(max) && rect.isValid()) {
        double density = 1. / (static_cast<double>(fm.height()) * m_style.labelDensityY);
        Axis axis = Axis::compute(min, max, rect.height(), density);
        const double eps = 1.0e-10;
        for (double i = axis.lmin; i <= axis.lmax + eps; i += axis.lstep)
            paintLabeledTick(i);
    }

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
        if (qFuzzyCompare(tickDistance, 0.) && deltaTime >= maxTime)
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

QRectF GraphicsView::rangeMinHandle(const QRectF &rect)
{
    QRectF labelRect = fontMetrics().boundingRect(QString("0"));
    labelRect.moveCenter(rect.center());

    qreal top = rect.bottom() - 2.;
    qreal bottom = labelRect.bottom() + 2.;
    QSize size(10, roundToInt(top - bottom));

    int handle = mapTimeToX(m_model->minimumTime()) - size.width();
    return QRectF(QPointF(handle, bottom), size);
}

QRectF GraphicsView::rangeMaxHandle(const QRectF &rect)
{
    QRectF labelRect = fontMetrics().boundingRect(QString("0"));
    labelRect.moveCenter(rect.center());

    qreal bottom = rect.bottom() - 2.;
    qreal top = labelRect.bottom() + 2.;

    QSize size(10, roundToInt(top - bottom));
    int handle = mapTimeToX(m_model->maximumTime());

    return QRectF(QPointF(handle, bottom), size);
}

QPoint GraphicsView::viewportCenter() const
{
    QPoint viewCenter = viewport()->rect().center();
    return viewport()->mapToGlobal(viewCenter);
}

} // End namespace QmlDesigner.
