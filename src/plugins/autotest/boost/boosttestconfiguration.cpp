/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "boosttestconfiguration.h"
#include "boosttestconstants.h"
#include "boosttestoutputreader.h"
#include "boosttestsettings.h"

#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <utils/stringutils.h>

namespace Autotest {
namespace Internal {

TestOutputReader *BoostTestConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                       QProcess *app) const
{
    auto settings = dynamic_cast<BoostTestSettings *>(framework()->frameworkSettings());
    return new BoostTestOutputReader(fi, app, buildDirectory(), projectFile(),
                                     settings->logLevel, settings->reportLevel);
}

enum class InterferingType { Options, EnvironmentVariables };

static QStringList interfering(InterferingType type)
{
    const QStringList knownInterfering { "log_level", "log_format", "log_sink",
                                         "report_level", "report_format", "report_sink",
                                         "output_format",
                                         "catch_system_errors", "no_catch_system_errors",
                                         "detect_fp_exceptions", "no_detect_fp_exceptions",
                                         "detect_memory_leaks", "random", "run_test",
                                         "show_progress", "result_code", "no_result_code",
                                         "help", "list_content", "list_labels", "version"
                                         };
    switch (type) {
    case InterferingType::Options:
        return Utils::transform(knownInterfering, [](const QString &item) {
            return QString("--" + item);
        });
    case InterferingType::EnvironmentVariables:
        return Utils::transform(knownInterfering, [](const QString &item) {
            return QString("BOOST_TEST_" + item).toUpper();
        });
    }
    return QStringList();
}

static QStringList filterInterfering(const QStringList &provided, QStringList *omitted)
{
    const QStringList knownInterfering = interfering(InterferingType::Options);
    const QString interferingSingleWithParam = "efklortcpsx?";
    QStringList allowed;
    bool filterNextArg = false;
    bool ignoreRest = false;
    for (const auto &arg : provided) {
        bool filterArg = filterNextArg;
        filterNextArg = false;
        if (ignoreRest) {
            allowed.append(arg);
            continue;
        }
        bool interferes = false;
        if (filterArg && !arg.startsWith('-')) {
            interferes = true;
        } else if (arg.startsWith("--")) {
            if (arg.size() == 2)
                ignoreRest = true;
            else
                interferes = knownInterfering.contains(arg.left(arg.indexOf('=')));
        } else if (arg.startsWith('-') && arg.size() > 1) {
            interferes = interferingSingleWithParam.contains(arg.at(1));
            filterNextArg = interferes;
        }
        if (!interferes)
            allowed.append(arg);
        else if (omitted)
            omitted->append(arg);
    }
    return allowed;
}

QStringList BoostTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    auto boostSettings = dynamic_cast<BoostTestSettings *>(framework()->frameworkSettings());
    QStringList arguments;
    arguments << "-l" << BoostTestSettings::logLevelToOption(boostSettings->logLevel);
    arguments << "-r" << BoostTestSettings::reportLevelToOption(boostSettings->reportLevel);

    if (boostSettings->randomize)
        arguments << QString("--random=").append(QString::number(boostSettings->seed));

    if (boostSettings->systemErrors)
        arguments << "-s";
    if (boostSettings->fpExceptions)
        arguments << "--detect_fp_exceptions";
    if (!boostSettings->memLeaks)
        arguments << "--detect_memory_leaks=0";

    // TODO improve the test case gathering and arguments building to avoid too long command lines
    for (const QString &test : testCases())
        arguments << "-t" << test;

    if (AutotestPlugin::settings()->processArgs) {
        arguments << filterInterfering(runnable().commandLineArguments.split(
                                           ' ', Qt::SkipEmptyParts), omitted);
    }
    return arguments;
}

Utils::Environment BoostTestConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    const QStringList interferingEnv = interfering(InterferingType::EnvironmentVariables);

    Utils::Environment result = original;
    if (!result.hasKey("BOOST_TEST_COLOR_OUTPUT"))
        result.set("BOOST_TEST_COLOR_OUTPUT", "1");  // use colored output by default
    for (const QString &key : interferingEnv)
        result.unset(key);
    return result;
}

} // namespace Internal
} // namespace Autotest
