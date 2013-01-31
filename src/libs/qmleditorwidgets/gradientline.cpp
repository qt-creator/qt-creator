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

#include "gradientline.h"
#include <QPainter>
#include <QMouseEvent>

static inline QPixmap tilePixMap(int size)
{
    const int checkerbordSize= size;
    QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
    tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
    return tilePixmap;
}

namespace QmlEditorWidgets {

void GradientLine::setGradient(const QLinearGradient &gradient)
{
    m_gradient = gradient;
    m_useGradient = true;
    readGradient();
}

static inline QColor invertColor(const QColor color)
{
    QColor c = color.toHsv();
    c.setHsv(c.hue(), c.saturation(), 255 - c.value());
    return c;
}

GradientLine::GradientLine(QWidget *parent) :
        QWidget(parent),
        m_activeColor(Qt::black),
        m_gradientName(QLatin1String("gradient")),
        m_colorIndex(0),
        m_dragActive(false),
        m_yOffset(0),
        m_create(false),
        m_active(false),
        m_dragOff(false),
        m_useGradient(true)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    setFocusPolicy(Qt::StrongFocus);
    setFixedHeight(50);
    setMinimumWidth(160);
    resize(160, 50);
    m_colorList << m_activeColor << QColor(Qt::white);
    m_stops << 0 << 1;
    updateGradient();
    setCurrentIndex(0);
}

void GradientLine::setGradientName(const QString &newName)
{
    if (newName == m_gradientName)
        return;
    m_gradientName = newName;
    setup();
    emit gradientNameChanged();
}

void GradientLine::setActiveColor(const QColor &newColor)
{
    if (newColor.name() == m_activeColor.name() && newColor.alpha() == m_activeColor.alpha())
        return;

    m_activeColor = newColor;
    m_colorList.removeAt(currentColorIndex());
    m_colorList.insert(currentColorIndex(), m_activeColor);
    updateGradient();
    emit gradientChanged();
    emit activeColorChanged();
    update();
}

void GradientLine::readGradient()
{
    if (m_useGradient) {
        m_colorList.clear();
        m_stops.clear();
        foreach (const QGradientStop &stop, m_gradient.stops()) {
            m_stops << stop.first;
            m_colorList << stop.second;
        }
    }
    updateGradient();
}

bool GradientLine::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride)
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Delete)) {
        event->accept();
        return true;
    }

    return QWidget::event(event);
}

void GradientLine::keyPressEvent(QKeyEvent * event)
{
    if (event->matches(QKeySequence::Delete)) {
        if ((currentColorIndex()) != 0 && (currentColorIndex() < m_stops.size() - 1)) {
            m_dragActive = false;
            m_stops.removeAt(currentColorIndex());
            m_colorList.removeAt(currentColorIndex());
            updateGradient();
            emit gradientChanged();
            setCurrentIndex(0);
            //delete item
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void GradientLine::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    if (!isEnabled()) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(0x444444));
        p.drawRect(9, 31, width() - 14, height() - 32);

        p.drawTiledPixmap(10, 32, width() - 16, height() - 34, tilePixMap(8));
    } else {

        QLinearGradient linearGradient(QPointF(0, 0), QPointF(width(), 0));

        for (int i =0; i < m_stops.size(); i++)
            linearGradient.setColorAt(m_stops.at(i), m_colorList.at(i));

        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(0x444444));
        p.drawRect(9, 31, width() - 14, height() - 32);


        p.drawTiledPixmap(9, 31, width() - 16, height() - 34, tilePixMap(8));

        p.setBrush(linearGradient);
        p.setPen(QColor(0x222222));
        p.drawRect(8, 30, width() - 14, height() - 32);
        p.setPen(QColor(255, 255, 255, 40));
        p.drawRect(9, 31, width() - 16, height() - 34);

        p.setPen(Qt::black);

        for (int i =0; i < m_colorList.size(); i++) {
            int localYOffset = 0;
            QColor arrowColor(Qt::black);
            if (i == currentColorIndex()) {
                localYOffset = m_yOffset;
                arrowColor = QColor(0x909090);
            }
            p.setPen(arrowColor);
            if (i == 0 || i == (m_colorList.size() - 1))
                localYOffset = 0;

            int pos = qreal((width() - 16)) * m_stops.at(i) + 9;
            p.setBrush(arrowColor);
            QVector<QPointF> points;
            if (localYOffset < -8)
                p.setOpacity(0.5);
            points.append(QPointF(pos + 0.5, 28.5 + localYOffset)); //triangle
            points.append(QPointF(pos - 3.5, 22.5 + localYOffset));
            points.append(QPointF(pos + 4.5, 22.5 + localYOffset));
            p.setRenderHint(QPainter::Antialiasing, true);
            p.drawPolygon(points);
            p.setRenderHint(QPainter::Antialiasing, false);
            p.setBrush(Qt::NoBrush);
            p.setPen(QColor(0x424242));
            p.drawRect(pos - 4, 9 + localYOffset, 10, 11);

            p.drawTiledPixmap(pos - 4, 9 + localYOffset, 9, 10, tilePixMap(5));
            p.setPen(QColor(0x424242));
            p.setBrush(m_colorList.at(i));
            p.drawRect(pos - 5, 8 + localYOffset, 10, 11);
            p.setBrush(Qt::NoBrush);
            p.setPen(QColor(255, 255, 255, 30));
            p.drawRect(pos - 4, 9 + localYOffset, 8, 9);
            p.setOpacity(1);
        }
    }
}

