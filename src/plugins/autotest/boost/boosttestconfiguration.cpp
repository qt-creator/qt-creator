// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "boosttestconfiguration.h"

#include "boosttestframework.h"
#include "boosttestoutputreader.h"

#include "../testsettings.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

TestOutputReader *BoostTestConfiguration::createOutputReader(Process *app) const
{
    BoostTestFramework &settings = theBoostTestFramework();
    return new BoostTestOutputReader(app, buildDirectory(), projectFile(),
                                     LogLevel(settings.logLevel()),
                                     ReportLevel(settings.reportLevel()));
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
    return {};
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
    BoostTestFramework &boostSettings = theBoostTestFramework();
    QStringList arguments;
    arguments << "-l" << BoostTestFramework::logLevelToOption(LogLevel(boostSettings.logLevel()));
    arguments << "-r" << BoostTestFramework::reportLevelToOption(ReportLevel(boostSettings.reportLevel()));

    if (boostSettings.randomize())
        arguments << QString("--random=").append(QString::number(boostSettings.seed()));

    if (boostSettings.systemErrors())
        arguments << "-s";
    if (boostSettings.fpExceptions())
        arguments << "--detect_fp_exceptions";
    if (!boostSettings.memLeaks())
        arguments << "--detect_memory_leaks=0";

    // TODO improve the test case gathering and arguments building to avoid too long command lines
    if (isDebugRunMode()) { // debugger has its own quoting
        for (const QString &test : testCases())
            arguments << "-t" << test;
    } else {
        for (const QString &test : testCases())
            arguments << "-t" << "\"" + test + "\"";
    }

    if (testSettings().processArgs()) {
        arguments << filterInterfering(runnable().command.arguments().split(
                                           ' ', Qt::SkipEmptyParts), omitted);
    }
    return arguments;
}

Environment BoostTestConfiguration::filteredEnvironment(const Environment &original) const
{
    const QStringList interferingEnv = interfering(InterferingType::EnvironmentVariables);

    Environment result = original;
    if (!result.hasKey("BOOST_TEST_COLOR_OUTPUT"))
        result.set("BOOST_TEST_COLOR_OUTPUT", "1");  // use colored output by default
    for (const QString &key : interferingEnv)
        result.unset(key);
    return result;
}

} // namespace Internal
} // namespace Autotest
