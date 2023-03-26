// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "json/json.hpp"

#include <QHash>
#include <QMap>
#include <QObject>
#include <QVector>

namespace Timeline {
class TimelineModelAggregator;
}

namespace CtfVisualizer {
namespace Internal {

class CtfStatisticsModel;
class CtfTimelineModel;

class CtfTraceManager : public QObject
{
    Q_OBJECT

public:
    explicit CtfTraceManager(QObject *parent,
                             Timeline::TimelineModelAggregator *modelAggregator,
                             CtfStatisticsModel *statisticsModel);

    qint64 traceDuration() const;
    qint64 traceBegin() const;
    qint64 traceEnd() const;

    void addEvent(const nlohmann::json &event);

    void load(const QString &filename);
    void finalize();

    bool isEmpty() const;

    int getSelectionId(const std::string &name);

    QList<CtfTimelineModel *> getSortedThreads() const;

    void setThreadRestriction(int tid, bool restrictToThisThread);
    bool isRestrictedTo(int tid) const;

signals:
    void detailsRequested(const QString &title);

protected:

    void addModelForThread(int threadId, int processId);
    void addModelsToAggregator();

    void updateStatistics();

    void clearAll();

    Timeline::TimelineModelAggregator *const m_modelAggregator;
    CtfStatisticsModel *const m_statisticsModel;

    QHash<qint64, CtfTimelineModel *> m_threadModels;
    QHash<qint64, QString> m_processNames;
    QHash<qint64, QString> m_threadNames;
    QMap<std::string, int> m_name2selectionId;
    QHash<qint64, bool> m_threadRestrictions;

    double m_traceBegin = std::numeric_limits<double>::max();
    double m_traceEnd = std::numeric_limits<double>::min();
    double m_timeOffset = -1.0;

};

} // namespace Internal
} // namespace CtfVisualizer
