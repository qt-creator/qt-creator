// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testtreeitem.h"

#include "autotestconstants.h"
#include "autotesttr.h"
#include "itestframework.h"
#include "itestparser.h"
#include "testconfiguration.h"

#include <texteditor/texteditor.h>
#include <utils/utilsicons.h>

#include <QIcon>

using namespace Utils;

namespace Autotest {

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[] = {
        QIcon(),
        Icons::OPENFILE.icon(),
        QIcon(":/autotest/images/suite.png"),
        CodeModelIcon::iconForType(CodeModelIcon::Class),
        CodeModelIcon::iconForType(CodeModelIcon::SlotPrivate),
        QIcon(":/autotest/images/data.png")
    };

    if (int(type) >= int(sizeof icons / sizeof *icons))
        return icons[3];
    return icons[type];
}

ITestTreeItem::ITestTreeItem(ITestBase *testBase, const QString &name,
                             const FilePath &filePath, Type type)
    : m_testBase(testBase)
    , m_name(name)
    , m_filePath(filePath)
    , m_type(type)
{}

QVariant ITestTreeItem::data(int /*column*/, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (m_type == Root && childCount() == 0)
            return Tr::tr("%1 (none)").arg(m_name);
        return m_name;
    case Qt::ToolTipRole:
        return m_filePath.toString();
    case Qt::DecorationRole:
        return testTreeIcon(m_type);
    case Qt::CheckStateRole:
        return QVariant();
    case ItalicRole:
        return false;
    case TypeRole:
        return m_type;
    case EnabledRole:
        return true;
    case FailedRole:
        return m_failed;
    }
    return QVariant();
}

bool ITestTreeItem::setData(int /*column*/, const QVariant &data, int role)
{
    if (role == Qt::CheckStateRole) {
        Qt::CheckState old = m_checked;
        m_checked = Qt::CheckState(data.toInt());
        return m_checked != old;
    } else if (role == FailedRole) {
        m_failed = data.toBool();
    }
    return false;
}

Qt::ItemFlags ITestTreeItem::flags(int /*column*/) const
{
    static const Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (type()) {
    case Root:
    case GroupNode:
        return Qt::ItemIsEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    case TestSuite:
    case TestCase:
        return defaultFlags | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    case TestFunction:
        return defaultFlags | Qt::ItemIsUserCheckable;
    default:
        return defaultFlags;
    }
}

Qt::CheckState ITestTreeItem::checked() const
{
    return m_checked;
}

bool ITestTreeItem::lessThan(const ITestTreeItem *other, ITestTreeItem::SortMode mode) const
{
    const QString &lhs = data(0, Qt::DisplayRole).toString();
    const QString &rhs = other->data(0, Qt::DisplayRole).toString();

    switch (mode) {
    case Alphabetically:
        if (lhs == rhs)
            return index().row() > other->index().row();
        return lhs.compare(rhs, Qt::CaseInsensitive) > 0;
    case Naturally: {
        if (type() == GroupNode && other->type() == GroupNode) {
            return filePath().toString().compare(other->filePath().toString(),
                                                 Qt::CaseInsensitive) > 0;
        }

        const Link &leftLink = data(0, LinkRole).value<Link>();
        const Link &rightLink = other->data(0, LinkRole).value<Link>();
        const int comparison = leftLink.targetFilePath.toString().compare(
                    rightLink.targetFilePath.toString(), Qt::CaseInsensitive);
        if (comparison == 0) {
            return leftLink.targetLine == rightLink.targetLine
                    ? leftLink.targetColumn > rightLink.targetColumn
                    : leftLink.targetLine > rightLink.targetLine;
        }
        return comparison > 0;
    }
    }
    return true;
}

ITestConfiguration *ITestTreeItem::asConfiguration(TestRunMode mode) const
{
    switch (mode) {
    case TestRunMode::Run:
    case TestRunMode::RunWithoutDeploy:
        return testConfiguration();
    default:
        return nullptr;
    }
}

