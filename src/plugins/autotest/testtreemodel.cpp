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

#include "testtreemodel.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testprojectsettings.h"
#include "testsettings.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Autotest::Internal;

namespace Autotest {

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.frameworkmanager", QtWarningMsg)

static TestTreeModel *s_instance = nullptr;

TestTreeModel::TestTreeModel(TestCodeParser *parser) :
    m_parser(parser)
{
    s_instance = this;

    connect(m_parser, &TestCodeParser::aboutToPerformFullParse, this,
            &TestTreeModel::removeAllTestItems, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testParseResultReady,
            this, &TestTreeModel::onParseResultReady, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::parsingFinished,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::parsingFailed,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::requestRemoveAll,
            this, &TestTreeModel::markAllForRemoval);
    connect(m_parser, &TestCodeParser::requestRemoval,
            this, &TestTreeModel::markForRemoval);
    connect(this, &QAbstractItemModel::dataChanged,
            this, &TestTreeModel::onDataChanged);
    setupParsingConnections();
}

TestTreeModel *TestTreeModel::instance()
{
    return s_instance;
}

TestTreeModel::~TestTreeModel()
{
    s_instance = nullptr;
}

void TestTreeModel::setupParsingConnections()
{
    static bool connectionsInitialized = false;
    if (connectionsInitialized)
        return;
    m_parser->setDirty();
    m_parser->setState(TestCodeParser::Idle);

    SessionManager *sm = SessionManager::instance();
    connect(sm, &SessionManager::startupProjectChanged, [this](Project *project) {
        synchronizeTestFrameworks(); // we might have project settings
        m_parser->onStartupProjectChanged(project);
        m_checkStateCache = project ? AutotestPlugin::projectSettings(project)->checkStateCache()
                                    : nullptr;
        m_failedStateCache.clear();
    });

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
    connectionsInitialized = true;
}

bool TestTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    ITestTreeItem *item = static_cast<ITestTreeItem *>(index.internalPointer());
    if (item && item->setData(index.column(), value, role)) {
        emit dataChanged(index, index, {role});
        if (role == Qt::CheckStateRole) {
            Qt::CheckState checked = item->checked();
            if (item->hasChildren() && checked != Qt::PartiallyChecked) {
                // handle the new checkstate for children as well...
                for (Utils::TreeItem *child : *item) {
                    const QModelIndex &idx = indexForItem(child);
                    setData(idx, checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
                }
            }
            if (item->parent() != rootItem()) {
                auto parent = static_cast<ITestTreeItem *>(item->parent());
                if (parent->checked() != checked)
                    revalidateCheckState(parent); // handle parent too
            }
            return true;
        } else if (role == FailedRole) { // FIXME limit to Frameworks
            m_failedStateCache.insert(static_cast<TestTreeItem *>(item), true);
        }
    }
    return false;
}

Qt::ItemFlags TestTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    ITestTreeItem *item = static_cast<ITestTreeItem *>(itemForIndex(index));
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
        result.append(static_cast<ITestTreeItem *>(frameworkRoot)->getAllTestConfigurations());
    return result;
}

QList<TestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<TestConfiguration *> result;
    for (Utils::TreeItem *frameworkRoot : *rootItem())
        result.append(static_cast<ITestTreeItem *>(frameworkRoot)->getSelectedTestConfigurations());
    return result;
}

QList<TestConfiguration *> TestTreeModel::getFailedTests() const
{
    QList<TestConfiguration *> result;
    for (Utils::TreeItem *frameworkRoot : *rootItem())
        result.append(static_cast<ITestTreeItem *>(frameworkRoot)->getFailedTestConfigurations());
    return result;
}

QList<TestConfiguration *> TestTreeModel::getTestsForFile(const Utils::FilePath &fileName) const
{
    QList<TestConfiguration *> result;
    for (Utils::TreeItem *frameworkRoot : *rootItem())
        result.append(static_cast<TestTreeItem *>(frameworkRoot)->getTestConfigurationsForFile(fileName));
    return result;
}

