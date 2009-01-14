/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include "core_global.h"

#include <QtCore/QRect>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QPalette>
#include <QtGui/QColor>

// Helper class holding all custom color values

class CORE_EXPORT StyleHelper
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

    // Sets the base color and makes sure all top level widgets are updated
    static void setBaseColor(const QColor &color);

    // Gradients used for panels
    static void horizontalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);
    static void verticalGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);
    static void menuGradient(QPainter *painter, const QRect &spanRect, const QRect &clipRect);

    // Pixmap cache should only be enabled for X11 due to slow gradients
    static bool usePixmapCache() {
        return true;
    }

private:
    static QColor m_baseColor;
};

#endif // STYLEHELPER_H