void GradientLine::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        event->accept();
        int xPos = event->pos().x();
        int yPos = event->pos().y();
        m_dragStart = event->pos();

        int draggedIndex = -1;
        m_create = false;
        m_dragActive = false;
        if ((yPos > 10) && (yPos < 30))
            for (int i =0; i < m_stops.size(); i++) {
            int pos = qreal((width() - 16)) * m_stops.at(i) + 9;
            if (((xPos + 8) > pos) && ((xPos - 8) < pos)) {
                draggedIndex = i;
                m_dragActive = true;
                setCurrentIndex(draggedIndex);
                update();
            }
        }
        if (draggedIndex == -1)
            m_create = true;
    }
    setFocus(Qt::MouseFocusReason);
}

void GradientLine::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
    m_dragActive = false;
    m_create = false;
    emit openColorDialog(event->pos());
}

void GradientLine::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        event->accept();

        if (m_dragActive) {
            m_yOffset += event->pos().y() - 14;
            if (m_yOffset > 0) {
                m_yOffset = 0;
            } else if ((m_yOffset < - 8) && (currentColorIndex()) != 0 && (currentColorIndex() < m_stops.size() - 1)) {
                m_yOffset = 0;
                m_dragActive = false;
                m_stops.removeAt(currentColorIndex());
                m_colorList.removeAt(currentColorIndex());
                updateGradient();
                emit gradientChanged();
                setCurrentIndex(0);
                //delete item
            }
        }

        if (m_dragActive == false && m_create) {
            qreal stopPos = qreal(event->pos().x() - 9) / qreal((width() - 15));
            int index = -1;
            for (int i =0; i < m_stops.size() - 1; i++) {
                if ((stopPos > m_stops.at(i)) && (index == -1))
                    index = i +1;
            }
            if (index != -1 && (m_useGradient) && abs(m_dragStart.x() - event->pos().x()) < 10) { //creating of items only in base state
                m_stops.insert(index, stopPos);
                m_colorList.insert(index, QColor(Qt::white));
                setCurrentIndex(index);
            }
        }
    }
    m_dragActive = false;
    m_yOffset = 0;
    updateGradient();
    emit gradientChanged();
    update();
    setFocus(Qt::MouseFocusReason);
}

void GradientLine::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragActive) {
        event->accept();
        int pos = qreal((width() - 20)) * m_stops.at(currentColorIndex()) + 8;

        m_dragOff = false;
        int xDistance = event->pos().x() - pos;
        qreal distance = qreal(xDistance) / qreal((width() - 20));
        qreal newStopPosition  = m_stops.at(currentColorIndex()) + distance;
        if (newStopPosition > 0.98) //snap to 1
            newStopPosition = 1;
        if (newStopPosition < 0.02) //snap to 0
            newStopPosition = 0;
        if ((newStopPosition >=0) && (newStopPosition <= 1))
            m_stops[currentColorIndex()] = newStopPosition;
        m_yOffset += event->pos().y() - 14;
        if (m_yOffset > 0)
            m_yOffset = 0;
        else if ((m_yOffset < - 10))
            m_yOffset = -10;

        update();
    }
}

void GradientLine::setup()
{

}

static inline QColor normalizeColor(const QColor &color)
{
    QColor newColor = QColor(color.name());
    newColor.setAlpha(color.alpha());
    return newColor;
}

static inline qreal roundReal(qreal real)
{
    int i = real * 100;
    return qreal(i) / 100;
}

void GradientLine::updateGradient()
{
    if (m_useGradient) {
        QGradientStops stops;
        for (int i = 0;i < m_stops.size(); i++) {
            stops.append(QPair<qreal, QColor>(m_stops.at(i), m_colorList.at(i)));
        }
        m_gradient.setStops(stops);
    } else {
        if (!active())
            return;
    }
}

void GradientLine::setCurrentIndex(int i)
{
    if (i == m_colorIndex)
        return;
    m_colorIndex = i;
    m_activeColor = m_colorList.at(i);
    emit activeColorChanged();
    update();
}

} //QmlEditorWidgets
