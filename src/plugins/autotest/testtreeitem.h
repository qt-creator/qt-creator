// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
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
                           const QString &name = {},
                           const Utils::FilePath &filePath = {},
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
    const Utils::FilePath filePath() const { return m_filePath; }
    void setFilePath(const Utils::FilePath &filePath) { m_filePath = filePath; }
    Type type() const { return m_type; }
    int line() const { return m_line; }
    void setLine(int line) { m_line = line;}
    ITestBase *testBase() const { return m_testBase; }

    virtual bool lessThan(const ITestTreeItem *other, SortMode mode) const;
    QString cacheName() const { return m_filePath.toString() + ':' + m_name; }

protected:
    void setType(Type type) { m_type = type; }
    Qt::CheckState m_checked = Qt::Checked;

private:
    ITestBase *m_testBase = nullptr; // not owned
    QString m_name;
    Utils::FilePath m_filePath;
    Type m_type;
    int m_line = 0;
    bool m_failed = false;
};

class TestTreeItem : public ITestTreeItem
{
public:
    explicit TestTreeItem(ITestFramework *testFramework,
                          const QString &name = {},
                          const Utils::FilePath &filePath = {},
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
    Utils::FilePath proFile() const { return m_proFile; }
    void setProFile(const Utils::FilePath &proFile) { m_proFile = proFile; }
    void markForRemoval(bool mark);
    void markForRemovalRecursively(bool mark);
    virtual void markForRemovalRecursively(const QSet<Utils::FilePath> &filePaths);
    virtual bool removeOnSweepIfEmpty() const { return type() == GroupNode; }
    bool markedForRemoval() const { return m_status == MarkedForRemoval; }
    bool newlyAdded() const { return m_status == NewlyAdded; }
    TestTreeItem *childItem(int at) const;
    TestTreeItem *parentItem() const;

    TestTreeItem *findChildByName(const QString &name);
    TestTreeItem *findChildByFile(const Utils::FilePath &filePath);
    TestTreeItem *findChildByFileAndType(const Utils::FilePath &filePath, Type type);
    TestTreeItem *findChildByNameAndFile(const QString &name, const Utils::FilePath &filePath);

    // search children for a test (function, ...) that matches the given testName
    // e.g. testName = { "Foo", "tst_bar" } will return a (grand)child item where name() matches
    // "tst_bar" and its parent matches "Foo"
    // filePath is used to distinguish this even more - if any of the items (item and parent) has
    // its file path set to filePath the found result is considered valid
    // function is expected to be called on a root node of a test framework
    TestTreeItem *findTestByNameAndFile(const QStringList &testName,
                                        const Utils::FilePath &filePath); // make virtual for other frameworks?

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

    void forAllChildItems(const std::function<void(TestTreeItem *)> &pred) const;
    void forFirstLevelChildItems(const std::function<void(TestTreeItem *)> &pred) const;
    TestTreeItem *findFirstLevelChildItem(const std::function<bool(TestTreeItem *)> &pred) const;
protected:
    void copyBasicDataFrom(const TestTreeItem *other);
    typedef std::function<bool(const TestTreeItem *)> CompareFunction;

private:
    bool modifyFilePath(const Utils::FilePath &filepath);
    bool modifyName(const QString &name);

    enum Status
    {
        NewlyAdded,
        MarkedForRemoval,
        ForcedRootRemoval,      // only valid on rootNode
        Cleared
    };

    int m_column = 0;
    Utils::FilePath m_proFile;
    Status m_status = NewlyAdded;

    friend class TestTreeModel; // grant access to (protected) findChildBy()
};

class TestCodeLocationAndType
{
public:
    QString m_name;
    Utils::FilePath m_filePath;
    int m_line = 0;
    int m_column = 0;
    TestTreeItem::Type m_type = TestTreeItem::Root;
};

typedef QVector<TestCodeLocationAndType> TestCodeLocationList;

} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::TestTreeItem *)
Q_DECLARE_METATYPE(Autotest::TestCodeLocationAndType)
