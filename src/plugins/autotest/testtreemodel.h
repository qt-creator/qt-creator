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

#include "testconfiguration.h"
#include "testtreeitem.h"

#include <utils/algorithm.h>
#include <utils/treemodel.h>

#include <QSortFilterProxyModel>

namespace Autotest {
namespace Internal {
class AutotestPluginPrivate;
class TestCodeParser;

template<class T>
class ItemDataCache
{
public:
    void insert(TestTreeItem *item, const T &value) { m_cache[item->cacheName()] = {0, value}; }
    void evolve()
    {
        auto it = m_cache.begin(), end = m_cache.end();
        while (it != end)
            it = it->generation++ >= maxGen ? m_cache.erase(it) : ++it;
    }

    Utils::optional<T> get(TestTreeItem *item)
    {
        auto entry = m_cache.find(item->cacheName());
        if (entry == m_cache.end())
            return Utils::nullopt;
        entry->generation = 0;
        return Utils::make_optional(entry->value);
    };

    void clear() { m_cache.clear(); }

private:
    static constexpr int maxGen = 10;
    struct Entry
    {
        int generation = 0;
        T value;
    };
    QHash<QString, Entry> m_cache;
};

} // namespace Internal

class TestParseResult;
using TestParseResultPtr = QSharedPointer<TestParseResult>;

class AUTOTESTSHARED_EXPORT TestTreeModel : public Utils::TreeModel<>
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
    QList<TestConfiguration *> getAllTestCases() const;
    QList<TestConfiguration *> getSelectedTests() const;
    QList<TestConfiguration *> getTestsForFile(const Utils::FilePath &fileName) const;
    QList<TestTreeItem *> testItemsByName(const QString &testName);
    void synchronizeTestFrameworks();
    void rebuild(const QList<Utils::Id> &frameworkIds);

    void updateCheckStateCache();
#ifdef WITH_TESTS
    int autoTestsCount() const;
    int namedQuickTestsCount() const;
    bool hasUnnamedQuickTests(const TestTreeItem *rootNode) const;
    int unnamedQuickTestsCount() const;
    TestTreeItem *unnamedQuickTests() const;
    int dataTagsCount() const;
    int gtestNamesCount() const;
    QMultiMap<QString, int> gtestNamesAndSets() const;
    int boostTestNamesCount() const;
    QMap<QString, int> boostTestSuitesAndTests() const;
#endif

    void markAllForRemoval();
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
    void handleParseResult(const TestParseResult *result, TestTreeItem *rootNode);
    void removeAllTestItems();
    void removeFiles(const QStringList &files);
    bool sweepChildren(TestTreeItem *item);
    void insertItemInParent(TestTreeItem *item, TestTreeItem *root, bool groupingEnabled);
    void revalidateCheckState(TestTreeItem *item);
    void setupParsingConnections();
    void filterAndInsert(TestTreeItem *item, TestTreeItem *root, bool groupingEnabled);
    QList<TestTreeItem *> testItemsByName(TestTreeItem *root, const QString &testName);

    Internal::TestCodeParser *m_parser = nullptr;
    Internal::ItemDataCache<Qt::CheckState> m_checkStateCache;
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
    void setSortMode(Autotest::TestTreeItem::SortMode sortMode);
    void setFilterMode(FilterMode filterMode);
    void toggleFilter(FilterMode filterMode);
    static FilterMode toFilterMode(int f);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    Autotest::TestTreeItem::SortMode m_sortMode = Autotest::TestTreeItem::Alphabetically;
    FilterMode m_filterMode = Basic;

};

} // namespace Internal
} // namespace Autotest
