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

#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include "utils_global.h"

#include <QtGui/QColor>

QT_BEGIN_NAMESPACE
class QPalette;
class QPainter;
class QRect;
QT_END_NAMESPACE

// Helper class holding all custom color values

namespace Utils {
class QTCREATOR_UTILS_EXPORT StyleHelper
{
public:
    // Height of the project explorer navigation bar
    static int navigationWidgetHeight() { return 24; }
    static qreal sidebarFontSize();
    static QPalette sidebarFontPalette(const QPalette &original);

    // This is our color table, all colors derive from baseColor
    static QColor baseColor();
    static QColor panelTextColor();
    static QColor highlightColor();
    static QColor shadowColor();
    static QColor borderColor();
    static QColor buttonTextColor() { return QColor(0x4c4c4c); }
    static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50);

    // Sets the base color and makes sure all top level widgets are updated
    static void setBaseColor(const QColor &color);

    // Gradients used for panels
    static void horizontalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);
    static void verticalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);
    static void menuGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);

    // Pixmap cache should only be enabled for X11 due to slow gradients
    static bool usePixmapCache() { return true; }

private:
    static QColor m_baseColor;
};

} // namespace Utils
#endif // STYLEHELPER_H
