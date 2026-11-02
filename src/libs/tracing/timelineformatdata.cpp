// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineformatdata.h"

#include <QLocale>

namespace Timeline {

QString formatTime(std::chrono::nanoseconds time, std::chrono::nanoseconds referenceTime)
{
    const qint64 timestamp = time.count();
    const qint64 reference = referenceTime.count();
    static const char *decimalUnits[] = {" ns", " \xb5s", " ms", " s"};
    constexpr qint64 second = 1'000'000'000;
    constexpr qint64 minute = 60 * second;
    constexpr qint64 hour = 60 * minute;

    // Resolve a hundredth of the reference, and at least a tenth of the unit.
    qint64 resolution = 1;
    while (resolution * 10 <= reference / 100)
        resolution *= 10;

    const auto rounded = [timestamp](qint64 step) {
        return (timestamp + step / 2) / step * step;
    };
    const auto number = [](qint64 value, qint64 unit) {
        return QString::number(double(value) / unit, 'f', QLocale::FloatingPointShortest);
    };

    qint64 unit = 1;
    for (const char *unitName : decimalUnits) {
        const qint64 value = rounded(qMax(qint64(1), qMin(resolution, unit / 10)));
        if (value < 1000 * unit)
            return number(value, unit) + QLatin1String(unitName);
        unit *= 1000;
    }

    const qint64 value = rounded(qMin(resolution, second / 10));
    const QString minutes = QString::number(value / minute % 60) + 'm';
    const QString seconds = number(value % minute, second) + 's';
    if (value < hour)
        return minutes + ' ' + seconds;

    if (resolution >= minute) {
        const qint64 wholeMinutes = rounded(minute);
        return QString::number(wholeMinutes / hour) + "h "
               + QString::number(wholeMinutes / minute % 60) + 'm';
    }
    return QString::number(value / hour) + "h " + minutes + ' ' + seconds;
}

QString formatDataSize(qint64 bytes)
{
    return QLocale::system().formattedDataSize(bytes, 3, QLocale::DataSizeTraditionalFormat);
}

QString formatNumber(qint64 value)
{
    return QLocale::system().toString(value);
}

QString formatPercentage(qint64 percent)
{
    return QLocale::system().toString(percent) + '%';
}

QString formatPixels(qint64 pixels)
{
    return QLocale::system().toString(pixels) + " px";
}

}
