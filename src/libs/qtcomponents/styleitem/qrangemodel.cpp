/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

/*!
    \class QRangeModel
    \brief The QRangeModel class, helps users to build components that depend
           on some value and/or position to be in a certain range previously defined

    With this class, the user sets a value range and a position range, which
    represent the valid values/positions the model can assume. It is worth telling
    that the value property always has priority over the position property. A nice use
    case, would be a Slider implementation with the help of QRangeModel. If the user sets
    a value range to [0,100], a position range to [50,100] and sets the value
    to 80, the equivalent position would be 90. After that, if the user decides to
    resize the slider, the value would be the same, but the knob position would
    be updated due to the new position range.

    \ingroup qt-components
*/

#include <QEvent>
#include <QApplication>
#include <QGraphicsSceneEvent>
#include <QDebug>

#ifndef QT_NO_ACCESSIBILITY
#include <QAccessible>
#endif

#include "qrangemodel.h"
#include "qrangemodel_p.h"

QRangeModelPrivate::QRangeModelPrivate(QRangeModel *qq)
    : q_ptr(qq)
{
}

QRangeModelPrivate::~QRangeModelPrivate()
{
}

void QRangeModelPrivate::init()
{
    minimum = 0;
    maximum = 99;
    stepSize = 0;
    value = 0;
    pos = 0;
    posatmin = 0;
    posatmax = 0;
    inverted = false;
}

/*!
    Calculates the position that is going to be seen outside by the component
    that is using QRangeModel. It takes into account the \l stepSize,
    \l positionAtMinimum, \l positionAtMaximum properties
    and \a position that is passed as parameter.
*/

qreal QRangeModelPrivate::publicPosition(qreal position) const
{
    // Calculate the equivalent stepSize for the position property.
    const qreal min = effectivePosAtMin();
    const qreal max = effectivePosAtMax();
    const qreal valueRange = maximum - minimum;
    const qreal positionValueRatio = valueRange ? (max - min) / valueRange : 0;
    const qreal positionStep = stepSize * positionValueRatio;

    if (positionStep == 0)
        return (min < max) ? qBound(min, position, max) : qBound(max, position, min);

    const int stepSizeMultiplier = (position - min) / positionStep;

    // Test whether value is below minimum range
    if (stepSizeMultiplier < 0)
        return min;

    qreal leftEdge = (stepSizeMultiplier * positionStep) + min;
    qreal rightEdge = ((stepSizeMultiplier + 1) * positionStep) + min;

    if (min < max) {
        leftEdge = qMin(leftEdge, max);
        rightEdge = qMin(rightEdge, max);
    } else {
        leftEdge = qMax(leftEdge, max);
        rightEdge = qMax(rightEdge, max);
    }

    if (qAbs(leftEdge - position) <= qAbs(rightEdge - position))
        return leftEdge;
    return rightEdge;
}

/*!
    Calculates the value that is going to be seen outside by the component
    that is using QRangeModel. It takes into account the \l stepSize,
    \l minimumValue, \l maximumValue properties
    and \a value that is passed as parameter.
*/

qreal QRangeModelPrivate::publicValue(qreal value) const
{
    // It is important to do value-within-range check this
    // late (as opposed to during setPosition()). The reason is
    // QML bindings; a position that is initially invalid because it lays
    // outside the range, might become valid later if the range changes.

    if (stepSize == 0)
        return qBound(minimum, value, maximum);

    const int stepSizeMultiplier = (value - minimum) / stepSize;

    // Test whether value is below minimum range
    if (stepSizeMultiplier < 0)
        return minimum;

    const qreal leftEdge = qMin(maximum, (stepSizeMultiplier * stepSize) + minimum);
    const qreal rightEdge = qMin(maximum, ((stepSizeMultiplier + 1) * stepSize) + minimum);
    const qreal middle = (leftEdge + rightEdge) / 2;

    return (value <= middle) ? leftEdge : rightEdge;
}

/*!
    Checks if the \l value or \l position, that is seen by the user, has changed and emits the changed signal if it
    has changed.
*/

void QRangeModelPrivate::emitValueAndPositionIfChanged(const qreal oldValue, const qreal oldPosition)
{
    Q_Q(QRangeModel);

    // Effective value and position might have changed even in cases when e.g. d->value is
    // unchanged. This will be the case when operating with values outside range:
    const qreal newValue = q->value();
    const qreal newPosition = q->position();
    if (!qFuzzyCompare(newValue, oldValue))
        emit q->valueChanged(newValue);
    if (!qFuzzyCompare(newPosition, oldPosition))
        emit q->positionChanged(newPosition);
}

