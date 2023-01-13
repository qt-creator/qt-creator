// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quicktestconfiguration.h"
#include "../qtest/qttestconstants.h"
#include "../qtest/qttestoutputreader.h"
#include "../qtest/qttestsettings.h"
#include "../qtest/qttest_utils.h"
#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <utils/stringutils.h>

namespace Autotest {
namespace Internal {

QuickTestConfiguration::QuickTestConfiguration(ITestFramework *framework)
    : DebuggableTestConfiguration(framework)
{
    setMixedDebugging(true);
}

TestOutputReader *QuickTestConfiguration::createOutputReader(
        const QFutureInterface<TestResultPtr> &fi, Utils::QtcProcess *app) const
{
    auto qtSettings = static_cast<QtTestSettings *>(framework()->testSettings());
    const QtTestOutputReader::OutputMode mode = qtSettings && qtSettings->useXMLOutput.value()
            ? QtTestOutputReader::XML
            : QtTestOutputReader::PlainText;
    return new QtTestOutputReader(fi, app, buildDirectory(), projectFile(),
                                  mode, TestType::QuickTest);
}

QStringList QuickTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (AutotestPlugin::settings()->processArgs) {
        arguments.append(QTestUtils::filterInterfering
                         (runnable().command.arguments().split(' ', Qt::SkipEmptyParts),
                          omitted, true));
    }

    auto qtSettings = static_cast<QtTestSettings *>(framework()->testSettings());
    if (!qtSettings)
        return arguments;
    if (qtSettings->useXMLOutput.value())
        arguments << "-xml";
    if (!testCases().isEmpty())
        arguments << testCases();

    const QString &metricsOption = QtTestSettings::metricsTypeToOption(MetricsType(qtSettings->metrics.value()));
    if (!metricsOption.isEmpty())
        arguments << metricsOption;

    if (isDebugRunMode()) {
        if (qtSettings->noCrashHandler.value())
            arguments << "-nocrashhandler";
    }

    if (qtSettings->limitWarnings.value() && qtSettings->maxWarnings.value() != 2000)
        arguments << "-maxwarnings" << QString::number(qtSettings->maxWarnings.value());

    return arguments;
}

Utils::Environment QuickTestConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    return QTestUtils::prepareBasicEnvironment(original);
}

} // namespace Internal
} // namespace Autotest
