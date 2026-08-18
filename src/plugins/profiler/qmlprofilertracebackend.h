// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profilertracebackend.h"

namespace Timeline { class RangeDetailsWidget; }

namespace Profiler::Internal {

class QmlProfilerModelManager;

// The QML profiler's views and models for one trace.
class QmlProfilerTraceBackend : public ProfilerTraceBackend
{
    Q_OBJECT

public:
    explicit QmlProfilerTraceBackend(Timeline::RangeDetailsWidget *details,
                                     QObject *parent = nullptr);
    ~QmlProfilerTraceBackend() override;

    QWidgetList views(QWidget *parent) override;

    void load(const Utils::FilePath &path) override;
    bool isSaveable() const override { return true; }
    // Writing runs in the background, so this reports that the write started,
    // not that it finished; saved() marks the point the trace is on disk.
    Utils::Result<> save(const Utils::FilePath &path) override;
    bool isModified() const override;
    void clear() override;
    std::chrono::milliseconds traceDuration() const override;

    QmlProfilerModelManager *modelManager() const;

signals:
    // A load or save is running; the editor disables itself meanwhile.
    void busyChanged(bool busy);
    void saved();

private:
    class QmlProfilerTraceBackendPrivate *d;
};

} // namespace Profiler::Internal
