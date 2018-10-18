/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "perfprofilertracemanager.h"
#include "perfresourcecounter.h"

#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>
#include <utils/runextensions.h>

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