QList<TestTreeItem *> TestTreeModel::testItemsByName(TestTreeItem *root, const QString &testName)
{
    QList<TestTreeItem *> result;

    root->forFirstLevelChildItems([&testName, &result, this](TestTreeItem *node) {
        if (node->type() == TestTreeItem::TestSuite || node->type() == TestTreeItem::TestCase) {
            if (node->name() == testName) {
                result << node;
                return; // prioritize test suites and cases over test functions
            }
            TestTreeItem *testCase = node->findFirstLevelChildItem([&testName](TestTreeItem *it) {
                QTC_ASSERT(it, return false);
                return (it->type() == TestTreeItem::TestCase
                        || it->type() == TestTreeItem::TestFunction) && it->name() == testName;
            }); // collect only actual tests, not special functions like init, cleanup etc.
            if (testCase)
                result << testCase;
        } else {
            result << testItemsByName(node, testName);
        }
    });
    return result;
}

QList<TestTreeItem *> TestTreeModel::testItemsByName(const QString &testName)
{
    QList<TestTreeItem *> result;
    // FIXME limit to frameworks
    for (Utils::TreeItem *frameworkRoot : *rootItem())
        result << testItemsByName(static_cast<TestTreeItem *>(frameworkRoot), testName);

    return result;
}

void TestTreeModel::synchronizeTestFrameworks()
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    TestFrameworks sorted;
    const QVariant useGlobal = project ? project->namedSettings(Constants::SK_USE_GLOBAL)
                                       : QVariant();
    if (!useGlobal.isValid() || AutotestPlugin::projectSettings(project)->useGlobalSettings()) {
        sorted = Utils::filtered(TestFrameworkManager::registeredFrameworks(),
                                 &ITestFramework::active);
        qCDebug(LOG) << "Active frameworks sorted by priority" << sorted;
    } else { // we've got custom project settings
        const TestProjectSettings *settings = AutotestPlugin::projectSettings(project);
        const QMap<ITestFramework *, bool> active = settings->activeFrameworks();
        sorted = Utils::filtered(active.keys(), [active](ITestFramework *framework) {
            return active.value(framework);
        });
        Utils::sort(sorted, &ITestFramework::priority);
    }

    // pre-check to avoid further processing when frameworks are unchanged
    Utils::TreeItem *invisibleRoot = rootItem();
    QSet<ITestFramework *> newlyAdded;
    QList<Utils::TreeItem *> oldFrameworkRoots;
    for (Utils::TreeItem *oldFrameworkRoot : *invisibleRoot)
        oldFrameworkRoots.append(oldFrameworkRoot);

    for (Utils::TreeItem *oldFrameworkRoot : oldFrameworkRoots)
        takeItem(oldFrameworkRoot);  // do NOT delete the ptr is still held by TestFrameworkManager

    for (ITestFramework *framework : sorted) {
        TestTreeItem *frameworkRootNode = framework->rootNode();
        invisibleRoot->appendChild(frameworkRootNode);
        if (!oldFrameworkRoots.removeOne(frameworkRootNode))
            newlyAdded.insert(framework);
    }
    for (Utils::TreeItem *oldFrameworkRoot : oldFrameworkRoots)
        oldFrameworkRoot->removeChildren();

    m_parser->syncTestFrameworks(sorted);
    if (!newlyAdded.isEmpty())
        m_parser->updateTestTree(newlyAdded);
    emit updatedActiveFrameworks(sorted.size());
}

void TestTreeModel::filterAndInsert(TestTreeItem *item, TestTreeItem *root, bool groupingEnabled)
{
    TestTreeItem *filtered = item->applyFilters();
    if (item->shouldBeAddedAfterFiltering())
        insertItemInParent(item, root, groupingEnabled);
    else // might be that all children have been filtered out
        delete item;
    if (filtered)
        insertItemInParent(filtered, root, groupingEnabled);
}

