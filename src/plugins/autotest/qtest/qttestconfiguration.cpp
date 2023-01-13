// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttestconfiguration.h"
#include "qttestconstants.h"
#include "qttestoutputreader.h"
#include "qttestsettings.h"
#include "qttest_utils.h"
#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <utils/algorithm.h>
#include <utils/stringutils.h>

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

TestOutputReader *QtTestConfiguration::createOutputReader(const QFutureInterface<TestResultPtr> &fi,
                                                           Utils::QtcProcess *app) const
{
    auto qtSettings = static_cast<QtTestSettings *>(framework()->testSettings());
    const QtTestOutputReader::OutputMode mode = qtSettings && qtSettings->useXMLOutput.value()
            ? QtTestOutputReader::XML
            : QtTestOutputReader::PlainText;
    return new QtTestOutputReader(fi, app, buildDirectory(), projectFile(), mode, TestType::QtTest);
}

QStringList QtTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (AutotestPlugin::settings()->processArgs) {
        arguments.append(QTestUtils::filterInterfering(
                             runnable().command.arguments().split(' ', Qt::SkipEmptyParts),
                             omitted, false));
    }
    auto qtSettings = static_cast<QtTestSettings *>(framework()->testSettings());
    if (!qtSettings)
        return arguments;
    if (qtSettings->useXMLOutput.value())
        arguments << "-xml";
    if (!testCases().isEmpty())
        arguments << quoteIfNeeded(testCases(), isDebugRunMode());

    const QString &metricsOption = QtTestSettings::metricsTypeToOption(MetricsType(qtSettings->metrics.value()));
    if (!metricsOption.isEmpty())
        arguments << metricsOption;

    if (qtSettings->verboseBench.value())
        arguments << "-vb";

    if (qtSettings->logSignalsSlots.value())
        arguments << "-vs";

    if (isDebugRunMode() && qtSettings->noCrashHandler.value())
        arguments << "-nocrashhandler";

    if (qtSettings->limitWarnings.value() && qtSettings->maxWarnings.value() != 2000)
        arguments << "-maxwarnings" << QString::number(qtSettings->maxWarnings.value());

    return arguments;
}

Utils::Environment QtTestConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    return QTestUtils::prepareBasicEnvironment(original);
}

} // namespace Internal
} // namespace Autotest
