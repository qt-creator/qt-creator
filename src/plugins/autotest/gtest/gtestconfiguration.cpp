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

#include "gtestconfiguration.h"
#include "gtestconstants.h"
#include "gtestoutputreader.h"
#include "gtestsettings.h"
#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <utils/algorithm.h>
#include <utils/stringutils.h>

namespace Autotest {
namespace Internal {

TestOutputReader *GTestConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                   QProcess *app) const
{
    return new GTestOutputReader(fi, app, buildDirectory(), projectFile());
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
                                                         "--gtest_print_time="
                                                         };

    QSet<QString> allowed = Utils::filtered(Utils::toSet(provided), [] (const QString &arg) {
        return Utils::allOf(knownInterferingOptions, [&arg] (const QString &interfering) {
            return !arg.startsWith(interfering);
        });
    });

    if (omitted) {
        QSet<QString> providedSet = Utils::toSet(provided);
        providedSet.subtract(allowed);
        omitted->append(Utils::toList(providedSet));
    }
    return Utils::toList(allowed);
}

QStringList GTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (AutotestPlugin::settings()->processArgs) {
        arguments << filterInterfering(runnable().commandLineArguments.split(
                                           ' ', Qt::SkipEmptyParts), omitted);
    }

    const QStringList &testSets = testCases();
    if (!testSets.isEmpty())
        arguments << "--gtest_filter=" + testSets.join(':');

    auto gSettings = dynamic_cast<GTestSettings *>(framework()->frameworkSettings());
    if (!gSettings)
        return arguments;

    if (gSettings->runDisabled)
        arguments << "--gtest_also_run_disabled_tests";
    if (gSettings->repeat)
        arguments << QString("--gtest_repeat=%1").arg(gSettings->iterations);
    if (gSettings->shuffle)
        arguments << "--gtest_shuffle" << QString("--gtest_random_seed=%1").arg(gSettings->seed);
    if (gSettings->throwOnFailure)
        arguments << "--gtest_throw_on_failure";

    if (isDebugRunMode()) {
        if (gSettings->breakOnFailure)
            arguments << "--gtest_break_on_failure";
    }
    return arguments;
}

Utils::Environment GTestConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    const QStringList interfering{"GTEST_FILTER", "GTEST_ALSO_RUN_DISABLED_TESTS",
                                  "GTEST_REPEAT", "GTEST_SHUFFLE", "GTEST_RANDOM_SEED",
                                  "GTEST_OUTPUT", "GTEST_BREAK_ON_FAILURE", "GTEST_PRINT_TIME",
                                  "GTEST_CATCH_EXCEPTIONS"};
    Utils::Environment result = original;
    if (!result.hasKey("GTEST_COLOR"))
        result.set("GTEST_COLOR", "1");  // use colored output by default
    for (const QString &key : interfering)
        result.unset(key);
    return result;
}

} // namespace Internal
} // namespace Autotest