void TestTreeModel::rebuild(const QList<Utils::Id> &frameworkIds)
{
    for (const Utils::Id &id : frameworkIds) {
        ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
        TestTreeItem *frameworkRoot = framework->rootNode();
        const bool groupingEnabled = framework->grouping();
        for (int row = frameworkRoot->childCount() - 1; row >= 0; --row) {
            auto testItem = frameworkRoot->childItem(row);
            if (testItem->type() == TestTreeItem::GroupNode) {
                // process children of group node and delete it afterwards if necessary
                for (int childRow = testItem->childCount() - 1; childRow >= 0; --childRow) {
                    // FIXME should this be done recursively until we have a non-GroupNode?
                    TestTreeItem *childTestItem = testItem->childItem(childRow);
                    takeItem(childTestItem);
                    filterAndInsert(childTestItem, frameworkRoot, groupingEnabled);
                }
                if (!groupingEnabled || testItem->childCount() == 0)
                    delete takeItem(testItem);
            } else {
                takeItem(testItem);
                filterAndInsert(testItem, frameworkRoot, groupingEnabled);
            }
        }
        revalidateCheckState(frameworkRoot);
    }
}

void TestTreeModel::updateCheckStateCache()
{
    m_checkStateCache->evolve();

    for (Utils::TreeItem *rootNode : *rootItem()) {
        // FIXME limit to framework items
        rootNode->forAllChildren([this](Utils::TreeItem *child) {
            auto childItem = static_cast<ITestTreeItem *>(child);
            m_checkStateCache->insert(childItem, childItem->checked());
        });
    }
}

bool TestTreeModel::hasFailedTests() const
{
    auto failedItem = rootItem()->findAnyChild([](Utils::TreeItem *it) {
        return it->data(0, FailedRole).toBool();
    });
    return failedItem != nullptr;
}

void TestTreeModel::clearFailedMarks()
{
    for (Utils::TreeItem *rootNode : *rootItem()) {
        rootNode->forAllChildren([](Utils::TreeItem *child) {
            child->setData(0, false, FailedRole);
        });
    }
    m_failedStateCache.clear();
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
        revalidateCheckState(root);
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
            revalidateCheckState(item);
            hasChanged = true;
        } else if (child->hasChildren()) {
            hasChanged |= sweepChildren(child);
            if (!child->hasChildren() && child->removeOnSweepIfEmpty()) {
                destroyItem(child);
                revalidateCheckState(item);
            }
        } else {
            hasChanged |= child->newlyAdded();
        }
    }
    return hasChanged;
}

static TestTreeItem *fullCopyOf(TestTreeItem *other)
{
    QTC_ASSERT(other, return nullptr);
    TestTreeItem *result = other->copyWithoutChildren();
    for (int row = 0, count = other->childCount(); row < count; ++row)
        result->appendChild(fullCopyOf(other->childItem(row)));
    return result;
}

static void applyParentCheckState(ITestTreeItem *parent, ITestTreeItem *newItem)
{
    QTC_ASSERT(parent && newItem, return);

    if (parent->checked() != newItem->checked()) {
        const Qt::CheckState checkState = parent->checked() == Qt::Unchecked ? Qt::Unchecked
                                                                             : Qt::Checked;
        newItem->setData(0, checkState, Qt::CheckStateRole);
        newItem->forAllChildren([checkState](Utils::TreeItem *it) {
            it->setData(0, checkState, Qt::CheckStateRole);
        });
    }
}

void TestTreeModel::insertItemInParent(TestTreeItem *item, TestTreeItem *root, bool groupingEnabled)
{
    TestTreeItem *parentNode = root;
    if (groupingEnabled && item->isGroupable()) {
        parentNode = root->findFirstLevelChildItem([item] (const TestTreeItem *it) {
            return it->isGroupNodeFor(item);
        });
        if (!parentNode) {
            parentNode = item->createParentGroupNode();
            if (!QTC_GUARD(parentNode)) // we might not get a group node at all
                parentNode = root;
            else
                root->appendChild(parentNode);
        }
    }
    // check if a similar item is already present (can happen for rebuild())
    if (auto otherItem = parentNode->findChild(item)) {
        // only handle item's children and add them to the already present one
        for (int row = 0, count = item->childCount(); row < count; ++row) {
            TestTreeItem *child = fullCopyOf(item->childItem(row));
            // use check state of the original
            child->setData(0, item->childAt(row)->checked(), Qt::CheckStateRole);
            otherItem->appendChild(child);
            revalidateCheckState(child);
        }
        delete item;
    } else {
        // restore former check state if available
        Utils::optional<Qt::CheckState> cached = m_checkStateCache->get(item);
        if (cached.has_value())
            item->setData(0, cached.value(), Qt::CheckStateRole);
        else
            applyParentCheckState(parentNode, item);
        // ..and the failed state if available
        Utils::optional<bool> failed = m_failedStateCache.get(item);
        if (failed.has_value())
            item->setData(0, *failed, FailedRole);
        parentNode->appendChild(item);
        revalidateCheckState(parentNode);
    }
}

