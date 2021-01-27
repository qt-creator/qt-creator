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

#include "autotest_global.h"

#include "itemdatacache.h"
#include "testtreeitem.h"

#include <utils/id.h>
#include <utils/treemodel.h>

#include <QSortFilterProxyModel>

namespace ProjectExplorer { class Target; }

namespace Autotest {
namespace Internal {
class AutotestPluginPrivate;
class TestCodeParser;
} // namespace Internal

class TestParseResult;
using TestParseResultPtr = QSharedPointer<TestParseResult>;

class AUTOTESTSHARED_EXPORT TestTreeModel : public Utils::TreeModel<Utils::TreeItem, ITestTreeItem>
{
    Q_OBJECT

    friend class Internal::AutotestPluginPrivate; // For ctor.
    explicit TestTreeModel(Internal::TestCodeParser *parser);

public:
    static TestTreeModel* instance();
    ~TestTreeModel() override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    Internal::TestCodeParser *parser() const { return m_parser; }
    bool hasTests() const;
    QList<ITestConfiguration *> getAllTestCases() const;
    QList<ITestConfiguration *> getSelectedTests() const;
    QList<ITestConfiguration *> getFailedTests() const;
    QList<ITestConfiguration *> getTestsForFile(const Utils::FilePath &fileName) const;
    QList<ITestTreeItem *> testItemsByName(const QString &testName);
    void synchronizeTestFrameworks();
    void synchronizeTestTools();
    void rebuild(const QList<Utils::Id> &frameworkIds);

    void updateCheckStateCache();
    bool hasFailedTests() const;
    void clearFailedMarks();
#ifdef WITH_TESTS
    int autoTestsCount() const;
    int namedQuickTestsCount() const;
    bool hasUnnamedQuickTests(const ITestTreeItem *rootNode) const;
    int unnamedQuickTestsCount() const;
    ITestTreeItem *unnamedQuickTests() const;
    int dataTagsCount() const;
    int gtestNamesCount() const;
    QMultiMap<QString, int> gtestNamesAndSets() const;
    int boostTestNamesCount() const;
    QMap<QString, int> boostTestSuitesAndTests() const;
#endif

    void markAllFrameworkItemsForRemoval();
    void markForRemoval(const QString &filePath);
    void sweep();

signals:
    void testTreeModelChanged();
    void updatedActiveFrameworks(int numberOfActiveFrameworks);
#ifdef WITH_TESTS
    void sweepingDone();
#endif

private:
    void onParseResultReady(const TestParseResultPtr result);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                       const QVector<int> &roles);
    void handleParseResult(const TestParseResult *result, TestTreeItem *rootNode);
    void removeAllTestItems();
    void removeAllTestToolItems();
    void removeFiles(const QStringList &files);
    bool sweepChildren(TestTreeItem *item);
    void insertItemInParent(TestTreeItem *item, TestTreeItem *root, bool groupingEnabled);
    void revalidateCheckState(ITestTreeItem *item);
    void setupParsingConnections();
    void filterAndInsert(TestTreeItem *item, TestTreeItem *root, bool groupingEnabled);
    void onTargetChanged(ProjectExplorer::Target *target);
    void onBuildSystemTestsUpdated();
    const QList<TestTreeItem *> frameworkRootNodes() const;
    const QList<ITestTreeItem *> testToolRootNodes() const;

    Internal::TestCodeParser *m_parser = nullptr;
    Internal::ItemDataCache<Qt::CheckState> *m_checkStateCache = nullptr; // not owned
    Internal::ItemDataCache<bool> m_failedStateCache;
};

namespace Internal {

class TestTreeSortFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum FilterMode {
        Basic,
        ShowInitAndCleanup = 0x01,
        ShowTestData       = 0x02,
        ShowAll            = ShowInitAndCleanup | ShowTestData
    };

    explicit TestTreeSortFilterModel(TestTreeModel *sourceModel, QObject *parent = nullptr);
    void setSortMode(ITestTreeItem::SortMode sortMode);
    void setFilterMode(FilterMode filterMode);
    void toggleFilter(FilterMode filterMode);
    static FilterMode toFilterMode(int f);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const final;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const final;

private:
    Autotest::TestTreeItem::SortMode m_sortMode = Autotest::TestTreeItem::Alphabetically;
    FilterMode m_filterMode = Basic;

};

} // namespace Internal
} // namespace Autotest
