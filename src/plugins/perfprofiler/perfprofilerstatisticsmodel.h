// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofilertracemanager.h"

#include <QAbstractItemModel>
#include <QFont>

namespace PerfProfiler {
namespace Internal {

struct PerfProfilerStatisticsData;
class PerfProfilerStatisticsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Relation { Main, Parents, Children, MaximumRelation };

    enum Column {
        Address,
        Function,
        SourceLocation,
        BinaryLocation,
        Caller,
        Callee,
        Occurrences,
        OccurrencesInPercent,
        RecursionInPercent,
        Samples,
        SamplesInPercent,
        Self,
        SelfInPercent,
        MaximumColumn
    };

    struct Frame {
        explicit Frame(int typeId = -1) : typeId(typeId), occurrences(0) {}
        int typeId;
        uint occurrences;
    };

    int columnCount(const QModelIndex &parent) const final;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const final;
    void resort();

protected:
    PerfProfilerStatisticsModel(Relation relation, QObject *parent = nullptr);

    int lastSortColumn;
    Qt::SortOrder lastSortOrder;
    QFont m_font;
    QVector<Column> m_columns;
};

class PerfProfilerStatisticsRelativesModel;
class PerfProfilerStatisticsMainModel : public PerfProfilerStatisticsModel {
    Q_OBJECT
public:
    PerfProfilerStatisticsMainModel(PerfProfilerTraceManager *parent);
    ~PerfProfilerStatisticsMainModel() override;
    PerfProfilerStatisticsRelativesModel *children() const { return m_children; }
    PerfProfilerStatisticsRelativesModel *parents() const { return m_parents; }

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void sort(int column, Qt::SortOrder order) override;
    void resort();

    QByteArray metaInfo(int typeId, Column column) const;
    quint64 address(int typeId) const;

    void initialize();
    void finalize(PerfProfilerStatisticsData *data);
    void clear(PerfProfilerStatisticsData *data);

    int typeId(int row) const
    {
        return m_data[m_forwardIndex[row]].typeId;
    }

    int rowForTypeId(int typeId) const;

    struct Data : public Frame {
        explicit Data(int typeId = -1) : Frame(typeId), samples(0), self(0) {}
        uint samples;
        uint self;
    };

private:
    QVector<Data> m_data;
    QVector<int> m_forwardIndex;
    QVector<int> m_backwardIndex;

    PerfProfilerStatisticsRelativesModel *m_children;
    PerfProfilerStatisticsRelativesModel *m_parents;

    qint64 m_startTime;
    qint64 m_endTime;
    uint m_totalSamples;

    QScopedPointer<PerfProfilerStatisticsData> m_offlineData;
};

class PerfProfilerStatisticsRelativesModel : public PerfProfilerStatisticsModel {
    Q_OBJECT
public:
    struct Data {
        uint totalOccurrences;
        QVector<Frame> data;
    };

    PerfProfilerStatisticsRelativesModel(Relation relation,
                                         PerfProfilerStatisticsMainModel *parent);
    int rowCount(const QModelIndex &parent) const final;
    QVariant data(const QModelIndex &index, int role) const final;
    void sort(int column, Qt::SortOrder order) final;
    void sortForInsert();

    void selectByTypeId(int typeId);

    void finalize(PerfProfilerStatisticsData *data);
    void clear();

    const PerfProfilerStatisticsMainModel *mainModel() const;

    int typeId(int row) const
    {
        return m_data.value(m_currentRelative).data.at(row).typeId;
    }

private:
    friend class PerfProfilerStatisticsMainModel;
    const Relation m_relation;

    QHash<int, Data> m_data;
    int m_currentRelative;
};

} // namespace Internal
} // namespace PerfProfiler
