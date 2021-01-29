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

#include <utils/treemodel.h>

#include <QList>
#include <QMetaType>
#include <QSet>
#include <QString>

namespace {
    enum ItemRole {
        LinkRole = Qt::UserRole + 2, // can be removed if AnnotationRole comes back
        ItalicRole, // used only inside the delegate
        TypeRole,
        EnabledRole,
        FailedRole  // marker for having failed in last run
    };
}

namespace CppTools { class CppModelManager; }
namespace Utils { class FilePath; }

namespace Autotest {

class ITestBase;
class ITestFramework;
class ITestConfiguration;
class TestParseResult;
enum class TestRunMode;

class ITestTreeItem : public Utils::TypedTreeItem<ITestTreeItem>
{
public:

    enum Type
    {
        Root,
        GroupNode,
        TestSuite,
        TestCase,
        TestFunction,
        TestDataTag,
        TestDataFunction,
        TestSpecialFunction
    };

    enum SortMode {
        Alphabetically,
        Naturally
    };

    explicit ITestTreeItem(ITestBase *testBase,
                           const QString &name = QString(),
                           const QString &filePath = QString(),
                           Type type = Root);

    virtual QVariant data(int column, int role) const override;
    virtual bool setData(int column, const QVariant &data, int role) override;
    virtual Qt::ItemFlags flags(int column) const override;

    virtual Qt::CheckState checked() const;
    virtual bool canProvideTestConfiguration() const { return false; }
    virtual ITestConfiguration *testConfiguration() const { return nullptr; }
    virtual ITestConfiguration *asConfiguration(TestRunMode mode) const;

    virtual QList<ITestConfiguration *> getAllTestConfigurations() const { return {}; }
    virtual QList<ITestConfiguration *> getSelectedTestConfigurations() const { return {}; };
    virtual QList<ITestConfiguration *> getFailedTestConfigurations() const { return {}; }

    const QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    const QString filePath() const { return m_filePath; }
    void setFilePath(const QString &filePath) { m_filePath = filePath; }
    Type type() const { return m_type; }
    int line() const { return m_line; }
    void setLine(int line) { m_line = line;}
    ITestBase *testBase() const { return m_testBase; }

    virtual bool lessThan(const ITestTreeItem *other, SortMode mode) const;
    QString cacheName() const { return m_filePath + ':' + m_name; }

protected:
    void setType(Type type) { m_type = type; }
    Qt::CheckState m_checked = Qt::Checked;

private:
    ITestBase *m_testBase = nullptr; // not owned
    QString m_name;
    QString m_filePath;
    Type m_type;
    int m_line = 0;
    bool m_failed = false;
};

class TestTreeItem : public ITestTreeItem
{
public:
    explicit TestTreeItem(ITestFramework *testFramework,
                          const QString &name = QString(),
                          const QString &filePath = QString(),
                          Type type = Root);

    virtual TestTreeItem *copyWithoutChildren() = 0;
    virtual QVariant data(int column, int role) const override;
    bool modifyTestCaseOrSuiteContent(const TestParseResult *result);
    bool modifyTestFunctionContent(const TestParseResult *result);
    bool modifyDataTagContent(const TestParseResult *result);
    bool modifyLineAndColumn(const TestParseResult *result);

    ITestFramework *framework() const;
    void setColumn(int column) { m_column = column; }
    int column() const { return m_column; }
    QString proFile() const { return m_proFile; }
    void setProFile(const QString &proFile) { m_proFile = proFile; }
    void markForRemoval(bool mark);
    void markForRemovalRecursively(bool mark);
    virtual void markForRemovalRecursively(const QString &filepath);
    virtual bool removeOnSweepIfEmpty() const { return type() == GroupNode; }
    bool markedForRemoval() const { return m_status == MarkedForRemoval; }
    bool newlyAdded() const { return m_status == NewlyAdded; }
    TestTreeItem *childItem(int at) const;
    TestTreeItem *parentItem() const;

    TestTreeItem *findChildByName(const QString &name);
    TestTreeItem *findChildByFile(const QString &filePath);
    TestTreeItem *findChildByFileAndType(const QString &filePath, Type type);
    TestTreeItem *findChildByNameAndFile(const QString &name, const QString &filePath);

    virtual ITestConfiguration *debugConfiguration() const { return nullptr; }
    virtual bool canProvideDebugConfiguration() const { return false; }
    ITestConfiguration *asConfiguration(TestRunMode mode) const final;
    virtual QList<ITestConfiguration *> getTestConfigurationsForFile(const Utils::FilePath &fileName) const;
    virtual TestTreeItem *find(const TestParseResult *result) = 0;
    virtual TestTreeItem *findChild(const TestTreeItem *other) = 0;
    virtual bool modify(const TestParseResult *result) = 0;
    virtual bool isGroupNodeFor(const TestTreeItem *other) const;
    virtual bool isGroupable() const;
    virtual TestTreeItem *createParentGroupNode() const = 0;
    // based on (internal) filters this will be used to filter out sub items (and remove them)
    // returns a copy of the item that contains the filtered out children or nullptr
    virtual TestTreeItem *applyFilters() { return nullptr; }
    // decide whether an item should still be added to the treemodel
    virtual bool shouldBeAddedAfterFiltering() const { return true; }
    virtual QSet<QString> internalTargets() const;

    void forAllChildItems(const std::function<void(TestTreeItem *)> &pred) const;
    void forFirstLevelChildItems(const std::function<void(TestTreeItem *)> &pred) const;
    TestTreeItem *findFirstLevelChildItem(const std::function<bool(TestTreeItem *)> &pred) const;
protected:
    void copyBasicDataFrom(const TestTreeItem *other);
    typedef std::function<bool(const TestTreeItem *)> CompareFunction;
    static QSet<QString> dependingInternalTargets(CppTools::CppModelManager *cppMM,
                                                  const QString &file);

private:
    bool modifyFilePath(const QString &filepath);
    bool modifyName(const QString &name);

    enum Status
    {
        NewlyAdded,
        MarkedForRemoval,
        Cleared
    };

    int m_column = 0;
    QString m_proFile;
    Status m_status = NewlyAdded;

    friend class TestTreeModel; // grant access to (protected) findChildBy()
};

class TestCodeLocationAndType
{
public:
    QString m_name;     // tag name for m_type == TestDataTag, file name for other values
    int m_line = 0;
    int m_column = 0;
    TestTreeItem::Type m_type = TestTreeItem::Root;
};

typedef QVector<TestCodeLocationAndType> TestCodeLocationList;

} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::TestTreeItem *)
Q_DECLARE_METATYPE(Autotest::TestCodeLocationAndType)
