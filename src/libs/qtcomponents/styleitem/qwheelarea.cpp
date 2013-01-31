/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOTgall
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qwheelarea.h"


QWheelArea::QWheelArea(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
      _horizontalMinimumValue(0),
      _horizontalMaximumValue(0),
      _verticalMinimumValue(0),
      _verticalMaximumValue(0),
      _horizontalValue(0),
      _verticalValue(0),
      _verticalDelta(0),
      _horizontalDelta(0)
{}

QWheelArea::~QWheelArea() {}

bool QWheelArea::event (QEvent * e) {
    switch (e->type()) {
    case QEvent::GraphicsSceneWheel: {
        QGraphicsSceneWheelEvent *we = static_cast<QGraphicsSceneWheelEvent*>(e);
        if (we) {
            switch (we->orientation()) {
                case Qt::Horizontal:
                    setHorizontalDelta(we->delta());
                    break;
                case Qt::Vertical:
                    setVerticalDelta(we->delta());
            }
            return true;
        }
    }
    case QEvent::Wheel: {
        QWheelEvent *we = static_cast<QWheelEvent*>(e);
        if (we) {
            switch (we->orientation()) {
                case Qt::Horizontal:
                    setHorizontalDelta(we->delta());

                    break;
                case Qt::Vertical:
                    setVerticalDelta(we->delta());

            }
            return true;
        }
    }
    default: break;
    }
    return QDeclarativeItem::event(e);
}

void QWheelArea::setHorizontalMinimumValue(qreal min)
{
    _horizontalMinimumValue = min;
}

qreal QWheelArea::horizontalMinimumValue() const
{
    return _horizontalMinimumValue;
}

void QWheelArea::setHorizontalMaximumValue(qreal max)
{
    _horizontalMaximumValue = max;
}
qreal QWheelArea::horizontalMaximumValue() const
{
    return _horizontalMaximumValue;
}

void QWheelArea::setVerticalMinimumValue(qreal min)
{
    _verticalMinimumValue = min;
}

qreal QWheelArea::verticalMinimumValue() const
{
    return _verticalMinimumValue;
}

void QWheelArea::setVerticalMaximumValue(qreal max)
{
    _verticalMaximumValue = max;
}

qreal QWheelArea::verticalMaximumValue() const
{
    return _verticalMaximumValue;
}

void QWheelArea::setHorizontalValue(qreal val)
{
    if (val > _horizontalMaximumValue)
       _horizontalValue = _horizontalMaximumValue;
    else if (val < _horizontalMinimumValue)
        _horizontalValue = _horizontalMinimumValue;
    else
        _horizontalValue = val;
    emit(horizontalValueChanged());
}

qreal QWheelArea::horizontalValue() const
{
    return _horizontalValue;
}

void QWheelArea::setVerticalValue(qreal val)
{
    if (val > _verticalMaximumValue)
       _verticalValue = _verticalMaximumValue;
    else if (val < _verticalMinimumValue)
        _verticalValue = _verticalMinimumValue;
    else
        _verticalValue = val;
    emit(verticalValueChanged());
}

qreal QWheelArea::verticalValue() const
{
    return _verticalValue;
}

void QWheelArea::setVerticalDelta(qreal d)
{
    _verticalDelta = d/5;
    setVerticalValue(_verticalValue - _verticalDelta);
    emit(verticalWheelMoved());
}

qreal QWheelArea::verticalDelta() const
{
    return _verticalDelta;
}

void QWheelArea::setHorizontalDelta(qreal d)
{
    _horizontalDelta = d/5;
    setHorizontalValue(_horizontalValue - _horizontalDelta);
    emit(horizontalWheelMoved());
}

qreal QWheelArea::horizontalDelta() const
{
    return _horizontalDelta;
}
