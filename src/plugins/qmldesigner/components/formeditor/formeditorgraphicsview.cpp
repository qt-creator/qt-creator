// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorgraphicsview.h"
#include "backgroundaction.h"
#include "formeditortracing.h"
#include "formeditorwidget.h"
#include "navigation2d.h"

#include <theme.h>

#include <utils/hostosinfo.h>

#include <QAction>
#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <QScrollBar>
#include <QWheelEvent>

#include <QTimer>

namespace QmlDesigner {

using FormEditorTracing::category;

FormEditorGraphicsView::FormEditorGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    NanotraceHR::Tracer tracer{"form editor graphics view constructor", category()};

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setAlignment(Qt::AlignCenter);
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setRenderHint(QPainter::Antialiasing, false);

    setFrameShape(QFrame::NoFrame);

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);

    activateCheckboardBackground();

    // as mousetracking only works for mouse key it is better to handle it in the
    // eventFilter method so it works also for the space scrolling case as expected
    QCoreApplication::instance()->installEventFilter(this);

    QmlDesigner::Navigation2dFilter *filter = new QmlDesigner::Navigation2dFilter(viewport());
    connect(filter, &Navigation2dFilter::zoomIn, this, &FormEditorGraphicsView::zoomIn);
    connect(filter, &Navigation2dFilter::zoomOut, this, &FormEditorGraphicsView::zoomOut);

    if (Utils::HostOsInfo::isMacHost()) {
        connect(filter, &Navigation2dFilter::panChanged, [this](const QPointF &direction) {
            Navigation2dFilter::scroll(direction, horizontalScrollBar(), verticalScrollBar());
        });
    }

    auto zoomChanged = &Navigation2dFilter::zoomChanged;
    connect(filter, zoomChanged, [this](double s, const QPointF &/*pos*/) {
        if (auto trans = transform() * QTransform::fromScale(1.0 + s, 1.0 + s); trans.m11() > 0) {
            setTransform(trans);
            emit this->zoomChanged(transform().m11());
        }
    });
    viewport()->installEventFilter(filter);
}

bool FormEditorGraphicsView::eventFilter(QObject *watched, QEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics view event filter", category()};

    if (m_isPanning != Panning::NotStarted) {
        if (event->type() == QEvent::Leave && m_isPanning == Panning::SpaceKeyStarted) {
            // there is no way to keep the cursor so we stop panning here
            stopPanning(event);
        }
        if (event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent *>(event);
            if (!m_panningStartPosition.isNull()) {
                horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                    (mouseEvent->position().x() - m_panningStartPosition.x()));
                verticalScrollBar()->setValue(verticalScrollBar()->value() -
                    (mouseEvent->position().y() - m_panningStartPosition.y()));
            }
            m_panningStartPosition = mouseEvent->position();
            event->accept();
            return true;
        }
    }
    return QGraphicsView::eventFilter(watched, event);
}

void FormEditorGraphicsView::wheelEvent(QWheelEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics view wheel event", category()};

    if (event->modifiers().testFlag(Qt::ControlModifier))
        event->ignore();

    QGraphicsView::wheelEvent(event);
}

void FormEditorGraphicsView::mousePressEvent(QMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics mouse press event", category()};

    if (m_isPanning == Panning::NotStarted) {
        if (event->buttons().testFlag(Qt::MiddleButton))
            startPanning(event);
        else
            QGraphicsView::mousePressEvent(event);
    }
}

void FormEditorGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics mouse release event", category()};

    // not sure why buttons() are empty here, but we have that information from the enum
    if (m_isPanning == Panning::MouseWheelStarted)
        stopPanning(event);
    else
        QGraphicsView::mouseReleaseEvent(event);
}

bool isTextInputItem(QGraphicsItem *item)
{
    if (item && item->isWidget()) {
        auto graphicsWidget = static_cast<QGraphicsWidget *>(item);
        auto textInputProxyWidget = qobject_cast<QGraphicsProxyWidget *>(graphicsWidget);
        if (textInputProxyWidget && textInputProxyWidget->widget() && (
                strcmp(textInputProxyWidget->widget()->metaObject()->className(), "QLineEdit") == 0 ||
                strcmp(textInputProxyWidget->widget()->metaObject()->className(), "QTextEdit") == 0)) {
            return true;
        }

    }
    return false;
}