static Qt::CheckState computeCheckStateByChildren(ITestTreeItem *item)
{
    Qt::CheckState newState = Qt::Checked;
    bool foundChecked = false;
    bool foundUnchecked = false;
    bool foundPartiallyChecked = false;

    item->forFirstLevelChildren([&](ITestTreeItem *child) {
        if (foundPartiallyChecked || (foundChecked && foundUnchecked)) {
            newState = Qt::PartiallyChecked;
            return;
        }
        switch (child->type()) {
        case TestTreeItem::TestDataFunction:
        case TestTreeItem::TestSpecialFunction:
            return;
        default:
            break;
        }

        foundChecked |= (child->checked() == Qt::Checked);
        foundUnchecked |= (child->checked() == Qt::Unchecked);
        foundPartiallyChecked |= (child->checked() == Qt::PartiallyChecked);
    });

    if (newState != Qt::PartiallyChecked)
        newState = foundUnchecked ? Qt::Unchecked : Qt::Checked;
    return newState;
}

void TestTreeModel::revalidateCheckState(ITestTreeItem *item)
{
    QTC_ASSERT(item, return);

    const ITestTreeItem::Type type = item->type();
    if (type == ITestTreeItem::TestSpecialFunction || type == ITestTreeItem::TestDataFunction
            || type == ITestTreeItem::TestDataTag) {
        return;
    }
    const Qt::CheckState oldState = Qt::CheckState(item->data(0, Qt::CheckStateRole).toInt());
    Qt::CheckState newState = computeCheckStateByChildren(item);
    if (oldState != newState) {
        item->setData(0, newState, Qt::CheckStateRole);
        emit dataChanged(item->index(), item->index(), {Qt::CheckStateRole});
        if (item->parent() != rootItem()) {
            auto parent = static_cast<ITestTreeItem *>(item->parent());
            if (parent->checked() != newState)
                revalidateCheckState(parent);
        }
    }
}

void TestTreeModel::onParseResultReady(const TestParseResultPtr result)
{
    TestTreeItem *rootNode = result->base->rootNode();
    QTC_ASSERT(rootNode, return);
    handleParseResult(result.data(), rootNode);
}

void Autotest::TestTreeModel::onDataChanged(const QModelIndex &topLeft,
                                            const QModelIndex &bottomRight,
                                            const QVector<int> &roles)
{
    const QModelIndex parent = topLeft.parent();
    QTC_ASSERT(parent == bottomRight.parent(), return);
    if (!roles.isEmpty() && !roles.contains(Qt::CheckStateRole))
        return;

    for (int row = topLeft.row(), endRow = bottomRight.row(); row <= endRow; ++row) {
        if (auto item = static_cast<ITestTreeItem *>(itemForIndex(index(row, 0, parent))))
            m_checkStateCache->insert(item, item->checked());
    }
}

