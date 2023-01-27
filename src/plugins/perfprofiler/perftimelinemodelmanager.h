// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofilertracemanager.h"
#include "perfresourcecounter.h"

#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>

#include <QPointer>
#include <unordered_map>

namespace ProjectExplorer {
class Kit;
} // namespace ProjectExplorer

namespace PerfProfiler {
namespace Internal {

class PerfDataReader;
class PerfTimelineModel;
class PerfTimelineModelManager : public Timeline::TimelineModelAggregator
{
    Q_OBJECT
public:
    explicit PerfTimelineModelManager(PerfProfilerTraceManager *traceManager);
    ~PerfTimelineModelManager();

    void loadEvent(const PerfEvent &event, const PerfEventType &type);
    void initialize();
    void finalize();
    void clear();

    const PerfProfilerTraceManager *traceManager() const { return m_traceManager.data(); }

    PerfResourceCounter<>::Container *resourceContainer(quint32 pid)
    {
        std::unique_ptr<PerfResourceCounter<>::Container> &container = m_resourceContainers[pid];
        if (!container)
            container.reset(new PerfResourceCounter<>::Container);
        return container.get();
    }

private:
    using ContainerMap
        = typename std::unordered_map<quint32, std::unique_ptr<PerfResourceCounter<>::Container>>;

    QHash<quint32, PerfTimelineModel *> m_unfinished;
    QPointer<PerfProfilerTraceManager> m_traceManager;
    ContainerMap m_resourceContainers;
};

} // namespace Internal
} // namespace PerfProfiler
