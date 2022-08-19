// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "navigation2d.h"

#include <QGestureEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QMetaMethod>

#include <cmath>

namespace QmlDesigner {

void Navigation2dFilter::scroll(const QPointF &direction, QScrollBar *sbx, QScrollBar *sby)
{
    auto doScroll = [](QScrollBar *sb, float distance) {
        if (sb) {
            // max - min + pageStep = sceneRect.size * scale
            float d1 = sb->maximum() - sb->minimum();
            float d2 = d1 + sb->pageStep();

            float val = (distance / d2) * d1;
            sb->setValue(sb->value() - val);
        }
    };

    doScroll(sbx, direction.x());
    doScroll(sby, direction.y());
}

Navigation2dFilter::Navigation2dFilter(QWidget *parent)
    : QObject(parent)
{
    if (parent)
        parent->grabGesture(Qt::PinchGesture);
}

bool Navigation2dFilter::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent *>(event));
    else if (event->type() == QEvent::Wheel)
        return wheelEvent(static_cast<QWheelEvent *>(event));

    return QObject::eventFilter(object, event);
}

bool Navigation2dFilter::gestureEvent(QGestureEvent *event)
{
    if (QPinchGesture *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
        QPinchGesture::ChangeFlags changeFlags = pinch->changeFlags();
        if (changeFlags.testFlag(QPinchGesture::ScaleFactorChanged)) {
            emit zoomChanged(-(1.0 - pinch->scaleFactor()), pinch->startCenterPoint());
            event->accept();
            return true;
        }
    }
    return false;
}

bool Navigation2dFilter::wheelEvent(QWheelEvent *event)
{
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (QPointF delta = event->pixelDelta(); !delta.isNull()) {
                double dist = std::abs(delta.x()) > std::abs(delta.y()) ? -delta.x() : delta.y();
                emit zoomChanged(dist/200.0, event->position());
                event->accept();
                return true;
            }
        } else {
            emit panChanged(QPointF(event->pixelDelta()));
            event->accept();
            return true;
        }
    } else if (event->source() == Qt::MouseEventNotSynthesized) {

        auto zoomInSignal = QMetaMethod::fromSignal(&Navigation2dFilter::zoomIn);
        bool zoomInConnected = QObject::isSignalConnected(zoomInSignal);

        auto zoomOutSignal = QMetaMethod::fromSignal(&Navigation2dFilter::zoomOut);
        bool zoomOutConnected = QObject::isSignalConnected(zoomOutSignal);

        if (zoomInConnected && zoomOutConnected) {
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                if (QPointF angle = event->angleDelta(); !angle.isNull()) {
                    double delta = std::abs(angle.x()) > std::abs(angle.y()) ? angle.x() : angle.y();
                    if (delta > 0)
                        emit zoomIn();
                    else
                        emit zoomOut();
                    event->accept();
                    return true;
                }
            }
        }
    }
    return false;
}

} // End namespace QmlDesigner.
