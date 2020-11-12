/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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
#include "navigation2d.h"
#include "gestures.h"

#include <QGestureEvent>
#include <QWheelEvent>

#include <cmath>

namespace QmlDesigner {

Navigation2dScrollBar::Navigation2dScrollBar(QWidget *parent)
    : QScrollBar(parent)
{}

bool Navigation2dScrollBar::postEvent(QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        wheelEvent(static_cast<QWheelEvent *>(event));
        return true;
    }
    return false;
}

void Navigation2dScrollBar::wheelEvent(QWheelEvent *event)
{
    if (!event->angleDelta().isNull())
        QScrollBar::wheelEvent(event);
}


Navigation2dFilter::Navigation2dFilter(QWidget *parent, Navigation2dScrollBar *scrollbar)
    : QObject(parent)
    , m_scrollbar(scrollbar)
{
    if (parent) {
        parent->grabGesture(Qt::PinchGesture);
        if (!scrollbar)
            parent->grabGesture(TwoFingerSwipe::type());
    }
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
    } else if (TwoFingerSwipe *swipe = static_cast<TwoFingerSwipe *>(
                   event->gesture(TwoFingerSwipe::type()))) {
        emit panChanged(swipe->direction());
        event->accept();
        return true;
    }
    return false;
}

bool Navigation2dFilter::wheelEvent(QWheelEvent *event)
{
    if (m_scrollbar) {
        if (m_scrollbar->postEvent(event))
            event->ignore();
    } else if (event->source() == Qt::MouseEventNotSynthesized) {
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
    return true;
}

} // End namespace QmlDesigner.