void FormEditorGraphicsView::keyPressEvent(QKeyEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics view key press event", category()};

    // check for autorepeat to avoid a stoped space panning by leave event to be restarted
    if (!event->isAutoRepeat() && m_isPanning == Panning::NotStarted && event->key() == Qt::Key_Space &&
        !isTextInputItem(scene()->focusItem())) {
        startPanning(event);
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void FormEditorGraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics view key release event", category()};

    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()
        && m_isPanning == Panning::SpaceKeyStarted)
        stopPanning(event);

    QGraphicsView::keyReleaseEvent(event);
}

void FormEditorGraphicsView::startPanning(QEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics view key release event", category()};

    if (event->type() == QEvent::KeyPress)
        m_isPanning = Panning::SpaceKeyStarted;
    else
        m_isPanning = Panning::MouseWheelStarted;
    viewport()->setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void FormEditorGraphicsView::stopPanning(QEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor graphics view stop panning", category()};

    m_isPanning = Panning::NotStarted;
    m_panningStartPosition = QPoint();
    viewport()->unsetCursor();
    event->accept();
}

void FormEditorGraphicsView::setRootItemRect(const QRectF &rect)
{
    NanotraceHR::Tracer tracer{"form editor graphics view set root item rect", category()};

    m_rootItemRect = rect;
    viewport()->update();
}

QRectF FormEditorGraphicsView::rootItemRect() const
{
    return m_rootItemRect;
}

void FormEditorGraphicsView::activateCheckboardBackground()
{
    NanotraceHR::Tracer tracer{"form editor graphics view activate checkboard background", category()};

    const int checkerbordSize = 20;
    QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
    tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
}

void FormEditorGraphicsView::activateColoredBackground(const QColor &color)
{
    NanotraceHR::Tracer tracer{"form editor graphics view activate colored background", category()};

    setBackgroundBrush(color);
}

void FormEditorGraphicsView::drawBackground(QPainter *painter, const QRectF &rectangle)
{
    NanotraceHR::Tracer tracer{"form editor graphics view draw background", category()};

    painter->save();
    painter->setBrushOrigin(0, 0);

    // paint rect around editable area

    if (backgroundBrush().color() == BackgroundAction::ContextImage) {
        painter->fillRect(rectangle.intersected(rootItemRect()), Qt::gray);
        painter->setOpacity(0.5);
        if (!m_backgroundImage.isNull())
            painter->drawImage(rootItemRect().topLeft() + m_backgroundImage.offset(),
                               m_backgroundImage);
        painter->setOpacity(1.0);
    } else {
        painter->fillRect(rectangle.intersected(rootItemRect()), backgroundBrush());
    }

    QPen pen(Utils::creatorColor(Utils::Theme::QmlDesigner_FormEditorSelectionColor));

    pen.setStyle(Qt::DotLine);
    pen.setWidth(1);

    painter->setPen(pen);

    painter->drawRect(rootItemRect().adjusted(-1, -1, 0, 0));

    painter->restore();
}

void FormEditorGraphicsView::frame(const QRectF &boundingRect)
{
    NanotraceHR::Tracer tracer{"form editor graphics view frame", category()};

    fitInView(boundingRect, Qt::KeepAspectRatio);
}

void FormEditorGraphicsView::setBackgoundImage(const QImage &image)
{
    NanotraceHR::Tracer tracer{"form editor graphics view set background image", category()};

    m_backgroundImage = image;
    update();
}

QImage FormEditorGraphicsView::backgroundImage() const
{
    NanotraceHR::Tracer tracer{"formeditor annotation icon background image", category()};

    return m_backgroundImage;
}

void FormEditorGraphicsView::setZoomFactor(double zoom)
{
    NanotraceHR::Tracer tracer{"form editor graphics view set zoom factor", category()};

    resetTransform();
    scale(zoom, zoom);
}

} // namespace QmlDesigner
