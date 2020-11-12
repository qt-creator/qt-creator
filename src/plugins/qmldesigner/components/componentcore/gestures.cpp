/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "gestures.h"

#include <QWidget>

namespace QmlDesigner {

Qt::GestureType TwoFingerSwipe::m_type = static_cast<Qt::GestureType>(0);

TwoFingerSwipe::TwoFingerSwipe() {}

Qt::GestureType TwoFingerSwipe::type()
{
    return m_type;
}

void TwoFingerSwipe::registerRecognizer()
{
    m_type = QGestureRecognizer::registerRecognizer(new TwoFingerSwipeRecognizer());
}

QPointF TwoFingerSwipe::direction() const
{
    return m_current.center() - m_last.center();
}

void TwoFingerSwipe::reset()
{
    m_start = QLineF();
    m_current = QLineF();
    m_last = QLineF();
}

QGestureRecognizer::Result TwoFingerSwipe::begin(QTouchEvent *event)
{
    Q_UNUSED(event);
    return QGestureRecognizer::MayBeGesture;
}

QGestureRecognizer::Result TwoFingerSwipe::update(QTouchEvent *event)
{
    if (event->touchPoints().size() != 2) {
        if (state() == Qt::NoGesture)
            return QGestureRecognizer::Ignore;
        else
            return QGestureRecognizer::FinishGesture;
    }

    QTouchEvent::TouchPoint p0 = event->touchPoints().at(0);
    QTouchEvent::TouchPoint p1 = event->touchPoints().at(1);

    QLineF line(p0.scenePos(), p1.screenPos());

    if (m_start.isNull()) {
        m_start = line;
        m_current = line;
        m_last = line;
    } else {
        auto deltaLength = line.length() - m_current.length();
        auto deltaCenter = QLineF(line.center(), m_current.center()).length();
        if (deltaLength > deltaCenter)
            return QGestureRecognizer::CancelGesture;

        m_last = m_current;
        m_current = line;
    }

    setHotSpot(m_current.center());

    return QGestureRecognizer::TriggerGesture;
}

QGestureRecognizer::Result TwoFingerSwipe::end(QTouchEvent *event)
{
    Q_UNUSED(event);
    bool finish = state() != Qt::NoGesture;

    reset();

    if (finish)
        return QGestureRecognizer::FinishGesture;
    else
        return QGestureRecognizer::CancelGesture;
}

TwoFingerSwipeRecognizer::TwoFingerSwipeRecognizer()
    : QGestureRecognizer()
{}

QGesture *TwoFingerSwipeRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType())
        qobject_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);

    return new TwoFingerSwipe;
}

QGestureRecognizer::Result TwoFingerSwipeRecognizer::recognize(QGesture *gesture,
                                                               QObject *,
                                                               QEvent *event)
{
    if (gesture->gestureType() != TwoFingerSwipe::type())
        return QGestureRecognizer::Ignore;

    TwoFingerSwipe *swipe = static_cast<TwoFingerSwipe *>(gesture);
    QTouchEvent *touch = static_cast<QTouchEvent *>(event);

    switch (event->type()) {
    case QEvent::TouchBegin:
        return swipe->begin(touch);

    case QEvent::TouchUpdate:
        return swipe->update(touch);

    case QEvent::TouchEnd:
        return swipe->end(touch);

    default:
        return QGestureRecognizer::Ignore;
    }
}

void TwoFingerSwipeRecognizer::reset(QGesture *gesture)
{
    if (gesture->gestureType() == TwoFingerSwipe::type()) {
        TwoFingerSwipe *swipe = static_cast<TwoFingerSwipe *>(gesture);
        swipe->reset();
    }
    QGestureRecognizer::reset(gesture);
}

} // End namespace QmlDesigner.
