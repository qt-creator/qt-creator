// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testresult.h"

#include <QAbstractItemModel>
#include <QFont>
#include <QSet>
#include <QSortFilterProxyModel>

#include <utils/treemodel.h>

#include <optional>

namespace Autotest {
namespace Internal {

class TestResultItem : public Utils::TypedTreeItem<TestResultItem, TestResultItem>
{
public:
    explicit TestResultItem(const TestResult &testResult);
    QVariant data(int column, int role) const override;
    TestResult testResult() const { return m_testResult; }
    void updateDescription(const QString &description);

    struct SummaryEvaluation
    {
        bool failed = false;
        bool warnings = false;

        bool operator==(const SummaryEvaluation &other) const
        { return failed == other.failed && warnings == other.warnings; }
        bool operator!=(const SummaryEvaluation &other) const
        { return !(*this == other); }
    };

    void updateResult(bool &changed, ResultType addedChildType,
                      const std::optional<SummaryEvaluation> &summary);

    TestResultItem *intermediateFor(const TestResultItem *item) const;
    TestResultItem *createAndAddIntermediateFor(const TestResultItem *child);
    QString resultString() const;
    std::optional<SummaryEvaluation> summaryResult() const { return m_summaryResult; }

    bool updateDescendantTypes(ResultType t);
    bool descendantTypesContainsAnyOf(const QSet<ResultType> &types) const;

private:
    TestResult m_testResult;
    QSet<ResultType> m_descendantsTypes;
    std::optional<SummaryEvaluation> m_summaryResult;
};

class TestResultModel : public Utils::TreeModel<TestResultItem>
{
public:
    explicit TestResultModel(QObject *parent = nullptr);

    void addTestResult(const TestResult &testResult, bool autoExpand = false);
    void removeCurrentTestMessage();
    void clearTestResults();

    TestResult testResult(const QModelIndex &idx);

    int maxWidthOfFileName(const QFont &font);
    int maxWidthOfLineNumber(const QFont &font);

    int resultTypeCount(ResultType type) const;
    int disabledTests() const { return m_disabled; }
    void raiseDisabledTests(int amount) { m_disabled += amount; }

private:
    void recalculateMaxWidthOfFileName(const QFont &font);
    void addFileName(const QString &fileName);
    TestResultItem *findParentItemFor(const TestResultItem *item,
                                      const TestResultItem *startItem = nullptr) const;
    void updateParent(const TestResultItem *item);
    QHash<QString, QMap<ResultType, int>> m_testResultCount;
    QHash<QString, QHash<ResultType, int>> m_reportedSummary;
    int m_widthOfLineNumber = 0;
    int m_maxWidthOfFileName = 0;
    int m_disabled = 0;
    QSet<QString> m_fileNames;  // TODO: check whether this caching is necessary at all
    QFont m_measurementFont;
};

class TestResultFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TestResultFilterModel(TestResultModel *sourceModel, QObject *parent = nullptr);

    void enableAllResultTypes(bool enabled);
    void toggleTestResultType(ResultType type);
    void clearTestResults();
    bool hasResults();
    TestResult testResult(const QModelIndex &index) const;
    TestResultItem *itemForIndex(const QModelIndex &index) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    TestResultModel *m_sourceModel;
    QSet<ResultType> m_enabled;
};

} // namespace Internal
} // namespace Autotest
