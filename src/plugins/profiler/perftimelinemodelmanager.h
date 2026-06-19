// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfresourcecounter.h"

#include <tracing/timelinemodelaggregator.h>

#include <QHash>

#include <unordered_map>

namespace PerfProfiler::Internal {

class PerfEvent;
class PerfEventType;
class PerfTimelineModel;

class PerfTimelineModelManager : public Timeline::TimelineModelAggregator
{
public:
    PerfTimelineModelManager();
    ~PerfTimelineModelManager();

    void loadEvent(const PerfEvent &event, const PerfEventType &type);
    void initialize();
    void finalize();
    void clear();

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
    ContainerMap m_resourceContainers;
};

PerfTimelineModelManager &modelManager();

} // namespace PerfProfiler::Internal
