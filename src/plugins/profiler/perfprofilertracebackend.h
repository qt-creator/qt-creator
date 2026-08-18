// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profilertracebackend.h"

namespace ProjectExplorer { class Kit; }
namespace Timeline { class RangeDetailsWidget; }
namespace Utils { class FileInProjectFinder; }

namespace Profiler::Internal {

class PerfProfilerTraceManager;
class PerfTimelineModelManager;

// The Performance Analyzer's views and models for one trace.
class PerfProfilerTraceBackend : public ProfilerTraceBackend
{
    Q_OBJECT

public:
    explicit PerfProfilerTraceBackend(Timeline::RangeDetailsWidget *details,
                                      QObject *parent = nullptr);
    ~PerfProfilerTraceBackend() override;

    QWidgetList views(QWidget *parent) override;

    void load(const Utils::FilePath &path) override;
    // A perf.data file, as recorded by perf itself, converted through perfparser.
    void loadPerfData(const Utils::FilePath &path, const Utils::FilePath &executableDir,
                      ProjectExplorer::Kit *kit);
    bool isSaveable() const override { return true; }
    Utils::Result<> save(const Utils::FilePath &path) override;
    void clear() override;
    std::chrono::milliseconds traceDuration() const override;

    PerfProfilerTraceManager *traceManager() const;
    PerfTimelineModelManager *modelManager() const;
    // Teaches the file finder where a recorded path might live on this machine.
    Utils::FileInProjectFinder *fileFinder() const;

signals:
    void saved();

private:
    class PerfProfilerTraceBackendPrivate *d;
};

} // namespace Profiler::Internal
