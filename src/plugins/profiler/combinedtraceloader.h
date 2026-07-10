// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/filepath.h>

#include <QObject>

namespace QmlProfiler::Internal {

// Turns a combined bundle (see CombinedSampler) into a single native-mixed
// sampler trace: it loads the bundle's QML ".qtd", reconstructs the JS/QML call
// stack as it varies over time, splices those frames into the native sampler
// trace with mergeQmlIntoSamples(), and writes the merged result to a fresh
// temporary CTF2 directory that the sampler views can load unchanged.
//
// Loading the QML trace is asynchronous (it runs on a worker thread), so the
// result arrives via merged()/failed() rather than a return value.
class PROFILER_EXPORT CombinedTraceLoader : public QObject
{
    Q_OBJECT

public:
    explicit CombinedTraceLoader(QObject *parent = nullptr);
    ~CombinedTraceLoader() override;

    // Starts loading and merging `bundleDir`. Emits merged() or failed() exactly
    // once. Calling it again while a load is in flight is ignored.
    void load(const Utils::FilePath &bundleDir);

signals:
    // The temporary directory holding the merged, native-mixed sampler trace.
    void merged(const Utils::FilePath &mergedSamplerDir);
    void failed(const QString &error);

private:
    void onQmlLoaded();

    class CombinedTraceLoaderPrivate *d;
};

} // namespace QmlProfiler::Internal
