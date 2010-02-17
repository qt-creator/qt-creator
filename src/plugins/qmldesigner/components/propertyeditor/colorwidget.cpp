/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "colorwidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QGradient>
#include <QPainter>
#include <QtDebug>
#include <modelnode.h>
#include <abstractview.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <qmlobjectnode.h>

QML_DEFINE_TYPE(Bauhaus,1,0,ColorButton,QmlDesigner::ColorButton);
QML_DEFINE_TYPE(Bauhaus,1,0,HueControl,QmlDesigner::HueControl);
QML_DEFINE_TYPE(Bauhaus,1,0,ColorBox,QmlDesigner::ColorBox);


namespace QmlDesigner {

    void ColorButton::paintEvent(QPaintEvent *event)
    {
        QToolButton::paintEvent(event);
        if (!isEnabled())
            return;

        QColor color(m_colorString);

        const int pixSize = 8;

        QPainter p(this);

        QRect r(0, 0, width(), height());
        if (isEnabled())
            p.setBrush(color);
        else
            p.setBrush(Qt::transparent);
        p.setPen(Qt::black);
        p.drawRect(r);

        QVector<QPointF> points;
        if (isChecked()) {
            points.append(QPointF(2, 3));
            points.append(QPointF(8, 3));
            points.append(QPointF(5, 9));
        } else {
            points.append(QPointF(8, 6));
            points.append(QPointF(2, 9));
            points.append(QPointF(2, 3));
        }
        p.setPen("#707070");
        p.setBrush(Qt::white);
        p.drawPolygon(points);
    }


    void HueControl::setCurrent(int y)
    {
        QColor color(m_colorString);
        if (y<0) y=0;
        if (y>119) y=119;
        color.setHsvF(qreal(y) / 120.0, color.hsvSaturationF(), color.valueF());
        setColor(color.name());
        setHue(qreal(y) / 120.0);
    }

    void HueControl::paintEvent(QPaintEvent *event)
    {
        QWidget::paintEvent(event);

        QPainter p(this);

        int height = 120;

        if (m_cache.isNull()) {
            m_cache = QPixmap(10, height);

            QPainter cacheP(&m_cache);

            for (int i = 0; i < height; i++)
            {
                QColor c;
                c.setHsvF(qreal(i) / 120.0, 1,1.0);
                cacheP.fillRect(0, i, 10, i + 1, c);
            }
        }

        p.drawPixmap(10, 5, m_cache);

        QVector<QPointF> points;

        int y = hue() * 120 + 5;

        points.append(QPointF(15, y));
        points.append(QPointF(25, y + 5));
        points.append(QPointF(25, y - 5));

        p.setPen(Qt::black);
        p.setBrush(QColor("#707070"));
        p.drawPolygon(points);
    }

    void ColorBox::setCurrent(int x, int y)
    {
        QColor newColor;
        if (x<0) x=0;
        if (x>120) x=120;
        if (y<0) y=0;
        if (y>120) y=120;
        newColor.setHsvF(hue(), qreal(x) / 120, 1.0 - qreal(y) / 120);

        QString newColorStr = QVariant(newColor).toString();
        setColor(newColorStr);
    }

    void ColorBox::paintEvent(QPaintEvent *event)
    {
        QWidget::paintEvent(event);

        QPainter p(this);
        QColor color(m_colorString);

        if (m_hue != (m_lastHue) || (m_cache.isNull())) {
            m_lastHue = m_hue;
            m_cache = QPixmap(120, 120);

            int height = 120;
            int width = 120;

            QPainter chacheP(&m_cache);

            if (color.hsvHueF() < 0)
                color.setHsvF(hue(), color.hsvSaturationF(), color.valueF());

            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++)
                {
                QColor c;
                c.setHsvF(color.hsvHueF(), qreal(x) / 120, 1.0 - qreal(y) / 120);
                chacheP.setPen(c);
                chacheP.drawPoint(x ,y);
            }
        }

        p.drawPixmap(5, 5, m_cache);

        int x = color.hsvSaturationF() * 120 + 5;
        int y = 120 - color.valueF() * 120 + 5;

        p.setPen(Qt::white);
        p.drawEllipse(x - 2, y - 2, 4, 4);
    }

}
