// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "ctftracemanager.h"

#include "ctfstatisticsmodel.h"
#include "ctftimelinemodel.h"
#include "ctfvisualizerconstants.h"
#include "ctfvisualizertr.h"

#include <coreplugin/icore.h>

#include <tracing/timelinemodelaggregator.h>

#include <QMessageBox>

namespace CtfVisualizer::Internal {

using json = nlohmann::json;
using namespace Constants;

CtfTraceManager::CtfTraceManager(QObject *parent,
                                 Timeline::TimelineModelAggregator *modelAggregator,
                                 CtfStatisticsModel *statisticsModel)
    : QObject(parent)
    , m_modelAggregator(modelAggregator)
    , m_statisticsModel(statisticsModel)
{
}

qint64 CtfTraceManager::traceDuration() const
{
    return qint64((m_traceEnd - m_traceBegin) * 1000);
}

qint64 CtfTraceManager::traceBegin() const
{
    return qint64((m_traceBegin - m_timeOffset) * 1000);
}

qint64 CtfTraceManager::traceEnd() const
{
    return qint64((m_traceEnd - m_timeOffset) * 1000);
}

void CtfTraceManager::addEvent(const json &event)
{
    try {
        const double timestamp = event.value(CtfTracingClockTimestampKey, -1.0);
        if (timestamp < 0) {
            // events without or with negative timestamp will be ignored
            return;
        }
        if (m_timeOffset < 0) {
            // the timestamp of the first event is used as the global offset
            m_timeOffset = timestamp;
        }

        static const auto getStringValue =
            [](const json &event, const char *key, const QString &def) {
                if (!event.contains(key))
                    return def;
                const json val = event[key];
                if (val.is_string())
                    return QString::fromStdString(val);
                if (val.is_number()) {
                    return QString::number(int(val));
                }
                return def;
            };
        const QString processId = getStringValue(event, CtfProcessIdKey, "0");
        const QString threadId = getStringValue(event, CtfThreadIdKey, processId);
        if (!m_threadModels.contains(threadId)) {
            addModelForThread(threadId, processId);
        }
        if (event.value(CtfEventPhaseKey, "") == CtfEventTypeMetadata) {
            const std::string name = event[CtfEventNameKey];
            if (name == "thread_name") {
                m_threadNames[threadId] = QString::fromStdString(event["args"]["name"]);
            } else if (name == "process_name") {
                m_processNames[processId] = QString::fromStdString(event["args"]["name"]);
            }
        }

        const QPair<bool, qint64> result = m_threadModels[threadId]->addEvent(event, m_timeOffset);
        const bool visibleOnTimeline = result.first;
        if (visibleOnTimeline) {
            m_traceBegin = std::min(m_traceBegin, timestamp);
            m_traceEnd = std::max(m_traceEnd, timestamp);
        } else if (m_timeOffset == timestamp) {
            // this timestamp was used as the time offset but it is not a visible element
            // -> reset the time offset again:
            m_timeOffset = -1.0;
        }
    } catch (...) {
        m_errorString = Tr::tr("Error while parsing CTF data: %1.")
                            .arg("<pre>" + QString::fromStdString(event.dump()) + "</pre>");
    }
}

void CtfTraceManager::finalize()
{
    bool userConsentToIgnoreDeepTraces = false;
    auto it = m_threadModels.begin();
    while (it != m_threadModels.end()) {
        if (it.value()->m_maxStackSize > 512) {
            if (!userConsentToIgnoreDeepTraces) {
                QMessageBox::StandardButton answer
                    = QMessageBox::question(Core::ICore::dialogParent(),
                                            Tr::tr("CTF Visualizer"),
                                            Tr::tr("The trace contains threads with stack depth > "
                                               "512.\nDo you want to display them anyway?"),
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No);
                if (answer == QMessageBox::No) {
                    userConsentToIgnoreDeepTraces = true;
                } else {
                    break;
                }
            }
            m_threadRestrictions.remove(it.key());
            it = m_threadModels.erase(it);
        } else {
            ++it;
        }
    }
    for (CtfTimelineModel *model: std::as_const(m_threadModels)) {
        model->finalize(m_traceBegin, m_traceEnd,
                        m_processNames[model->m_processId], m_threadNames[model->m_threadId]);
    }
    // TimelineModelAggregator::addModel() is called here because it
    // needs to be run in the main thread
    addModelsToAggregator();
}

bool CtfTraceManager::isEmpty() const
{
    return m_threadModels.isEmpty();
}

int CtfTraceManager::getSelectionId(const std::string &name)
{
    auto it = m_name2selectionId.find(name);
    if (it == m_name2selectionId.end())
        it = m_name2selectionId.insert(name, m_name2selectionId.size());
    return *it;
}

QList<CtfTimelineModel *> CtfTraceManager::getSortedThreads() const
{
    QList<CtfTimelineModel *> models = m_threadModels.values();
    std::sort(models.begin(),
              models.end(),
              [](const CtfTimelineModel *a, const CtfTimelineModel *b) {
                  return (a->m_processId != b->m_processId) ? (a->m_processId < b->m_processId)
                                                            : (a->m_threadId < b->m_threadId);
              });
    return models;
}

void CtfTraceManager::setThreadRestriction(const QString &tid, bool restrictToThisThread)
{
    if (m_threadRestrictions.value(tid) == restrictToThisThread)
        return;

    m_threadRestrictions[tid] = restrictToThisThread;
    addModelsToAggregator();
}

bool CtfTraceManager::isRestrictedTo(const QString &tid) const
{
    return m_threadRestrictions.value(tid);
}

void CtfTraceManager::addModelForThread(const QString &threadId, const QString &processId)
{
    CtfTimelineModel *model = new CtfTimelineModel(m_modelAggregator, this, threadId, processId);
    m_threadModels.insert(threadId, model);
    m_threadRestrictions.insert(threadId, false);
    connect(model, &CtfTimelineModel::detailsRequested, this,
            &CtfTraceManager::detailsRequested);
}

void CtfTraceManager::addModelsToAggregator()
{
    const QList<CtfTimelineModel *> models = getSortedThreads();

    const bool showAll = std::none_of(m_threadRestrictions.begin(), m_threadRestrictions.end(), [](bool value) {
        return value;
    });

    QVariantList modelsToAdd;
    for (CtfTimelineModel *model: models) {
        if (showAll || isRestrictedTo(model->tid()))
            modelsToAdd.append(QVariant::fromValue(model));
    }
    m_modelAggregator->setModels(modelsToAdd);
    updateStatistics();
}

void CtfTraceManager::updateStatistics()
{
    const bool showAll = std::none_of(m_threadRestrictions.begin(), m_threadRestrictions.end(), [](bool value) {
        return value;
    });

    m_statisticsModel->beginLoading();
    for (auto thread : std::as_const(m_threadModels)) {
        if (showAll || m_threadRestrictions[thread->tid()])
        {
            const int eventCount = thread->count();
            for (int i = 0; i < eventCount; ++i) {
                QString title = thread->eventTitle(i);
                m_statisticsModel->addEvent(title, thread->duration(i));
            }
        }
    }
    m_statisticsModel->setMeasurementDuration(qint64((m_traceEnd - m_traceBegin) * 1000));
    m_statisticsModel->endLoading();
}

void CtfTraceManager::clearAll()
{
    m_errorString.clear();
    m_modelAggregator->clear();
    for (CtfTimelineModel *model: std::as_const(m_threadModels)) {
        model->deleteLater();
    }
    m_threadModels.clear();
    m_traceBegin = std::numeric_limits<double>::max();
    m_traceEnd = std::numeric_limits<double>::min();
    m_timeOffset = -1;
}

QString CtfTraceManager::errorString() const
{
    return m_errorString;
}

} // namespace CtfVisualizer::Internal
