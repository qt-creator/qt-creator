/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef STYLEHELPER_H
#define STYLEHELPER_H

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

    // Height of the project explorer navigation bar
    static int navigationWidgetHeight() { return 24; }
    static qreal sidebarFontSize();
    static QPalette sidebarFontPalette(const QPalette &original);

    // This is our color table, all colors derive from baseColor
    static QColor requestedBaseColor() { return m_requestedBaseColor; }
    static QColor baseColor(bool lightColored = false);
    static QColor panelTextColor(bool lightColored = false);
    static QColor highlightColor(bool lightColored = false);
    static QColor shadowColor(bool lightColored = false);
    static QColor borderColor(bool lightColored = false);
    static QColor buttonTextColor() { return QColor(0x4c4c4c); }
    static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50);

    static QColor sidebarHighlight() { return QColor(255, 255, 255, 40); }
    static QColor sidebarShadow() { return QColor(0, 0, 0, 40); }

    static QColor notTooBrightHighlightColor();

    // Sets the base color and makes sure all top level widgets are updated
    static void setBaseColor(const QColor &color);

    // Draws a shaded anti-aliased arrow
    static void drawArrow(QStyle::PrimitiveElement element, QPainter *painter, const QStyleOption *option);

    // Gradients used for panels
    static void horizontalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect, bool lightColored = false);
    static void verticalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect, bool lightColored = false);
    static void menuGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);
    static bool usePixmapCache() { return true; }

    static void drawIconWithShadow(const QIcon &icon, const QRect &rect, QPainter *p, QIcon::Mode iconMode,
                                   int dipRadius = 3, const QColor &color = QColor(0, 0, 0, 130),
                                   const QPoint &dipOffset = QPoint(1, -2));
    static void drawCornerImage(const QImage &img, QPainter *painter, const QRect &rect,
                         int left = 0, int top = 0, int right = 0, int bottom = 0);

    static void tintImage(QImage &img, const QColor &tintColor);
    static QLinearGradient statusBarGradient(const QRect &statusBarRect);

    static QString dpiSpecificImageFile(const QString &fileName);
    static QString imageFileWithResolution(const QString &fileName, int dpr);
    static QList<int> availableImageResolutions(const QString &fileName);

private:
    static QColor m_baseColor;
    static QColor m_requestedBaseColor;
};

} // namespace Utils
#endif // STYLEHELPER_H
