// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "presetsparser.h"

namespace CMakeProjectManager::Internal {

// Helper that converts a CMake test preset into a list of command‑line args.
QStringList presetToCTestArgs(const PresetsDetails::TestPreset &preset)
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
        else if (verb == "debug")
            args << "--debug";

        if (out.outputOnFailure.value_or(false))
            args << "--output-on-failure";

        if (out.quiet.value_or(false))
            args << "--quiet";

        if (out.outputLogFile)
            args << "--output-log" << out.outputLogFile->toFSPathString();

        if (out.outputJUnitFile)
            args << "--output-junit" << out.outputJUnitFile->toFSPathString();

        if (out.labelSummary.has_value() && !out.labelSummary.value())
            args << "--no-label-summary";

        if (out.subprojectSummary.has_value() && !out.subprojectSummary.value())
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
                if (idx.start)
                    args << "--start" << QString::number(*idx.start);
                if (idx.end)
                    args << "--end" << QString::number(*idx.end);
                if (idx.stride)
                    args << "--stride" << QString::number(*idx.stride);
                if (idx.specificTests)
                    for (int t : *idx.specificTests)
                        args << "--specific-test" << QString::number(t);
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

        if (exe.jobs)
            args << "--parallel" << QString::number(*exe.jobs);

        if (exe.resourceSpecFile)
            args << "--resource-spec-file" << exe.resourceSpecFile->toFSPathString();

        if (exe.testLoad)
            args << "--test-load" << QString::number(*exe.testLoad);

        if (exe.showOnly)
            args << QString("--show-only=%1").arg(*exe.showOnly);

        if (exe.repeat) {
            const PresetsDetails::Execution::Repeat &r = *exe.repeat;
            args << "--repeat" << r.mode << "--count" << QString::number(r.count);
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

    return args;
}
} // namespace CMakeProjectManager::Internal
