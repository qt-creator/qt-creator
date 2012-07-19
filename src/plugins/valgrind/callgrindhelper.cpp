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
    return QString("<") + locale.toString(0.01f) + locale.percent();
}

} // namespace Internal
} // namespace Valgrind
