/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTTREEMODEL_H
#define TESTTREEMODEL_H

#include "testconfiguration.h"

#include <cplusplus/CppDocument.h>

#include <utils/treemodel.h>

#include <QSortFilterProxyModel>

namespace Autotest {
namespace Internal {

struct TestCodeLocationAndType;
class TestCodeParser;
class TestInfo;
class TestTreeItem;

class TestTreeModel : public Utils::TreeModel
{
    Q_OBJECT
public:
    enum Type {
        AutoTest,
        QuickTest
    };

    static TestTreeModel* instance();
    ~TestTreeModel();
    void enableParsing();
    void disableParsing();

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    TestCodeParser *parser() const { return m_parser; }
    bool hasTests() const;
    QList<TestConfiguration *> getAllTestCases() const;
    QList<TestConfiguration *> getSelectedTests() const;
    TestConfiguration *getTestConfiguration(const TestTreeItem *item) const;
    QString getMainFileForUnnamedQuickTest(const QString &qmlFile) const;
    void qmlFilesForMainFile(const QString &mainFile, QSet<QString> *filePaths) const;
    QList<QString> getUnnamedQuickTestFunctions() const;
    bool hasUnnamedQuickTests() const;

#ifdef WITH_TESTS
    int autoTestsCount() const;
    int namedQuickTestsCount() const;
    int unnamedQuickTestsCount() const;
#endif

signals:
    void testTreeModelChanged();

public slots:

private:
    void addTestTreeItem(TestTreeItem *item, Type type);
    void updateUnnamedQuickTest(const QString &mainFile,
                                const QMap<QString, TestCodeLocationAndType> &functions);
    void modifyTestTreeItem(TestTreeItem *item, Type type, const QStringList &file);
    void removeAllTestItems();
    void removeTestTreeItems(const QString &filePath, Type type);
    void removeUnnamedQuickTests(const QString &filePath);

    TestTreeItem *unnamedQuickTests() const;
    TestTreeItem *rootItemForType(Type type);
    QModelIndex rootIndexForType(Type type);

    explicit TestTreeModel(QObject *parent = 0);
    void modifyTestSubtree(QModelIndex &toBeModifiedIndex, const TestTreeItem *newItem);
    void processChildren(QModelIndex &parentIndex, const TestTreeItem *newItem,
                         const int upperBound, const QHash<QString, Qt::CheckState> &checkStates);

    TestTreeItem *m_autoTestRootItem;
    TestTreeItem *m_quickTestRootItem;
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

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestTreeModel::Type)

#endif // TESTTREEMODEL_H
