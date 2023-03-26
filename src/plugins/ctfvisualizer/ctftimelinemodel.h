// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "json/json.hpp"

#include <tracing/timelinemodel.h>

#include <QMap>
#include <QSet>
#include <QStack>
#include <QVector>

namespace Timeline {
class TimelineModelAggregator;
}

namespace CtfVisualizer {
namespace Internal {

class CtfTraceManager;

class CtfTimelineModel : public Timeline::TimelineModel
{
    Q_OBJECT

    friend class CtfTraceManager;

public:
    explicit CtfTimelineModel(Timeline::TimelineModelAggregator *parent,
                              CtfTraceManager *traceManager, int tid, int pid);

    QRgb color(int index) const override;
    QVariantList labels() const override;
    QVariantMap orderedDetails(int index) const override;
    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;

    int typeId(int index) const override;
    bool handlesTypeId(int typeId) const override;
    float relativeHeight(int index) const override;

    QPair<bool, qint64> addEvent(const nlohmann::json &event, double traceBegin);

    void finalize(double traceBegin, double traceEnd, const QString &processName, const QString &threadName);

    int tid() const;
    QString eventTitle(int index) const;

signals:
    void detailsRequested(const QString &eventName) const;

private:
    void updateName();

    qint64 newStackEvent(const nlohmann::json &event, qint64 normalizedTime,
                         const std::string &eventPhase, const std::string &name, int selectionId);

    qint64 closeStackEvent(const nlohmann::json &event, double timestamp, qint64 normalizedTime);

    void addCounterValue(const nlohmann::json &event, qint64 normalizedTime, const std::string &name, int selectionId);

    const QString &reuse(const QString &value);

protected:
    CtfTraceManager *const m_traceManager;

    int m_threadId;
    QString m_threadName;
    int m_processId;
    QString m_processName;

    int m_maxStackSize = 0;
    QVector<int> m_nestingLevels;
    QVector<QMap<int, QPair<QString, QString>>> m_details;
    QSet<int> m_handledTypeIds;
    QStack<int> m_openEventIds;
    QSet<QString> m_reusableStrings;

    struct CounterData {
        qint64 end = 0;
        int startEventIndex = -1;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();
    };

    QVector<std::string> m_counterNames;
    QVector<CounterData> m_counterData;
    QVector<float> m_counterValues;
    QVector<int> m_itemToCounterIdx;
    QVector<int> m_counterIndexToRow;
};

} // namespace Internal
} // namespace CtfVisualizer
