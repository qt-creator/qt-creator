/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
** info@kdab.com, author Tim Henning <tim.henning@kdab.com>
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
#include "ctftracemanager.h"

#include "ctftimelinemodel.h"
#include "ctfstatisticsmodel.h"
#include "ctfvisualizerconstants.h"

#include <coreplugin/icore.h>
#include <tracing/timelinemodelaggregator.h>

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QMessageBox>

#include <fstream>


namespace CtfVisualizer {
namespace Internal {

using json = nlohmann::json;

using namespace Constants;


class CtfJsonParserCallback
{

public:

    explicit CtfJsonParserCallback(CtfTraceManager *traceManager)
        : m_traceManager(traceManager)
    {}

    bool callback(int depth, nlohmann::json::parse_event_t event, nlohmann::json &parsed)
    {
        if ((event == json::parse_event_t::array_start && depth == 0)
                || (event == json::parse_event_t::key && depth == 1 && parsed == json(CtfTraceEventsKey))) {
            m_isInTraceArray = true;
            m_traceArrayDepth = depth;
            return true;
        }
        if (m_isInTraceArray && event == json::parse_event_t::array_end && depth == m_traceArrayDepth) {
            m_isInTraceArray = false;
            return false;
        }
        if (m_isInTraceArray && event == json::parse_event_t::object_end && depth == m_traceArrayDepth + 1) {
            m_traceManager->addEvent(parsed);
            return false;
        }
        if (m_isInTraceArray || (event == json::parse_event_t::object_start && depth == 0)) {
            // keep outer object and values in trace objects:
            return true;
        }
        // discard any objects outside of trace array:
        // TODO: parse other data, e.g. stack frames
        return false;
    }

protected:
    CtfTraceManager *m_traceManager;

    bool m_isInTraceArray = false;
    int m_traceArrayDepth = 0;
};

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
    const double timestamp = event.value(CtfTracingClockTimestampKey, -1.0);
    if (timestamp < 0) {
        // events without or with negative timestamp will be ignored
        return;
    }
    if (m_timeOffset < 0) {
        // the timestamp of the first event is used as the global offset
        m_timeOffset = timestamp;
    }

    const int processId = event.value(CtfProcessIdKey, 0);
    const int threadId = event.contains(CtfThreadIdKey) ? int(event[CtfThreadIdKey]) : processId;
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
}

void CtfTraceManager::load(const QString &filename)
{
    clearAll();

    std::ifstream file(filename.toStdString());
    if (!file.is_open()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("CTF Visualizer"),
                             tr("Cannot read the CTF file."));
        return;
    }
    CtfJsonParserCallback ctfParser(this);
    json::parser_callback_t callback = [&ctfParser](int depth, json::parse_event_t event, json &parsed) {
        return ctfParser.callback(depth, event, parsed);
    };
    json unusedValues = json::parse(file, callback, /*allow_exceptions*/ false);
    file.close();
    updateStatistics();
}

void CtfTraceManager::finalize()
{
    bool userConsentToIgnoreDeepTraces = false;
    for (qint64 tid: m_threadModels.keys()) {
        if (m_threadModels[tid]->m_maxStackSize > 512) {
            if (!userConsentToIgnoreDeepTraces) {
                QMessageBox::StandardButton answer
                    = QMessageBox::question(Core::ICore::dialogParent(),
                                            tr("CTF Visualizer"),
                                            tr("The trace contains threads with stack depth > "
                                               "512.\nDo you want to display them anyway?"),
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No);
                if (answer == QMessageBox::No) {
                    userConsentToIgnoreDeepTraces = true;
                } else {
                    break;
                }
            }
            m_threadModels.remove(tid);
            m_threadRestrictions.remove(tid);
        }
    }
    for (CtfTimelineModel *model: m_threadModels) {
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
    std::sort(models.begin(), models.end(), [](const CtfTimelineModel *a, const CtfTimelineModel *b) -> bool {
        return (a->m_processId != b->m_processId) ? (a->m_processId < b->m_processId)
                                                  : (std::abs(a->m_threadId) < std::abs(b->m_threadId));
    });
    return models;
}

void CtfTraceManager::setThreadRestriction(int tid, bool restrictToThisThread)
{
    if (m_threadRestrictions.value(tid) == restrictToThisThread)
        return;

    m_threadRestrictions[tid] = restrictToThisThread;
    addModelsToAggregator();
}

bool CtfTraceManager::isRestrictedTo(int tid) const
{
    return m_threadRestrictions.value(tid);
}

void CtfTraceManager::addModelForThread(int threadId, int processId)
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
    for (auto thread : m_threadModels) {
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
    m_modelAggregator->clear();
    for (CtfTimelineModel *model: m_threadModels) {
        model->deleteLater();
    }
    m_threadModels.clear();
    m_traceBegin = std::numeric_limits<double>::max();
    m_traceEnd = std::numeric_limits<double>::min();
    m_timeOffset = -1;
}


} // namespace Internal
} // namespace CtfVisualizer
