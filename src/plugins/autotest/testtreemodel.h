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

#ifndef TESTTREEMODEL_H
#define TESTTREEMODEL_H

#include "testconfiguration.h"
#include "testtreeitem.h"

#include <cplusplus/CppDocument.h>

#include <utils/treemodel.h>

#include <QSortFilterProxyModel>

namespace Autotest {
namespace Internal {

class TestCodeParser;
struct TestParseResult;

class TestTreeModel : public Utils::TreeModel
{
    Q_OBJECT
public:
    enum Type {
        Invalid,
        AutoTest,
        QuickTest,
        GoogleTest
    };

    static TestTreeModel* instance();
    ~TestTreeModel();
    void enableParsing();
    void enableParsingFromSettings();
    void disableParsing();
    void disableParsingFromSettings();

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    TestCodeParser *parser() const { return m_parser; }
    bool hasTests() const;
    QList<TestConfiguration *> getAllTestCases() const;
    QList<TestConfiguration *> getSelectedTests() const;
    TestConfiguration *getTestConfiguration(const TestTreeItem *item) const;
    bool hasUnnamedQuickTests() const;

#ifdef WITH_TESTS
    int autoTestsCount() const;
    int namedQuickTestsCount() const;
    int unnamedQuickTestsCount() const;
    int dataTagsCount() const;
    int gtestNamesCount() const;
    QMultiMap<QString, int> gtestNamesAndSets() const;
#endif

    void markAllForRemoval();
    void markForRemoval(const QString &filePath);
    void sweep();
    QHash<QString, QString> testCaseNamesForFiles(QStringList files);

signals:
    void testTreeModelChanged();
#ifdef WITH_TESTS
    void sweepingDone();
#endif

public slots:

private:
    void onParseResultReady(const TestParseResult &result);
    void handleParseResult(const TestParseResult &result);
    void handleUnnamedQuickParseResult(const TestParseResult &result);
    void handleGTestParseResult(const TestParseResult &result);
    void removeAllTestItems();
    void removeFiles(const QStringList &files);
    void markForRemoval(const QString &filePath, Type type);
    bool sweepChildren(TestTreeItem *item);

    TestTreeItem *unnamedQuickTests() const;
    TestTreeItem *rootItemForType(Type type);

    explicit TestTreeModel(QObject *parent = 0);
    void setupParsingConnections();

    AutoTestTreeItem *m_autoTestRootItem;
    QuickTestTreeItem *m_quickTestRootItem;
    GoogleTestTreeItem *m_googleTestRootItem;
    TestCodeParser *m_parser;
    bool m_connectionsInitialized;
    QAtomicInt m_refCounter;
};

class TestTreeSortFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum SortMode {
        Alphabetically,
        Naturally
    };

    enum FilterMode {
        Basic,
        ShowInitAndCleanup = 0x01,
        ShowTestData       = 0x02,
        ShowAll            = ShowInitAndCleanup | ShowTestData
    };

    TestTreeSortFilterModel(TestTreeModel *sourceModel, QObject *parent = 0);
    void setSortMode(SortMode sortMode);
    void setFilterMode(FilterMode filterMode);
    void toggleFilter(FilterMode filterMode);
    static FilterMode toFilterMode(int f);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    TestTreeModel *m_sourceModel;
    SortMode m_sortMode;
    FilterMode m_filterMode;

};

struct TestParseResult
{
    TestParseResult(TestTreeModel::Type t = TestTreeModel::Invalid) : type(t) {}

    TestTreeModel::Type type;
    QString fileName;
    QString proFile;
    QString testCaseName;
    unsigned line = 0;
    unsigned column = 0;
    bool parameterized = false;
    bool typed = false;
    bool disabled = false;
    QMap<QString, TestCodeLocationAndType> functions;
    QMap<QString, TestCodeLocationList> dataTagsOrTestSets;
};

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestTreeModel::Type)
Q_DECLARE_METATYPE(Autotest::Internal::TestParseResult)

#endif // TESTTREEMODEL_H
