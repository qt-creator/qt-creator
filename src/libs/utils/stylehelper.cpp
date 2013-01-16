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

#include "stylehelper.h"

#include "hostosinfo.h"

#include <QPixmapCache>
#include <QPainter>
#include <QApplication>
#include <QStyleOption>
#include <qmath.h>

// Clamps float color values within (0, 255)
static int clamp(float x)
{
    const int val = x > 255 ? 255 : static_cast<int>(x);
    return val < 0 ? 0 : val;
}

// Clamps float color values within (0, 255)
/*
static int range(float x, int min, int max)
{
    int val = x > max ? max : x;
    return val < min ? min : val;
}
*/

namespace Utils {

QColor StyleHelper::mergedColors(const QColor &colorA, const QColor &colorB, int factor)
{
    const int maxFactor = 100;
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

qreal StyleHelper::sidebarFontSize()
{
    return HostOsInfo::isMacHost() ? 10 : 7.5;
}

QPalette StyleHelper::sidebarFontPalette(const QPalette &original)
{
    QPalette palette = original;
    palette.setColor(QPalette::Active, QPalette::Text, panelTextColor());
    palette.setColor(QPalette::Active, QPalette::WindowText, panelTextColor());
    palette.setColor(QPalette::Inactive, QPalette::Text, panelTextColor().darker());
    palette.setColor(QPalette::Inactive, QPalette::WindowText, panelTextColor().darker());
    return palette;
}

QColor StyleHelper::panelTextColor(bool lightColored)
{
    //qApp->palette().highlightedText().color();
    if (!lightColored)
        return Qt::white;
    else
        return Qt::black;
}

// Invalid by default, setBaseColor needs to be called at least once
QColor StyleHelper::m_baseColor;
QColor StyleHelper::m_requestedBaseColor;

QColor StyleHelper::baseColor(bool lightColored)
{
    if (!lightColored)
        return m_baseColor;
    else
        return m_baseColor.lighter(230);
}

QColor StyleHelper::highlightColor(bool lightColored)
{
    QColor result = baseColor(lightColored);
    if (!lightColored)
        result.setHsv(result.hue(),
                  clamp(result.saturation()),
                  clamp(result.value() * 1.16));
    else
        result.setHsv(result.hue(),
                  clamp(result.saturation()),
                  clamp(result.value() * 1.06));
    return result;
}

QColor StyleHelper::shadowColor(bool lightColored)
{
    QColor result = baseColor(lightColored);
    result.setHsv(result.hue(),
                  clamp(result.saturation() * 1.1),
                  clamp(result.value() * 0.70));
    return result;
}

QColor StyleHelper::borderColor(bool lightColored)
{
    QColor result = baseColor(lightColored);
    result.setHsv(result.hue(),
                  result.saturation(),
                  result.value() / 2);
    return result;
}

// We try to ensure that the actual color used are within
// reasonalbe bounds while generating the actual baseColor
// from the users request.
void StyleHelper::setBaseColor(const QColor &newcolor)
{
    m_requestedBaseColor = newcolor;

    QColor color;
    color.setHsv(newcolor.hue(),
                 newcolor.saturation() * 0.7,
                 64 + newcolor.value() / 3);

    if (color.isValid() && color != m_baseColor) {
        m_baseColor = color;
        foreach (QWidget *w, QApplication::topLevelWidgets())
            w->update();
    }
}

static void verticalGradientHelper(QPainter *p, const QRect &spanRect, const QRect &rect, bool lightColored)
{
    QColor highlight = StyleHelper::highlightColor(lightColored);
    QColor shadow = StyleHelper::shadowColor(lightColored);
    QLinearGradient grad(spanRect.topRight(), spanRect.topLeft());
    grad.setColorAt(0, highlight.lighter(117));
    grad.setColorAt(1, shadow.darker(109));
    p->fillRect(rect, grad);

    QColor light(255, 255, 255, 80);
    p->setPen(light);
    p->drawLine(rect.topRight() - QPoint(1, 0), rect.bottomRight() - QPoint(1, 0));
    QColor dark(0, 0, 0, 90);
    p->setPen(dark);
    p->drawLine(rect.topLeft(), rect.bottomLeft());
}

void StyleHelper::verticalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect, bool lightColored)
{
    if (StyleHelper::usePixmapCache()) {
        QString key;
        QColor keyColor = baseColor(lightColored);
        key.sprintf("mh_vertical %d %d %d %d %d",
            spanRect.width(), spanRect.height(), clipRect.width(),
            clipRect.height(), keyColor.rgb());;

        QPixmap pixmap;
        if (!QPixmapCache::find(key, pixmap)) {
            pixmap = QPixmap(clipRect.size());
            QPainter p(&pixmap);
            QRect rect(0, 0, clipRect.width(), clipRect.height());
            verticalGradientHelper(&p, spanRect, rect, lightColored);
            p.end();
            QPixmapCache::insert(key, pixmap);
        }

        painter->drawPixmap(clipRect.topLeft(), pixmap);
    } else {
        verticalGradientHelper(painter, spanRect, clipRect, lightColored);
    }
}

static void horizontalGradientHelper(QPainter *p, const QRect &spanRect, const
QRect &rect, bool lightColored)
{
    if (lightColored) {
        QLinearGradient shadowGradient(rect.topLeft(), rect.bottomLeft());
        shadowGradient.setColorAt(0, 0xf0f0f0);
        shadowGradient.setColorAt(1, 0xcfcfcf);
        p->fillRect(rect, shadowGradient);
        return;
    }

    QColor base = StyleHelper::baseColor(lightColored);
    QColor highlight = StyleHelper::highlightColor(lightColored);
    QColor shadow = StyleHelper::shadowColor(lightColored);
    QLinearGradient grad(rect.topLeft(), rect.bottomLeft());
    grad.setColorAt(0, highlight.lighter(120));
    if (rect.height() == StyleHelper::navigationWidgetHeight()) {
        grad.setColorAt(0.4, highlight);
        grad.setColorAt(0.401, base);
    }
    grad.setColorAt(1, shadow);
    p->fillRect(rect, grad);

    QLinearGradient shadowGradient(spanRect.topLeft(), spanRect.topRight());
        shadowGradient.setColorAt(0, QColor(0, 0, 0, 30));
    QColor lighterHighlight;
    lighterHighlight = highlight.lighter(130);
    lighterHighlight.setAlpha(100);
    shadowGradient.setColorAt(0.7, lighterHighlight);
        shadowGradient.setColorAt(1, QColor(0, 0, 0, 40));
    p->fillRect(rect, shadowGradient);
}

void StyleHelper::horizontalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect, bool lightColored)
{
    if (StyleHelper::usePixmapCache()) {
        QString key;
        QColor keyColor = baseColor(lightColored);
        key.sprintf("mh_horizontal %d %d %d %d %d %d",
            spanRect.width(), spanRect.height(), clipRect.width(),
            clipRect.height(), keyColor.rgb(), spanRect.x());

        QPixmap pixmap;
        if (!QPixmapCache::find(key, pixmap)) {
            pixmap = QPixmap(clipRect.size());
            QPainter p(&pixmap);
            QRect rect = QRect(0, 0, clipRect.width(), clipRect.height());
            horizontalGradientHelper(&p, spanRect, rect, lightColored);
            p.end();
            QPixmapCache::insert(key, pixmap);
        }

        painter->drawPixmap(clipRect.topLeft(), pixmap);

    } else {
        horizontalGradientHelper(painter, spanRect, clipRect, lightColored);
    }
}

