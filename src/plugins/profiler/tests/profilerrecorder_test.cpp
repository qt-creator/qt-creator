// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilerrecorder_test.h"

#include <profiler/profilerrecorder.h>
#include <profiler/sampler.h>

#include <utils/commandline.h>
#include <utils/filepath.h>

#include <QScopeGuard>
#include <QTest>

using namespace Utils;

namespace Profiler::Internal {

// The launch target is shared by the whole process, Qt Creator's own profiler
// mode included, so a test that changes it puts it back.
static QScopeGuard<std::function<void()>> restoreLaunchTarget()
{
    LaunchTargetSettings &target = LaunchTargetSettings::instance();
    return qScopeGuard(std::function<void()>(
        [&target,
         executable = target.executable(),
         arguments = target.arguments(),
         workingDirectory = target.workingDirectory(),
         seededExecutable = target.seededExecutable(),
         seededArguments = target.seededArguments(),
         seededWorkingDirectory = target.seededWorkingDirectory()] {
            target.executable.setValue(executable);
            target.arguments.setValue(arguments);
            target.workingDirectory.setValue(workingDirectory);
            target.seededExecutable.setValue(seededExecutable);
            target.seededArguments.setValue(seededArguments);
            target.seededWorkingDirectory.setValue(seededWorkingDirectory);
        }));
}

void ProfilerRecorderTest::testBackendsShareTheLaunchTarget()
{
    ProfilerRecorder recorder;
    const auto restore = restoreLaunchTarget();
    const QList<Id> ids = recorder.backendIds();
    if (ids.size() < 2)
        QSKIP("Fewer than two backends are offered here.");

    SamplerSettings *first = recorder.backendById(ids.at(0))->settings();
    SamplerSettings *second = recorder.backendById(ids.at(1))->settings();
    QVERIFY(first);
    QVERIFY(second);

    first->executable.setValue(FilePath::fromString("/test/first/app"));
    first->arguments.setValue("--first");
    first->workingDirectory.setValue(FilePath::fromString("/test/first"));
    QCOMPARE(second->executable(), FilePath::fromString("/test/first/app"));
    QCOMPARE(second->arguments(), QString("--first"));
    QCOMPARE(second->workingDirectory(), FilePath::fromString("/test/first"));

    second->arguments.setValue("--second");
    QCOMPARE(first->arguments(), QString("--second"));
}

void ProfilerRecorderTest::testSeedingKeepsWhatTheUserTyped()
{
    ProfilerRecorder recorder;
    const auto restore = restoreLaunchTarget();
    const QList<Id> ids = recorder.backendIds();
    if (ids.size() < 2)
        QSKIP("Fewer than two backends are offered here.");
    SamplerSettings *second = recorder.backendById(ids.at(1))->settings();
    QVERIFY(second);

    LaunchTargetSettings &target = LaunchTargetSettings::instance();
    target.executable.setValue(FilePath());
    target.arguments.setValue({});
    target.workingDirectory.setValue(FilePath());
    target.seededExecutable.setValue(FilePath());
    target.seededArguments.setValue({});
    target.seededWorkingDirectory.setValue(FilePath());

    recorder.seedLaunchTarget({FilePath::fromString("/seed/app"), "--seed", CommandLine::Raw},
                              FilePath::fromString("/seed"));
    QCOMPARE(second->executable(), FilePath::fromString("/seed/app"));
    QCOMPARE(second->arguments(), QString("--seed"));
    QCOMPARE(second->workingDirectory(), FilePath::fromString("/seed"));

    second->executable.setValue(FilePath::fromString("/user/app"));
    recorder.seedLaunchTarget({FilePath::fromString("/other/app"), "--other", CommandLine::Raw},
                              FilePath::fromString("/other"));
    QCOMPARE(second->executable(), FilePath::fromString("/user/app"));
    QCOMPARE(second->arguments(), QString("--other"));
    QCOMPARE(second->workingDirectory(), FilePath::fromString("/other"));
}

} // namespace Profiler::Internal
