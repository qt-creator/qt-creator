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

#include "testresult.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QFont>
#include <QSet>

#include <utils/optional.h>
#include <utils/treemodel.h>

namespace Autotest {
namespace Internal {

class TestResultItem : public Utils::TypedTreeItem<TestResultItem, TestResultItem>
{
public:
    explicit TestResultItem(const TestResultPtr &testResult);
    QVariant data(int column, int role) const override;
    const TestResult *testResult() const { return m_testResult.data(); }
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
                      const Utils::optional<SummaryEvaluation> &summary);

    TestResultItem *intermediateFor(const TestResultItem *item) const;
    TestResultItem *createAndAddIntermediateFor(const TestResultItem *child);
    QString resultString() const;
    Utils::optional<SummaryEvaluation> summaryResult() const { return m_summaryResult; }

private:
    TestResultPtr m_testResult;
    Utils::optional<SummaryEvaluation> m_summaryResult;
};

class TestResultModel : public Utils::TreeModel<TestResultItem>
{
public:
    explicit TestResultModel(QObject *parent = nullptr);

    void addTestResult(const TestResultPtr &testResult, bool autoExpand = false);
    void removeCurrentTestMessage();
    void clearTestResults();

    const TestResult *testResult(const QModelIndex &idx);

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
    const TestResult *testResult(const QModelIndex &index) const;
    TestResultItem *itemForIndex(const QModelIndex &index) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool acceptTestCaseResult(const QModelIndex &srcIndex) const;
    TestResultModel *m_sourceModel;
    QSet<ResultType> m_enabled;
};

} // namespace Internal
} // namespace Autotest
