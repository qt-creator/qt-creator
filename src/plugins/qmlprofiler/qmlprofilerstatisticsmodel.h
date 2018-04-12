/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilernotesmodel.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlprofilerconstants.h"

#include <QHash>
#include <QStack>
#include <QVector>
#include <QObject>
#include <QPointer>

namespace QmlProfiler {
class QmlProfilerModelManager;
class QmlProfilerStatisticsRelativesModel;

enum QmlProfilerStatisticsRelation {
    QmlProfilerStatisticsCallees,
    QmlProfilerStatisticsCallers
};

class QmlProfilerStatisticsModel : public QObject
{
    Q_OBJECT
public:
    static QString nameForType(RangeType typeNumber);

    struct QmlEventStats {
        qint64 duration = 0;
        qint64 durationSelf = 0;
        qint64 durationRecursive = 0;
        qint64 calls = 0;
        qint64 minTime = std::numeric_limits<qint64>::max();
        qint64 maxTime = 0;
        qint64 medianTime = 0;
    };

    double durationPercent(int typeId) const;
    double durationSelfPercent(int typeId) const;

    QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager);
    ~QmlProfilerStatisticsModel() override = default;

    void restrictToFeatures(quint64 features);
    bool isRestrictedToRange() const;

    QStringList details(int typeIndex) const;
    QString summary(const QVector<int> &typeIds) const;
    const QHash<int, QmlEventStats> &getData() const;
    const QVector<QmlEventType> &getTypes() const;
    const QHash<int, QString> &getNotes() const;

    int count() const;
    void clear();

    void setRelativesModel(QmlProfilerStatisticsRelativesModel *childModel,
                           QmlProfilerStatisticsRelation relation);

signals:
    void dataAvailable();
    void notesAvailable(int typeIndex);

private:
    void loadEvent(const QmlEvent &event, const QmlEventType &type);
    void finalize();

    void dataChanged();
    void notesChanged(int typeIndex);

    QHash<int, QmlEventStats> m_data;

    QPointer<QmlProfilerStatisticsRelativesModel> m_calleesModel;
    QPointer<QmlProfilerStatisticsRelativesModel> m_callersModel;
    QPointer<QmlProfilerModelManager> m_modelManager;

    QList<RangeType> m_acceptedTypes;
    QHash<int, QString> m_notes;

    QStack<QmlEvent> m_callStack;
    QStack<QmlEvent> m_compileStack;
    QHash<int, QVector<qint64>> m_durations;
};

class QmlProfilerStatisticsRelativesModel : public QObject
{
    Q_OBJECT
public:

    struct QmlStatisticsRelativesData {
        QmlStatisticsRelativesData(qint64 duration = 0, qint64 calls = 0, bool isRecursive = false)
            : duration(duration), calls(calls), isRecursive(isRecursive) {}
        qint64 duration;
        qint64 calls;
        bool isRecursive;
    };
    typedef QHash <int, QmlStatisticsRelativesData> QmlStatisticsRelativesMap;

    QmlProfilerStatisticsRelativesModel(QmlProfilerModelManager *modelManager,
                                        QmlProfilerStatisticsModel *statisticsModel,
                                        QmlProfilerStatisticsRelation relation);

    int count() const;
    void clear();

    const QmlStatisticsRelativesMap &getData(int typeId) const;
    const QVector<QmlEventType> &getTypes() const;

    void loadEvent(RangeType type, const QmlEvent &event, bool isRecursive);

    QmlProfilerStatisticsRelation relation() const;

signals:
    void dataAvailable();

protected:
    QHash<int, QmlStatisticsRelativesMap> m_data;
    QPointer<QmlProfilerModelManager> m_modelManager;

    struct Frame {
        Frame(qint64 startTime = 0, int typeId = -1) : startTime(startTime), typeId(typeId) {}
        qint64 startTime;
        int typeId;
    };
    QStack<Frame> m_callStack;
    QStack<Frame> m_compileStack;
    const QmlProfilerStatisticsRelation m_relation;
};

} // namespace QmlProfiler
