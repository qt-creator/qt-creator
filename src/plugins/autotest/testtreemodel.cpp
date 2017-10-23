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

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testsettings.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/texteditor.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

TestTreeModel::TestTreeModel(QObject *parent) :
    TreeModel<>(parent),
    m_parser(new TestCodeParser(this))
{
    connect(m_parser, &TestCodeParser::aboutToPerformFullParse, this,
            &TestTreeModel::removeAllTestItems, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testParseResultReady,
            this, &TestTreeModel::onParseResultReady, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::parsingFinished,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::parsingFailed,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
    setupParsingConnections();
}

static TestTreeModel *s_instance = nullptr;

TestTreeModel *TestTreeModel::instance()
{
    if (!s_instance)
        s_instance = new TestTreeModel;
    return s_instance;
}

TestTreeModel::~TestTreeModel()
{
    removeTestRootNodes();
    s_instance = nullptr;
}

void TestTreeModel::setupParsingConnections()
{
    if (!m_connectionsInitialized)
        m_parser->setDirty();

    m_parser->setState(TestCodeParser::Idle);
    if (m_connectionsInitialized)
        return;

    ProjectExplorer::SessionManager *sm = ProjectExplorer::SessionManager::instance();
    connect(sm, &ProjectExplorer::SessionManager::startupProjectChanged,
            m_parser, &TestCodeParser::onStartupProjectChanged);

    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    connect(cppMM, &CppTools::CppModelManager::documentUpdated,
            m_parser, &TestCodeParser::onCppDocumentUpdated, Qt::QueuedConnection);
    connect(cppMM, &CppTools::CppModelManager::aboutToRemoveFiles,
            this, &TestTreeModel::removeFiles, Qt::QueuedConnection);
    connect(cppMM, &CppTools::CppModelManager::projectPartsUpdated,
            m_parser, &TestCodeParser::onProjectPartsUpdated);

    QmlJS::ModelManagerInterface *qmlJsMM = QmlJS::ModelManagerInterface::instance();
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::documentUpdated,
            m_parser, &TestCodeParser::onQmlDocumentUpdated, Qt::QueuedConnection);
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::aboutToRemoveFiles,
            this, &TestTreeModel::removeFiles, Qt::QueuedConnection);
    m_connectionsInitialized = true;
}

bool TestTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
    if (item && item->setData(index.column(), value, role)) {
        emit dataChanged(index, index);
        if (role == Qt::CheckStateRole) {
            switch (item->type()) {
            case TestTreeItem::Root:
            case TestTreeItem::TestCase:
                if (item->childCount() > 0)
                    emit dataChanged(index.child(0, 0), index.child(item->childCount() - 1, 0));
                break;
            case TestTreeItem::TestFunctionOrSet:
                emit dataChanged(index.parent(), index.parent());
                break;
            default: // avoid warning regarding unhandled enum member
                break;
            }
        }
        return true;
    }
    return false;
}

Qt::ItemFlags TestTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    TestTreeItem *item = static_cast<TestTreeItem *>(itemForIndex(index));
    return item->flags(index.column());
}

bool TestTreeModel::hasTests() const
{
    for (Utils::TreeItem *frameworkRoot : *rootItem()) {
        if (frameworkRoot->hasChildren())
            return true;
    }
    return false;
}

QList<TestConfiguration *> TestTreeModel::getAllTestCases() const
{
    QList<TestConfiguration *> result;
    for (Utils::TreeItem *frameworkRoot : *rootItem())
        result.append(static_cast<TestTreeItem *>(frameworkRoot)->getAllTestConfigurations());
    return result;
}

QList<TestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<TestConfiguration *> result;
    for (Utils::TreeItem *frameworkRoot : *rootItem())
        result.append(static_cast<TestTreeItem *>(frameworkRoot)->getSelectedTestConfigurations());
    return result;
}

void TestTreeModel::syncTestFrameworks()
{
    // remove all currently registered
    removeTestRootNodes();

    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    QVector<Core::Id> sortedIds = frameworkManager->sortedActiveFrameworkIds();
    for (const Core::Id &id : sortedIds)
        rootItem()->appendChild(frameworkManager->rootNodeForTestFramework(id));

    m_parser->syncTestFrameworks(sortedIds);
    emit updatedActiveFrameworks(sortedIds.size());
}

void TestTreeModel::removeFiles(const QStringList &files)
{
    for (const QString &file : files)
        markForRemoval(file);
    sweep();
}

