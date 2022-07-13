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

#include "../autotestconstants.h"
#include "../autotesttr.h"
#include "qttestconstants.h"

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

QtTestSettings::QtTestSettings()
{
    setSettingsGroups("Autotest", "QtTest");
    setAutoApply(false);

    registerAspect(&metrics);
    metrics.setSettingsKey("Metrics");
    metrics.setDefaultValue(Walltime);
    metrics.addOption(Tr::tr("Walltime"), Tr::tr("Uses walltime metrics for executing benchmarks (default)."));
    metrics.addOption(Tr::tr("Tick counter"), Tr::tr("Uses tick counter when executing benchmarks."));
    metrics.addOption(Tr::tr("Event counter"), Tr::tr("Uses event counter when executing benchmarks."));
    metrics.addOption({
        Tr::tr("Callgrind"),
        Tr::tr("Uses Valgrind Callgrind when executing benchmarks (it must be installed)."),
        HostOsInfo::isAnyUnixHost()  // valgrind available on UNIX
    });
    metrics.addOption({
        Tr::tr("Perf"),
        Tr::tr("Uses Perf when executing benchmarks (it must be installed)."),
        HostOsInfo::isLinuxHost() // according to docs perf Linux only
    });

    registerAspect(&noCrashHandler);
    noCrashHandler.setSettingsKey("NoCrashhandlerOnDebug");
    noCrashHandler.setDefaultValue(true);
    noCrashHandler.setLabelText(Tr::tr("Disable crash handler while debugging"));
    noCrashHandler.setToolTip(Tr::tr("Enables interrupting tests on assertions."));

    registerAspect(&useXMLOutput);
    useXMLOutput.setSettingsKey("UseXMLOutput");
    useXMLOutput.setDefaultValue(true);
    useXMLOutput.setLabelText(Tr::tr("Use XML output"));
    useXMLOutput.setToolTip(Tr::tr("XML output is recommended, because it avoids parsing issues, "
        "while plain text is more human readable.\n\n"
        "Warning: Plain text misses some information, such as duration."));

    registerAspect(&verboseBench);
    verboseBench.setSettingsKey("VerboseBench");
    verboseBench.setLabelText(Tr::tr("Verbose benchmarks"));

    registerAspect(&logSignalsSlots);
    logSignalsSlots.setSettingsKey("LogSignalsSlots");
    logSignalsSlots.setLabelText(Tr::tr("Log signals and slots"));
    logSignalsSlots.setToolTip(Tr::tr("Log every signal emission and resulting slot invocations."));

    registerAspect(&limitWarnings);
    limitWarnings.setSettingsKey("LimitWarnings");
    limitWarnings.setLabelText(Tr::tr("Limit warnings"));
    limitWarnings.setToolTip(Tr::tr("Set the maximum number of warnings. 0 means that the number "
                                "is not limited."));

    registerAspect(&maxWarnings);
    maxWarnings.setSettingsKey("MaxWarnings");
    maxWarnings.setRange(0, 10000);
    maxWarnings.setDefaultValue(2000);
    maxWarnings.setSpecialValueText(Tr::tr("Unlimited"));
    maxWarnings.setEnabler(&limitWarnings);
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

QtTestSettingsPage::QtTestSettingsPage(QtTestSettings *settings, Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(Tr::tr(QtTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        QtTestSettings &s = *settings;
        using namespace Layouting;

        Column col {
            s.noCrashHandler,
            s.useXMLOutput,
            s.verboseBench,
            s.logSignalsSlots,
            Row {
                s.limitWarnings, s.maxWarnings
            },
            Group {
                title(Tr::tr("Benchmark Metrics")),
                Column { s.metrics }
            },
        };

        Column { Row { col, st }, st }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Autotest
