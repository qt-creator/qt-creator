// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttestframework.h"

#include "../autotestconstants.h"
#include "../autotesttr.h"
#include "qttestconstants.h"
#include "qttestparser.h"
#include "qttesttreeitem.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Layouting;
using namespace Utils;

namespace Autotest::Internal {

QtTestFramework &theQtTestFramework()
{
    static QtTestFramework framework;
    return framework;
}

QtTestFramework::QtTestFramework()
{
    setActive(true);
    setId(QtTest::Constants::FRAMEWORK_ID);
    setDisplayName(Tr::tr(QtTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setPriority(QtTest::Constants::FRAMEWORK_PRIORITY);
    setSettingsGroups("Autotest", "QtTest");

    setLayouter([this] {
        return Row { Form {
            noCrashHandler, br,
            useXMLOutput, br,
            verboseBench, br,
            logSignalsSlots, br,
            limitWarnings, maxWarnings, br,
            Group {
                title(Tr::tr("Benchmark Metrics")),
                Column { metrics }
            }, br,
            quickCheckForDerivedTests, br
        }, st };
    });

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

    noCrashHandler.setSettingsKey("NoCrashhandlerOnDebug");
    noCrashHandler.setDefaultValue(true);
    noCrashHandler.setLabelText(Tr::tr("Disable crash handler while debugging"));
    noCrashHandler.setToolTip(Tr::tr("Enables interrupting tests on assertions."));

    useXMLOutput.setSettingsKey("UseXMLOutput");
    useXMLOutput.setDefaultValue(true);
    useXMLOutput.setLabelText(Tr::tr("Use XML output"));
    useXMLOutput.setToolTip(Tr::tr("XML output is recommended, because it avoids parsing issues, "
                                   "while plain text is more human readable.\n\nWarning: "
                                   "Plain text misses some information, such as duration."));

    verboseBench.setSettingsKey("VerboseBench");
    verboseBench.setLabelText(Tr::tr("Verbose benchmarks"));

    logSignalsSlots.setSettingsKey("LogSignalsSlots");
    logSignalsSlots.setLabelText(Tr::tr("Log signals and slots"));
    logSignalsSlots.setToolTip(Tr::tr("Log every signal emission and resulting slot invocations."));

    limitWarnings.setSettingsKey("LimitWarnings");
    limitWarnings.setLabelText(Tr::tr("Limit warnings"));
    limitWarnings.setToolTip(Tr::tr("Set the maximum number of warnings. 0 means that the number "
                                "is not limited."));

    maxWarnings.setSettingsKey("MaxWarnings");
    maxWarnings.setRange(0, 10000);
    maxWarnings.setDefaultValue(2000);
    maxWarnings.setSpecialValueText(Tr::tr("Unlimited"));

    quickCheckForDerivedTests.setSettingsKey("QuickCheckForDerivedTests");
    quickCheckForDerivedTests.setDefaultValue(false);
    quickCheckForDerivedTests.setLabelText(Tr::tr("Check for derived Qt Quick tests"));
    quickCheckForDerivedTests.setToolTip(
        Tr::tr("Search for Qt Quick tests that are derived from TestCase.\nWarning: Enabling this "
               "feature significantly increases scan time."));

    readSettings();

    maxWarnings.setEnabler(&limitWarnings);
}

QString QtTestFramework::metricsTypeToOption(const MetricsType type)
{
    switch (type) {
    case MetricsType::Walltime:
        return {};
    case MetricsType::TickCounter:
        return QString("-tickcounter");
    case MetricsType::EventCounter:
        return QString("-eventcounter");
    case MetricsType::CallGrind:
        return QString("-callgrind");
    case MetricsType::Perf:
        return QString("-perf");
    }
    return {};
}

ITestParser *QtTestFramework::createTestParser()
{
    return new QtTestParser(this);
}

ITestTreeItem *QtTestFramework::createRootNode()
{
    return new QtTestTreeItem(this, displayName(), {}, ITestTreeItem::Root);
}

QStringList QtTestFramework::testNameForSymbolName(const QString &symbolName) const
{
    int index = symbolName.lastIndexOf("::");
    if (index == -1)
        return {};
    return { symbolName.left(index), symbolName.mid(index + 2) };
}

// QtTestSettingsPage

class QtTestSettingPage final : public Core::IOptionsPage
{
public:
    QtTestSettingPage()
    {
        setId(Id(Constants::SETTINGSPAGE_PREFIX).withSuffix(QString("%1.QtTest")
            .arg(QtTest::Constants::FRAMEWORK_PRIORITY)));
        setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
        setDisplayName(Tr::tr(QtTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
        setSettingsProvider([] { return &theQtTestFramework(); });
    }
};

const QtTestSettingPage settingsPage;

} // Autotest::Internal
