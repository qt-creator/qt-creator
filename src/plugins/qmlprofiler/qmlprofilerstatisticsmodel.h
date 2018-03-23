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

#include <utils/qtcassert.h>

#include <QHash>
#include <QStack>
#include <QVector>
#include <QPointer>
#include <QAbstractTableModel>

namespace QmlProfiler {
class QmlProfilerModelManager;
class QmlProfilerStatisticsRelativesModel;

enum QmlProfilerStatisticsRelation {
    QmlProfilerStatisticsCallees,
    QmlProfilerStatisticsCallers
};

enum ItemRole {
    SortRole = Qt::UserRole + 1, // Sort by data, not by displayed string
    FilterRole,                  // Filter out non-range types
    TypeIdRole,
    FilenameRole,
    LineRole,
    ColumnRole
};

enum MainField {
    MainLocation,
    MainType,
    MainTimeInPercent,
    MainTotalTime,
    MainSelfTimeInPercent,
    MainSelfTime,
    MainCallCount,
    MainTimePerCall,
    MainMedianTime,
    MainMaxTime,
    MainMinTime,
    MainDetails,
    MaxMainField
};

enum RelativeField {
    RelativeLocation,
    RelativeType,
    RelativeTotalTime,
    RelativeCallCount,
    RelativeDetails,
    MaxRelativeField
};

class QmlProfilerStatisticsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    static QString nameForType(RangeType typeNumber);

    struct QmlEventStats {
        std::vector<qint64> durations;
        qint64 total = 0;
        qint64 self = 0;
        qint64 recursive = 0;
        qint64 minimum = 0;
        qint64 maximum = 0;
        qint64 median = 0;
        qint64 calls = 0;

        void finalize()
        {
            static const qint64 qint64Max = std::numeric_limits<qint64>::max();
            size_t size = durations.size();
            QTC_ASSERT(size <= qint64Max, size = qint64Max);
            calls = static_cast<qint64>(size);

            if (size == 0)
                return;

            std::sort(durations.begin(), durations.end());
            const auto avg
                    = [](const qint64 a, const qint64 b) { return a / 2ll + b / 2ll + ((a & 1) + (b & 1)) / 2ll; };

            const size_t half = size / 2;
            median = (size & 1) == 0 ? avg(durations[half - 1], durations[half]) : durations[half];
            minimum = durations.front();
            maximum = durations.back();

            durations.clear();
        }

        qint64 average() const
        {
            return calls == 0 ? 0 : total / calls;
        }

        qint64 totalNonRecursive() const
        {
            return total - recursive;
        }
    };

    double durationPercent(int typeId) const;
    double durationSelfPercent(int typeId) const;

    QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager);
    ~QmlProfilerStatisticsModel() override = default;

    void restrictToFeatures(quint64 features);
    bool isRestrictedToRange() const;

    QStringList details(int typeIndex) const;
    QString summary(const QVector<int> &typeIds) const;
    const QVector<QmlEventStats> &getData() const;
    const QVector<QmlEventType> &getTypes() const;
    const QHash<int, QString> &getNotes() const;
    qint64 rootDuration() const { return m_rootDuration; }

    int count() const;
    void clear();

    void setRelativesModel(QmlProfilerStatisticsRelativesModel *childModel,
                           QmlProfilerStatisticsRelation relation);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

signals:
    void dataAvailable();
    void notesAvailable(int typeIndex);

private:
    void loadEvent(const QmlEvent &event, const QmlEventType &type);
    void finalize();

    void modelManagerStateChanged();
    void notesChanged(int typeIndex);

    QVariant dataForMainEntry(const QModelIndex &index, int role) const;

    QVector<QmlEventStats> m_data;

    QPointer<QmlProfilerStatisticsRelativesModel> m_calleesModel;
    QPointer<QmlProfilerStatisticsRelativesModel> m_callersModel;
    QPointer<QmlProfilerModelManager> m_modelManager;

    QList<RangeType> m_acceptedTypes;
    QHash<int, QString> m_notes;

    QStack<QmlEvent> m_callStack;
    QStack<QmlEvent> m_compileStack;

    qint64 m_rootDuration = 0;
};

class QmlProfilerStatisticsRelativesModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    struct QmlStatisticsRelativesData {
        QmlStatisticsRelativesData(qint64 duration = 0, qint64 calls = 0, int typeIndex = -1,
                                   bool isRecursive = false)
            : duration(duration), calls(calls), typeIndex(typeIndex), isRecursive(isRecursive) {}
        qint64 duration;
        qint64 calls;
        int typeIndex;
        bool isRecursive;
    };

    QmlProfilerStatisticsRelativesModel(QmlProfilerModelManager *modelManager,
                                        QmlProfilerStatisticsModel *statisticsModel,
                                        QmlProfilerStatisticsRelation relation);

    int count() const;
    void clear();

    const QVector<QmlStatisticsRelativesData> &getData(int typeId) const;
    const QVector<QmlEventType> &getTypes() const;

    void loadEvent(RangeType type, const QmlEvent &event, bool isRecursive);

    QmlProfilerStatisticsRelation relation() const;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

signals:
    void dataAvailable();

protected:
    QVariant dataForMainEntry(qint64 totalDuration, int role, int column) const;

    QHash<int, QVector<QmlStatisticsRelativesData>> m_data;
    QPointer<QmlProfilerModelManager> m_modelManager;

    int m_relativeTypeIndex = std::numeric_limits<int>::max();

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
