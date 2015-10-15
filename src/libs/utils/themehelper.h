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

#ifndef THEMEHELPER_H
#define THEMEHELPER_H

#include "utils_global.h"

QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace Utils {

class QTCREATOR_UTILS_EXPORT ThemeHelper
{
public:
    // Returns a recolored icon with shadow and custom disabled state for a
    // grayscale mask. The mask can range from a single image filename to
    // a list of filename|color,... pairs.
    // The color can be a Theme::Color enum key name. If invalid, it is run
    // through QColor(const QString &name).
    static QIcon themedIcon(const QString &mask);
    // Same as themedIcon() but without disabled state.
    static QPixmap themedIconPixmap(const QString &mask);

    // Simple recoloring of a mask. No shadow. maskImage is a single file.
    static QPixmap recoloredPixmap(const QString &maskImage, const QColor &color);

    static QColor inputfieldIconColor();
};

} // namespace Utils

#endif // THEMEHELPER_H