static void menuGradientHelper(QPainter *p, const QRect &spanRect, const QRect &rect)
{
    QLinearGradient grad(spanRect.topLeft(), spanRect.bottomLeft());
    QColor menuColor = StyleHelper::mergedColors(StyleHelper::baseColor(), QColor(244, 244, 244), 25);
    grad.setColorAt(0, menuColor.lighter(112));
    grad.setColorAt(1, menuColor);
    p->fillRect(rect, grad);
}

void StyleHelper::drawArrow(QStyle::PrimitiveElement element, QPainter *painter, const QStyleOption *option)
{
    // From windowsstyle but modified to enable AA
    if (option->rect.width() <= 1 || option->rect.height() <= 1)
        return;

    QRect r = option->rect;
    int size = qMin(r.height(), r.width());
    QPixmap pixmap;
    QString pixmapName;
    pixmapName.sprintf("arrow-%s-%d-%d-%d-%lld",
                       "$qt_ia",
                       uint(option->state), element,
                       size, option->palette.cacheKey());
    if (!QPixmapCache::find(pixmapName, pixmap)) {
        int border = size/5;
        int sqsize = 2*(size/2);
        QImage image(sqsize, sqsize, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter imagePainter(&image);
        imagePainter.setRenderHint(QPainter::Antialiasing, true);
        imagePainter.translate(0.5, 0.5);
        QPolygon a;
        switch (element) {
            case QStyle::PE_IndicatorArrowUp:
                a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize - border, sqsize/2);
                break;
            case QStyle::PE_IndicatorArrowDown:
                a.setPoints(3, border, sqsize/2,  sqsize/2, sqsize - border,  sqsize - border, sqsize/2);
                break;
            case QStyle::PE_IndicatorArrowRight:
                a.setPoints(3, sqsize - border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
                break;
            case QStyle::PE_IndicatorArrowLeft:
                a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
                break;
            default:
                break;
        }

        int bsx = 0;
        int bsy = 0;

        if (option->state & QStyle::State_Sunken) {
            bsx = qApp->style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal);
            bsy = qApp->style()->pixelMetric(QStyle::PM_ButtonShiftVertical);
        }

        QRect bounds = a.boundingRect();
        int sx = sqsize / 2 - bounds.center().x() - 1;
        int sy = sqsize / 2 - bounds.center().y() - 1;
        imagePainter.translate(sx + bsx, sy + bsy);

        if (!(option->state & QStyle::State_Enabled)) {
            QColor foreGround(150, 150, 150, 150);
            imagePainter.setBrush(option->palette.mid().color());
            imagePainter.setPen(option->palette.mid().color());
        } else {
            QColor shadow(0, 0, 0, 100);
            imagePainter.translate(0, 1);
            imagePainter.setPen(shadow);
            imagePainter.setBrush(shadow);
            QColor foreGround(255, 255, 255, 210);
            imagePainter.drawPolygon(a);
            imagePainter.translate(0, -1);
            imagePainter.setPen(foreGround);
            imagePainter.setBrush(foreGround);
        }
        imagePainter.drawPolygon(a);
        imagePainter.end();
        pixmap = QPixmap::fromImage(image);
        QPixmapCache::insert(pixmapName, pixmap);
    }
    int xOffset = r.x() + (r.width() - size)/2;
    int yOffset = r.y() + (r.height() - size)/2;
    painter->drawPixmap(xOffset, yOffset, pixmap);
}