void TestTreeModel::handleParseResult(const TestParseResult *result, TestTreeItem *parentNode)
{
    const bool groupingEnabled = static_cast<ITestFramework *>(result->base)->grouping();
    // lookup existing items
    if (TestTreeItem *toBeModified = parentNode->find(result)) {
        // found existing item... Do not remove
        toBeModified->markForRemoval(false);
        // if it's a reparse we need to mark the group node as well to avoid purging it in sweep()
        if (groupingEnabled) {
            if (auto directParent = toBeModified->parentItem()) {
                if (directParent->type() == TestTreeItem::GroupNode)
                    directParent->markForRemoval(false);
            }
        }
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

    // restore former check state and fail state if available
    newItem->forAllChildItems([this](TestTreeItem *childItem) {
        Utils::optional<Qt::CheckState> cached = m_checkStateCache->get(childItem);
        if (cached.has_value())
            childItem->setData(0, cached.value(), Qt::CheckStateRole);
        Utils::optional<bool> failed = m_failedStateCache.get(childItem);
        if (failed.has_value())
            childItem->setData(0, *failed, FailedRole);
    });
    // it might be necessary to "split" created item
    filterAndInsert(newItem, parentNode, groupingEnabled);
}

void TestTreeModel::removeAllTestItems()
{
    for (Utils::TreeItem *item : *rootItem()) {
        item->removeChildren();
        TestTreeItem *testTreeItem = static_cast<TestTreeItem *>(item);
        if (testTreeItem->checked() == Qt::PartiallyChecked)
            testTreeItem->setData(0, Qt::Checked, Qt::CheckStateRole);
    }
    emit testTreeModelChanged();
}

#ifdef WITH_TESTS
// we're inside tests - so use some internal knowledge to make testing easier
static TestTreeItem *qtRootNode()
{
    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix("QtTest");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

static TestTreeItem *quickRootNode()
{
    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix("QtQuickTest");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

static TestTreeItem *gtestRootNode()
{
    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix("GTest");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

static TestTreeItem *boostTestRootNode()
{
    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix("Boost");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

int TestTreeModel::autoTestsCount() const
{
    TestTreeItem *rootNode = qtRootNode();
    return rootNode ? rootNode->childCount() : 0;
}

bool TestTreeModel::hasUnnamedQuickTests(const TestTreeItem *rootNode) const
{
    for (int row = 0, count = rootNode->childCount(); row < count; ++row) {
        if (rootNode->childAt(row)->name().isEmpty())
            return true;
    }
    return false;
}

TestTreeItem *TestTreeModel::unnamedQuickTests() const
{
    TestTreeItem *rootNode = quickRootNode();
    if (!rootNode)
        return nullptr;

    return rootNode->findFirstLevelChildItem([](TestTreeItem *it) { return it->name().isEmpty(); });
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
    rootNode->forFirstLevelChildren([&dataTagCount](ITestTreeItem *classItem) {
        classItem->forFirstLevelChildren([&dataTagCount](ITestTreeItem *functionItem) {
            dataTagCount += functionItem->childCount();
        });
    });
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
        rootNode->forFirstLevelChildren([&result](ITestTreeItem *child) {
            result.insert(child->name(), child->childCount());
        });
    }
    return result;
}

int TestTreeModel::boostTestNamesCount() const
{
    TestTreeItem *rootNode = boostTestRootNode();
    return rootNode ? rootNode->childCount() : 0;
}

QMap<QString, int> TestTreeModel::boostTestSuitesAndTests() const
{
    QMap<QString, int> result;

    if (TestTreeItem *rootNode = boostTestRootNode()) {
        rootNode->forFirstLevelChildItems([&result](TestTreeItem *child) {
            result.insert(child->name() + '|' + child->proFile(), child->childCount());
        });
    }
    return result;
}

#endif

/***************************** Sort/Filter Model **********************************/

TestTreeSortFilterModel::TestTreeSortFilterModel(TestTreeModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
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
    const ITestTreeItem *leftItem = static_cast<ITestTreeItem *>(left.internalPointer());
    if (leftItem->type() == ITestTreeItem::Root)
        return left.row() > right.row();

    const ITestTreeItem *rightItem = static_cast<ITestTreeItem *>(right.internalPointer());
    return leftItem->lessThan(rightItem, m_sortMode);
}

bool TestTreeSortFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index = sourceModel()->index(sourceRow, 0,sourceParent);
    if (!index.isValid())
        return false;

    const ITestTreeItem *item = static_cast<ITestTreeItem *>(index.internalPointer());

    switch (item->type()) {
    case ITestTreeItem::TestDataFunction:
        return m_filterMode & ShowTestData;
    case ITestTreeItem::TestSpecialFunction:
        return m_filterMode & ShowInitAndCleanup;
    default:
        return true;
    }
}

} // namespace Autotest
