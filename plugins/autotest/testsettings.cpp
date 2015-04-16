/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "testsettings.h"

#include <QSettings>

namespace Autotest {
namespace Internal {

static const char group[] = "Autotest";
static const char timeoutKey[] = "Timeout";
static const char metricsKey[] = "Metrics";
static const char omitInternalKey[] = "OmitInternal";
static const char omitRunConfigWarnKey[] = "OmitRCWarnings";
static const char limitResultOutputKey[] = "LimitResultOutput";
static const char autoScrollKey[] = "AutoScrollResults";
static const int defaultTimeout = 60000;

TestSettings::TestSettings()
    : timeout(defaultTimeout), metrics(Walltime), omitInternalMssg(true), omitRunConfigWarn(false),
      limitResultOutput(true), autoScroll(true)
{
}

void TestSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(group));
    s->setValue(QLatin1String(timeoutKey), timeout);
    s->setValue(QLatin1String(metricsKey), metrics);
    s->setValue(QLatin1String(omitInternalKey), omitInternalMssg);
    s->setValue(QLatin1String(omitRunConfigWarnKey), omitRunConfigWarn);
    s->setValue(QLatin1String(limitResultOutputKey), limitResultOutput);
    s->setValue(QLatin1String(autoScrollKey), autoScroll);
    s->endGroup();
}

static MetricsType intToMetrics(int value)
{
    switch (value) {
    case Walltime:
        return Walltime;
    case TickCounter:
        return TickCounter;
    case EventCounter:
        return EventCounter;
    case CallGrind:
        return CallGrind;
    case Perf:
        return Perf;
    default:
        return Walltime;
    }
}

void TestSettings::fromSettings(const QSettings *s)
{
    const QString root = QLatin1String(group) + QLatin1Char('/');
    timeout = s->value(root + QLatin1String(timeoutKey), defaultTimeout).toInt();
    metrics = intToMetrics(s->value(root + QLatin1String(metricsKey), Walltime).toInt());
    omitInternalMssg = s->value(root + QLatin1String(omitInternalKey), true).toBool();
    omitRunConfigWarn = s->value(root + QLatin1String(omitRunConfigWarnKey), false).toBool();
    limitResultOutput = s->value(root + QLatin1String(limitResultOutputKey), true).toBool();
    autoScroll = s->value(root + QLatin1String(autoScrollKey), true).toBool();
}

bool TestSettings::equals(const TestSettings &rhs) const
{
    return timeout == rhs.timeout && metrics == rhs.metrics
            && omitInternalMssg == rhs.omitInternalMssg
            && omitRunConfigWarn == rhs.omitRunConfigWarn
            && limitResultOutput == rhs.limitResultOutput
            && autoScroll == rhs.autoScroll;
}

QString TestSettings::metricsTypeToOption(const MetricsType type)
{
    switch (type) {
    case MetricsType::Walltime:
        return QString();
    case MetricsType::TickCounter:
        return QLatin1String("-tickcounter");
    case MetricsType::EventCounter:
        return QLatin1String("-eventcounter");
    case MetricsType::CallGrind:
        return QLatin1String("-callgrind");
    case MetricsType::Perf:
        return QLatin1String("-perf");
    default:
        return QString();
    }
}

} // namespace Internal
} // namespace Autotest
