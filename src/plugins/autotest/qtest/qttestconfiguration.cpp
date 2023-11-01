// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttestconfiguration.h"

#include "qttestframework.h"
#include "qttestoutputreader.h"
#include "qttest_utils.h"

#include "../testsettings.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

static QStringList quoteIfNeeded(const QStringList &testCases, bool debugMode)
{
    if (debugMode)
        return testCases;

    return Utils::transform(testCases, [](const QString &testCase){
        return testCase.contains(' ') ? '"' + testCase + '"' : testCase;
    });
}

TestOutputReader *QtTestConfiguration::createOutputReader(Process *app) const
{
    const QtTestOutputReader::OutputMode mode = theQtTestFramework().useXMLOutput()
            ? QtTestOutputReader::XML
            : QtTestOutputReader::PlainText;
    return new QtTestOutputReader(app, buildDirectory(), projectFile(), mode, TestType::QtTest);
}

QStringList QtTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (testSettings().processArgs()) {
        arguments.append(QTestUtils::filterInterfering(
                             runnable().command.arguments().split(' ', Qt::SkipEmptyParts),
                             omitted, false));
    }
    QtTestFramework &qtSettings = theQtTestFramework();
    if (qtSettings.useXMLOutput())
        arguments << "-xml";
    if (!testCases().isEmpty())
        arguments << quoteIfNeeded(testCases(), isDebugRunMode());

    const QString metricsOption = QtTestFramework::metricsTypeToOption(MetricsType(qtSettings.metrics()));
    if (!metricsOption.isEmpty())
        arguments << metricsOption;

    if (qtSettings.verboseBench())
        arguments << "-vb";

    if (qtSettings.logSignalsSlots())
        arguments << "-vs";

    if (isDebugRunMode() && qtSettings.noCrashHandler())
        arguments << "-nocrashhandler";

    if (qtSettings.limitWarnings() && qtSettings.maxWarnings() != 2000)
        arguments << "-maxwarnings" << QString::number(qtSettings.maxWarnings());

    return arguments;
}

Environment QtTestConfiguration::filteredEnvironment(const Environment &original) const
{
    return QTestUtils::prepareBasicEnvironment(original);
}

} // namespace Internal
} // namespace Autotest
