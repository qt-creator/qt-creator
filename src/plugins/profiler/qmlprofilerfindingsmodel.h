// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerfindings.h"

#include <utils/store.h>

#include <QAbstractItemModel>

#include <array>

namespace Profiler::Internal {

class QmlProfilerModelManager;

class QmlProfilerFindingsModel final : public QAbstractItemModel
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

    // Rates with defaultFindingRules(), and again whenever the settings change.
    explicit QmlProfilerFindingsModel(QmlProfilerModelManager *modelManager);
    QmlProfilerFindingsModel(QmlProfilerModelManager *modelManager, FindingRules rules);
    ~QmlProfilerFindingsModel() override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    const QList<Finding> &findings() const { return m_findings; }

    void clear();
    // Replaces the rules and rates the loaded trace with them.
    void setRules(FindingRules rules);
    // Rates the loaded trace against another target frame rate, from what the rules collected.
    void setTargetFrameRate(int framesPerSecond);

private:
    // Returns the features of all rules.
    quint64 sortRulesByFeature();
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    void finalize();
    void mergeFindings();

    QmlProfilerModelManager *m_modelManager = nullptr;
    FindingRules m_rules;
    std::array<QList<FindingRule *>, QmlDebug::MaximumProfileFeature> m_rulesByFeature;
    QList<Finding> m_findings;
    // What each rule found when it was last finalized, in the order of m_rules.
    QList<QList<Finding>> m_findingsByRule;
    // The settings the rules were made with, apart from the target frame rate.
    Utils::Store m_thresholds;
    bool m_loading = false;
    bool m_rulesStale = false;
};

// Only findings that carry a line can be shown in an editor. A failed image load is located
// by its URL and has none; the trace reports line 0 for those, and for Compiling ranges, so
// anything below 1 must not be turned into a jump.
bool findingIsInSource(const QModelIndex &index);

} // namespace Profiler::Internal