void StyleHelper::menuGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect)
{
    if (StyleHelper::usePixmapCache()) {
        QString key;
        key.sprintf("mh_menu %d %d %d %d %d",
            spanRect.width(), spanRect.height(), clipRect.width(),
            clipRect.height(), StyleHelper::baseColor().rgb());

        QPixmap pixmap;
        if (!QPixmapCache::find(key, pixmap)) {
            pixmap = QPixmap(clipRect.size());
            QPainter p(&pixmap);
            QRect rect = QRect(0, 0, clipRect.width(), clipRect.height());
            menuGradientHelper(&p, spanRect, rect);
            p.end();
            QPixmapCache::insert(key, pixmap);
        }

        painter->drawPixmap(clipRect.topLeft(), pixmap);
    } else {
        menuGradientHelper(painter, spanRect, clipRect);
    }
}

static qreal pixmapDevicePixelRatio(const QPixmap &pixmap)
{
#if QT_VERSION > 0x050000
    return pixmap.devicePixelRatio();
#else
    return 1.0;
#endif
}

// Draws a cached pixmap with shadow
void StyleHelper::drawIconWithShadow(const QIcon &icon, const QRect &rect,
                                     QPainter *p, QIcon::Mode iconMode, int dipRadius, const QColor &color, const QPoint &dipOffset)
{
    QPixmap cache;
    QString pixmapName = QString::fromLatin1("icon %0 %1 %2").arg(icon.cacheKey()).arg(iconMode).arg(rect.height());

    if (!QPixmapCache::find(pixmapName, cache)) {
        // High-dpi support: The in parameters (rect, radius, offset) are in
        // device-independent pixels. The call to QIcon::pixmap() below might
        // return a high-dpi pixmap, which will in that case have a devicePixelRatio
        // different than 1. The shadow drawing caluculations are done in device
        // pixels.
        QPixmap px = icon.pixmap(rect.size());
        int devicePixelRatio = qCeil(pixmapDevicePixelRatio(px));
        int radius = dipRadius * devicePixelRatio;
        QPoint offset = dipOffset * devicePixelRatio;
        cache = QPixmap(px.size() + QSize(radius * 2, radius * 2));
        cache.fill(Qt::transparent);

        QPainter cachePainter(&cache);
        if (iconMode == QIcon::Disabled) {
            QImage im = px.toImage().convertToFormat(QImage::Format_ARGB32);
            for (int y=0; y<im.height(); ++y) {
                QRgb *scanLine = (QRgb*)im.scanLine(y);
                for (int x=0; x<im.width(); ++x) {
                    QRgb pixel = *scanLine;
                    char intensity = qGray(pixel);
                    *scanLine = qRgba(intensity, intensity, intensity, qAlpha(pixel));
                    ++scanLine;
                }
            }
            px = QPixmap::fromImage(im);
        }

        // Draw shadow
        QImage tmp(px.size() + QSize(radius * 2, radius * 2 + 1), QImage::Format_ARGB32_Premultiplied);
        tmp.fill(Qt::transparent);

        QPainter tmpPainter(&tmp);
        tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
        tmpPainter.drawPixmap(QRect(radius, radius, px.width(), px.height()), px);
        tmpPainter.end();

        // blur the alpha channel
        QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
        blurred.fill(Qt::transparent);
        QPainter blurPainter(&blurred);
        qt_blurImage(&blurPainter, tmp, radius, false, true);
        blurPainter.end();

        tmp = blurred;

        // blacken the image...
        tmpPainter.begin(&tmp);
        tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tmpPainter.fillRect(tmp.rect(), color);
        tmpPainter.end();

        tmpPainter.begin(&tmp);
        tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tmpPainter.fillRect(tmp.rect(), color);
        tmpPainter.end();

        // draw the blurred drop shadow...
        cachePainter.drawImage(QRect(0, 0, cache.rect().width(), cache.rect().height()), tmp);

        // Draw the actual pixmap...
        cachePainter.drawPixmap(QRect(QPoint(radius, radius) + offset, QSize(px.width(), px.height())), px);
#if QT_VERSION > 0x050000
        cache.setDevicePixelRatio(devicePixelRatio);
#endif
        QPixmapCache::insert(pixmapName, cache);
    }

    QRect targetRect = cache.rect();
    targetRect.setSize(targetRect.size() / pixmapDevicePixelRatio(cache));
    targetRect.moveCenter(rect.center() - dipOffset);
    p->drawPixmap(targetRect, cache);
}