void TestTreeModel::markAllForRemoval()
{
    for (Utils::TreeItem *frameworkRoot : *rootItem()) {
        for (Utils::TreeItem *item : *frameworkRoot)
            static_cast<TestTreeItem *>(item)->markForRemovalRecursively(true);
    }
}

void TestTreeModel::markForRemoval(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    for (Utils::TreeItem *frameworkRoot : *rootItem()) {
        TestTreeItem *root = static_cast<TestTreeItem *>(frameworkRoot);
        for (int childRow = root->childCount() - 1; childRow >= 0; --childRow) {
            TestTreeItem *child = root->childItem(childRow);
            child->markForRemovalRecursively(filePath);
        }
    }
}

void TestTreeModel::sweep()
{
    for (Utils::TreeItem *frameworkRoot : *rootItem()) {
        TestTreeItem *root = static_cast<TestTreeItem *>(frameworkRoot);
        sweepChildren(root);
    }
    // even if nothing has changed by the sweeping we might had parse which added or modified items
    emit testTreeModelChanged();
#ifdef WITH_TESTS
    if (m_parser->state() == TestCodeParser::Idle && !m_parser->furtherParsingExpected())
        emit sweepingDone();
#endif
}

/**
 * @note after calling this function emit testTreeModelChanged() if it returns true
 */
bool TestTreeModel::sweepChildren(TestTreeItem *item)
{
    bool hasChanged = false;
    for (int row = item->childCount() - 1; row >= 0; --row) {
        TestTreeItem *child = item->childItem(row);

        if (child->type() != TestTreeItem::Root && child->markedForRemoval()) {
            destroyItem(child);
            item->revalidateCheckState();
            hasChanged = true;
        } else if (child->hasChildren()) {
            hasChanged |= sweepChildren(child);
        } else {
            hasChanged |= child->newlyAdded();
        }
    }
    return hasChanged;
}

void TestTreeModel::onParseResultReady(const TestParseResultPtr result)
{
    TestTreeItem *rootNode
            = TestFrameworkManager::instance()->rootNodeForTestFramework(result->frameworkId);
    QTC_ASSERT(rootNode, return);
    handleParseResult(result.data(), rootNode);
}

void TestTreeModel::handleParseResult(const TestParseResult *result, TestTreeItem *parentNode)
{
    // lookup existing items
    if (TestTreeItem *toBeModified = parentNode->find(result)) {
        // found existing item... Do not remove
        toBeModified->markForRemoval(false);
        // modify and when content has changed inform ui
        if (toBeModified->modify(result)) {
            const QModelIndex &idx = indexForItem(toBeModified);
            emit dataChanged(idx, idx);
        }
        // recursively handle children of this item
        for (const TestParseResult *child : result->children)
            handleParseResult(child, toBeModified);
        return;
    }
    // if there's no matching item, add the new one
    TestTreeItem *newItem = result->createTestTreeItem();
    QTC_ASSERT(newItem, return);
    parentNode->appendChild(newItem);
    // new items are checked by default - revalidation of parents might be necessary
    if (parentNode->checked() != Qt::Checked) {
        parentNode->revalidateCheckState();
        const QModelIndex &idx = indexForItem(parentNode);
        emit dataChanged(idx, idx);
    }
}

void TestTreeModel::removeAllTestItems()
{
    for (Utils::TreeItem *item : *rootItem()) {
        item->removeChildren();
        TestTreeItem *testTreeItem = static_cast<TestTreeItem *>(item);
        if (testTreeItem->checked() == Qt::PartiallyChecked)
            testTreeItem->setChecked(Qt::Checked);
    }
    emit testTreeModelChanged();
}

void TestTreeModel::removeTestRootNodes()
{
    const Utils::TreeItem *invisibleRoot = rootItem();
    const int frameworkRootCount = invisibleRoot ? invisibleRoot->childCount() : 0;
    for (int row = frameworkRootCount - 1; row >= 0; --row) {
        Utils::TreeItem *item = invisibleRoot->childAt(row);
        item->removeChildren();
        takeItem(item); // do NOT delete the item as it's still a ptr held by TestFrameworkManager
    }
}

#ifdef WITH_TESTS
// we're inside tests - so use some internal knowledge to make testing easier
TestTreeItem *qtRootNode()
{
    return TestFrameworkManager::instance()->rootNodeForTestFramework(
                Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix("QtTest"));
}

TestTreeItem *quickRootNode()
{
    return TestFrameworkManager::instance()->rootNodeForTestFramework(
                Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix("QtQuickTest"));
}

TestTreeItem *gtestRootNode()
{
    return TestFrameworkManager::instance()->rootNodeForTestFramework(
                Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix("GTest"));
}

