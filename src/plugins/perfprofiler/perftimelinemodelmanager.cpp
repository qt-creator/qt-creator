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

#include "perftimelinemodel.h"
#include "perftimelinemodelmanager.h"

#include <utils/qtcassert.h>

namespace PerfProfiler {
namespace Internal {

PerfTimelineModelManager::PerfTimelineModelManager(PerfProfilerTraceManager *traceManager) :
    Timeline::TimelineModelAggregator(traceManager), m_traceManager(traceManager)
{
    traceManager->registerFeatures(PerfEventType::allFeatures(),
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

static QString displayNameForThread(const PerfProfilerTraceManager::Thread &thread,
                                    PerfProfilerTraceManager *manager)
{
    return QString::fromLatin1("%1 (%2)")
            .arg(QString::fromUtf8(manager->string(thread.name)))
            .arg(thread.tid);
}

void PerfTimelineModelManager::initialize()
{
    for (const PerfProfilerTraceManager::Thread &thread : m_traceManager->threads()) {
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
    QHash<quint32, PerfProfilerTraceManager::Thread> threads = m_traceManager->threads();
    for (auto it = m_unfinished.begin(), end = m_unfinished.end(); it != end; ++it) {
        PerfTimelineModel *model = *it;

        const PerfProfilerTraceManager::Thread &thread = m_traceManager->thread(model->tid());
        if (thread.enabled) {
            model->setDisplayName(displayNameForThread(thread, m_traceManager));
            model->finalize();
            finished.append(model);
        } else {
            delete model;
        }
    }
    m_unfinished.clear();

    const qint64 frequency = m_traceManager->samplingFrequency();
    for (PerfTimelineModel *model : qAsConst(finished)) {
        model->setSamplingFrequency(frequency);
        threads.remove(model->tid());
    }

    for (const PerfProfilerTraceManager::Thread &remaining : threads) {
        if (!remaining.enabled)
            continue;
        PerfTimelineModel *model = new PerfTimelineModel(
                    remaining.pid, remaining.tid, remaining.firstEvent, remaining.lastEvent, this);
        model->setDisplayName(displayNameForThread(remaining, m_traceManager));
        model->finalize();
        model->setSamplingFrequency(frequency);
        finished.append(model);
    }

    std::sort(finished.begin(), finished.end(), [](PerfTimelineModel *a, PerfTimelineModel *b) {
        return a->tid() < b->tid();
    });

    QVariantList modelsToAdd;
    for (PerfTimelineModel *model : finished)
        modelsToAdd.append(QVariant::fromValue(model));
    setModels(modelsToAdd);
}

void PerfTimelineModelManager::loadEvent(const PerfEvent &event, const PerfEventType &type)
{
    Q_UNUSED(type);
    const int parallel = m_traceManager->threads().size();
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

} // namespace Internal
} // namespace PerfProfiler
