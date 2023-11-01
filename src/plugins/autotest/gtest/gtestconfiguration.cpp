// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestconfiguration.h"

#include "gtestframework.h"
#include "gtestoutputreader.h"

#include "../testsettings.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

TestOutputReader *GTestConfiguration::createOutputReader(Process *app) const
{
    return new GTestOutputReader(app, buildDirectory(), projectFile());
}

QStringList filterInterfering(const QStringList &provided, QStringList *omitted)
{
    static const QSet<QString> knownInterferingOptions { "--gtest_list_tests",
                                                         "--gtest_filter=",
                                                         "--gtest_also_run_disabled_tests",
                                                         "--gtest_repeat=",
                                                         "--gtest_shuffle",
                                                         "--gtest_random_seed=",
                                                         "--gtest_output=",
                                                         "--gtest_stream_result_to=",
                                                         "--gtest_break_on_failure",
                                                         "--gtest_throw_on_failure",
                                                         "--gtest_catch_exceptions=",
                                                         "--gtest_print_time="
                                                         };

    QStringList allowed = Utils::filtered(provided, [](const QString &arg) {
        return Utils::allOf(knownInterferingOptions, [&arg](const QString &interfering) {
            return !arg.startsWith(interfering);
        });
    });

    if (omitted && allowed.size() < provided.size()) {
        QSet<QString> providedSet = Utils::toSet(provided);
        providedSet.subtract(Utils::toSet(allowed));
        omitted->append(Utils::toList(providedSet));
    }
    return allowed;
}

QStringList GTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (testSettings().processArgs()) {
        arguments << filterInterfering(runnable().command.arguments().split(
                                           ' ', Qt::SkipEmptyParts), omitted);
    }

    const QStringList &testSets = testCases();
    if (!testSets.isEmpty()) {
        if (isDebugRunMode()) // debugger does its own special quoting
            arguments << "--gtest_filter=" + testSets.join(':');
        else
            arguments << "--gtest_filter=\"" + testSets.join(':') + '"';
    }

    GTestFramework &gSettings = theGTestFramework();

    if (gSettings.runDisabled())
        arguments << "--gtest_also_run_disabled_tests";
    if (gSettings.repeat())
        arguments << QString("--gtest_repeat=%1").arg(gSettings.iterations());
    if (gSettings.shuffle())
        arguments << "--gtest_shuffle" << QString("--gtest_random_seed=%1").arg(gSettings.seed());
    if (gSettings.throwOnFailure())
        arguments << "--gtest_throw_on_failure";

    if (isDebugRunMode()) {
        if (gSettings.breakOnFailure())
            arguments << "--gtest_break_on_failure";
        arguments << "--gtest_catch_exceptions=0";
    }
    return arguments;
}

Environment GTestConfiguration::filteredEnvironment(const Environment &original) const
{
    const QStringList interfering{"GTEST_FILTER", "GTEST_ALSO_RUN_DISABLED_TESTS",
                                  "GTEST_REPEAT", "GTEST_SHUFFLE", "GTEST_RANDOM_SEED",
                                  "GTEST_OUTPUT", "GTEST_BREAK_ON_FAILURE", "GTEST_PRINT_TIME",
                                  "GTEST_CATCH_EXCEPTIONS"};
    Environment result = original;
    if (!result.hasKey("GTEST_COLOR"))
        result.set("GTEST_COLOR", "1");  // use colored output by default
    for (const QString &key : interfering)
        result.unset(key);
    return result;
}

} // namespace Internal
} // namespace Autotest