int TestTreeModel::autoTestsCount() const
{
    TestTreeItem *rootNode = qtRootNode();
    return rootNode ? rootNode->childCount() : 0;
}

bool TestTreeModel::hasUnnamedQuickTests(const TestTreeItem *rootNode) const
{
    for (int row = 0, count = rootNode->childCount(); row < count; ++row) {
        if (rootNode->childItem(row)->name().isEmpty())
            return true;
    }
    return false;
}

TestTreeItem *TestTreeModel::unnamedQuickTests() const
{
    TestTreeItem *rootNode = quickRootNode();
    if (!rootNode)
        return nullptr;

    for (int row = 0, count = rootNode->childCount(); row < count; ++row) {
        TestTreeItem *child = rootNode->childItem(row);
        if (child->name().isEmpty())
            return child;
    }
    return nullptr;
}

int TestTreeModel::namedQuickTestsCount() const
{
    TestTreeItem *rootNode = quickRootNode();
    return rootNode
            ? rootNode->childCount() - (hasUnnamedQuickTests(rootNode) ? 1 : 0)
            : 0;
}

int TestTreeModel::unnamedQuickTestsCount() const
{
    if (TestTreeItem *unnamed = unnamedQuickTests())
        return unnamed->childCount();
    return 0;
}

int TestTreeModel::dataTagsCount() const
{
    TestTreeItem *rootNode = qtRootNode();
    if (!rootNode)
        return 0;

    int dataTagCount = 0;
    for (Utils::TreeItem *item : *rootNode) {
        TestTreeItem *classItem = static_cast<TestTreeItem *>(item);
        for (Utils::TreeItem *functionItem : *classItem)
            dataTagCount += functionItem->childCount();
   }
    return dataTagCount;
}

int TestTreeModel::gtestNamesCount() const
{
    TestTreeItem *rootNode = gtestRootNode();
    return rootNode ? rootNode->childCount() : 0;
}

QMultiMap<QString, int> TestTreeModel::gtestNamesAndSets() const
{
    QMultiMap<QString, int> result;

    if (TestTreeItem *rootNode = gtestRootNode()) {
        for (int row = 0, count = rootNode->childCount(); row < count; ++row) {
            const TestTreeItem *current = rootNode->childItem(row);
            result.insert(current->name(), current->childCount());
        }
    }
    return result;
}
#endif

/***************************** Sort/Filter Model **********************************/

TestTreeSortFilterModel::TestTreeSortFilterModel(TestTreeModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_sourceModel(sourceModel)
{
    setSourceModel(sourceModel);
}

void TestTreeSortFilterModel::setSortMode(TestTreeItem::SortMode sortMode)
{
    m_sortMode = sortMode;
    invalidate();
}

void TestTreeSortFilterModel::setFilterMode(FilterMode filterMode)
{
    m_filterMode = filterMode;
    invalidateFilter();
}

void TestTreeSortFilterModel::toggleFilter(FilterMode filterMode)
{
    m_filterMode = toFilterMode(m_filterMode ^ filterMode);
    invalidateFilter();
}

TestTreeSortFilterModel::FilterMode TestTreeSortFilterModel::toFilterMode(int f)
{
    switch (f) {
    case TestTreeSortFilterModel::ShowInitAndCleanup:
        return TestTreeSortFilterModel::ShowInitAndCleanup;
    case TestTreeSortFilterModel::ShowTestData:
        return TestTreeSortFilterModel::ShowTestData;
    case TestTreeSortFilterModel::ShowAll:
        return TestTreeSortFilterModel::ShowAll;
    default:
        return TestTreeSortFilterModel::Basic;
    }
}

bool TestTreeSortFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // root items keep the intended order
    const TestTreeItem *leftItem = static_cast<TestTreeItem *>(left.internalPointer());
    if (leftItem->type() == TestTreeItem::Root)
        return left.row() > right.row();

    const TestTreeItem *rightItem = static_cast<TestTreeItem *>(right.internalPointer());
    return leftItem->lessThan(rightItem, m_sortMode);
}

bool TestTreeSortFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = m_sourceModel->index(sourceRow, 0,sourceParent);
    if (!index.isValid())
        return false;

    const TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());

    switch (item->type()) {
    case TestTreeItem::TestDataFunction:
        return m_filterMode & ShowTestData;
    case TestTreeItem::TestSpecialFunction:
        return m_filterMode & ShowInitAndCleanup;
    default:
        return true;
    }
}

} // namespace Internal
} // namespace Autotest