/****************************** TestTreeItem ********************************************/

TestTreeItem::TestTreeItem(ITestFramework *testFramework, const QString &name,
                           const FilePath &filePath, Type type)
    : ITestTreeItem(testFramework, name, filePath, type)
{
    switch (type) {
    case Root:
    case GroupNode:
    case TestSuite:
    case TestCase:
    case TestFunction:
        m_checked = Qt::Checked;
        break;
    default:
        m_checked = Qt::Unchecked;
        break;
    }
}

QVariant TestTreeItem::data(int column, int role) const
{
    if (role == LinkRole) {
        if (type() == GroupNode)
            return QVariant();
        QVariant itemLink;
        itemLink.setValue(
            Link(filePath(), line(), int(m_column)));
        return itemLink;
    }
    return ITestTreeItem::data(column, role);
}

bool TestTreeItem::modifyTestCaseOrSuiteContent(const TestParseResult *result)
{
    bool hasBeenModified = modifyName(result->name);
    hasBeenModified |= modifyLineAndColumn(result);
    return hasBeenModified;
}

bool TestTreeItem::modifyTestFunctionContent(const TestParseResult *result)
{
    bool hasBeenModified = modifyFilePath(result->fileName);
    hasBeenModified |= modifyLineAndColumn(result);
    return hasBeenModified;
}

bool TestTreeItem::modifyDataTagContent(const TestParseResult *result)
{

    bool hasBeenModified = modifyTestFunctionContent(result);
    hasBeenModified |= modifyName(result->name);
    return hasBeenModified;
}

