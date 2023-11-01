// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "json/json.hpp"

#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>

namespace Timeline { class TimelineModelAggregator; }

namespace CtfVisualizer::Internal {

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

    void finalize();

    bool isEmpty() const;

    int getSelectionId(const std::string &name);

    QList<CtfTimelineModel *> getSortedThreads() const;

    void setThreadRestriction(const QString &tid, bool restrictToThisThread);
    bool isRestrictedTo(const QString &tid) const;

    void updateStatistics();
    void clearAll();

    QString errorString() const;

signals:
    void detailsRequested(const QString &title);

protected:
    void addModelForThread(const QString &threadId, const QString &processId);
    void addModelsToAggregator();

    Timeline::TimelineModelAggregator *const m_modelAggregator;
    CtfStatisticsModel *const m_statisticsModel;

    QHash<QString, CtfTimelineModel *> m_threadModels;
    QHash<QString, QString> m_processNames;
    QHash<QString, QString> m_threadNames;
    QMap<std::string, int> m_name2selectionId;
    QHash<QString, bool> m_threadRestrictions;

    double m_traceBegin = std::numeric_limits<double>::max();
    double m_traceEnd = std::numeric_limits<double>::min();
    double m_timeOffset = -1.0;

    QString m_errorString;
};

} // namespace CtfVisualizer::Internal
