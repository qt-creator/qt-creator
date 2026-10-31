// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "json/json.hpp"

#include <tracing/timelinemodel.h>

#include <QList>
#include <QMap>
#include <QSet>
#include <QStack>

#include <chrono>

namespace Timeline { class TimelineModelAggregator; }

namespace Profiler::Internal {

class CtfTraceManager;

// A time span given in microseconds, as a CTF trace states it, as nanoseconds.
std::chrono::nanoseconds fromMicroseconds(double microseconds);

class CtfTimelineModel : public Timeline::TimelineModel
{
    Q_OBJECT

    friend class CtfTraceManager;

public:
    explicit CtfTimelineModel(Timeline::TimelineModelAggregator *parent,
                              CtfTraceManager *traceManager,
                              const QString &tid,
                              const QString &pid);

    QRgb color(int index) const override;
    Timeline::RowLabels labels() const override;
    Timeline::OrderedItemDetails orderedDetails(int index) const override;
    QString itemLabel(int index) const override;
    Timeline::ItemLocation location(int index) const override;
    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;

    int typeId(int index) const override;
    bool handlesTypeId(int typeId) const override;
    float relativeHeight(int index) const override;

    QPair<bool, std::chrono::nanoseconds> addEvent(const nlohmann::json &event, double traceBegin);

    // `manyProcesses` says whether the trace holds more than one, which is when
    // naming this lane's process tells the reader which lane it is looking at.
    // `mainThread` says that this lane is the one its process runs on, which is
    // where the process is named instead of the thread (see updateName()).
    void finalize(double traceBegin, double traceEnd, const QString &processName,
                  const QString &threadName, bool manyProcesses, bool mainThread);

    QString tid() const;
    QString eventTitle(int index) const;

signals:
    void detailsRequested(const QString &eventName) const;

private:
    void updateName();

    std::chrono::nanoseconds newStackEvent(const nlohmann::json &event,
                                           std::chrono::nanoseconds normalizedTime,
                                           const std::string &eventPhase,
                                           const std::string &name, int selectionId);

    std::chrono::nanoseconds closeStackEvent(const nlohmann::json &event, double timestamp,
                                             std::chrono::nanoseconds normalizedTime);

    void addCounterValue(const nlohmann::json &event, std::chrono::nanoseconds normalizedTime,
                         const std::string &name, int selectionId);

    QString reuse(const QString &value);

protected:
    CtfTraceManager *const m_traceManager;

    QString m_threadId;
    QString m_threadName;
    QString m_processId;
    QString m_processName;
    // The ids the trace stated, which is what a reader is shown: the ones above
    // identify a lane across a whole load, and are qualified for that where the
    // trace's own are not unique (see ctfloader.cpp). Equal to them unless a
    // thread_name/process_name event said otherwise.
    QString m_threadDisplayId;
    QString m_processDisplayId;
    bool m_manyProcesses = true;
    bool m_mainThread = false;

    int m_maxStackSize = 0;
    QList<int> m_rows;
    QList<QMap<int, QPair<QString, QString>>> m_details;
    QList<Timeline::ItemLocation> m_locations; // parallel to m_details
    QSet<int> m_handledTypeIds;
    QStack<int> m_openEventIds;
    QSet<QString> m_reusableStrings;

    struct CounterData {
        std::chrono::nanoseconds end{0};
        int startEventIndex = -1;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();
    };

    QList<std::string> m_counterNames;
    QList<CounterData> m_counterData;
    QList<float> m_counterValues;
    QList<int> m_itemToCounterIdx;
    QList<int> m_counterIndexToRow;
};

} // namespace Profiler::Internal