// Draws a CSS-like border image where the defined borders are not stretched
void StyleHelper::drawCornerImage(const QImage &img, QPainter *painter, QRect rect,
                                  int left, int top, int right, int bottom)
{
    QSize size = img.size();
    if (top > 0) { //top
        painter->drawImage(QRect(rect.left() + left, rect.top(), rect.width() -right - left, top), img,
                           QRect(left, 0, size.width() -right - left, top));
        if (left > 0) //top-left
            painter->drawImage(QRect(rect.left(), rect.top(), left, top), img,
                               QRect(0, 0, left, top));
        if (right > 0) //top-right
            painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top(), right, top), img,
                               QRect(size.width() - right, 0, right, top));
    }
    //left
    if (left > 0)
        painter->drawImage(QRect(rect.left(), rect.top()+top, left, rect.height() - top - bottom), img,
                           QRect(0, top, left, size.height() - bottom - top));
    //center
    painter->drawImage(QRect(rect.left() + left, rect.top()+top, rect.width() -right - left,
                             rect.height() - bottom - top), img,
                       QRect(left, top, size.width() -right -left,
                             size.height() - bottom - top));
    if (right > 0) //right
        painter->drawImage(QRect(rect.left() +rect.width() - right, rect.top()+top, right, rect.height() - top - bottom), img,
                           QRect(size.width() - right, top, right, size.height() - bottom - top));
    if (bottom > 0) { //bottom
        painter->drawImage(QRect(rect.left() +left, rect.top() + rect.height() - bottom,
                                 rect.width() - right - left, bottom), img,
                           QRect(left, size.height() - bottom,
                                 size.width() - right - left, bottom));
    if (left > 0) //bottom-left
        painter->drawImage(QRect(rect.left(), rect.top() + rect.height() - bottom, left, bottom), img,
                           QRect(0, size.height() - bottom, left, bottom));
    if (right > 0) //bottom-right
        painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top() + rect.height() - bottom, right, bottom), img,
                           QRect(size.width() - right, size.height() - bottom, right, bottom));
    }
}

// Tints an image with tintColor, while preserving alpha and lightness
void StyleHelper::tintImage(QImage &img, const QColor &tintColor)
{
    QPainter p(&img);
    p.setCompositionMode(QPainter::CompositionMode_Screen);

    for (int x = 0; x < img.width(); ++x) {
        for (int y = 0; y < img.height(); ++y) {
            QRgb rgbColor = img.pixel(x, y);
            int alpha = qAlpha(rgbColor);
            QColor c = QColor(rgbColor);

            if (alpha > 0) {
                c.toHsl();
                qreal l = c.lightnessF();
                QColor newColor = QColor::fromHslF(tintColor.hslHueF(), tintColor.hslSaturationF(), l);
                newColor.setAlpha(alpha);
                img.setPixel(x, y, newColor.rgba());
            }
        }
    }
}

QLinearGradient StyleHelper::statusBarGradient(const QRect &statusBarRect)
{
    QLinearGradient grad(statusBarRect.topLeft(), QPoint(statusBarRect.center().x(), statusBarRect.bottom()));
    QColor startColor = shadowColor().darker(164);
    QColor endColor = baseColor().darker(130);
    grad.setColorAt(0, startColor);
    grad.setColorAt(1, endColor);
    return grad;
}

} // namespace Utils
