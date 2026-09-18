// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stylehelper.h"

#include "algorithm.h"
#include "hostosinfo.h"
#include "qtcassert.h"

#include <QApplication>
#include <QCommonStyle>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QStyleOption>
#include <QWindow>

#include <qmath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

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

static StyleHelper::ToolbarStyle s_toolbarStyle = StyleHelper::ToolbarStyle::Compact;
// Invalid by default, setBaseColor needs to be called at least once
static QColor s_baseColor;
static QColor s_requestedBaseColor;

QColor StyleHelper::TextFormat::color() const
{
    return Utils::creatorColor(themeColor);
}

QFont StyleHelper::TextFormat::font(bool underlined) const
{
    QFont result = Utils::StyleHelper::uiFont(uiElement);
    result.setUnderline(underlined);
    return result;
}

int StyleHelper::TextFormat::lineHeight() const
{
    return Utils::StyleHelper::uiFontLineHeight(uiElement);
}

void StyleHelper::applyTf(QLabel *label, const StyleHelper::TextFormat &tf, bool singleLine)
{
    if (singleLine)
        label->setFixedHeight(tf.lineHeight());
    label->setFont(tf.font());
    label->setAlignment(Qt::Alignment(tf.drawTextFlags));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, tf.color());
    label->setPalette(pal);
}