/*!
    Constructs a QRangeModel with \a parent
*/

QRangeModel::QRangeModel(QObject *parent)
    : QObject(parent), d_ptr(new QRangeModelPrivate(this))
{
    Q_D(QRangeModel);
    d->init();
}

/*!
    \internal
    Constructs a QRangeModel with private class pointer \a dd and \a parent
*/

QRangeModel::QRangeModel(QRangeModelPrivate &dd, QObject *parent)
    : QObject(parent), d_ptr(&dd)
{
    Q_D(QRangeModel);
    d->init();
}

/*!
    Destroys the QRangeModel
*/

QRangeModel::~QRangeModel()
{
    delete d_ptr;
    d_ptr = 0;
}

/*!
    Sets the range of valid positions, that \l position can assume externally, with
    \a min and \a max.
    Such range is represented by \l positionAtMinimum and \l positionAtMaximum
*/

void QRangeModel::setPositionRange(qreal min, qreal max)
{
    Q_D(QRangeModel);

    bool emitPosAtMinChanged = !qFuzzyCompare(min, d->posatmin);
    bool emitPosAtMaxChanged = !qFuzzyCompare(max, d->posatmax);

    if (!(emitPosAtMinChanged || emitPosAtMaxChanged))
        return;

    const qreal oldPosition = position();
    d->posatmin = min;
    d->posatmax = max;

    // When a new positionRange is defined, the position property must be updated based on the value property.
    // For instance, imagine that you have a valueRange of [0,100] and a position range of [20,100],
    // if a user set the value to 50, the position would be 60. If this positionRange is updated to [0,100], then
    // the new position, based on the value (50), will be 50.
    // If the newPosition is different than the old one, it must be updated, in order to emit
    // the positionChanged signal.
    d->pos = d->equivalentPosition(d->value);

    if (emitPosAtMinChanged)
        emit positionAtMinimumChanged(d->posatmin);
    if (emitPosAtMaxChanged)
        emit positionAtMaximumChanged(d->posatmax);

    d->emitValueAndPositionIfChanged(value(), oldPosition);
}
/*!
    Sets the range of valid values, that \l value can assume externally, with
    \a min and \a max. The range has the following constraint: \a min must be less or equal \a max
    Such range is represented by \l minimumValue and \l maximumValue
*/

void QRangeModel::setRange(qreal min, qreal max)
{
    Q_D(QRangeModel);

    bool emitMinimumChanged = !qFuzzyCompare(min, d->minimum);
    bool emitMaximumChanged = !qFuzzyCompare(max, d->maximum);

    if (!(emitMinimumChanged || emitMaximumChanged))
        return;

    const qreal oldValue = value();
    const qreal oldPosition = position();

    d->minimum = min;
    d->maximum = qMax(min, max);

    // Update internal position if it was changed. It can occurs if internal value changes, due to range update
    d->pos = d->equivalentPosition(d->value);

    if (emitMinimumChanged)
        emit minimumChanged(d->minimum);
    if (emitMaximumChanged)
        emit maximumChanged(d->maximum);

    d->emitValueAndPositionIfChanged(oldValue, oldPosition);
}

/*!
    \property QRangeModel::minimumValue
    \brief the minimum value that \l value can assume

    This property's default value is 0
*/

void QRangeModel::setMinimum(qreal min)
{
    Q_D(const QRangeModel);
    setRange(min, d->maximum);
}

qreal QRangeModel::minimum() const
{
    Q_D(const QRangeModel);
    return d->minimum;
}

/*!
    \property QRangeModel::maximumValue
    \brief the maximum value that \l value can assume

    This property's default value is 99
*/

void QRangeModel::setMaximum(qreal max)
{
    Q_D(const QRangeModel);
    // if the new maximum value is smaller than
    // minimum, update minimum too
    setRange(qMin(d->minimum, max), max);
}

qreal QRangeModel::maximum() const
{
    Q_D(const QRangeModel);
    return d->maximum;
}

/*!
    \property QRangeModel::stepSize
    \brief the value that is added to the \l value and \l position property

    Example: If a user sets a range of [0,100] and stepSize
    to 30, the valid values that are going to be seen externally would be: 0, 30, 60, 90, 100.
*/

void QRangeModel::setStepSize(qreal stepSize)
{
    Q_D(QRangeModel);

    stepSize = qMax(qreal(0.0), stepSize);
    if (qFuzzyCompare(stepSize, d->stepSize))
        return;

    const qreal oldValue = value();
    const qreal oldPosition = position();
    d->stepSize = stepSize;

    emit stepSizeChanged(d->stepSize);
    d->emitValueAndPositionIfChanged(oldValue, oldPosition);
}

