// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerTraceFileTest : public QObject
{
    Q_OBJECT

private slots:
    void testPausedRangesRoundTrip();
    void testNoPausedRanges();
    void testMalformedPausedRangesAreSkipped();
    void testCancelStopsTheLoadingThread();
    void testLoadPreparesInLoadingThread_data();
    void testLoadPreparesInLoadingThread();
    void testFinalizePreparesUnloadedTrace();
    void testLoadedEventsAreStored_data();
    void testLoadedEventsAreStored();
    void testSavedChunksKeepTheirOrder();
    void testRestrictionReplaysInAnotherThread();
    void testCancelStopsTheRestrictionThread();
    void testRestrictionReleasesTheStorage();
    void testFeatureRestrictionReplaysInAnotherThread();
    void testFeatureRestrictionDuringRangeRestriction();
};

} // namespace Profiler::Internal