QColor StyleHelper::mergedColors(const QColor &colorA, const QColor &colorB, int factor)
{
    const int maxFactor = 100;
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

QColor StyleHelper::alphaBlendedColors(const QColor &colorA, const QColor &colorB)
{
    const int alpha = colorB.alpha();
    const int antiAlpha = 255 - alpha;

    return QColor(
                (colorA.red() * antiAlpha + colorB.red() * alpha) / 255,
                (colorA.green() * antiAlpha + colorB.green() * alpha) / 255,
                (colorA.blue() * antiAlpha + colorB.blue() * alpha) / 255
                );
}

QColor StyleHelper::sidebarHighlight()
{
    return QColor(255, 255, 255, 40);
}

QColor StyleHelper::sidebarShadow()
{
    return QColor(0, 0, 0, 40);
}

QColor StyleHelper::toolBarDropShadowColor()
{
    return QColor(0, 0, 0, 70);
}

int StyleHelper::navigationWidgetHeight()
{
    return s_toolbarStyle == ToolbarStyle::Compact ? 24 : 30;
}

void StyleHelper::setToolbarStyle(ToolbarStyle style)
{
    s_toolbarStyle = style;
}

StyleHelper::ToolbarStyle StyleHelper::toolbarStyle()
{
    return s_toolbarStyle;
}

StyleHelper::ToolbarStyle StyleHelper::defaultToolbarStyle()
{
    return creatorTheme() ? creatorTheme()->defaultToolbarStyle() : ToolbarStyle::Compact;
}

QColor StyleHelper::notTooBrightHighlightColor()
{
    QColor highlightColor = QApplication::palette().highlight().color();
    if (0.5 * highlightColor.saturationF() + 0.75 - highlightColor.valueF() < 0)
        highlightColor.setHsvF(highlightColor.hsvHueF(), 0.1 + highlightColor.saturationF() * 2.0, highlightColor.valueF());
    return highlightColor;
}


QPalette StyleHelper::sidebarFontPalette(const QPalette &original)
{
    QPalette palette = original;
    const QColor textColor = creatorColor(Theme::ProgressBarTitleColor);
    palette.setColor(QPalette::WindowText, textColor);
    palette.setColor(QPalette::Text, textColor);
    return palette;
}

QColor StyleHelper::panelTextColor(bool lightColored)
{
    if (!lightColored)
        return Qt::white;
    else
        return Qt::black;
}

QColor StyleHelper::baseColor(bool lightColored)
{
    static const QColor windowColor = QApplication::palette().color(QPalette::Window);
    static const bool windowColorAsBase = creatorTheme()->flag(Theme::WindowColorAsBase);

    return (lightColored || windowColorAsBase) ? windowColor : s_baseColor;
}

QColor StyleHelper::requestedBaseColor()
{
    return s_requestedBaseColor;
}

QColor StyleHelper::toolbarBaseColor(bool lightColored)
{
    return StyleHelper::baseColor(lightColored);
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

QColor StyleHelper::toolBarBorderColor()
{
    if (const QColor sepColor = creatorColor(Theme::FancyToolBarSeparatorColor);
            sepColor == creatorColor(Theme::SplitterColor))
        return sepColor; // QTCREATORBUG-31682: Unify all separating line colors if two are the same

    const QColor base = baseColor();
    return QColor::fromHsv(base.hue(),
                           base.saturation() ,
                           clamp(base.value() * 0.80f));
}

// We try to ensure that the actual color used are within
// reasonalbe bounds while generating the actual baseColor
// from the users request.
void StyleHelper::setBaseColor(const QColor &newcolor)
{
    s_requestedBaseColor = newcolor;

    const QColor themeBaseColor = creatorColor(Theme::PanelStatusBarBackgroundColor);
    const QColor defaultBaseColor = QColor(DEFAULT_BASE_COLOR);
    QColor color;

    if (defaultBaseColor == newcolor) {
        color = themeBaseColor;
    } else {
        const int valueDelta = (newcolor.value() - defaultBaseColor.value()) / 3;
        const int value = qBound(0, themeBaseColor.value() + valueDelta, 255);

        color.setHsv(newcolor.hue(),
                     newcolor.saturation() * 0.7,
                     value);
    }

    if (color.isValid() && color != s_baseColor) {
        s_baseColor = color;
        const QWidgetList widgets = QApplication::allWidgets();
        for (QWidget *w : widgets)
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

        QColor keyColor = baseColor(lightColored);
        const QString key = QString::asprintf("mh_vertical %d %d %d %d %d",
            spanRect.width(), spanRect.height(), clipRect.width(),
            clipRect.height(), keyColor.rgb());

        QPixmap pixmap;
        if (!QPixmapCache::find(key, &pixmap)) {
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

        QColor keyColor = baseColor(lightColored);
        const QString key = QString::asprintf("mh_horizontal %d %d %d %d %d %d",
            spanRect.width(), spanRect.height(), clipRect.width(),
            clipRect.height(), keyColor.rgb(), spanRect.x());

        QPixmap pixmap;
        if (!QPixmapCache::find(key, &pixmap)) {
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
    if (option->rect.width() <= 1 || option->rect.height() <= 1)
        return;

    const qreal devicePixelRatio = painter->device()->devicePixelRatio();
    const bool enabled = option->state & QStyle::State_Enabled;
    QRect r = option->rect;
    int size = qMin(r.height(), r.width());
    QPixmap pixmap;
    const QString pixmapName = QString::asprintf("StyleHelper::drawArrow-%d-%d-%d-%f",
                       element, size, enabled, devicePixelRatio);
    if (!QPixmapCache::find(pixmapName, &pixmap)) {
        QImage image(size * devicePixelRatio, size * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);

        QStyleOption tweakedOption(*option);
        tweakedOption.state = QStyle::State_Enabled;

        const QCommonStyle *const style = qobject_cast<QCommonStyle *>(QApplication::style());
        auto drawCommonStyleArrow = [&tweakedOption,
                                     element,
                                     &painter,
                                     style](const QRect &rect, const QColor &color) -> void {
            if (!style)
                return;

            // Workaround for QTCREATORBUG-28470
            QPalette pal = tweakedOption.palette;
            pal.setBrush(QPalette::Base, pal.text()); // Base and Text differ, causing a detachment.
                                                      // Inspired by tst_QPalette::cacheKey()
            pal.setColor(QPalette::ButtonText, color.rgb());

            tweakedOption.palette = pal;
            tweakedOption.rect = rect;
            painter.setOpacity(color.alphaF());
            style->QCommonStyle::drawPrimitive(element, &tweakedOption, &painter);
        };

        if (!enabled) {
            drawCommonStyleArrow(image.rect(), creatorColor(Theme::IconsDisabledColor));
        } else {
            if (creatorTheme()->flag(Theme::ToolBarIconShadow))
                drawCommonStyleArrow(image.rect().translated(0, devicePixelRatio), toolBarDropShadowColor());
            drawCommonStyleArrow(image.rect(), creatorColor(Theme::IconsBaseColor));
        }
        painter.end();
        pixmap = QPixmap::fromImage(image);
        pixmap.setDevicePixelRatio(devicePixelRatio);
        QPixmapCache::insert(pixmapName, pixmap);
    }
    int xOffset = r.x() + (r.width() - size)/2;
    int yOffset = r.y() + (r.height() - size)/2;
    painter->drawPixmap(xOffset, yOffset, pixmap);
}

void StyleHelper::drawMinimalArrow(QStyle::PrimitiveElement element, QPainter *painter, const QStyleOption *option)
{
    if (option->rect.width() <= 1 || option->rect.height() <= 1)
        return;

    const qreal devicePixelRatio = painter->device()->devicePixelRatio();
    const bool enabled = option->state & QStyle::State_Enabled;
    QRect r = option->rect;
    int size = qMin(r.height(), r.width());
    QPixmap pixmap;
    const QString pixmapName = QString::asprintf("StyleHelper::drawMinimalArrow-%d-%d-%d-%f",
                                                 element, size, enabled, devicePixelRatio);
    if (!QPixmapCache::find(pixmapName, &pixmap)) {
        QImage image(size * devicePixelRatio, size * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        QStyleOption tweakedOption(*option);

        double rotation = 0;
        switch (element) {
        case QStyle::PE_IndicatorArrowLeft:
            rotation = 45;
            break;
        case QStyle::PE_IndicatorArrowUp:
            rotation = 135;
            break;
        case QStyle::PE_IndicatorArrowRight:
            rotation = 225;
            break;
        case QStyle::PE_IndicatorArrowDown:
            rotation = 315;
            break;
        default:
            break;
        }

        auto drawArrow = [&tweakedOption, rotation, &painter](const QRect &rect, const QColor &color) -> void
        {
            static const QCommonStyle* const style = qobject_cast<QCommonStyle*>(QApplication::style());
            if (!style)
                return;

            // Workaround for QTCREATORBUG-28470
            QPalette pal = tweakedOption.palette;
            pal.setBrush(QPalette::Base, pal.text()); // Base and Text differ, causing a detachment.
            // Inspired by tst_QPalette::cacheKey()
            pal.setColor(QPalette::ButtonText, color.rgb());

            tweakedOption.palette = pal;
            tweakedOption.rect = rect;

            painter.save();
            painter.setOpacity(color.alphaF());

            double minDim = std::min(rect.width(), rect.height());
            double innerWidth = minDim/M_SQRT2;
            int penWidth = std::max(innerWidth/4, 1.0);
            innerWidth -= penWidth;

            QPen pPen(pal.color(QPalette::ButtonText), penWidth);
            pPen.setJoinStyle(Qt::MiterJoin);
            painter.setBrush(pal.text());
            painter.setPen(pPen);

            painter.translate(rect.center());
            painter.rotate(rotation);
            painter.translate(-innerWidth/2, -innerWidth/2);

            const QPointF points[3] = {
                {0, 0},
                {0, innerWidth},
                {innerWidth, innerWidth}
            };

            painter.drawPolyline(points, 3);
            painter.restore();
        };

        if (enabled) {
            if (creatorTheme()->flag(Theme::ToolBarIconShadow))
                drawArrow(image.rect().translated(0, devicePixelRatio), toolBarDropShadowColor());
            drawArrow(image.rect(), creatorColor(Theme::IconsBaseColor));
        } else {
            drawArrow(image.rect(), creatorColor(Theme::IconsDisabledColor));
        }
        painter.end();
        pixmap = QPixmap::fromImage(image);
        pixmap.setDevicePixelRatio(devicePixelRatio);
        QPixmapCache::insert(pixmapName, pixmap);
    }
    int xOffset = r.x() + (r.width() - size)/2;
    int yOffset = r.y() + (r.height() - size)/2;
    painter->drawPixmap(xOffset, yOffset, pixmap);
}

void StyleHelper::drawPanelBgRect(QPainter *painter, const QRectF &rect, const QBrush &brush)
{
    if (toolbarStyle() == ToolbarStyle::Compact) {
        painter->fillRect(rect.toRect(), brush);
    } else {
        constexpr int margin = SpacingTokens::PaddingVXxs;
        drawCardBg(painter, rect.adjusted(margin, margin, -margin, -margin), brush);
    }
}

void StyleHelper::drawCardBg(QPainter *painter, const QRectF &rect,
                             const QBrush &fill, const QPen &pen, qreal rounding)
{
    const qreal strokeWidth = pen.style() == Qt::NoPen ? 0 : pen.widthF();
    const qreal strokeShrink = strokeWidth / 2;
    const QRectF itemRectAdjusted = rect.adjusted(strokeShrink, strokeShrink,
                                                  -strokeShrink, -strokeShrink);
    const qreal roundingAdjusted = rounding - strokeShrink;
    QPainterPath itemOutlinePath;
    itemOutlinePath.addRoundedRect(itemRectAdjusted, roundingAdjusted, roundingAdjusted);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(fill);
    painter->setPen(pen);
    painter->drawPath(itemOutlinePath);
    painter->restore();
}

void StyleHelper::menuGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect)
{
    if (StyleHelper::usePixmapCache()) {
        const QString key = QString::asprintf("mh_menu %d %d %d %d %d",
            spanRect.width(), spanRect.height(), clipRect.width(),
            clipRect.height(), StyleHelper::baseColor().rgb());

        QPixmap pixmap;
        if (!QPixmapCache::find(key, &pixmap)) {
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

bool StyleHelper::usePixmapCache()
{
    return true;
}

QPixmap StyleHelper::disabledSideBarIcon(const QPixmap &enabledicon)
{
    QImage im = enabledicon.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y=0; y<im.height(); ++y) {
        auto scanLine = reinterpret_cast<QRgb*>(im.scanLine(y));
        for (int x=0; x<im.width(); ++x) {
            QRgb pixel = *scanLine;
            char intensity = char(qGray(pixel));
            *scanLine = qRgba(intensity, intensity, intensity, qAlpha(pixel));
            ++scanLine;
        }
    }
    return QPixmap::fromImage(im);
}

// Draws a cached pixmap with shadow
void StyleHelper::drawIconWithShadow(const QIcon &icon, const QRect &rect,
                                     QPainter *p, QIcon::Mode iconMode, QIcon::State iconState,
                                     int dipRadius, const QColor &color, const QPoint &dipOffset)
{
    QPixmap cache;
    const qreal devicePixelRatio = p->device()->devicePixelRatioF();
    QString pixmapName = QString::fromLatin1("icon %0 %1 %2 %3")
            .arg(icon.cacheKey()).arg(iconMode).arg(rect.height()).arg(devicePixelRatio);

    if (!QPixmapCache::find(pixmapName, &cache)) {
        // High-dpi support: The in parameters (rect, radius, offset) are in
        // device-independent pixels. The call to QIcon::pixmap() below might
        // return a high-dpi pixmap, which will in that case have a devicePixelRatio
        // different than 1. The shadow drawing caluculations are done in device
        // pixels.
        QPixmap px = icon.pixmap(rect.size(), devicePixelRatio, iconMode, iconState);
        // icon.pixmap() never upscales: if no source pixmap with a high enough
        // resolution is available (e.g. icons only have up to @2x variants, but
        // devicePixelRatio is higher), it returns a smaller pixmap with a lower
        // devicePixelRatio instead. Use that actual ratio, otherwise the icon
        // would be painted too small.
        const qreal pixmapDevicePixelRatio = px.devicePixelRatio();
        int radius = int(dipRadius * pixmapDevicePixelRatio);
        QPoint offset = dipOffset * pixmapDevicePixelRatio;
        cache = QPixmap(px.size() + QSize(radius * 2, radius * 2));
        cache.fill(Qt::transparent);

        QPainter cachePainter(&cache);
        if (iconMode == QIcon::Disabled) {
            const bool hasDisabledState =
                    icon.availableSizes().count() == icon.availableSizes(QIcon::Disabled).count();
            if (!hasDisabledState)
                px = disabledSideBarIcon(icon.pixmap(rect.size(), devicePixelRatio));
        } else if (creatorTheme()->flag(Theme::ToolBarIconShadow)) {
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

            // draw the blurred drop shadow...
            cachePainter.drawImage(QRect(0, 0, cache.rect().width(), cache.rect().height()), tmp);
        }

        // Draw the actual pixmap...
        cachePainter.drawPixmap(QRect(QPoint(radius, radius) + offset, QSize(px.width(), px.height())), px);
        cachePainter.end();
        cache.setDevicePixelRatio(pixmapDevicePixelRatio);
        QPixmapCache::insert(pixmapName, cache);
    }

    QRect targetRect = cache.rect();
    targetRect.setSize(targetRect.size() / cache.devicePixelRatio());
    targetRect.moveCenter(rect.center() - dipOffset);
    p->drawPixmap(targetRect, cache);
}

// Draws a CSS-like border image where the defined borders are not stretched
// Unit for rect, left, top, right and bottom is user pixels
void StyleHelper::drawCornerImage(const QImage &img, QPainter *painter, const QRect &rect,
                                  int left, int top, int right, int bottom)
{
    // source rect for drawImage() calls needs to be specified in DIP unit of the image
    const qreal imagePixelRatio = img.devicePixelRatio();
    const qreal leftDIP = left * imagePixelRatio;
    const qreal topDIP = top * imagePixelRatio;
    const qreal rightDIP = right * imagePixelRatio;
    const qreal bottomDIP = bottom * imagePixelRatio;

    const QSize size = img.size();
    if (top > 0) { //top
        painter->drawImage(QRectF(rect.left() + left, rect.top(), rect.width() -right - left, top), img,
                           QRectF(leftDIP, 0, size.width() - rightDIP - leftDIP, topDIP));
        if (left > 0) //top-left
            painter->drawImage(QRectF(rect.left(), rect.top(), left, top), img,
                               QRectF(0, 0, leftDIP, topDIP));
        if (right > 0) //top-right
            painter->drawImage(QRectF(rect.left() + rect.width() - right, rect.top(), right, top), img,
                               QRectF(size.width() - rightDIP, 0, rightDIP, topDIP));
    }
    //left
    if (left > 0)
        painter->drawImage(QRectF(rect.left(), rect.top()+top, left, rect.height() - top - bottom), img,
                           QRectF(0, topDIP, leftDIP, size.height() - bottomDIP - topDIP));
    //center
    painter->drawImage(QRectF(rect.left() + left, rect.top()+top, rect.width() -right - left,
                              rect.height() - bottom - top), img,
                       QRectF(leftDIP, topDIP, size.width() - rightDIP - leftDIP,
                              size.height() - bottomDIP - topDIP));
    if (right > 0) //right
        painter->drawImage(QRectF(rect.left() +rect.width() - right, rect.top()+top, right, rect.height() - top - bottom), img,
                           QRectF(size.width() - rightDIP, topDIP, rightDIP, size.height() - bottomDIP - topDIP));
    if (bottom > 0) { //bottom
        painter->drawImage(QRectF(rect.left() +left, rect.top() + rect.height() - bottom,
                                  rect.width() - right - left, bottom), img,
                           QRectF(leftDIP, size.height() - bottomDIP,
                                  size.width() - rightDIP - leftDIP, bottomDIP));
        if (left > 0) //bottom-left
            painter->drawImage(QRectF(rect.left(), rect.top() + rect.height() - bottom, left, bottom), img,
                               QRectF(0, size.height() - bottomDIP, leftDIP, bottomDIP));
        if (right > 0) //bottom-right
            painter->drawImage(QRectF(rect.left() + rect.width() - right, rect.top() + rect.height() - bottom, right, bottom), img,
                               QRectF(size.width() - rightDIP, size.height() - bottomDIP, rightDIP, bottomDIP));
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

QPixmap StyleHelper::tintedPixmap(const QPixmap &pixmap, const QColor &color)
{
    if (pixmap.isNull())
        return pixmap;
    QPixmap result(pixmap.size());
    result.setDevicePixelRatio(pixmap.devicePixelRatio());
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.drawPixmap(0, 0, pixmap);
    // Keep the source alpha as a mask and flatten it to a single color. Unlike
    // Icon's luminance mask this preserves transparency, so a transparent SVG
    // does not turn into a solid rectangle.
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(result.rect(), color);
    p.end();
    return result;
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

void StyleHelper::setPanelWidget(QWidget *widget, bool value)
{
    widget->setProperty(C_PANEL_WIDGET, value);
}

void StyleHelper::setPanelWidgetSingleRow(QWidget *widget, bool value)
{
    widget->setProperty(C_PANEL_WIDGET_SINGLE_ROW, value);
}

Qt::HighDpiScaleFactorRoundingPolicy StyleHelper::defaultHighDpiScaleFactorRoundingPolicy()
{
    return HostOsInfo::isMacHost() ? Qt::HighDpiScaleFactorRoundingPolicy::Unset
                                   : Qt::HighDpiScaleFactorRoundingPolicy::Round;
}

QIcon StyleHelper::getIconFromIconFont(const QString &fontName, const QList<IconFontHelper> &parameters)
{
    QTC_ASSERT(QFontDatabase::hasFamily(fontName), {});

    QIcon icon;

    for (const IconFontHelper &p : parameters) {
        const int maxDpr = qRound(qApp->devicePixelRatio());
        for (int dpr = 1; dpr <= maxDpr; dpr++) {
            QPixmap pixmap(p.size() * dpr);
            pixmap.setDevicePixelRatio(dpr);
            pixmap.fill(Qt::transparent);

            QFont font(fontName);
            font.setPixelSize(p.size().height());

            QPainter painter(&pixmap);
            painter.save();
            painter.setPen(p.color());
            painter.setFont(font);
            painter.drawText(QRectF(QPoint(0, 0), p.size()), p.iconSymbol());
            painter.restore();

            icon.addPixmap(pixmap, p.mode(), p.state());
        }
    }

    return icon;
}

QIcon StyleHelper::getIconFromIconFont(const QString &fontName, const QString &iconSymbol, int fontSize, int iconSize, QColor color)
{
    QTC_ASSERT(QFontDatabase::hasFamily(fontName), {});

    QIcon icon;
    QSize size(iconSize, iconSize);

    const int maxDpr = qRound(qApp->devicePixelRatio());
    for (int dpr = 1; dpr <= maxDpr; dpr++) {
        QPixmap pixmap(size * dpr);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);

        QFont font(fontName);
        font.setPixelSize(fontSize);

        QPainter painter(&pixmap);
        painter.save();
        painter.setPen(color);
        painter.setFont(font);
        painter.drawText(QRectF(QPoint(0, 0), size), Qt::AlignCenter, iconSymbol);
        painter.restore();

        icon.addPixmap(pixmap);
    }

    return icon;
}

QIcon StyleHelper::getIconFromIconFont(const QString &fontName, const QString &iconSymbol, int fontSize, int iconSize)
{
    QColor penColor = QApplication::palette("QWidget").color(QPalette::Normal, QPalette::ButtonText);
    return getIconFromIconFont(fontName, iconSymbol, fontSize, iconSize, penColor);
}

QIcon StyleHelper::getCursorFromIconFont(const QString &fontName, const QString &cursorFill, const QString &cursorOutline,
                                         int fontSize, int iconSize)
{
    QTC_ASSERT(QFontDatabase::hasFamily(fontName), {});

    const QColor outlineColor = Qt::black;
    const QColor fillColor = Qt::white;

    QIcon icon;
    QSize size(iconSize, iconSize);

    const int maxDpr = qRound(qApp->devicePixelRatio());
    for (int dpr = 1; dpr <= maxDpr; dpr++) {
        QPixmap pixmap(size * dpr);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);

        QFont font(fontName);
        font.setPixelSize(fontSize);

        QPainter painter(&pixmap);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::LosslessImageRendering, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        painter.setFont(font);
        painter.setPen(outlineColor);
        painter.drawText(QRectF(QPointF(0.0, 0.0), size),
                         Qt::AlignCenter, cursorOutline);

        painter.setPen(fillColor);
        painter.drawText(QRectF(QPointF(0.0, 0.0), size),
                         Qt::AlignCenter, cursorFill);

        painter.restore();

        icon.addPixmap(pixmap);
    }

    return icon;
}

QString StyleHelper::dpiSpecificImageFile(const QString &fileName)
{
    // See QIcon::addFile()
    if (qApp->devicePixelRatio() > 1.0) {
        const QString atDprfileName =
                imageFileWithResolution(fileName, qRound(qApp->devicePixelRatio()));
        if (QFileInfo::exists(atDprfileName))
            return atDprfileName;
    }
    return fileName;
}

QString StyleHelper::imageFileWithResolution(const QString &fileName, int dpr)
{
    return qt_findAtNxFile(fileName, dpr);
}

QList<int> StyleHelper::availableImageResolutions(const QString &fileName)
{
    QList<int> result;
    const int maxResolutions = qApp->devicePixelRatio();
    for (int i = 1; i <= maxResolutions; ++i)
        if (QFileInfo::exists(imageFileWithResolution(fileName, i)))
            result.append(i);
    return result;
}

double StyleHelper::luminance(const QColor &color)
{
    // calculate the luminance based on
    // https://www.w3.org/TR/2008/REC-WCAG20-20081211/#relativeluminancedef
    auto val = [](const double &colorVal) {
        return colorVal < 0.03928 ? colorVal / 12.92 : std::pow((colorVal + 0.055) / 1.055, 2.4);
    };

    static QHash<QRgb, double> cache;
    const QRgb rgb = color.rgb();
    QHash<QRgb, double>::iterator it = cache.find(rgb);
    if (it == cache.end()) {
        it = cache.insert(rgb, 0.2126 * val(qRed(rgb) / 255.)
                          + 0.7152 * val(qGreen(rgb) / 255.)
                          + 0.0722 * val(qBlue(rgb) / 255.));
    }
    return it.value();
}

static double contrastRatio(const QColor &color1, const QColor &color2)
{
    // calculate the contrast ratio based on
    // https://www.w3.org/TR/2008/REC-WCAG20-20081211/#contrast-ratiodef
    auto contrast = (StyleHelper::luminance(color1) + .05) / (StyleHelper::luminance(color2) + .05);
    if (contrast < 1)
        return 1 / contrast;
    return contrast;
}

bool StyleHelper::isReadableOn(const QColor &background, const QColor &foreground)
{
    // following the W3C Recommendation on contrast for large Text
    // https://www.w3.org/TR/2008/REC-WCAG20-20081211/#contrast-ratiodef
    return contrastRatio(background, foreground) > 3;
}

QColor StyleHelper::ensureReadableOn(const QColor &background, const QColor &desiredForeground)
{
    if (isReadableOn(background, desiredForeground))
        return desiredForeground;

    int hue = 0;
    int saturation = 0;
    int value = 0;
    desiredForeground.getHsv(&hue, &saturation, &value);

    const auto atValue = [&](int v) {
        return QColor::fromHsv(hue, saturation, v, desiredForeground.alpha());
    };
    const auto atSaturation = [&](int s) {
        return QColor::fromHsv(hue, s, 255, desiredForeground.alpha());
    };
    // getHsv() and fromHsv() quantize, so the color at the desired value is not
    // quite the desired color, and the round trip can land on a readable one.
    // That one is then the nearest there is - it is a step away from what was
    // asked for - and taking it leaves the bisection below with the unreadable
    // end it assumes.
    if (isReadableOn(background, atValue(value)))
        return atValue(value);

    // Readability is monotonic in the coordinate once the direction is fixed,
    // so the readable color nearest the unreadable one is a bisection away.
    const auto nearestReadable =
        [&](const auto &colorAt, int unreadable, int limit) -> std::optional<int> {
        if (!isReadableOn(background, colorAt(limit)))
            return std::nullopt;
        int readable = limit;
        while (qAbs(readable - unreadable) > 1) {
            const int middle = (readable + unreadable) / 2;
            if (isReadableOn(background, colorAt(middle)))
                readable = middle;
            else
                unreadable = middle;
        }
        return readable;
    };

    const std::optional<int> lighter = nearestReadable(atValue, value, 255);
    const std::optional<int> darker = nearestReadable(atValue, value, 0);
    if (lighter && darker)
        return atValue(*lighter - value <= value - *darker ? *lighter : *darker);
    if (lighter)
        return atValue(*lighter);
    if (darker)
        return atValue(*darker);

    // A saturated hue has a luminance ceiling that no value lifts: pure blue
    // stays too dark for a dark background however bright it is made. Giving up
    // saturation raises that ceiling, so the hue is kept and the value goes to
    // the top with the saturation: of the three, only the hue survives here.
    //
    // A saturation that reads always exists: neither direction of the value
    // reaching 3:1 means pure black does not read on this background, which
    // leaves it dark enough for pure white to.
    //
    // The hue survives because that saturation is never 0. Reaching this line
    // at all needs black to be unreadable, and black is atValue(0), so
    // (luminance(background) + .05) / .05 <= 3, that is luminance <= .1. The
    // bisection returns 0 only where one step of saturation already fails while
    // white reads, and the least luminous color one step from white - hue 211,
    // luminance .9917 - puts that at luminance >= (.9917 + .05) / 3 - .05, that
    // is >= .297. No background is both.
    const std::optional<int> desaturated = nearestReadable(atSaturation, saturation, 0);
    QTC_ASSERT(desaturated, return desiredForeground);
    return atSaturation(*desaturated);
}

// A color in linear sRGB: the light the components stand for, before the
// gamma encoding sRGB keeps them in.
using LinearRgb = std::array<double, 3>;

// A component may land a hair outside the range it has to be in, which is
// rounding in the conversion rather than a color out of gamut. A ten
// thousandth of the range is well below what an eight-bit component keeps.
const double GamutTolerance = 0.0001;

static LinearRgb oklchLinearRgb(double lightness, double chroma, double hue)
{
    const double a = chroma * std::cos(qDegreesToRadians(hue));
    const double b = chroma * std::sin(qDegreesToRadians(hue));
    // The inverses of the two matrices oklab() ends with: Oklab's axes back to
    // the cone responses of the eye, and those back to linear sRGB.
    const double longCone = std::pow(lightness + 0.3963377774 * a + 0.2158037573 * b, 3);
    const double mediumCone = std::pow(lightness - 0.1055613458 * a - 0.0638541728 * b, 3);
    const double shortCone = std::pow(lightness - 0.0894841775 * a - 1.2914855480 * b, 3);
    return {4.0767416621 * longCone - 3.3077115913 * mediumCone + 0.2309699292 * shortCone,
            -1.2684380046 * longCone + 2.6097574011 * mediumCone - 0.3413193965 * shortCone,
            -0.0041960863 * longCone - 0.7034186147 * mediumCone + 1.7076147010 * shortCone};
}

// The largest chroma up to the one asked for that this lightness and hue have
// in sRGB. Desaturating until the color fits keeps the lightness a palette is
// built on, clamping the components would not.
double StyleHelper::oklchFittingChroma(double lightness, double chroma, double hue)
{
    const auto fits = [lightness, hue](double atChroma) {
        const LinearRgb rgb = oklchLinearRgb(lightness, atChroma, hue);
        return std::all_of(rgb.cbegin(), rgb.cend(), [](double component) {
            return component >= -GamutTolerance && component <= 1 + GamutTolerance;
        });
    };

    if (fits(chroma))
        return chroma;
    // Every step halves the interval the gamut boundary is known to lie in, so
    // sixteen of them come within a 65536th of the chroma asked for - finer
    // than an eight-bit component can tell apart.
    const int BisectionSteps = 16;
    double tooLow = 0;
    double tooHigh = chroma;
    for (int step = 0; step < BisectionSteps; ++step) {
        const double middle = (tooLow + tooHigh) / 2;
        if (fits(middle))
            tooLow = middle;
        else
            tooHigh = middle;
    }
    return tooLow;
}

// An Oklch color, converted to sRGB. The hue is in degrees, the rest is in
// [0, 1].
QColor StyleHelper::oklchColor(double lightness, double chroma, double hue)
{
    // Oklch is linear about light, sRGB is not: the transfer function of sRGB
    // (https://en.wikipedia.org/wiki/SRGB) is what QColor takes its components
    // in, and leaving it out paints a far darker color than the one asked for.
    const auto gammaEncoded = [](double component) {
        component = std::clamp(component, 0.0, 1.0);
        return component <= 0.0031308 ? 12.92 * component
                                      : 1.055 * std::pow(component, 1 / 2.4) - 0.055;
    };
    const LinearRgb rgb =
        oklchLinearRgb(lightness, oklchFittingChroma(lightness, chroma, hue), hue);
    return QColor::fromRgbF(gammaEncoded(rgb[0]), gammaEncoded(rgb[1]), gammaEncoded(rgb[2]));
}

// The lightness at which a hue has the most chroma in sRGB.
double StyleHelper::oklchMostChromaticLightness(double hue)
{
    // The range text is worth drawing in, walked finely enough to place the
    // peak within a hundredth of a lightness of where it is.
    const double DarkestLightness = 0.3;
    const double LightestLightness = 0.95;
    const int LightnessSteps = 64;

    double bestLightness = DarkestLightness;
    double bestChroma = 0;
    for (int step = 0; step <= LightnessSteps; ++step) {
        const double lightness = DarkestLightness
                                 + step * (LightestLightness - DarkestLightness) / LightnessSteps;
        const double chroma = oklchFittingChroma(lightness, oklchFullChroma, hue);
        if (chroma > bestChroma) {
            bestChroma = chroma;
            bestLightness = lightness;
        }
    }
    return bestLightness;
}

// A color in Oklab: the lightness oklchColor() takes one on, and the a and b
// that a chroma is the length of.
StyleHelper::OklabColor StyleHelper::oklab(const QColor &color)
{
    // Back through the transfer function, to the light the matrices below take.
    const auto linear = [](double component) {
        return component <= 0.04045 ? component / 12.92
                                    : std::pow((component + 0.055) / 1.055, 2.4);
    };
    const double r = linear(color.redF());
    const double g = linear(color.greenF());
    const double b = linear(color.blueF());
    // Light into the cone responses of the eye - long, medium and short
    // wavelength - and their cube roots into the axes of Oklab, which is what
    // gives it a lightness that matches what is seen. The matrices are the
    // ones in https://en.wikipedia.org/wiki/Oklab_color_space.
    const double longCone = std::cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    const double mediumCone = std::cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    const double shortCone = std::cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
    return {0.2104542553 * longCone + 0.7936177850 * mediumCone - 0.0040720468 * shortCone,
            1.9779984951 * longCone - 2.4285922050 * mediumCone + 0.4505937099 * shortCone,
            0.0259040371 * longCone + 0.7827717662 * mediumCone - 0.8086757660 * shortCone};
}

static const QStringList &applicationFontFamilies()
{
    const static QStringList families = [] {
        // Font is either installed in the system, or was loaded from share/qtcreator/fonts/
        const QStringList candidates = {"Inter", "Inter Variable"};
        const QString family = Utils::findOrDefault(candidates, &QFontDatabase::hasFamily);
        return family.isEmpty() ? QStringList() : QStringList(family);
    }();
    return families;
}

static const QStringList &brandFontFamilies()
{
    const static QStringList families = []{
        const int id = QFontDatabase::addApplicationFont(":/studiofonts/TitilliumWeb-Regular.ttf");
        return id >= 0 ? QFontDatabase::applicationFontFamilies(id) : QStringList();
    }();
    return families;
}

struct UiFontMetrics {
    // Original "text token" values are defined in pixels
    const int pixelSize = -1;
    const int lineHeight = -1;
    const QFont::Weight weight = QFont::Normal;
};

static const UiFontMetrics& uiFontMetrics(StyleHelper::UiElement element)
{
    static const std::map<StyleHelper::UiElement, UiFontMetrics> metrics {
        {StyleHelper::UiElementH1,                  {36, 54, QFont::DemiBold}},
        {StyleHelper::UiElementH2,                  {28, 44, QFont::DemiBold}},
        {StyleHelper::UiElementH3,                  {18, 24, QFont::DemiBold}},
        {StyleHelper::UiElementH4,                  {16, 20, QFont::Bold}},
        {StyleHelper::UiElementH5,                  {14, 16, QFont::DemiBold}},
        {StyleHelper::UiElementH6,                  {12, 14, QFont::DemiBold}},
        {StyleHelper::UiElementH6Capital,           {12, 14, QFont::DemiBold}},
        {StyleHelper::UiElementBody1,               {14, 20, QFont::Light}},
        {StyleHelper::UiElementBody2,               {12, 20, QFont::Light}},
        {StyleHelper::UiElementButtonMedium,        {12, 16, QFont::Bold}},
        {StyleHelper::UiElementButtonSmall,         {10, 12, QFont::Bold}},
        {StyleHelper::UiElementLabelMedium,         {12, 16, QFont::DemiBold}},
        {StyleHelper::UiElementLabelSmall,          {10, 12, QFont::DemiBold}},
        {StyleHelper::UiElementCaptionStrong,       {10, 12, QFont::DemiBold}},
        {StyleHelper::UiElementCaption,             {10, 12, QFont::Normal}},
        {StyleHelper::UiElementIconStandard,        {12, 16, QFont::Medium}},
        {StyleHelper::UiElementIconActive,          {12, 16, QFont::DemiBold}},
    };
    QTC_ASSERT(metrics.count(element) > 0, return metrics.at(StyleHelper::UiElementCaptionStrong));
    return metrics.at(element);
}

QFont StyleHelper::uiFont(UiElement element)
{
    QFont font;

    switch (element) {
    case UiElementH1:
        font.setFamilies(brandFontFamilies());
        font.setWordSpacing(2);
        break;
    case UiElementH2:
        font.setFamilies(brandFontFamilies());
        break;
    case UiElementH6Capital:
        font.setCapitalization(QFont::AllUppercase);
        [[fallthrough]];
    default:
        if (!applicationFontFamilies().isEmpty())
            font.setFamilies(applicationFontFamilies());
        break;
    }

    const UiFontMetrics &metrics = uiFontMetrics(element);

    // On macOS, by default 72 dpi are assumed for conversion between point and pixel size.
    // For non-macOS, it is 96 dpi.
    constexpr qreal defaultDpi = HostOsInfo::isMacHost() ? 72.0 : 96.0;
    constexpr qreal pixelsToPointSizeFactor = 72.0 / defaultDpi;
    const qreal qrealPointSize = metrics.pixelSize * pixelsToPointSizeFactor;
    font.setPointSizeF(qrealPointSize);

    // Intermediate font weights can produce blurry rendering and are harder to read.
    // For "non-retina" screens, apply the weight only for some fonts.
    static const bool isHighDpi = qApp->devicePixelRatio() >= 2;
    const bool setWeight = isHighDpi || element == UiElementCaptionStrong
                           || element <= UiElementH4;
    if (setWeight)
        font.setWeight(metrics.weight);

    return font;
}

int StyleHelper::uiFontLineHeight(UiElement element)
{
    const UiFontMetrics &metrics = uiFontMetrics(element);
    const qreal lineHeightToPixelSizeRatio = qreal(metrics.lineHeight) / metrics.pixelSize;
    const QFontInfo fontInfo(uiFont(element));
    return qCeil(fontInfo.pixelSize() * lineHeightToPixelSizeRatio);
}

QString StyleHelper::fontToCssProperties(const QFont &font)
{
    const QString fontSize = font.pixelSize() != -1 ? QString::number(font.pixelSize()) + "px"
                                                    : QString::number(font.pointSizeF()) + "pt";
    const QString fontStyle = QLatin1String(font.style() == QFont::StyleNormal
                                                ? "normal" : font.style() == QFont::StyleItalic
                                                      ? "italic" : "oblique");
    const QString fontShorthand = fontStyle + " " + QString::number(font.weight()) + " "
                                  + fontSize + " '" + font.family() + "'";
    const QString textDecoration = QLatin1String(font.underline() ? "underline" : "none");
    const QString textTransform = QLatin1String(font.capitalization() == QFont::AllUppercase
                                                    ? "uppercase"
                                                    : font.capitalization() == QFont::AllLowercase
                                                          ? "lowercase" : "none");
    const QString propertyTemplate = "%1: %2";
    const QStringList cssProperties = {
        propertyTemplate.arg("font").arg(fontShorthand),
        propertyTemplate.arg("text-decoration").arg(textDecoration),
        propertyTemplate.arg("text-transform").arg(textTransform),
        propertyTemplate.arg("word-spacing").arg(font.wordSpacing()),
    };
    const QString fontCssStyle = cssProperties.join("; ");
    return fontCssStyle;
}

void StyleHelper::modifyPaletteBase(QWidget *widget, const QColor &color)
{
    QTC_ASSERT(widget, return);
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Base, color);
    widget->setPalette(palette);
}

void StyleHelper::setBackgroundColor(QWidget *widget, Theme::Color colorRole)
{
    QPalette palette = widget->palette();
    const QPalette::ColorRole role = QPalette::Window;
    palette.setBrush(role, {});
    palette.setColor(role, creatorColor(colorRole));
    widget->setPalette(palette);
    widget->setBackgroundRole(role);
    widget->setAutoFillBackground(true);
}

} // namespace Utils