qreal QRangeModel::stepSize() const
{
    Q_D(const QRangeModel);
    return d->stepSize;
}

/*!
    Returns a valid position, respecting the \l positionAtMinimum,
    \l positionAtMaximum and the \l stepSize properties.
    Such calculation is based on the parameter \a value (which is valid externally).
*/

qreal QRangeModel::positionForValue(qreal value) const
{
    Q_D(const QRangeModel);

    const qreal unconstrainedPosition = d->equivalentPosition(value);
    return d->publicPosition(unconstrainedPosition);
}

/*!
    \property QRangeModel::position
    \brief the current position of the model

    Represents a valid external position, based on the \l positionAtMinimum,
    \l positionAtMaximum and the \l stepSize properties.
    The user can set it internally with a position, that is not within the current position range,
    since it can become valid if the user changes the position range later.
*/

qreal QRangeModel::position() const
{
    Q_D(const QRangeModel);

    // Return the internal position but observe boundaries and
    // stepSize restrictions.
    return d->publicPosition(d->pos);
}

void QRangeModel::setPosition(qreal newPosition)
{
    Q_D(QRangeModel);

    if (qFuzzyCompare(newPosition, d->pos))
        return;

    const qreal oldPosition = position();
    const qreal oldValue = value();

    // Update position and calculate new value
    d->pos = newPosition;
    d->value = d->equivalentValue(d->pos);
    d->emitValueAndPositionIfChanged(oldValue, oldPosition);
}

/*!
    \property QRangeModel::positionAtMinimum
    \brief the minimum value that \l position can assume

    This property's default value is 0
*/

void QRangeModel::setPositionAtMinimum(qreal min)
{
    Q_D(QRangeModel);
    setPositionRange(min, d->posatmax);
}

qreal QRangeModel::positionAtMinimum() const
{
    Q_D(const QRangeModel);
    return d->posatmin;
}

/*!
    \property QRangeModel::positionAtMaximum
    \brief the maximum value that \l position can assume

    This property's default value is 0
*/

void QRangeModel::setPositionAtMaximum(qreal max)
{
    Q_D(QRangeModel);
    setPositionRange(d->posatmin, max);
}

qreal QRangeModel::positionAtMaximum() const
{
    Q_D(const QRangeModel);
    return d->posatmax;
}

/*!
    Returns a valid value, respecting the \l minimumValue,
    \l maximumValue and the \l stepSize properties.
    Such calculation is based on the parameter \a position (which is valid externally).
*/

qreal QRangeModel::valueForPosition(qreal position) const
{
    Q_D(const QRangeModel);

    const qreal unconstrainedValue = d->equivalentValue(position);
    return d->publicValue(unconstrainedValue);
}

/*!
    \property QRangeModel::value
    \brief the current value of the model

    Represents a valid external value, based on the \l minimumValue,
    \l maximumValue and the \l stepSize properties.
    The user can set it internally with a value, that is not within the current range,
    since it can become valid if the user changes the range later.
*/

qreal QRangeModel::value() const
{
    Q_D(const QRangeModel);

    // Return internal value but observe boundaries and
    // stepSize restrictions
    return d->publicValue(d->value);
}

void QRangeModel::setValue(qreal newValue)
{
    Q_D(QRangeModel);

    if (qFuzzyCompare(newValue, d->value))
        return;

    const qreal oldValue = value();
    const qreal oldPosition = position();

    // Update relative value and position
    d->value = newValue;
    d->pos = d->equivalentPosition(d->value);
    d->emitValueAndPositionIfChanged(oldValue, oldPosition);
}

/*!
    \property QRangeModel::inverted
    \brief the model is inverted or not

    The model can be represented with an inverted behavior, e.g. when \l value assumes
    the maximum value (represented by \l maximumValue) the \l position will be at its
    minimum (represented by \l positionAtMinimum).
*/

void QRangeModel::setInverted(bool inverted)
{
    Q_D(QRangeModel);
    if (inverted == d->inverted)
        return;

    d->inverted = inverted;
    emit invertedChanged(d->inverted);

    // After updating the internal value, the position property can change.
    setPosition(d->equivalentPosition(d->value));
}

bool QRangeModel::inverted() const
{
    Q_D(const QRangeModel);
    return d->inverted;
}

/*!
    Sets the \l value to \l minimumValue.
*/

void QRangeModel::toMinimum()
{
    Q_D(const QRangeModel);
    setValue(d->minimum);
}

/*!
    Sets the \l value to \l maximumValue.
*/

void QRangeModel::toMaximum()
{
    Q_D(const QRangeModel);
    setValue(d->maximum);
}
