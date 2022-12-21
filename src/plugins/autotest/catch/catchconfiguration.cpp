// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchconfiguration.h"
#include "catchoutputreader.h"
#include "catchtestsettings.h"

#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <utils/stringutils.h>

namespace Autotest {
namespace Internal {

TestOutputReader *CatchConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                   Utils::QtcProcess *app) const
{
    return new CatchOutputReader(fi, app, buildDirectory(), projectFile());
}

static QStringList filterInterfering(const QStringList &provided, QStringList *omitted)
{
    static const QSet<QString> singleOptions { "-l", "--list-tests",
                                               "--list-test-names-only",
                                               "-t", "--list-tags",
                                               "--list-reporters",
                                               "-s", "--success",
                                               "-b", "--break",
                                               "-e", "--nothrow",
                                               "-a", "--abort",
                                               "-#", "--filenames-as-tags",
                                               "--benchmark-no-analysis",
                                               "--help"
                                             };
    static const QSet<QString> paramOptions { "-o", "--out",
                                              "-r", "--reporter",
                                              "-x", "--abortx",
                                              "-w", "--warn",
                                              "-d", "--durations",
                                              "-f", "--input-file",
                                              "-c", "--section",
                                              "--wait-for-keypress",
                                              "--benchmark-samples",
                                              "--benchmark-resamples",
                                              "--benchmark-confidence-interval",
                                              "--benchmark-warmup-time",
                                              "--use-color"
                                            };

    QStringList allowed;
    bool filterNext = false;
    for (const auto &arg : provided) {
        bool interferes = false;
        if (filterNext) {
            if (omitted)
                omitted->append(arg);
            filterNext = false;
            continue;
        }
        if (singleOptions.contains(arg)) {
            interferes = true;
        } else if (paramOptions.contains(arg)) {
            interferes = true;
            filterNext = true;
        }
        if (!interferes)
            allowed.append(arg);
        else if (omitted)
            omitted->append(arg);
    }
    return allowed;
}

QStringList CatchConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (testCaseCount())
        arguments << "\"" + testCases().join("\", \"") + "\"";
    arguments << "--reporter" << "xml";

    if (AutotestPlugin::settings()->processArgs) {
        arguments << filterInterfering(runnable().command.arguments().split(
                                           ' ', Qt::SkipEmptyParts), omitted);
    }

    auto settings = static_cast<CatchTestSettings *>(framework()->testSettings());
    if (!settings)
        return arguments;

    if (settings->abortAfterChecked.value())
        arguments << "-x" << QString::number(settings->abortAfter.value());
    if (settings->samplesChecked.value())
        arguments << "--benchmark-samples" << QString::number(settings->benchmarkSamples.value());
    if (settings->resamplesChecked.value())
        arguments << "--benchmark-resamples" << QString::number(settings->benchmarkResamples.value());
    if (settings->warmupChecked.value())
        arguments << "--benchmark-warmup-time" << QString::number(settings->benchmarkWarmupTime.value());
    if (settings->confidenceIntervalChecked.value())
        arguments << "--benchmark-confidence-interval" << QString::number(settings->confidenceInterval.value());
    if (settings->noAnalysis.value())
        arguments << "--benchmark-no-analysis";
    if (settings->showSuccess.value())
        arguments << "-s";
    if (settings->noThrow.value())
        arguments << "-e";
    if (settings->visibleWhitespace.value())
        arguments << "-i";
    if (settings->warnOnEmpty.value())
        arguments << "-w" << "NoAssertions";

    if (isDebugRunMode() && settings->breakOnFailure.value())
        arguments << "-b";
    return arguments;
}

Utils::Environment CatchConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    return original;
}

} // namespace Internal
} // namespace Autotest
