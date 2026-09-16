// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testpresetshelper.h"

#include "presetsparser.h"

namespace CMakeProjectManager::Internal {

// The "--" separator that forwards arguments to the test executables, and the
// "testPassthroughArguments" field that fills it, both arrived with CMake 4.4. An older
// ctest answers the separator with "Unknown argument: --" and runs no test at all.
const QVersionNumber &ctestPassthroughArgumentsVersion()
{
    static const QVersionNumber version(4, 4);
    return version;
}

// Helper that converts a CMake test preset into a list of command-line args.
QStringList presetToCTestArgs(const PresetsDetails::TestPreset &preset,
                              const QVersionNumber &ctestVersion)
{
    QStringList args;

    if (preset.output) {
        const PresetsDetails::Output &out = *preset.output;

        if (out.shortProgress.value_or(false))
            args << "--progress";

        const QString verb = out.verbosity.value_or("default");
        if (verb == "verbose")
            args << "--verbose";
        else if (verb == "extra")
            args << "--extra-verbose";

        if (out.outputOnFailure.value_or(false))
            args << "--output-on-failure";

        if (verb == "debug" || out.debug.value_or(false))
            args << "--debug";

        if (out.quiet.value_or(false))
            args << "--quiet";

        if (out.outputLogFile)
            args << "--output-log" << out.outputLogFile->toFSPathString();

        if (out.outputJUnitFile)
            args << "--output-junit" << out.outputJUnitFile->toFSPathString();

        if (out.labelSummary.has_value() && !*out.labelSummary)
            args << "--no-label-summary";

        if (out.subprojectSummary.has_value() && !*out.subprojectSummary)
            args << "--no-subproject-summary";

        if (out.maxPassedTestOutputSize)
            args << "--test-output-size-passed" << QString::number(*out.maxPassedTestOutputSize);

        if (out.maxFailedTestOutputSize)
            args << "--test-output-size-failed" << QString::number(*out.maxFailedTestOutputSize);

        if (out.testOutputTruncation)
            args << "--test-output-truncation" << *out.testOutputTruncation;

        if (out.maxTestNameWidth)
            args << "--max-width" << QString::number(*out.maxTestNameWidth);
    }

    if (preset.filter) {
        const PresetsDetails::Filter &filt = *preset.filter;

        if (filt.include) {
            const PresetsDetails::Filter::Include &inc = *filt.include;
            if (inc.name)
                args << "--tests-regex" << *inc.name;
            if (inc.label)
                args << "--label-regex" << *inc.label;
            if (inc.useUnion.value_or(false))
                args << "--union";

            if (inc.index) {
                const PresetsDetails::Filter::Include::Index &idx = *inc.index;
                if (idx.indexFile) {
                    args << "--tests-information" << *idx.indexFile;
                } else {
                    // An empty start, end or stride field makes ctest ignore the whole
                    // filter, a zero one is the default that it stands for.
                    QStringList information{QString::number(idx.start.value_or(0)),
                                            QString::number(idx.end.value_or(0)),
                                            QString::number(idx.stride.value_or(0))};
                    if (idx.specificTests) {
                        for (int test : *idx.specificTests)
                            information << QString::number(test);
                    }
                    args << "--tests-information" << information.join(",");
                }
            }
        }

        if (filt.exclude) {
            const PresetsDetails::Filter::Exclude &exc = *filt.exclude;
            if (exc.name)
                args << "--exclude-regex" << *exc.name;
            if (exc.label)
                args << "--label-exclude" << *exc.label;

            if (exc.fixtures) {
                const PresetsDetails::Filter::Exclude::Fixtures &f = *exc.fixtures;
                if (f.any)
                    args << "--fixture-exclude-any" << *f.any;
                if (f.setup)
                    args << "--fixture-exclude-setup" << *f.setup;
                if (f.cleanup)
                    args << "--fixture-exclude-cleanup" << *f.cleanup;
            }
        }
    }

    if (preset.execution) {
        const PresetsDetails::Execution &exe = *preset.execution;

        if (exe.stopOnFailure.value_or(false))
            args << "--stop-on-failure";

        if (exe.enableFailover.value_or(false))
            args << "-F";

        if (exe.jobs) {
            args << "--parallel";
            if (*exe.jobs)
                args << QString::number(**exe.jobs);
        }

        if (exe.resourceSpecFile)
            args << "--resource-spec-file" << exe.resourceSpecFile->toFSPathString();

        if (exe.testLoad)
            args << "--test-load" << QString::number(*exe.testLoad);

        if (exe.showOnly)
            args << QString("--show-only=%1").arg(*exe.showOnly);

        if (exe.repeat) {
            const PresetsDetails::Execution::Repeat &r = *exe.repeat;
            args << "--repeat" << r.mode + ":" + QString::number(r.count);
        }

        if (exe.interactiveDebugging.value_or(false))
            args << "--interactive-debug-mode=1";
        else
            args << "--interactive-debug-mode=0";

        if (exe.scheduleRandom.value_or(false))
            args << "--schedule-random";

        if (exe.timeout)
            args << "--timeout" << QString::number(*exe.timeout);

        if (exe.noTestsAction) {
            if (*exe.noTestsAction == "error")
                args << "--no-tests=error";
            else if (*exe.noTestsAction == "ignore")
                args << "--no-tests=ignore";
        }
    }

    if (preset.configuration)
        args << "--build-config" << *preset.configuration;

    if (preset.overwriteConfigurationFile) {
        for (const QString &option : *preset.overwriteConfigurationFile)
            args << "--overwrite" << option;
    }

    // Has to stay last, everything after it is passed on to the tests
    if (preset.execution && preset.execution->testPassthroughArguments
        && ctestVersion >= ctestPassthroughArgumentsVersion()) {
        args << "--" << *preset.execution->testPassthroughArguments;
    }

    return args;
}
} // namespace CMakeProjectManager::Internal
