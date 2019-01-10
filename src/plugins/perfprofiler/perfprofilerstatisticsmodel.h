/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
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
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    void sort(int column, Qt::SortOrder order);
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
