/****************************************************************************
**
** Copyright (C) 2019 Jochen Seemann
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

#include "catchconfiguration.h"
#include "catchoutputreader.h"
#include "catchtestsettings.h"

#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <utils/stringutils.h>

namespace Autotest {
namespace Internal {

TestOutputReader *CatchConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi, QProcess *app) const
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
        arguments << "\"" + testCases().join("\",\"") + "\"";
    arguments << "--reporter" << "xml";

    if (AutotestPlugin::settings()->processArgs) {
        arguments << filterInterfering(runnable().commandLineArguments.split(
                                           ' ', Qt::SkipEmptyParts), omitted);
    }

    auto settings = dynamic_cast<CatchTestSettings *>(framework()->frameworkSettings());
    if (!settings)
        return arguments;

    if (settings->abortAfterChecked)
        arguments << "-x" << QString::number(settings->abortAfter);
    if (settings->samplesChecked)
        arguments << "--benchmark-samples" << QString::number(settings->benchmarkSamples);
    if (settings->resamplesChecked)
        arguments << "--benchmark-resamples" << QString::number(settings->benchmarkResamples);
    if (settings->warmupChecked)
        arguments << "--benchmark-warmup-time" << QString::number(settings->benchmarkWarmupTime);
    if (settings->confidenceIntervalChecked)
        arguments << "--benchmark-confidence-interval" << QString::number(settings->confidenceInterval);
    if (settings->noAnalysis)
        arguments << "--benchmark-no-analysis";
    if (settings->showSuccess)
        arguments << "-s";
    if (settings->noThrow)
        arguments << "-e";
    if (settings->visibleWhitespace)
        arguments << "-i";
    if (settings->warnOnEmpty)
        arguments << "-w" << "NoAssertions";

    if (isDebugRunMode() && settings->breakOnFailure)
        arguments << "-b";
    return arguments;
}

Utils::Environment CatchConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    return original;
}

} // namespace Internal
} // namespace Autotest
