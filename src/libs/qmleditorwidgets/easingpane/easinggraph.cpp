/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "easinggraph.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <math.h>

QT_BEGIN_NAMESPACE

EasingGraph::EasingGraph(QWidget *parent):QWidget(parent)
{
//    setFlag(QGraphicsItem::ItemHasNoContents, false);

    // populate the hash
    m_availableNames["Linear"]=QEasingCurve::Linear;
    m_availableNames["InQuad"]=QEasingCurve::InQuad;
    m_availableNames["OutQuad"]=QEasingCurve::OutQuad;
    m_availableNames["InOutQuad"]=QEasingCurve::InOutQuad;
    m_availableNames["OutInQuad"]=QEasingCurve::OutInQuad;
    m_availableNames["InCubic"]=QEasingCurve::InCubic;
    m_availableNames["OutCubic"]=QEasingCurve::OutCubic;
    m_availableNames["InOutCubic"]=QEasingCurve::InOutCubic;
    m_availableNames["OutInCubic"]=QEasingCurve::OutInCubic;
    m_availableNames["InQuart"]=QEasingCurve::InQuart;
    m_availableNames["OutQuart"]=QEasingCurve::OutQuart;
    m_availableNames["InOutQuart"]=QEasingCurve::InOutQuart;
    m_availableNames["OutInQuart"]=QEasingCurve::OutInQuart;
    m_availableNames["InQuint"]=QEasingCurve::InQuint;
    m_availableNames["OutQuint"]=QEasingCurve::OutQuint;
    m_availableNames["InOutQuint"]=QEasingCurve::InOutQuint;
    m_availableNames["OutInQuint"]=QEasingCurve::OutInQuint;
    m_availableNames["InSine"]=QEasingCurve::InSine;
    m_availableNames["OutSine"]=QEasingCurve::OutSine;
    m_availableNames["InOutSine"]=QEasingCurve::InOutSine;
    m_availableNames["OutInSine"]=QEasingCurve::OutInSine;
    m_availableNames["InExpo"]=QEasingCurve::InExpo;
    m_availableNames["OutExpo"]=QEasingCurve::OutExpo;
    m_availableNames["InOutExpo"]=QEasingCurve::InOutExpo;
    m_availableNames["OutInExpo"]=QEasingCurve::OutInExpo;
    m_availableNames["InCirc"]=QEasingCurve::InCirc;
    m_availableNames["OutCirc"]=QEasingCurve::OutCirc;
    m_availableNames["InOutCirc"]=QEasingCurve::InOutCirc;
    m_availableNames["OutInCirc"]=QEasingCurve::OutInCirc;
    m_availableNames["InElastic"]=QEasingCurve::InElastic;
    m_availableNames["OutElastic"]=QEasingCurve::OutElastic;
    m_availableNames["InOutElastic"]=QEasingCurve::InOutElastic;
    m_availableNames["OutInElastic"]=QEasingCurve::OutInElastic;
    m_availableNames["InBack"]=QEasingCurve::InBack;
    m_availableNames["OutBack"]=QEasingCurve::OutBack;
    m_availableNames["InOutBack"]=QEasingCurve::InOutBack;
    m_availableNames["OutInBack"]=QEasingCurve::OutInBack;
    m_availableNames["InBounce"]=QEasingCurve::InBounce;
    m_availableNames["OutBounce"]=QEasingCurve::OutBounce;
    m_availableNames["InOutBounce"]=QEasingCurve::InOutBounce;
    m_availableNames["OutInBounce"]=QEasingCurve::OutInBounce;
    m_availableNames["InCurve"]=QEasingCurve::InCurve;
    m_availableNames["OutCurve"]=QEasingCurve::OutCurve;
    m_availableNames["SineCurve"]=QEasingCurve::SineCurve;
    m_availableNames["CosineCurve"]=QEasingCurve::CosineCurve;

    m_color = Qt::magenta;
    m_zeroColor = Qt::gray;
    m_easingExtremes = "In";
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
    if (name.left(5)=="InOut") return name.right(name.length()-5);
    if (name.left(5)=="OutIn") return name.right(name.length()-5);
    if (name.left(3)=="Out") return name.right(name.length()-3);
    if (name.left(2)=="In") return name.right(name.length()-2);
    return name;
}

void EasingGraph::setEasingShape(const QString &newShape)
{
    if (easingShape() != newShape) {
        if (newShape=="Linear")
            setEasingName(newShape);
        else
            setEasingName(m_easingExtremes+newShape);
    }
}

QString EasingGraph::easingExtremes() const
{
    QString name = easingName();
    if (name.left(5)=="InOut") return "InOut";
    if (name.left(5)=="OutIn") return "OutIn";
    if (name.left(3)=="Out") return "Out";
    if (name.left(2)=="In") return "In";
    return QString();
}

void EasingGraph::setEasingExtremes(const QString &newExtremes)
{
    if (m_easingExtremes != newExtremes) {
        m_easingExtremes = newExtremes;
        if (easingShape()!="Linear")
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
    if ((overshoot() != newOvershoot) && (easingShape()=="Back")) {
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
    if ((amplitude() != newAmplitude) && ((easingShape()=="Bounce") ||(easingShape()=="Elastic"))) {
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
    if ((period() != newPeriod) && (easingShape()=="Elastic")) {
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
