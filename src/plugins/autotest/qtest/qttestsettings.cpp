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

#include "qttestsettings.h"

namespace Autotest {
namespace Internal {

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

QtTestSettings::QtTestSettings()
{
    setSettingsGroups("Autotest", "QtTest");
    setAutoApply(false);

    registerAspect(&metrics);
    metrics.setSettingsKey("Metrics");
    metrics.setDefaultValue(Walltime);

    registerAspect(&noCrashHandler);
    noCrashHandler.setSettingsKey("NoCrashhandlerOnDebug");
    noCrashHandler.setDefaultValue(true);

    registerAspect(&useXMLOutput);
    useXMLOutput.setSettingsKey("UseXMLOutput");
    useXMLOutput.setDefaultValue(true);

    registerAspect(&verboseBench);
    verboseBench.setSettingsKey("VerboseBench");

    registerAspect(&logSignalsSlots);
    logSignalsSlots.setSettingsKey("LogSignalsSlots");
}

QString QtTestSettings::metricsTypeToOption(const MetricsType type)
{
    switch (type) {
    case MetricsType::Walltime:
        return QString();
    case MetricsType::TickCounter:
        return QString("-tickcounter");
    case MetricsType::EventCounter:
        return QString("-eventcounter");
    case MetricsType::CallGrind:
        return QString("-callgrind");
    case MetricsType::Perf:
        return QString("-perf");
    }
    return QString();
}

} // namespace Internal
} // namespace Autotest
