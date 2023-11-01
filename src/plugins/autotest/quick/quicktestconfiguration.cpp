// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quicktestconfiguration.h"

#include "../qtest/qttestoutputreader.h"
#include "../qtest/qttestframework.h"
#include "../qtest/qttest_utils.h"
#include "../testsettings.h"

using namespace Utils;

namespace Autotest {
namespace Internal {

QuickTestConfiguration::QuickTestConfiguration(ITestFramework *framework)
    : DebuggableTestConfiguration(framework)
{
    setMixedDebugging(true);
}

TestOutputReader *QuickTestConfiguration::createOutputReader(Process *app) const
{
    const QtTestOutputReader::OutputMode mode = theQtTestFramework().useXMLOutput()
            ? QtTestOutputReader::XML
            : QtTestOutputReader::PlainText;
    return new QtTestOutputReader(app, buildDirectory(), projectFile(), mode, TestType::QuickTest);
}

QStringList QuickTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (testSettings().processArgs()) {
        arguments.append(QTestUtils::filterInterfering
                         (runnable().command.arguments().split(' ', Qt::SkipEmptyParts),
                          omitted, true));
    }

    QtTestFramework &qtSettings = theQtTestFramework();
    if (qtSettings.useXMLOutput())
        arguments << "-xml";
    if (!testCases().isEmpty())
        arguments << testCases();

    const QString &metricsOption = QtTestFramework::metricsTypeToOption(MetricsType(qtSettings.metrics()));
    if (!metricsOption.isEmpty())
        arguments << metricsOption;

    if (isDebugRunMode()) {
        if (qtSettings.noCrashHandler())
            arguments << "-nocrashhandler";
    }

    if (qtSettings.limitWarnings() && qtSettings.maxWarnings() != 2000)
        arguments << "-maxwarnings" << QString::number(qtSettings.maxWarnings());

    return arguments;
}

Environment QuickTestConfiguration::filteredEnvironment(const Environment &original) const
{
    return QTestUtils::prepareBasicEnvironment(original);
}

} // namespace Internal
} // namespace Autotest
