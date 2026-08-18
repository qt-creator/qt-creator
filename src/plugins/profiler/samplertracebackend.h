// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profilertracebackend.h"

namespace Timeline { class RangeDetailsWidget; }

namespace Profiler::Internal {

// The call-stack sampler's views for one recorded trace: CPU usage over time
// and the merged call tree.
class SamplerTraceBackend : public ProfilerTraceBackend
{
    Q_OBJECT

public:
    explicit SamplerTraceBackend(Timeline::RangeDetailsWidget *details, QObject *parent = nullptr);
    ~SamplerTraceBackend() override;

    QWidgetList views(QWidget *parent) override;

    void load(const Utils::FilePath &path) override;
    void clear() override;
    std::chrono::milliseconds traceDuration() const override;

private:
    class SamplerTraceBackendPrivate *d;
};

} // namespace Profiler::Internal
