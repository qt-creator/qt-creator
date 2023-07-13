// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchconfiguration.h"

#include "catchtestframework.h"
#include "catchoutputreader.h"

#include "../testsettings.h"

using namespace Utils;

namespace Autotest {
namespace Internal {

TestOutputReader *CatchConfiguration::createOutputReader(Process *app) const
{
    return new CatchOutputReader(app, buildDirectory(), projectFile());
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

    if (testSettings().processArgs()) {
        arguments << filterInterfering(runnable().command.arguments().split(
                                           ' ', Qt::SkipEmptyParts), omitted);
    }

    CatchFramework &settings = theCatchFramework();

    if (settings.abortAfterChecked())
        arguments << "-x" << QString::number(settings.abortAfter());
    if (settings.samplesChecked())
        arguments << "--benchmark-samples" << QString::number(settings.benchmarkSamples());
    if (settings.resamplesChecked())
        arguments << "--benchmark-resamples" << QString::number(settings.benchmarkResamples());
    if (settings.warmupChecked())
        arguments << "--benchmark-warmup-time" << QString::number(settings.benchmarkWarmupTime());
    if (settings.confidenceIntervalChecked())
        arguments << "--benchmark-confidence-interval" << QString::number(settings.confidenceInterval());
    if (settings.noAnalysis())
        arguments << "--benchmark-no-analysis";
    if (settings.showSuccess())
        arguments << "-s";
    if (settings.noThrow())
        arguments << "-e";
    if (settings.visibleWhitespace())
        arguments << "-i";
    if (settings.warnOnEmpty())
        arguments << "-w" << "NoAssertions";

    if (isDebugRunMode() && settings.breakOnFailure())
        arguments << "-b";
    return arguments;
}

Environment CatchConfiguration::filteredEnvironment(const Environment &original) const
{
    return original;
}

} // namespace Internal
} // namespace Autotest
