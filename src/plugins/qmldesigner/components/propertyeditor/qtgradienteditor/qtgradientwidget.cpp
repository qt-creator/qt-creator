/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtgradientwidget.h"
#include <QtCore/QMap>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QMouseEvent>

#define _USE_MATH_DEFINES


#include "math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QT_BEGIN_NAMESPACE

class QtGradientWidgetPrivate
{
    QtGradientWidget *q_ptr;
    Q_DECLARE_PUBLIC(QtGradientWidget)
public:
    QPointF fromViewport(const QPointF &point) const;
    QPointF toViewport(const QPointF &point) const;
//    void setupDrag(QtGradientStop *stop, int x);

    QPointF checkRange(const QPointF &point) const;
    QRectF pointRect(const QPointF &point, double size) const;

    double correctAngle(double angle) const;
    void setAngleConical(double angle);

    void paintPoint(QPainter *painter, const QPointF &point, double size) const;

    double m_handleSize;
    bool m_backgroundCheckered;

    QGradientStops m_gradientStops;
    QGradient::Type m_gradientType;
    QGradient::Spread m_gradientSpread;
    QPointF m_startLinear;
    QPointF m_endLinear;
    QPointF m_centralRadial;
    QPointF m_focalRadial;
    qreal m_radiusRadial;
    QPointF m_centralConical;
    qreal m_angleConical;

    enum Handle {
        NoHandle,
        StartLinearHandle,
        EndLinearHandle,
        CentralRadialHandle,
        FocalRadialHandle,
        RadiusRadialHandle,
        CentralConicalHandle,
        AngleConicalHandle
    };

    Handle m_dragHandle;
    QPointF m_dragOffset;
    //double m_radiusOffset;
    double m_radiusFactor;
    double m_dragRadius;
    double m_angleOffset;
    double m_dragAngle;
};

double QtGradientWidgetPrivate::correctAngle(double angle) const
{
    double a = angle;
    while (a >= 360)
        a -= 360;
    while (a < 0)
        a += 360;
    return a;
}

void QtGradientWidgetPrivate::setAngleConical(double angle)
{
    double a = correctAngle(angle);
    if (m_angleConical == a)
        return;
    m_angleConical = a;
    emit q_ptr->angleConicalChanged(m_angleConical);
}

QRectF QtGradientWidgetPrivate::pointRect(const QPointF &point, double size) const
{
    return QRectF(point.x() - size / 2, point.y() - size / 2, size, size);
}

QPointF QtGradientWidgetPrivate::checkRange(const QPointF &point) const
{
    QPointF p = point;
    if (p.x() > 1)
        p.setX(1);
    else if (p.x() < 0)
        p.setX(0);
    if (p.y() > 1)
        p.setY(1);
    else if (p.y() < 0)
        p.setY(0);
    return p;
}

QPointF QtGradientWidgetPrivate::fromViewport(const QPointF &point) const
{
    QSize size = q_ptr->size();
    return QPointF(point.x() / size.width(), point.y() / size.height());
}

QPointF QtGradientWidgetPrivate::toViewport(const QPointF &point) const
{
    QSize size = q_ptr->size();
    return QPointF(point.x() * size.width(), point.y() * size.height());
}

void QtGradientWidgetPrivate::paintPoint(QPainter *painter, const QPointF &point, double size) const
{
    QPointF pf = toViewport(point);
    QRectF rf = pointRect(pf, size);

    QPen pen;
    pen.setWidthF(1);
    QColor alphaZero = Qt::white;
    alphaZero.setAlpha(0);

    painter->save();
    painter->drawEllipse(rf);

    /*
    painter->save();

    QLinearGradient lgV(0, rf.top(), 0, rf.bottom());
    lgV.setColorAt(0, alphaZero);
    lgV.setColorAt(0.25, Qt::white);
    lgV.setColorAt(0.25, Qt::white);
    lgV.setColorAt(1, alphaZero);
    pen.setBrush(lgV);
    painter->setPen(pen);

    painter->drawLine(QPointF(pf.x(), rf.top()), QPointF(pf.x(), rf.bottom()));

    QLinearGradient lgH(rf.left(), 0, rf.right(), 0);
    lgH.setColorAt(0, alphaZero);
    lgH.setColorAt(0.5, Qt::white);
    lgH.setColorAt(1, alphaZero);
    pen.setBrush(lgH);
    painter->setPen(pen);

    painter->drawLine(QPointF(rf.left(), pf.y()), QPointF(rf.right(), pf.y()));

    painter->restore();
    */

    painter->restore();
}

