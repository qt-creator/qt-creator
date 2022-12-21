// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineformattime.h"
#include <QtQml>

namespace Timeline {

QString formatTime(qint64 timestamp, qint64 reference)
{
    static const char *decimalUnits[] = {" ns", " \xb5s", " ms", " s"};
    static const double second = 1e9;
    static const double minute = 60;
    static const double hour = 60;

    int round = 1;
    qint64 barrier = 1;

    for (uint i = 0, end = sizeof(decimalUnits) / sizeof(char *); i < end; ++i) {
        const double dividend = barrier;
        if (reference < barrier) {
            round += 3;
            barrier *= 1000;
        } else {
            barrier *= 10;
            if (reference < barrier) {
                round += 2;
                barrier *= 100;
            } else {
                barrier *= 10;
                if (reference < barrier)
                    round += 1;
                barrier *= 10;
            }
        }
        if (timestamp < barrier) {
            return QString::number(timestamp / dividend, 'g', qMax(round, 3))
                    + QString::fromLatin1(decimalUnits[i]);
        }
    }

    double seconds = timestamp / second;
    if (seconds < minute) {
        return QString::number(seconds, 'g', qMax(round, 3)) + " s";
    } else {
        int minutes = seconds / minute;
        seconds -= minutes * minute;
        if (minutes < hour) {
            return QString::fromLatin1("%1m %2s").arg(QString::number(minutes),
                                                      QString::number(seconds, 'g', round));
        } else {
            int hours = minutes / hour;
            minutes -= hours * hour;
            if (reference < barrier * minute) {
                return QString::fromLatin1("%1h %2m %3s").arg(QString::number(hours),
                                                              QString::number(minutes),
                                                              QString::number(seconds, 'g', round));
            } else {
                return QString::fromLatin1("%1h %2m").arg(QString::number(hours),
                                                          QString::number(minutes));
            }

        }
    }
}

}
