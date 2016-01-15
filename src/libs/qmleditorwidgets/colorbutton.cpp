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

#include "colorbutton.h"
#include <QPainter>

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

static inline bool isColorString(const QString &colorString)
{
    bool ok = true;
    if (colorString.size() == 9 && colorString.at(0) == QLatin1Char('#')) {
        // #rgba
        for (int i = 1; i < 9; ++i) {
            const QChar c = colorString.at(i);
            if ((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
                || (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
                || (c >= QLatin1Char('A') && c <= QLatin1Char('F')))
                continue;
            ok = false;
            break;
        }
    } else {
        ok = QColor::isValidColor(colorString);
    }

    return ok;
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

namespace QmlEditorWidgets {

void ColorButton::setColor(const QVariant &colorStr)
{
    if (m_colorString == colorStr.toString())
        return;


    setEnabled(isColorString(colorStr.toString()));

    m_colorString = colorStr.toString();
    update();
    emit colorChanged();
}

QColor ColorButton::convertedColor() const
{
    return properColor(m_colorString);
}

void ColorButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);
    if (!isEnabled())
        return;

    QColor color = properColor(m_colorString);

    QPainter p(this);


    QRect r(0, 0, width() - 2, height() - 2);
    p.drawTiledPixmap(r.adjusted(1, 1, -1, -1), tilePixMap(9));
    if (isEnabled())
        p.setBrush(color);
    else
        p.setBrush(Qt::transparent);

    if (color.value() > 80)
        p.setPen(QColor(0x444444));
    else
        p.setPen(QColor(0x9e9e9e));
    p.drawRect(r.translated(1, 1));

    if (m_showArrow) {
        p.setRenderHint(QPainter::Antialiasing, true);
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
        p.translate(0.5, 0.5);
        p.setBrush(QColor(0xaaaaaa));
        p.setPen(QColor(0x444444));
        p.drawPolygon(points);
    }
}

} //QmlEditorWidgets
