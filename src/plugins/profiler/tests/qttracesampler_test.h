// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Profiler::Internal {

class QtTraceSamplerTest : public QObject
{
    Q_OBJECT

private slots:
    void testSessionFileNamesTheSession();
    void testSessionFileSelectsProviders();
    void testEarlierTraceIsLeftAlone();
    void testUnusableTraceDirectoryFailsBeforeTheTargetRuns();
    void testTraceIsCollectedWhenTheTargetIsGone();
    void testTraceWrittenOnStopIsCollected();
    void testFailingTargetIsReportedRatherThanTheEmptyTrace();
    void testCollectTraceWithoutTrace();
    void testCollectTraceWithoutEvents();
    void testCollectTraceFromSessionSubdirectory();
    void testRecordsATracedApplication();
};

} // namespace Profiler::Internal
