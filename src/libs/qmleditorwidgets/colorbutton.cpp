/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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

void ColorButton::setColor(const QString &colorStr)
{
    if (m_colorString == colorStr)
        return;


    setEnabled(isColorString(colorStr));

    m_colorString = colorStr;
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
