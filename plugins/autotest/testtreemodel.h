/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#ifndef TESTTREEMODEL_H
#define TESTTREEMODEL_H

#include "testconfiguration.h"

#include <cplusplus/CppDocument.h>

#include <QAbstractItemModel>

namespace {
    enum ItemRole {
//        AnnotationRole = Qt::UserRole + 1,
        LinkRole = Qt::UserRole + 2 // can be removed if AnnotationRole comes back
    };
}

namespace Autotest {
namespace Internal {

class TestCodeParser;
class TestInfo;
class TestTreeItem;

class TestTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    static TestTreeModel* instance();
    ~TestTreeModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    bool hasChildren(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool removeRows(int row, int count, const QModelIndex &parent);

    TestCodeParser *parser() const { return m_parser; }
    bool hasTests() const;
    QList<TestConfiguration *> getAllTestCases() const;
    QList<TestConfiguration *> getSelectedTests() const;
    TestTreeItem *unnamedQuickTests() const;

    void modifyAutoTestSubtree(int row, TestTreeItem *newItem);
    void removeAutoTestSubtreeByFilePath(const QString &file);
    void addAutoTest(TestTreeItem *newItem);
    void removeAllAutoTests();

    void modifyQuickTestSubtree(int row, TestTreeItem *newItem);
    void removeQuickTestSubtreeByFilePath(const QString &file);
    void addQuickTest(TestTreeItem *newItem);
    void removeAllQuickTests();
    void removeUnnamedQuickTest(const QString &filePath);

signals:
    void testTreeModelChanged();

public slots:

private:
    explicit TestTreeModel(QObject *parent = 0);
    void modifyTestSubtree(QModelIndex &toBeModifiedIndex, TestTreeItem *newItem);

    TestTreeItem *m_rootItem;
    TestTreeItem *m_autoTestRootItem;
    TestTreeItem *m_quickTestRootItem;
    TestCodeParser *m_parser;

};

} // namespace Internal
} // namespace Autotest

#endif // TESTTREEMODEL_H
