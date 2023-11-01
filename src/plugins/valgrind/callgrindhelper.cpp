// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindhelper.h"

#include <cstdlib>

#include <QColor>
#include <QMap>
#include <QRandomGenerator>
#include <QString>

namespace Valgrind::Internal {

QColor CallgrindHelper::colorForString(const QString &text)
{
    static QMap<QString, QColor> colorCache;

    if (colorCache.contains(text))
        return colorCache.value(text);

    // Minimum lightness of 100 to be readable with black text.
    const QColor color = QColor::fromHsl(QRandomGenerator::global()->generate() % 360,
                                         QRandomGenerator::global()->generate() % 256,
                                         QRandomGenerator::global()->generate() % 128 + 128);
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
    return '<' + locale.toString(0.01f) + locale.percent();
}

} // namespace Valgrind::Internal
