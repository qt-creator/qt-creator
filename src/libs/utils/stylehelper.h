// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QStyle>

QT_BEGIN_NAMESPACE
class QPalette;
class QPainter;
class QRect;
// Note, this is exported but in a private header as qtopengl depends on it.
// We should consider adding this as a public helper function.
void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

// Helper class holding all custom color values

namespace Utils {
class QTCREATOR_UTILS_EXPORT StyleHelper
{
public:
    static const unsigned int DEFAULT_BASE_COLOR = 0x666666;
    static const int progressFadeAnimationDuration = 600;

    enum ToolbarStyle {
        ToolbarStyleCompact,
        ToolbarStyleRelaxed,
    };

    // Height of the project explorer navigation bar
    static int navigationWidgetHeight();
    static void setToolbarStyle(ToolbarStyle style);
    static ToolbarStyle toolbarStyle();
    static constexpr ToolbarStyle defaultToolbarStyle = ToolbarStyleCompact;
    static qreal sidebarFontSize();
    static QPalette sidebarFontPalette(const QPalette &original);

    // This is our color table, all colors derive from baseColor
    static QColor requestedBaseColor() { return m_requestedBaseColor; }
    static QColor baseColor(bool lightColored = false);
    static QColor toolbarBaseColor(bool lightColored = false);
    static QColor panelTextColor(bool lightColored = false);
    static QColor highlightColor(bool lightColored = false);
    static QColor shadowColor(bool lightColored = false);
    static QColor borderColor(bool lightColored = false);
    static QColor toolBarBorderColor();
    static QColor buttonTextColor() { return QColor(0x4c4c4c); }
    static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50);
    static QColor alphaBlendedColors(const QColor &colorA, const QColor &colorB);

    static QColor sidebarHighlight() { return QColor(255, 255, 255, 40); }
    static QColor sidebarShadow() { return QColor(0, 0, 0, 40); }

    static QColor toolBarDropShadowColor() { return QColor(0, 0, 0, 70); }

    static QColor notTooBrightHighlightColor();

    // Sets the base color and makes sure all top level widgets are updated
    static void setBaseColor(const QColor &color);

    // Draws a shaded anti-aliased arrow
    static void drawArrow(QStyle::PrimitiveElement element, QPainter *painter, const QStyleOption *option);
    static void drawMinimalArrow(QStyle::PrimitiveElement element, QPainter *painter, const QStyleOption *option);

    static void drawPanelBgRect(QPainter *painter, const QRectF &rect, const QBrush &brush);

    // Gradients used for panels
    static void horizontalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect, bool lightColored = false);
    static void verticalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect, bool lightColored = false);
    static void menuGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);
    static bool usePixmapCache() { return true; }

    static QPixmap disabledSideBarIcon(const QPixmap &enabledicon);
    static void drawIconWithShadow(const QIcon &icon, const QRect &rect, QPainter *p, QIcon::Mode iconMode,
                                   int dipRadius = 3, const QColor &color = QColor(0, 0, 0, 130),
                                   const QPoint &dipOffset = QPoint(1, -2));
    static void drawCornerImage(const QImage &img, QPainter *painter, const QRect &rect,
                         int left = 0, int top = 0, int right = 0, int bottom = 0);

    static void tintImage(QImage &img, const QColor &tintColor);
    static QLinearGradient statusBarGradient(const QRect &statusBarRect);
    static void setPanelWidget(QWidget *widget, bool value = true);
    static void setPanelWidgetSingleRow(QWidget *widget, bool value = true);

    static bool isQDSTheme();

    class IconFontHelper
    {
        public:
            IconFontHelper(const QString &iconSymbol,
                           const QColor &color,
                           const QSize &size,
                           QIcon::Mode mode = QIcon::Normal,
                           QIcon::State state = QIcon::Off)
                : m_iconSymbol(iconSymbol)
                , m_color(color)
                , m_size(size)
                , m_mode(mode)
                , m_state(state)
            {}

            QString iconSymbol() const { return m_iconSymbol; }
            QColor color() const { return m_color; }
            QSize size() const { return m_size; }
            QIcon::Mode mode() const { return m_mode; }
            QIcon::State state() const { return m_state; }

        private:
            QString m_iconSymbol;
            QColor m_color;
            QSize m_size;
            QIcon::Mode m_mode;
            QIcon::State m_state;
    };

    static QIcon getIconFromIconFont(const QString &fontName, const QList<IconFontHelper> &parameters);
    static QIcon getIconFromIconFont(const QString &fontName, const QString &iconSymbol, int fontSize, int iconSize, QColor color);
    static QIcon getIconFromIconFont(const QString &fontName, const QString &iconSymbol, int fontSize, int iconSize);
    static QIcon getCursorFromIconFont(const QString &fontname, const QString &cursorFill, const QString &cursorOutline,
                                       int fontSize, int iconSize);

    static QString dpiSpecificImageFile(const QString &fileName);
    static QString imageFileWithResolution(const QString &fileName, int dpr);
    static QList<int> availableImageResolutions(const QString &fileName);

    static double luminance(const QColor &color);
    static bool isReadableOn(const QColor &background, const QColor &foreground);
    // returns a foreground color readable on background (desiredForeground if already readable or adaption fails)
    static QColor ensureReadableOn(const QColor &background, const QColor &desiredForeground);

private:
    static ToolbarStyle m_toolbarStyle;
    static QColor m_baseColor;
    static QColor m_requestedBaseColor;
};

} // namespace Utils
