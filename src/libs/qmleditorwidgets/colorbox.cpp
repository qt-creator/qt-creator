/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "colorbox.h"
#include <QPainter>
#include <QMouseEvent>

static inline QString properName(const QColor &color)
{
    QString s;
    if (color.alpha() == 255)
        s.sprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    else
        s.sprintf("#%02x%02x%02x%02x", color.alpha(), color.red(), color.green(), color.blue());
    return s;
}

static inline QColor properColor(const QString &str)
{
    if (str.isEmpty())
        return QColor();
    int lalpha = 255;
    QString lcolorStr = str;
    const QChar hash = QLatin1Char('#');
    if (lcolorStr.at(0) == hash && lcolorStr.length() == 9) {
        QString alphaStr = lcolorStr;
        alphaStr.truncate(3);
        lcolorStr.remove(0, 3);
        lcolorStr = hash + lcolorStr;
        alphaStr.remove(0,1);
        bool v;
        lalpha = alphaStr.toInt(&v, 16);
        if (!v)
            lalpha = 255;
    }
    QColor lcolor(lcolorStr);
    if (lcolorStr.contains(hash))
        lcolor.setAlpha(lalpha);
    return lcolor;
}

static inline int clamp(int x, int lower, int upper)
{
    if (x < lower)
        x = lower;
    if (x > upper)
        x = upper;
    return x;
}

namespace QmlEditorWidgets {

void ColorBox::setHue(int newHue)
{
    if (m_color.hsvHue() == newHue)
        return;

    int oldAlpha = m_color.alpha();
    m_color.setHsv(newHue,m_color.hsvSaturation(),m_color.value());
    m_color.setAlpha(oldAlpha);
    update();
    emit hueChanged();
    emit colorChanged();
}

int ColorBox::hue() const
{
    int retval = m_color.hsvHue();
    if (retval<0) retval=0;
    if (retval>359) retval=359;
    return retval;
}

void ColorBox::setAlpha(int newAlpha)
{
    if (m_color.alpha() == newAlpha)
        return;

    m_color.setAlpha(newAlpha);
    update();
    emit alphaChanged();
    emit colorChanged();
}

QString ColorBox::strColor() const
{
    return properName(m_color);
}

void ColorBox::setStrColor(const QString &colorStr)
{
    if (properName(m_color) == colorStr)
        return;

    setColor(properColor(colorStr));
}

void ColorBox::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    int oldsaturation = m_color.hsvSaturation();
    int oldvalue = m_color.value();
    int oldhue = m_color.hsvHue();
    int oldAlpha = m_color.alpha();
    m_color=color;
    update();
    if (oldhue != m_color.hsvHue()) emit hueChanged();
    if (oldsaturation != saturation()) emit saturationChanged();
    if (oldvalue != value()) emit valueChanged();
    if (oldAlpha != alpha()) emit alphaChanged();
}

void ColorBox::setSaturation(int newsaturation)
{
    if (m_color.hsvSaturation()==newsaturation) return;
    int oldAlpha = m_color.alpha();
    m_color.setHsv(m_color.hsvHue(),newsaturation,m_color.value());
    m_color.setAlpha(oldAlpha);
    update();
    emit saturationChanged();
    emit colorChanged();
}

void ColorBox::setCurrent(int x, int y)
{
    QColor newColor;
    x = clamp(x, 0, 120);
    y = clamp(y, 0, 120);
    int oldAlpha = m_color.alpha();
    newColor.setHsv(hue(), (x*255) / 120, 255 - (y*255) / 120);
    newColor.setAlpha(oldAlpha);
    setColor(newColor);
}

void ColorBox::setValue(int newvalue)
{
    if (m_color.value()==newvalue) return;
    int oldAlpha = m_color.alpha();
    m_color.setHsv(m_color.hsvHue(),m_color.hsvSaturation(),newvalue);
    m_color.setAlpha(oldAlpha);
    update();
    emit valueChanged();
    emit colorChanged();
}

void ColorBox::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    if ((m_color.saturation()>1) && (m_color.value()>1))
        m_saturatedColor.setHsv(m_color.hsvHue(),255,255);

    if ((hue() != m_lastHue) || (m_cache.isNull())) {
        m_lastHue = hue();

        int fixedHue = clamp(m_lastHue, 0, 359);

        QImage cache = QImage(120, 120, QImage::Format_RGB32);

        int height = 120;
        int width = 120;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                QColor c;
                c.setHsv(fixedHue, (x*255) / width, 255 - (y*255) / height);
                cache.setPixel(x, y, c.rgb());
            }
        }
        m_cache = QPixmap::fromImage(cache);
    }

    p.drawPixmap(5, 5, m_cache);

    int x = clamp(m_color.hsvSaturationF() * 120, 0, 119) + 5;
    int y = clamp(120 - m_color.valueF() * 120, 0, 119) + 5;

    p.setPen(QColor(255, 255, 255, 50));
    p.drawLine(5, y, x-1, y);
    p.drawLine(x+1, y, width()-7, y);
    p.drawLine(x, 5, x, y-1);
    p.drawLine(x, y+1, x, height()-7);

}

void ColorBox::mousePressEvent(QMouseEvent *e)
{
    // The current cell marker is set to the cell the mouse is pressed in
    QPoint pos = e->pos();
    m_mousePressed = true;
    setCurrent(pos.x() - 5, pos.y() - 5);
}

void ColorBox::mouseReleaseEvent(QMouseEvent * /* event */)
{
    if (m_mousePressed)
        emit colorChanged();
    m_mousePressed = false;
}

void ColorBox::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_mousePressed)
        return;
    QPoint pos = e->pos();
    setCurrent(pos.x() - 5, pos.y() - 5);
}


} //QmlEditorWidgets