bool TestTreeItem::modifyLineAndColumn(const TestParseResult *result)
{
    bool hasBeenModified = false;
    if (line() != result->line) {
        setLine(result->line);
        hasBeenModified = true;
    }
    if (m_column != result->column) {
        m_column = result->column;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

void TestTreeItem::markForRemoval(bool mark)
{
    if (type() == Root)
        m_status = mark ? ForcedRootRemoval : NewlyAdded;
    else
        m_status = mark ? MarkedForRemoval : Cleared;
}

void TestTreeItem::markForRemovalRecursively(bool mark)
{
    if (type() != Root)
        markForRemoval(mark);
    for (int row = 0, count = childCount(); row < count; ++row)
        childItem(row)->markForRemovalRecursively(mark);
}

void TestTreeItem::markForRemovalRecursively(const QSet<FilePath> &filePaths)
{
    bool mark = filePaths.contains(filePath());
    forFirstLevelChildItems([&mark, &filePaths](TestTreeItem *child) {
        child->markForRemovalRecursively(filePaths);
        mark &= child->markedForRemoval();
    });
    if (type() != Root)
        markForRemoval(mark);
}

TestTreeItem *TestTreeItem::childItem(int at) const
{
    return static_cast<TestTreeItem *>(childAt(at));
}

TestTreeItem *TestTreeItem::parentItem() const
{
    return static_cast<TestTreeItem *>(parent());
}

TestTreeItem *TestTreeItem::findChildByName(const QString &name)
{
    return findFirstLevelChildItem([name](const TestTreeItem *other) {
        return other->name() == name;
    });
}

TestTreeItem *TestTreeItem::findChildByFile(const FilePath &filePath)
{
    return findFirstLevelChildItem([filePath](const TestTreeItem *other) {
        return other->filePath() == filePath;
    });
}

TestTreeItem *TestTreeItem::findChildByFileAndType(const FilePath &filePath, Type tType)
{
    return findFirstLevelChildItem([filePath, tType](const TestTreeItem *other) {
        return other->type() == tType && other->filePath() == filePath;
    });}

TestTreeItem *TestTreeItem::findChildByNameAndFile(const QString &name, const FilePath &filePath)
{
    return findFirstLevelChildItem([name, filePath](const TestTreeItem *other) {
        return other->filePath() == filePath && other->name() == name;
    });
}

static TestTreeItem *findMatchingTestAt(TestTreeItem *parent, const QStringList &testName,
                                        const FilePath &filePath)
{
    const QString &first = testName.first();
    const QString &last = testName.last();
    for (int i = 0, iEnd = parent->childCount(); i < iEnd; ++i) {
        auto it = parent->childItem(i);
        if (it->name() != first)
            continue;

        for (int j = 0, jEnd = it->childCount(); j < jEnd; ++j) {
            auto grandchild = it->childItem(j);
            if (grandchild->name() != last)
                continue;

            if (it->filePath() == filePath || grandchild->filePath() == filePath)
                return grandchild;
        }
    }
    return nullptr;
}

TestTreeItem *TestTreeItem::findTestByNameAndFile(const QStringList &testName,
                                                  const FilePath &filePath)
{
    QTC_ASSERT(type() == Root, return nullptr);
    QTC_ASSERT(testName.size() == 2, return nullptr);

    if (!childCount())
        return nullptr;

    if (childAt(0)->type() != GroupNode) // process root's children directly
        return findMatchingTestAt(this, testName, filePath);

    // process children of groups instead of root
    for (int i = 0, end = childCount(); i < end; ++i) {
        if (TestTreeItem *found = findMatchingTestAt(childItem(i), testName, filePath))
            return found;
    }
    return nullptr;
}

ITestConfiguration *TestTreeItem::asConfiguration(TestRunMode mode) const
{
    switch (mode) {
    case TestRunMode::Debug:
    case TestRunMode::DebugWithoutDeploy:
        return debugConfiguration();
    default:
        return ITestTreeItem::asConfiguration(mode);
    }
}

QList<ITestConfiguration *> TestTreeItem::getTestConfigurationsForFile(const FilePath &) const
{
    return {};
}

bool TestTreeItem::isGroupNodeFor(const TestTreeItem *other) const
{
    QTC_ASSERT(other, return false);
    if (type() != TestTreeItem::GroupNode)
        return false;

    // for now there's only the possibility to have 'Folder' nodes
    return other->filePath().absolutePath() == filePath();
}

bool TestTreeItem::isGroupable() const
{
    return true;
}

void TestTreeItem::forAllChildItems(const std::function<void(TestTreeItem *)> &pred) const
{
    for (int row = 0, end = childCount(); row < end; ++row) {
        TestTreeItem *child = childItem(row);
        pred(child);
        child->forAllChildItems(pred);
    }
}

void TestTreeItem::forFirstLevelChildItems(const std::function<void(TestTreeItem *)> &pred) const
{
    for (int row = 0, end = childCount(); row < end; ++row)
        pred(childItem(row));
}

TestTreeItem *TestTreeItem::findFirstLevelChildItem(const std::function<bool(TestTreeItem *)> &pred) const
{
    for (int row = 0, end = childCount(); row < end; ++row) {
        TestTreeItem *child = childItem(row);
        if (pred(child))
            return child;
    }
    return nullptr;
}

void TestTreeItem::copyBasicDataFrom(const TestTreeItem *other)
{
    if (!other)
        return;

    setName(other->name());
    setFilePath(other->filePath());
    setType(other->type());
    setLine(other->line());
    setData(0, other->checked(), Qt::CheckStateRole);
    setData(0, other->data(0, FailedRole), FailedRole);

    m_column = other->m_column;
    m_proFile = other->m_proFile;
    m_status = other->m_status;
}

inline bool TestTreeItem::modifyFilePath(const FilePath &filepath)
{
    if (filePath() != filepath) {
        setFilePath(filepath);
        return true;
    }
    return false;
}

inline bool TestTreeItem::modifyName(const QString &newName)
{
    if (name() != newName) {
        setName(newName);
        return true;
    }
    return false;
}

ITestFramework *TestTreeItem::framework() const
{
    return static_cast<ITestFramework *>(testBase());
}

} // namespace Autotest
