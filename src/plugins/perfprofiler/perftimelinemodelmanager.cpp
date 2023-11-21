// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perftimelinemodel.h"
#include "perftimelinemodelmanager.h"

#include <utils/qtcassert.h>

namespace PerfProfiler {
namespace Internal {

PerfTimelineModelManager::PerfTimelineModelManager()
   : Timeline::TimelineModelAggregator(&traceManager())
{
    traceManager().registerFeatures(PerfEventType::allFeatures(),
                                   std::bind(&PerfTimelineModelManager::loadEvent, this,
                                             std::placeholders::_1, std::placeholders::_2),
                                   std::bind(&PerfTimelineModelManager::initialize, this),
                                   std::bind(&PerfTimelineModelManager::finalize, this),
                                   std::bind(&PerfTimelineModelManager::clear, this));
}

PerfTimelineModelManager::~PerfTimelineModelManager()
{
    clear();
}

static QString displayNameForThread(const PerfProfilerTraceManager::Thread &thread)
{
    return QString::fromLatin1("%1 (%2)")
            .arg(QString::fromUtf8(traceManager().string(thread.name)))
            .arg(thread.tid);
}

void PerfTimelineModelManager::initialize()
{
    for (const PerfProfilerTraceManager::Thread &thread : traceManager().threads()) {
        if (thread.enabled) {
            m_unfinished.insert(thread.tid, new PerfTimelineModel(
                                    thread.pid, thread.tid, thread.firstEvent, thread.lastEvent,
                                    this));
        }
    }
}

void PerfTimelineModelManager::finalize()
{
    QVector<PerfTimelineModel *> finished;
    QHash<quint32, PerfProfilerTraceManager::Thread> threads = traceManager().threads();
    for (auto it = m_unfinished.begin(), end = m_unfinished.end(); it != end; ++it) {
        PerfTimelineModel *model = *it;

        const PerfProfilerTraceManager::Thread &thread = traceManager().thread(model->tid());
        if (thread.enabled) {
            model->setDisplayName(displayNameForThread(thread));
            model->finalize();
            finished.append(model);
        } else {
            delete model;
        }
    }
    m_unfinished.clear();

    const qint64 frequency = traceManager().samplingFrequency();
    for (PerfTimelineModel *model : std::as_const(finished)) {
        model->setSamplingFrequency(frequency);
        threads.remove(model->tid());
    }

    for (const PerfProfilerTraceManager::Thread &remaining : threads) {
        if (!remaining.enabled)
            continue;
        PerfTimelineModel *model = new PerfTimelineModel(
                    remaining.pid, remaining.tid, remaining.firstEvent, remaining.lastEvent, this);
        model->setDisplayName(displayNameForThread(remaining));
        model->finalize();
        model->setSamplingFrequency(frequency);
        finished.append(model);
    }

    std::sort(finished.begin(), finished.end(), [](PerfTimelineModel *a, PerfTimelineModel *b) {
        return a->tid() < b->tid();
    });

    QVariantList modelsToAdd;
    for (PerfTimelineModel *model : std::as_const(finished))
        modelsToAdd.append(QVariant::fromValue(model));
    setModels(modelsToAdd);
}

void PerfTimelineModelManager::loadEvent(const PerfEvent &event, const PerfEventType &type)
{
    Q_UNUSED(type)
    const int parallel = traceManager().threads().size();
    auto i = m_unfinished.find(event.tid());
    if (i == m_unfinished.end()) {
        i = m_unfinished.insert(event.tid(), new PerfTimelineModel(
                                    event.pid(), event.tid(), event.timestamp(), event.timestamp(),
                                    this));
    }
    (*i)->loadEvent(event, parallel);
}

void PerfTimelineModelManager::clear()
{
    QVariantList perfModels = models();
    Timeline::TimelineModelAggregator::clear();
    for (QVariant &var : perfModels)
        delete qvariant_cast<PerfTimelineModel *>(var);
    qDeleteAll(m_unfinished);
    m_unfinished.clear();
    m_resourceContainers.clear();
}

PerfTimelineModelManager &modelManager()
{
    static PerfTimelineModelManager thePerfTimelineModelManager;
    return thePerfTimelineModelManager;
}

} // namespace Internal
} // namespace PerfProfiler
