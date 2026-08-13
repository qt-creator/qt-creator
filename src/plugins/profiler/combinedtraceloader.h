// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/filepath.h>

#include <QtTaskTree/QTaskTree>

#include <QObject>

#include <functional>

namespace QmlProfiler::Internal {

// Turns a combined bundle (see CombinedSampler) into a single native-mixed
// sampler trace: it loads the bundle's QML ".qtd", reconstructs the JS/QML call
// stack as it varies over time, splices those frames into the native sampler
// trace with mergeQmlIntoSamples(), and writes the merged result into the
// bundle's own combinedMergedSubdir, which the sampler views load unchanged.
// Keeping it in the bundle means a bundle is merged once, not once per open.
//
// Both halves run on worker threads -- loading the QML trace, then decoding,
// merging and rewriting the native sampler trace -- so the result arrives via
// merged()/failed() rather than a return value. Those signals are emitted on
// the thread that called load().
class PROFILER_EXPORT CombinedTraceLoader : public QObject
{
    Q_OBJECT

public:
    explicit CombinedTraceLoader(QObject *parent = nullptr);
    ~CombinedTraceLoader() override;

    // Starts loading and merging `bundleDir`. Emits merged() or failed() exactly
    // once. Calling it again while a load is in flight is ignored.
    //
    // A bundle that already carries a merged trace (combinedMergedSubdir, written
    // when it was recorded) short-circuits to merged() without redoing any of the
    // work; everything below describes the case where it has to be built.
    void load(const Utils::FilePath &bundleDir);

    // Drops a load in flight without emitting anything. Call it when the result is
    // no longer wanted, for example because another trace is being shown.
    void cancel();

signals:
    // The directory holding the merged, native-mixed sampler trace: the bundle's
    // own combinedMergedSubdir once the merge has succeeded.
    void merged(const Utils::FilePath &mergedSamplerDir);
    void failed(const QString &error);

    // 0..100 across decoding the native trace and writing the merged one, the two
    // steps that scale with trace size. Not emitted on the short-circuit path.
    void progress(int percent);

private:
    void onQmlLoaded();

    class CombinedTraceLoaderPrivate *d;
};

// A task-tree step that merges `bundleDir` in place, so a recording's
// post-processing can absorb the merge instead of leaving it to the subsequent
// load. `reportProgress`, if set, is called on the GUI thread with 0..100 while
// the merge runs; the caller maps that onto whatever part of its own progress
// range this step occupies.
//
// The step always succeeds: a failed merge still leaves a valid bundle, so it is
// not worth failing a finished recording over. The error surfaces at load time,
// when CombinedTraceLoader finds no merged trace and builds one itself.
PROFILER_EXPORT QtTaskTree::ExecutableItem mergeCombinedBundleRecipe(
    const Utils::FilePath &bundleDir, const std::function<void(int)> &reportProgress);

} // namespace QmlProfiler::Internal
