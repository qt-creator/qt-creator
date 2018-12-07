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
#include "../testframeworkmanager.h"
#include "../testsettings.h"

namespace Autotest {
namespace Internal {

static QSharedPointer<BoostTestSettings> getBoostSettings()
{
    const Core::Id id = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(
                BoostTest::Constants::FRAMEWORK_NAME);
    TestFrameworkManager *manager = TestFrameworkManager::instance();
    return qSharedPointerCast<BoostTestSettings>(manager->settingsForTestFramework(id));
}

TestOutputReader *BoostTestConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                       QProcess *app) const
{
    auto settings = getBoostSettings();
    return new BoostTestOutputReader(fi, app, buildDirectory(), projectFile(),
                                     settings->logLevel, settings->reportLevel);
}

static QStringList filterInterfering(const QStringList &provided, QStringList *omitted)
{
    // TODO complete these...
    const QSet<QString> knownInterfering { "--log_level=",
                                           "--report_level=",
                                           "--color_output=", "--no_color_output",
                                           "--catch_system_errors", "--detect_fp_exceptions",
                                           "--detect_memory_leaks", "--random="
                                         };
    QSet<QString> allowed = Utils::filtered(provided.toSet(), [&knownInterfering](const QString &arg) {
        return Utils::allOf(knownInterfering, [&arg](const QString &interfering) {
            return !arg.startsWith(interfering);
        });
    });

    // TODO handle single letter options correctly
    if (omitted) {
        QSet<QString> providedSet = provided.toSet();
        providedSet.subtract(allowed);
        omitted->append(providedSet.toList());
    }
    return allowed.toList();
}

QStringList BoostTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (AutotestPlugin::settings()->processArgs) {
        arguments << filterInterfering(runnable().commandLineArguments.split(
                                           ' ', QString::SkipEmptyParts), omitted);
    }

    // TODO improve the test case gathering and arguments building to avoid too long command lines
    for (const QString &test : testCases())
        arguments << "-t" << test;

    auto boostSettings = getBoostSettings();
    arguments << "-l" << BoostTestSettings::logLevelToOption(boostSettings->logLevel);
    arguments << "-r" << BoostTestSettings::reportLevelToOption(boostSettings->reportLevel);

    if (boostSettings->randomize)
        arguments << QString("--random=").append(boostSettings->seed);

    if (boostSettings->systemErrors)
        arguments << "-s";
    if (boostSettings->fpExceptions)
        arguments << "--detect_fp_exceptions";
    if (!boostSettings->memLeaks)
        arguments << "--detect_memory_leaks=0";
    return arguments;
}

Utils::Environment BoostTestConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    // TODO complete these..
    const QStringList interfering{"BOOST_TEST_LOG_FORMAT", "BOOST_TEST_REPORT_FORMAT",
                                  "BOOST_TEST_COLOR_OUTPUT", "BOOST_TEST_CATCH_SYSTEM_ERRORS",
                                  "BOOST_TEST_DETECT_FP_EXCEPTIONS", "BOOST_TEST_RANDOM",
                                  "BOOST_TEST_DETECT_MEMORY_LEAK"};

    Utils::Environment result = original;
    for (const QString &key : interfering)
        result.unset(key);
    return result;
}

} // namespace Internal
} // namespace Autotest
