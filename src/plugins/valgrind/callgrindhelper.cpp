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

#include "callgrindhelper.h"

#include <cstdlib>

#include <QColor>
#include <QMap>
#include <QString>

namespace Valgrind {
namespace Internal {

QColor CallgrindHelper::colorForString(const QString &text)
{
    static QMap<QString, QColor> colorCache;

    if (colorCache.contains(text))
        return colorCache.value(text);

    // Minimum lightness of 100 to be readable with black text.
    const QColor color = QColor::fromHsl(((qreal)qrand() / RAND_MAX * 359),
                                         ((qreal)qrand() / RAND_MAX * 255),
                                         ((qreal)qrand() / RAND_MAX * 127) + 128);
    colorCache[text] = color;
    return color;
}

QColor CallgrindHelper::colorForCostRatio(qreal ratio)
{
    ratio = qBound(qreal(0.0), ratio, qreal(1.0));
    return QColor::fromHsv(120 - ratio * 120, 255, 255, (-((ratio-1) * (ratio-1))) * 120 + 120);
}

QString CallgrindHelper::toPercent(float costs, const QLocale &locale)
{
    if (costs > 99.9f)
        return locale.toString(100) + locale.percent();
    if (costs > 9.99f)
        return locale.toString(costs, 'f', 1) + locale.percent();
    if (costs > 0.009f)
        return locale.toString(costs, 'f', 2) + locale.percent();
    return QLatin1Char('<') + locale.toString(0.01f) + locale.percent();
}

} // namespace Internal
} // namespace Valgrind
