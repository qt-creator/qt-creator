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

#include "easinggraph.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <math.h>

QT_BEGIN_NAMESPACE

EasingGraph::EasingGraph(QWidget *parent):QWidget(parent),
    m_color(Qt::magenta), m_zeroColor(Qt::gray),m_duration(0),
    m_easingExtremes(QLatin1String("In"))
{
//    setFlag(QGraphicsItem::ItemHasNoContents, false);

    // populate the hash
    m_availableNames.insert(QLatin1String("Linear"), QEasingCurve::Linear);
    m_availableNames.insert(QLatin1String("InQuad"), QEasingCurve::InQuad);
    m_availableNames.insert(QLatin1String("OutQuad"), QEasingCurve::OutQuad);
    m_availableNames.insert(QLatin1String("InOutQuad"), QEasingCurve::InOutQuad);
    m_availableNames.insert(QLatin1String("OutInQuad"), QEasingCurve::OutInQuad);
    m_availableNames.insert(QLatin1String("InCubic"), QEasingCurve::InCubic);
    m_availableNames.insert(QLatin1String("OutCubic"), QEasingCurve::OutCubic);
    m_availableNames.insert(QLatin1String("InOutCubic"), QEasingCurve::InOutCubic);
    m_availableNames.insert(QLatin1String("OutInCubic"), QEasingCurve::OutInCubic);
    m_availableNames.insert(QLatin1String("InQuart"), QEasingCurve::InQuart);
    m_availableNames.insert(QLatin1String("OutQuart"), QEasingCurve::OutQuart);
    m_availableNames.insert(QLatin1String("InOutQuart"), QEasingCurve::InOutQuart);
    m_availableNames.insert(QLatin1String("OutInQuart"), QEasingCurve::OutInQuart);
    m_availableNames.insert(QLatin1String("InQuint"), QEasingCurve::InQuint);
    m_availableNames.insert(QLatin1String("OutQuint"), QEasingCurve::OutQuint);
    m_availableNames.insert(QLatin1String("InOutQuint"), QEasingCurve::InOutQuint);
    m_availableNames.insert(QLatin1String("OutInQuint"), QEasingCurve::OutInQuint);
    m_availableNames.insert(QLatin1String("InSine"), QEasingCurve::InSine);
    m_availableNames.insert(QLatin1String("OutSine"), QEasingCurve::OutSine);
    m_availableNames.insert(QLatin1String("InOutSine"), QEasingCurve::InOutSine);
    m_availableNames.insert(QLatin1String("OutInSine"), QEasingCurve::OutInSine);
    m_availableNames.insert(QLatin1String("InExpo"), QEasingCurve::InExpo);
    m_availableNames.insert(QLatin1String("OutExpo"), QEasingCurve::OutExpo);
    m_availableNames.insert(QLatin1String("InOutExpo"), QEasingCurve::InOutExpo);
    m_availableNames.insert(QLatin1String("OutInExpo"), QEasingCurve::OutInExpo);
    m_availableNames.insert(QLatin1String("InCirc"), QEasingCurve::InCirc);
    m_availableNames.insert(QLatin1String("OutCirc"), QEasingCurve::OutCirc);
    m_availableNames.insert(QLatin1String("InOutCirc"), QEasingCurve::InOutCirc);
    m_availableNames.insert(QLatin1String("OutInCirc"), QEasingCurve::OutInCirc);
    m_availableNames.insert(QLatin1String("InElastic"), QEasingCurve::InElastic);
    m_availableNames.insert(QLatin1String("OutElastic"), QEasingCurve::OutElastic);
    m_availableNames.insert(QLatin1String("InOutElastic"), QEasingCurve::InOutElastic);
    m_availableNames.insert(QLatin1String("OutInElastic"), QEasingCurve::OutInElastic);
    m_availableNames.insert(QLatin1String("InBack"), QEasingCurve::InBack);
    m_availableNames.insert(QLatin1String("OutBack"), QEasingCurve::OutBack);
    m_availableNames.insert(QLatin1String("InOutBack"), QEasingCurve::InOutBack);
    m_availableNames.insert(QLatin1String("OutInBack"), QEasingCurve::OutInBack);
    m_availableNames.insert(QLatin1String("InBounce"), QEasingCurve::InBounce);
    m_availableNames.insert(QLatin1String("OutBounce"), QEasingCurve::OutBounce);
    m_availableNames.insert(QLatin1String("InOutBounce"), QEasingCurve::InOutBounce);
    m_availableNames.insert(QLatin1String("OutInBounce"), QEasingCurve::OutInBounce);
    m_availableNames.insert(QLatin1String("InCurve"), QEasingCurve::InCurve);
    m_availableNames.insert(QLatin1String("OutCurve"), QEasingCurve::OutCurve);
    m_availableNames.insert(QLatin1String("SineCurve"), QEasingCurve::SineCurve);
    m_availableNames.insert(QLatin1String("CosineCurve"), QEasingCurve::CosineCurve);
}

EasingGraph::~EasingGraph()
{
}

QEasingCurve::Type EasingGraph::easingType() const
{
    return m_curveFunction.type();
}

QEasingCurve EasingGraph::easingCurve() const
{
    return m_curveFunction;
}

QString EasingGraph::easingShape() const
{
    QString name = easingName();
    if (name.left(5)==QLatin1String("InOut")) return name.right(name.length()-5);
    if (name.left(5)==QLatin1String("OutIn")) return name.right(name.length()-5);
    if (name.left(3)==QLatin1String("Out")) return name.right(name.length()-3);
    if (name.left(2)==QLatin1String("In")) return name.right(name.length()-2);
    return name;
}