/*
void QtGradientWidgetPrivate::setupDrag(QtGradientStop *stop, int x)
{
    m_model->setCurrentStop(stop);

    int viewportX = qRound(toViewport(stop->position()));
    m_dragOffset = x - viewportX;

    QList<QtGradientStop *> stops = m_stops;
    m_stops.clear();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (m_model->isSelected(s) || s == stop) {
            m_dragStops[s] = s->position() - stop->position();
            m_stops.append(s);
        } else {
            m_dragOriginal[s->position()] = s->color();
        }
    }
    itStop.toFront();
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (!m_model->isSelected(s))
            m_stops.append(s);
    }
    m_stops.removeAll(stop);
    m_stops.prepend(stop);
}
*/
////////////////////////////

QtGradientWidget::QtGradientWidget(QWidget *parent)
    : QWidget(parent), d_ptr(new QtGradientWidgetPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->m_backgroundCheckered = true;
    d_ptr->m_handleSize = 20.0;
    d_ptr->m_gradientType = QGradient::LinearGradient;
    d_ptr->m_startLinear = QPointF(0, 0);
    d_ptr->m_endLinear = QPointF(1, 1);
    d_ptr->m_centralRadial = QPointF(0.5, 0.5);
    d_ptr->m_focalRadial = QPointF(0.5, 0.5);
    d_ptr->m_radiusRadial = 0.5;
    d_ptr->m_centralConical = QPointF(0.5, 0.5);
    d_ptr->m_angleConical = 0;
    d_ptr->m_dragHandle = QtGradientWidgetPrivate::NoHandle;

    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
}

QtGradientWidget::~QtGradientWidget()
{
}

QSize QtGradientWidget::sizeHint() const
{
    return QSize(176, 176);
}

QSize QtGradientWidget::minimumSizeHint() const
{
    return QSize(128, 128);
}

int QtGradientWidget::heightForWidth(int w) const
{
    return w;
}

void QtGradientWidget::setBackgroundCheckered(bool checkered)
{
    if (d_ptr->m_backgroundCheckered == checkered)
        return;
    d_ptr->m_backgroundCheckered = checkered;
    update();
}

bool QtGradientWidget::isBackgroundCheckered() const
{
    return d_ptr->m_backgroundCheckered;
}

void QtGradientWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    QPoint p = e->pos();
    if (d_ptr->m_gradientType == QGradient::LinearGradient) {
        QPointF startPoint = d_ptr->toViewport(d_ptr->m_startLinear);
        double x = p.x() - startPoint.x();
        double y = p.y() - startPoint.y();

        if ((d_ptr->m_handleSize * d_ptr->m_handleSize / 4) > (x * x + y * y)) {
            d_ptr->m_dragHandle = QtGradientWidgetPrivate::StartLinearHandle;
            d_ptr->m_dragOffset = QPointF(x, y);
            update();
            return;
        }

        QPointF endPoint = d_ptr->toViewport(d_ptr->m_endLinear);
        x = p.x() - endPoint.x();
        y = p.y() - endPoint.y();

        if ((d_ptr->m_handleSize * d_ptr->m_handleSize / 4) > (x * x + y * y)) {
            d_ptr->m_dragHandle = QtGradientWidgetPrivate::EndLinearHandle;
            d_ptr->m_dragOffset = QPointF(x, y);
            update();
            return;
        }
    } else if (d_ptr->m_gradientType == QGradient::RadialGradient) {
        QPointF focalPoint = d_ptr->toViewport(d_ptr->m_focalRadial);
        double x = p.x() - focalPoint.x();
        double y = p.y() - focalPoint.y();

        if ((d_ptr->m_handleSize * d_ptr->m_handleSize / 9) > (x * x + y * y)) {
            d_ptr->m_dragHandle = QtGradientWidgetPrivate::FocalRadialHandle;
            d_ptr->m_dragOffset = QPointF(x, y);
            update();
            return;
        }

        QPointF centralPoint = d_ptr->toViewport(d_ptr->m_centralRadial);
        x = p.x() - centralPoint.x();
        y = p.y() - centralPoint.y();

        if ((d_ptr->m_handleSize * d_ptr->m_handleSize / 4) > (x * x + y * y)) {
            d_ptr->m_dragHandle = QtGradientWidgetPrivate::CentralRadialHandle;
            d_ptr->m_dragOffset = QPointF(x, y);
            update();
            return;
        }

        QPointF central = d_ptr->toViewport(d_ptr->m_centralRadial);
        QRectF r = d_ptr->pointRect(central, 2 * d_ptr->m_handleSize / 3);
        QRectF r1(0, r.y(), size().width(), r.height());
        QRectF r2(r.x(), 0, r.width(), r.y());
        QRectF r3(r.x(), r.y() + r.height(), r.width(), size().height() - r.y() - r.height());
        QPointF pF(p.x(), p.y());
        if (r1.contains(pF) || r2.contains(pF) || r3.contains(pF)) {
            x = pF.x() / size().width() - d_ptr->m_centralRadial.x();
            y = pF.y() / size().height() - d_ptr->m_centralRadial.y();
            double clickRadius = sqrt(x * x + y * y);
            //d_ptr->m_radiusOffset = d_ptr->m_radiusRadial - clickRadius;
            d_ptr->m_radiusFactor = d_ptr->m_radiusRadial / clickRadius;
            if (d_ptr->m_radiusFactor == 0)
                d_ptr->m_radiusFactor = 1;
            d_ptr->m_dragRadius = d_ptr->m_radiusRadial;
            d_ptr->m_dragHandle = QtGradientWidgetPrivate::RadiusRadialHandle;
            mouseMoveEvent(e);
            update();
            return;
        }
    } else if (d_ptr->m_gradientType == QGradient::ConicalGradient) {
        QPointF centralPoint = d_ptr->toViewport(d_ptr->m_centralConical);
        double x = p.x() - centralPoint.x();
        double y = p.y() - centralPoint.y();

        if ((d_ptr->m_handleSize * d_ptr->m_handleSize / 4) > (x * x + y * y)) {
            d_ptr->m_dragHandle = QtGradientWidgetPrivate::CentralConicalHandle;
            d_ptr->m_dragOffset = QPointF(x, y);
            update();
            return;
        }
        double radius = size().width();
        if (size().height() < radius)
            radius = size().height();
        radius /= 2;
        double corr = d_ptr->m_handleSize / 3;
        radius -= corr;
        QPointF vp = d_ptr->toViewport(d_ptr->m_centralConical);
        x = p.x() - vp.x();
        y = p.y() - vp.y();
        if (((radius - corr) * (radius - corr) < (x * x + y * y)) &&
            ((radius + corr) * (radius + corr) > (x * x + y * y))) {
            QPointF central = d_ptr->toViewport(d_ptr->m_centralConical);
            QPointF current(e->pos().x(), e->pos().y());
            x = current.x() - central.x();
            y = current.y() - central.y();
            x /= size().width() / 2;
            y /= size().height() / 2;
            double r = sqrt(x * x + y * y);

            double arcSin = asin(y / r);
            double arcCos = acos(x / r);

            double angle = arcCos * 180 / M_PI;
            if (arcSin > 0) {
                angle = -angle;
            }

            d_ptr->m_angleOffset = d_ptr->m_angleConical - angle;
            d_ptr->m_dragAngle = d_ptr->m_angleConical;
            d_ptr->m_dragHandle = QtGradientWidgetPrivate::AngleConicalHandle;
            update();
            return;
        }
    }
}

void QtGradientWidget::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    d_ptr->m_dragHandle = QtGradientWidgetPrivate::NoHandle;
    update();
}

void QtGradientWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::NoHandle)
        return;

    QPointF newPos = QPointF((double)e->pos().x() - d_ptr->m_dragOffset.x(),
                (double)e->pos().y() - d_ptr->m_dragOffset.y());
    QPointF newPoint = d_ptr->fromViewport(newPos);
    if (newPoint.x() < 0)
        newPoint.setX(0);
    else if (newPoint.x() > 1)
        newPoint.setX(1);
    if (newPoint.y() < 0)
        newPoint.setY(0);
    else if (newPoint.y() > 1)
        newPoint.setY(1);

    if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::StartLinearHandle) {
        d_ptr->m_startLinear = newPoint;
        emit startLinearChanged(newPoint);
    } else if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::EndLinearHandle) {
        d_ptr->m_endLinear = newPoint;
        emit endLinearChanged(newPoint);
    } else if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::CentralRadialHandle) {
        d_ptr->m_centralRadial = newPoint;
        emit centralRadialChanged(newPoint);
    } else if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::FocalRadialHandle) {
        d_ptr->m_focalRadial = newPoint;
        emit focalRadialChanged(newPoint);
    } else if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::RadiusRadialHandle) {
        QPointF centralPoint = d_ptr->toViewport(d_ptr->m_centralRadial);
        QPointF pF(e->pos().x(), e->pos().y());
        double x = pF.x() - centralPoint.x();
        double y = pF.y() - centralPoint.y();

        if ((d_ptr->m_handleSize * d_ptr->m_handleSize / 4) > (x * x + y * y)) {
            if (d_ptr->m_radiusRadial != d_ptr->m_dragRadius) {
                d_ptr->m_radiusRadial = d_ptr->m_dragRadius;
                emit radiusRadialChanged(d_ptr->m_radiusRadial);
            }
        } else {
            x = pF.x() / size().width() - d_ptr->m_centralRadial.x();
            y = pF.y() / size().height() - d_ptr->m_centralRadial.y();
            double moveRadius = sqrt(x * x + y * y);
            //double newRadius = moveRadius + d_ptr->m_radiusOffset;
            double newRadius = moveRadius * d_ptr->m_radiusFactor;
            if (newRadius > 2)
                newRadius = 2;
            d_ptr->m_radiusRadial = newRadius;
            emit radiusRadialChanged(d_ptr->m_radiusRadial);
        }
    } else if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::CentralConicalHandle) {
        d_ptr->m_centralConical = newPoint;
        emit centralConicalChanged(newPoint);
    } else if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::AngleConicalHandle) {
        QPointF centralPoint = d_ptr->toViewport(d_ptr->m_centralConical);
        QPointF pF(e->pos().x(), e->pos().y());
        double x = pF.x() - centralPoint.x();
        double y = pF.y() - centralPoint.y();

        if ((d_ptr->m_handleSize * d_ptr->m_handleSize / 4) > (x * x + y * y)) {
            if (d_ptr->m_angleConical != d_ptr->m_dragAngle) {
                d_ptr->m_angleConical = d_ptr->m_dragAngle;
                emit angleConicalChanged(d_ptr->m_angleConical);
            }
        } else {
            QPointF central = d_ptr->toViewport(d_ptr->m_centralConical);
            QPointF current = pF;
            x = current.x() - central.x();
            y = current.y() - central.y();
            x /= size().width() / 2;
            y /= size().height() / 2;
            double r = sqrt(x * x + y * y);

            double arcSin = asin(y / r);
            double arcCos = acos(x / r);

            double angle = arcCos * 180 / M_PI;
            if (arcSin > 0) {
                angle = -angle;
            }

            angle += d_ptr->m_angleOffset;

            d_ptr->setAngleConical(angle);
        }
    }
    update();
}

void QtGradientWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    mousePressEvent(e);
}

void QtGradientWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QPainter p(this);

    if (d_ptr->m_backgroundCheckered) {
        int pixSize = 40;
        QPixmap pm(2 * pixSize, 2 * pixSize);

        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::white);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::white);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::black);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::black);

        p.setBrushOrigin((size().width() % pixSize + pixSize) / 2, (size().height() % pixSize + pixSize) / 2);
        p.fillRect(rect(), pm);
        p.setBrushOrigin(0, 0);
    }

    QGradient *gradient = 0;
    switch (d_ptr->m_gradientType) {
        case QGradient::LinearGradient:
            gradient = new QLinearGradient(d_ptr->m_startLinear, d_ptr->m_endLinear);
            break;
        case QGradient::RadialGradient:
            gradient = new QRadialGradient(d_ptr->m_centralRadial, d_ptr->m_radiusRadial, d_ptr->m_focalRadial);
            break;
        case QGradient::ConicalGradient:
            gradient = new QConicalGradient(d_ptr->m_centralConical, d_ptr->m_angleConical);
            break;
        default:
            break;
    }
    if (!gradient)
        return;

    gradient->setStops(d_ptr->m_gradientStops);
    gradient->setSpread(d_ptr->m_gradientSpread);

    p.save();
    p.scale(size().width(), size().height());
    p.fillRect(QRect(0, 0, 1, 1), *gradient);
    p.restore();

    p.setRenderHint(QPainter::Antialiasing);

    QColor c = QColor::fromRgbF(0.5, 0.5, 0.5, 0.5);
    QBrush br(c);
    p.setBrush(br);
    QPen pen(Qt::white);
    pen.setWidthF(1);
    p.setPen(pen);
    QPen dragPen = pen;
    dragPen.setWidthF(2);
    if (d_ptr->m_gradientType == QGradient::LinearGradient) {
        p.save();
        if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::StartLinearHandle)
            p.setPen(dragPen);
        d_ptr->paintPoint(&p, d_ptr->m_startLinear, d_ptr->m_handleSize);
        p.restore();

        p.save();
        if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::EndLinearHandle)
            p.setPen(dragPen);
        d_ptr->paintPoint(&p, d_ptr->m_endLinear, d_ptr->m_handleSize);
        p.restore();
    } else if (d_ptr->m_gradientType == QGradient::RadialGradient) {
        QPointF central = d_ptr->toViewport(d_ptr->m_centralRadial);

        p.save();
        QRectF r = d_ptr->pointRect(central, 2 * d_ptr->m_handleSize / 3);
        QRectF r1(0, r.y(), size().width(), r.height());
        QRectF r2(r.x(), 0, r.width(), r.y());
        QRectF r3(r.x(), r.y() + r.height(), r.width(), size().height() - r.y() - r.height());
        p.fillRect(r1, c);
        p.fillRect(r2, c);
        p.fillRect(r3, c);
        p.setBrush(Qt::NoBrush);
        p.save();
        if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::CentralRadialHandle)
            p.setPen(dragPen);
        d_ptr->paintPoint(&p, d_ptr->m_centralRadial, d_ptr->m_handleSize);
        p.restore();

        QRectF rect = QRectF(central.x() - d_ptr->m_radiusRadial * size().width(),
                        central.y() - d_ptr->m_radiusRadial * size().height(),
                        2 * d_ptr->m_radiusRadial * size().width(),
                        2 * d_ptr->m_radiusRadial * size().height());
        p.setClipRect(r1);
        p.setClipRect(r2, Qt::UniteClip);
        p.setClipRect(r3, Qt::UniteClip);
        p.drawEllipse(rect);
        if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::RadiusRadialHandle) {
            p.save();
            p.setPen(dragPen);
            QRectF rect = QRectF(central.x() - d_ptr->m_radiusRadial / d_ptr->m_radiusFactor * size().width(),
                    central.y() - d_ptr->m_radiusRadial / d_ptr->m_radiusFactor * size().height(),
                    2 * d_ptr->m_radiusRadial / d_ptr->m_radiusFactor * size().width(),
                    2 * d_ptr->m_radiusRadial / d_ptr->m_radiusFactor * size().height());
            p.drawEllipse(rect);

            p.restore();
        }
        p.restore();

        p.save();
        if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::FocalRadialHandle)
            p.setPen(dragPen);
        d_ptr->paintPoint(&p, d_ptr->m_focalRadial, 2 * d_ptr->m_handleSize / 3);
        p.restore();
    } else if (d_ptr->m_gradientType == QGradient::ConicalGradient) {
        double radius = size().width();
        if (size().height() < radius)
            radius = size().height();
        radius /= 2;
        double corr = d_ptr->m_handleSize / 3;
        radius -= corr;
        QPointF central = d_ptr->toViewport(d_ptr->m_centralConical);

        p.save();
        p.setBrush(Qt::NoBrush);
        QPen pen2(c);
        pen2.setWidthF(2 * d_ptr->m_handleSize / 3);
        p.setPen(pen2);
        p.drawEllipse(d_ptr->pointRect(central, 2 * radius));
        p.restore();

        p.save();
        p.setBrush(Qt::NoBrush);
        int pointCount = 2;
        for (int i = 0; i < pointCount; i++) {
            QPointF ang(cos(M_PI * (i * 180.0 / pointCount + d_ptr->m_angleConical) / 180) * size().width() / 2,
                    -sin(M_PI * (i * 180.0 / pointCount + d_ptr->m_angleConical) / 180) * size().height() / 2);
            double mod = sqrt(ang.x() * ang.x() + ang.y() * ang.y());
            p.drawLine(QPointF(central.x() + ang.x() * (radius - corr) / mod,
                        central.y() + ang.y() * (radius - corr) / mod),
                    QPointF(central.x() + ang.x() * (radius + corr) / mod,
                        central.y() + ang.y() * (radius + corr) / mod));
            p.drawLine(QPointF(central.x() - ang.x() * (radius - corr) / mod,
                        central.y() - ang.y() * (radius - corr) / mod),
                    QPointF(central.x() - ang.x() * (radius + corr) / mod,
                        central.y() - ang.y() * (radius + corr) / mod));
        }
        if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::AngleConicalHandle) {
            p.save();
            p.setPen(dragPen);
            QPointF ang(cos(M_PI * (d_ptr->m_angleConical - d_ptr->m_angleOffset) / 180) * size().width() / 2,
                    -sin(M_PI * (d_ptr->m_angleConical - d_ptr->m_angleOffset) / 180) * size().height() / 2);
            double mod = sqrt(ang.x() * ang.x() + ang.y() * ang.y());
            p.drawLine(QPointF(central.x() + ang.x() * (radius - corr) / mod,
                        central.y() + ang.y() * (radius - corr) / mod),
                    QPointF(central.x() + ang.x() * (radius + corr) / mod,
                        central.y() + ang.y() * (radius + corr) / mod));
            p.restore();
        }

        p.restore();

        p.save();
        if (d_ptr->m_dragHandle == QtGradientWidgetPrivate::CentralConicalHandle)
            p.setPen(dragPen);
        d_ptr->paintPoint(&p, d_ptr->m_centralConical, d_ptr->m_handleSize);
        p.restore();

    }

    delete gradient;
}

