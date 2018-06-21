/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "timelineformattime.h"
#include <QtQml>

namespace Timeline {

QString formatTime(qint64 timestamp, qint64 reference)
{
    static const char *decimalUnits[] = {"ns", "\xb5s", "ms", "s"};
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
        return QString::number(seconds, 'g', qMax(round, 3)) + "s";
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

static QObject *createFormatter(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    return new TimeFormatter;
}

void TimeFormatter::setupTimeFormatter()
{
    static const int typeIndex = qmlRegisterSingletonType<TimeFormatter>(
                "TimelineTimeFormatter", 1, 0, "TimeFormatter", createFormatter);
    Q_UNUSED(typeIndex);
}

}