void EasingGraph::setEasingShape(const QString &newShape)
{
    if (easingShape() != newShape) {
        if (newShape==QLatin1String("Linear"))
            setEasingName(newShape);
        else
            setEasingName(m_easingExtremes+newShape);
    }
}

QString EasingGraph::easingExtremes() const
{
    QString name = easingName();
    if (name.left(5)==QLatin1String("InOut")) return QLatin1String("InOut");
    if (name.left(5)==QLatin1String("OutIn")) return QLatin1String("OutIn");
    if (name.left(3)==QLatin1String("Out")) return QLatin1String("Out");
    if (name.left(2)==QLatin1String("In")) return QLatin1String("In");
    return QString();
}

void EasingGraph::setEasingExtremes(const QString &newExtremes)
{
    if (m_easingExtremes != newExtremes) {
        m_easingExtremes = newExtremes;
        if (easingShape()!=QLatin1String("Linear"))
            setEasingName(newExtremes+easingShape());
    }
}

QString EasingGraph::easingName() const
{
    return m_availableNames.key(m_curveFunction.type());
}

void EasingGraph::setEasingName(const QString &newName)
{
    if (easingName() != newName) {

        if (!m_availableNames.contains(newName)) return;
        m_curveFunction = QEasingCurve(m_availableNames.value(newName));
        emit easingNameChanged();
        emit easingExtremesChanged();
        emit easingShapeChanged();
        update();
    }
}

qreal EasingGraph::overshoot() const
{
    return m_curveFunction.overshoot();
}

void EasingGraph::setOvershoot(qreal newOvershoot)
{
    if ((overshoot() != newOvershoot) && (easingShape()==QLatin1String("Back"))) {
        m_curveFunction.setOvershoot(newOvershoot);
        emit overshootChanged();
        update();
    }
}
qreal EasingGraph::amplitude() const
{
    return m_curveFunction.amplitude();
}

void EasingGraph::setAmplitude(qreal newAmplitude)
{
    if ((amplitude() != newAmplitude) && ((easingShape()==QLatin1String("Bounce")) ||(easingShape()==QLatin1String("Elastic")))) {
        m_curveFunction.setAmplitude(newAmplitude);
        emit amplitudeChanged();
        update();
    }
}

qreal EasingGraph::period() const
{
    return m_curveFunction.period();
}

void EasingGraph::setPeriod(qreal newPeriod)
{
    if ((period() != newPeriod) && (easingShape()==QLatin1String("Elastic"))) {
        m_curveFunction.setPeriod(newPeriod);
        emit periodChanged();
        update();
    }
}

qreal EasingGraph::duration() const
{
    return m_duration;
}

void EasingGraph::setDuration(qreal newDuration)
{
    if (m_duration != newDuration) {
        m_duration = newDuration;
        emit durationChanged();
    }
}

QColor EasingGraph::color() const
{
    return m_color;
}

void EasingGraph::setColor(const QColor &newColor)
{
    if (m_color != newColor) {
        m_color = newColor;
        emit colorChanged();
        update();
    }
}

QColor EasingGraph::zeroColor() const{
    return m_zeroColor;
}

void EasingGraph::setZeroColor(const QColor &newColor)
{
    if (m_zeroColor != newColor) {
        m_zeroColor = newColor;
        emit zeroColorChanged();
        update();
    }
}

QRectF EasingGraph::boundingRect() const
{
    return QRectF(0, 0, width(), height());
}

void EasingGraph::paintEvent(QPaintEvent *event)
//void EasingGraph::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QWidget::paintEvent(event);

    QPainter *painter = new QPainter(this);
    painter->save();
    bool drawZero = false;

    // no background

    int length = width();
    int breadth = height()-2;
    QPainterPath path;
    path.moveTo(0,int((1-m_curveFunction.valueForProgress(0))*breadth));
    for (int i=0;i<length;i++) {
        qreal progress = i/qreal(length);
        qreal value = m_curveFunction.valueForProgress(progress);
        int x = int(length*progress);
        int y = int(breadth*(1-value));
        path.lineTo(x,y);
    }

    QRectF pathRect = path.controlPointRect();
    if ( (pathRect.height()>breadth)) {
        // scale vertically
        qreal scale = breadth/pathRect.height();
        qreal displacement = -pathRect.top();

        // reset path and recompute scaled version
        path = QPainterPath();
        path.moveTo(0,int(scale*((1-m_curveFunction.valueForProgress(0))*breadth+displacement)));
        for (int i=0;i<length;i++) {
            qreal progress = i/qreal(length);
            qreal value = m_curveFunction.valueForProgress(progress);
            int x = int(length*progress);
            int y = int(scale*(breadth*(1-value)+displacement));
            path.lineTo(x,y);
        }

        drawZero = true;
    }


    painter->setBrush(Qt::transparent);

    if (drawZero) {
        // "zero" and "one" lines
        QPen zeroPen = QPen(m_zeroColor);
        zeroPen.setStyle(Qt::DashLine);
        painter->setPen(zeroPen);
        int y = int(-pathRect.top()*breadth/pathRect.height());
        if (y>0)
            painter->drawLine(0,y,length,y);
        y = int(breadth/pathRect.height()*(breadth-pathRect.top()));
        if (y<breadth)
            painter->drawLine(0,y,length,y);
    }

    painter->setPen(m_color);
    painter->drawPath(path);
    painter->restore();
    delete painter;
}

QT_END_NAMESPACE
