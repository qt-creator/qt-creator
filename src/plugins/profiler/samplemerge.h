// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"
#include "sampletrace.h"

#include <QList>
#include <QString>

namespace QmlProfiler::Internal {

// One QML/JS activation taken from a QML profiler trace recorded concurrently
// with the native sampler. Timestamps are on the same clock as
// SampleTraceData::ThreadSample::tsUs (microseconds since recording start); the
// caller is responsible for converting the QML profiler's clock into that base
// before building these (see design-docs/native-mixed-profiler-design.md,
// Section 7).
//
// Ranges are perfectly nested per the QML profiler, so `parent` indexes the
// enclosing range in the same list (or -1 at the top level). A range covers the
// half-open interval [startUs, endUs).
struct QmlRange
{
    quint64 startUs = 0;
    quint64 endUs = 0;
    QString name; // JS function / binding / signal handler name
    QString file; // source .qml/.js path (may be empty)
    int line = 0;
    int parent = -1; // index into the range list, or -1 for a top-level range
};

struct MergeOptions
{
    // Keep the native engine frames and insert the JS frames right after them
    // (interleave) instead of replacing the engine-frame run with the JS frames.
    bool revealEngineFrames = false;

    // Added to each native sample's timestamp before looking up the active QML
    // range, to put the sampler clock and the QML profiler clock on one axis (see
    // the clock-correlation anchor in design-docs/native-mixed-profiler-design.md).
    // May be
    // negative; the adjusted time is clamped at 0.
    qint64 sampleTimeOffsetUs = 0;
};

// Splice QML/JS frames into a native sampler trace, using a concurrently
// recorded QML profiler stream as the source of exact JS frames.
//
// For each sample, the contiguous run of QML engine frames (see isEngineFrame)
// is replaced by -- or, with MergeOptions::revealEngineFrames, followed by --
// the JS call stack active at the sample's timestamp. A sample is returned
// unchanged when it has no engine-frame run (nothing to attribute) or when no
// JS is active at its time (so engine housekeeping is not dropped).
//
// The returned trace shares `native`'s labels and appends interned JS labels;
// `native` itself is not modified.
PROFILER_EXPORT SampleTraceData mergeQmlIntoSamples(
    const SampleTraceData &native,
    const QList<QmlRange> &qmlRanges,
    const MergeOptions &options = {});

// Whether `label` names a QML engine frame -- the machinery frame run that the
// merge treats as the JS boundary. Exposed for testing.
PROFILER_EXPORT bool isEngineFrame(const SampleTraceData::Label &label);

} // namespace QmlProfiler::Internal
