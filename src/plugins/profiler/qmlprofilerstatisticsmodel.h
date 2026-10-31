// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventtype.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/qtcassert.h>

#include <QHash>
#include <QStack>
#include <QPointer>
#include <QAbstractTableModel>

namespace Profiler::Internal {

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

class QmlProfilerStatisticsModel final : public QAbstractTableModel
{
public:
    struct QmlEventStats {
        std::vector<std::chrono::nanoseconds> durations;
        std::chrono::nanoseconds total{0};
        std::chrono::nanoseconds self{0};
        std::chrono::nanoseconds recursive{0};
        std::chrono::nanoseconds minimum{0};
        std::chrono::nanoseconds maximum{0};
        std::chrono::nanoseconds median{0};
        qint64 calls = 0;

        void finalize()
        {
            static const qint64 qint64Max = std::numeric_limits<qint64>::max();
            size_t size = durations.size();
            QTC_ASSERT(sizeof(size_t) < sizeof(qint64) || size <= qint64Max, size = qint64Max);
            calls = static_cast<qint64>(size);

            if (size == 0)
                return;

            std::sort(durations.begin(), durations.end());
            const auto avg = [](std::chrono::nanoseconds a, std::chrono::nanoseconds b) {
                return a / 2 + b / 2
                       + std::chrono::nanoseconds(((a.count() & 1) + (b.count() & 1)) / 2);
            };

            const size_t half = size / 2;
            median = (size & 1) == 0 ? avg(durations[half - 1], durations[half]) : durations[half];
            minimum = durations.front();
            maximum = durations.back();

            // One entry per call adds up to gigabytes over a long trace; clear() would
            // keep all of it allocated.
            std::vector<std::chrono::nanoseconds>().swap(durations);
        }

        std::chrono::nanoseconds average() const
        {
            return calls == 0 ? std::chrono::nanoseconds(0)
                              : std::chrono::nanoseconds(total / calls);
        }

        std::chrono::nanoseconds totalNonRecursive() const
        {
            return total - recursive;
        }
    };

    QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager);
    ~QmlProfilerStatisticsModel() override = default;

    void restrictToFeatures(quint64 features);
    bool isRestrictedToRange() const;

    QStringList details(int typeIndex) const;
    QString summary(const QList<int> &typeIds) const;

    void clear();

    void setRelativesModel(QmlProfilerStatisticsRelativesModel *childModel,
                           QmlProfilerStatisticsRelation relation);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    static const int s_mainEntryTypeId = std::numeric_limits<int>::max();
    static const int s_invalidTypeId = -1;

private:
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    void prepare();
    void finalize();

    void typeDetailsChanged(int typeIndex);
    void notesChanged(int typeIndex);

    QVariant dataForMainEntry(const QModelIndex &index, int role) const;

    double durationPercent(int typeId) const;
    double durationSelfPercent(int typeId) const;

    QList<QmlEventStats> m_data;

    QPointer<QmlProfilerStatisticsRelativesModel> m_calleesModel;
    QPointer<QmlProfilerStatisticsRelativesModel> m_callersModel;
    QPointer<QmlProfilerModelManager> m_modelManager;

    QList<QmlDebug::RangeType> m_acceptedTypes;
    QHash<int, QString> m_notes;

    QStack<QmlDebug::QmlEvent> m_callStack;
    QStack<QmlDebug::QmlEvent> m_compileStack;

    std::chrono::nanoseconds m_rootDuration{0};
};

class QmlProfilerStatisticsRelativesModel final : public QAbstractTableModel
{
public:
    struct QmlStatisticsRelativesData {
        QmlStatisticsRelativesData(std::chrono::nanoseconds duration = std::chrono::nanoseconds(0),
                                   qint64 calls = 0,
                                   int typeIndex = QmlProfilerStatisticsModel::s_invalidTypeId,
                                   bool isRecursive = false)
            : duration(duration), calls(calls), typeIndex(typeIndex), isRecursive(isRecursive) {}
        std::chrono::nanoseconds duration;
        qint64 calls;
        int typeIndex;
        bool isRecursive;
    };

    QmlProfilerStatisticsRelativesModel(QmlProfilerModelManager *modelManager,
                                        QmlProfilerStatisticsModel *statisticsModel,
                                        QmlProfilerStatisticsRelation relation);

    void clear();
    void loadEvent(QmlDebug::RangeType type, const QmlDebug::QmlEvent &event, bool isRecursive);

    int rowCount(const QModelIndex &parent) const final;
    int columnCount(const QModelIndex &parent) const final;
    QVariant data(const QModelIndex &index, int role) const final;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const final;
    bool setData(const QModelIndex &index, const QVariant &value, int role) final;

private:
    QVariant dataForMainEntry(std::chrono::nanoseconds totalDuration, int role, int column) const;
    void typeDetailsChanged(int typeId);

    QHash<int, QList<QmlStatisticsRelativesData>> m_data;
    QPointer<QmlProfilerModelManager> m_modelManager;

    int m_relativeTypeIndex = QmlProfilerStatisticsModel::s_invalidTypeId;

    struct Frame {
        Frame(std::chrono::nanoseconds startTime = std::chrono::nanoseconds(0),
              int typeId = QmlProfilerStatisticsModel::s_invalidTypeId)
            : startTime(startTime), typeId(typeId) {}
        std::chrono::nanoseconds startTime;
        int typeId;
    };
    QStack<Frame> m_callStack;
    QStack<Frame> m_compileStack;
    const QmlProfilerStatisticsRelation m_relation;
};

} // namespace Profiler::Internal