void QtGradientWidget::setGradientStops(const QGradientStops &stops)
{
    d_ptr->m_gradientStops = stops;
    update();
}

QGradientStops QtGradientWidget::gradientStops() const
{
    return d_ptr->m_gradientStops;
}

void QtGradientWidget::setGradientType(QGradient::Type type)
{
    if (type == QGradient::NoGradient)
        return;
    if (d_ptr->m_gradientType == type)
        return;

    d_ptr->m_gradientType = type;
    update();
}

QGradient::Type QtGradientWidget::gradientType() const
{
    return d_ptr->m_gradientType;
}

void QtGradientWidget::setGradientSpread(QGradient::Spread spread)
{
    if (d_ptr->m_gradientSpread == spread)
        return;

    d_ptr->m_gradientSpread = spread;
    update();
}

QGradient::Spread QtGradientWidget::gradientSpread() const
{
    return d_ptr->m_gradientSpread;
}

void QtGradientWidget::setStartLinear(const QPointF &point)
{
    if (d_ptr->m_startLinear == point)
        return;

    d_ptr->m_startLinear = d_ptr->checkRange(point);
    update();
}

QPointF QtGradientWidget::startLinear() const
{
    return d_ptr->m_startLinear;
}

void QtGradientWidget::setEndLinear(const QPointF &point)
{
    if (d_ptr->m_endLinear == point)
        return;

    d_ptr->m_endLinear = d_ptr->checkRange(point);
    update();
}

QPointF QtGradientWidget::endLinear() const
{
    return d_ptr->m_endLinear;
}

void QtGradientWidget::setCentralRadial(const QPointF &point)
{
    if (d_ptr->m_centralRadial == point)
        return;

    d_ptr->m_centralRadial = point;
    update();
}

QPointF QtGradientWidget::centralRadial() const
{
    return d_ptr->m_centralRadial;
}

void QtGradientWidget::setFocalRadial(const QPointF &point)
{
    if (d_ptr->m_focalRadial == point)
        return;

    d_ptr->m_focalRadial = point;
    update();
}

QPointF QtGradientWidget::focalRadial() const
{
    return d_ptr->m_focalRadial;
}

void QtGradientWidget::setRadiusRadial(qreal radius)
{
    if (d_ptr->m_radiusRadial == radius)
        return;

    d_ptr->m_radiusRadial = radius;
    update();
}

qreal QtGradientWidget::radiusRadial() const
{
    return d_ptr->m_radiusRadial;
}

void QtGradientWidget::setCentralConical(const QPointF &point)
{
    if (d_ptr->m_centralConical == point)
        return;

    d_ptr->m_centralConical = point;
    update();
}

QPointF QtGradientWidget::centralConical() const
{
    return d_ptr->m_centralConical;
}

void QtGradientWidget::setAngleConical(qreal angle)
{
    if (d_ptr->m_angleConical == angle)
        return;

    d_ptr->m_angleConical = angle;
    update();
}

qreal QtGradientWidget::angleConical() const
{
    return d_ptr->m_angleConical;
}

QT_END_NAMESPACE
