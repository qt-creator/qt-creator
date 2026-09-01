// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerfindings.h"

#include <QAbstractTableModel>

namespace Profiler::Internal {

class QmlProfilerModelManager;

class QmlProfilerFindingsModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColumnSeverity,
        ColumnFinding,
        ColumnLocation,
        ColumnCost,
        ColumnOccurrences,
        MaxColumn
    };

    enum Role {
        SortRole = Qt::UserRole + 1,
        TypeIdRole,
        FilenameRole,
        LineRole,
        ColumnRole,
        WhyRole,
        SuggestionRole,
        RuleIdRole
    };

    explicit QmlProfilerFindingsModel(QmlProfilerModelManager *modelManager,
                                      FindingRules rules = defaultFindingRules());
    ~QmlProfilerFindingsModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    const QList<Finding> &findings() const { return m_findings; }

    void clear();

private:
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    void finalize();

    QmlProfilerModelManager *m_modelManager = nullptr;
    FindingRules m_rules;
    QList<Finding> m_findings;
};

// Only findings that carry a line can be shown in an editor. A failed image load is located
// by its URL and has none; the trace reports line 0 for those, and for Compiling ranges, so
// anything below 1 must not be turned into a jump.
bool findingIsInSource(const QModelIndex &index);

} // namespace Profiler::Internal
