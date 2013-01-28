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

#include "huecontrol.h"
#include <QPainter>
#include <QMouseEvent>

static inline int clamp(int x, int lower, int upper)
{
    if (x < lower)
        x = lower;
    if (x > upper)
        x = upper;
    return x;
}

namespace QmlEditorWidgets {

void HueControl::setCurrent(int y)
{
    y = clamp(y, 0, 120);
    int oldAlpha = m_color.alpha();
    m_color.setHsv((y * 359)/120, m_color.hsvSaturation(), m_color.value());
    m_color.setAlpha(oldAlpha);
    update(); // redraw pointer
    emit hueChanged(m_color.hsvHue());
}

void HueControl::setHue(int newHue)
{
    if (m_color.hsvHue() == newHue)
        return;
    m_color.setHsv(newHue, m_color.hsvSaturation(), m_color.value());
    update();
    emit hueChanged(m_color.hsvHue());
}

void HueControl::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    int localHeight = 120;

    if (m_cache.isNull()) {
        m_cache = QPixmap(10, localHeight);

        QPainter cacheP(&m_cache);

        for (int i = 0; i < localHeight; i++)
        {
            QColor c;
            c.setHsv( (i*359) / 120.0, 255,255);
            cacheP.fillRect(0, i, 10, i + 1, c);
        }
    }

    p.drawPixmap(0, 5, m_cache);

    QVector<QPointF> points;

    int y = m_color.hueF() * 120 + 5;

    points.append(QPointF(5, y));
    points.append(QPointF(15, y + 5));
    points.append(QPointF(15, y - 5));


    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(0.5, 1.5);
    p.setPen(QColor(0, 0, 0, 120));
    p.drawPolygon(points);
    p.translate(0, -1);
    p.setPen(0x222222);
    p.setBrush(QColor(0x707070));
    p.drawPolygon(points);
}

void HueControl::mousePressEvent(QMouseEvent *e)
{
    // The current cell marker is set to the cell the mouse is pressed in
    QPoint pos = e->pos();
    m_mousePressed = true;
    setCurrent(pos.y() - 5);
}

void HueControl::mouseReleaseEvent(QMouseEvent * /* event */)
{
    m_mousePressed = false;
}

void HueControl::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_mousePressed)
        return;
    QPoint pos = e->pos();
    setCurrent(pos.y() - 5);
}

} //QmlEditorWidgets
