// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profilertracebackend.h"

namespace Timeline { class RangeDetailsWidget; }

namespace Profiler::Internal {

// The Chrome Trace Format / Common Trace Format views for one trace. Which of
// the two is loaded follows from the path: a directory is CTF2, a file is a
// Chrome trace (see identifyTrace()).
class CtfTraceBackend : public ProfilerTraceBackend
{
    Q_OBJECT

public:
    explicit CtfTraceBackend(Timeline::RangeDetailsWidget *details, QObject *parent = nullptr);
    ~CtfTraceBackend() override;

    QWidgetList views(QWidget *parent) override;

    void load(const Utils::FilePath &path) override;
    void clear() override;
    std::chrono::milliseconds traceDuration() const override;

private:
    class CtfTraceBackendPrivate *d;
};

} // namespace Profiler::Internal
