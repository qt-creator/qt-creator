// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testtreemodel.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testprojectsettings.h"

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

using namespace Autotest::Internal;
using namespace ProjectExplorer;
using namespace Utils;

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
            this, &TestTreeModel::onParseResultReady);
    connect(m_parser, &TestCodeParser::parsingFinished,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::parsingFailed,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::requestRemoveAllFrameworkItems,
            this, &TestTreeModel::markAllFrameworkItemsForRemoval);
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

    ProjectManager *sm = ProjectManager::instance();
    connect(sm, &ProjectManager::startupProjectChanged, this, [this, sm](Project *project) {
        synchronizeTestFrameworks(); // we might have project settings
        m_parser->onStartupProjectChanged(project);
        removeAllTestToolItems();
        synchronizeTestTools();
        m_checkStateCache = project ? AutotestPlugin::projectSettings(project)->checkStateCache()
                                    : nullptr;
        onBuildSystemTestsUpdated(); // we may have old results if project was open before switching
        m_failedStateCache.clear();
        if (project) {
            if (sm->startupBuildSystem()) {
                connect(sm->startupBuildSystem(), &BuildSystem::testInformationUpdated,
                        this, &TestTreeModel::onBuildSystemTestsUpdated, Qt::UniqueConnection);
            } else {
                connect(project, &Project::activeTargetChanged,
                        this, &TestTreeModel::onTargetChanged);
            }
        }
    });

    CppEditor::CppModelManager *cppMM = CppEditor::CppModelManager::instance();
    connect(cppMM, &CppEditor::CppModelManager::documentUpdated,
            m_parser, &TestCodeParser::onCppDocumentUpdated, Qt::QueuedConnection);
    connect(cppMM, &CppEditor::CppModelManager::aboutToRemoveFiles,
            this, [this](const QStringList &files) {
                markForRemoval(transform<QSet>(files, &FilePath::fromString));
                sweep();
            }, Qt::QueuedConnection);
    connect(cppMM, &CppEditor::CppModelManager::projectPartsUpdated,
            m_parser, &TestCodeParser::onProjectPartsUpdated);

    QmlJS::ModelManagerInterface *qmlJsMM = QmlJS::ModelManagerInterface::instance();
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::documentUpdated,
            m_parser, &TestCodeParser::onQmlDocumentUpdated, Qt::QueuedConnection);
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::aboutToRemoveFiles,
            this, [this](const FilePaths &filePaths) {
                markForRemoval(Utils::toSet(filePaths));
                sweep();
            }, Qt::QueuedConnection);
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
                for (TreeItem *child : *item) {
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
        } else if (role == FailedRole) {
            if (item->testBase()->type() == ITestBase::Framework)
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
    for (TreeItem *frameworkRoot : *rootItem()) {
        if (frameworkRoot->hasChildren())
            return true;
    }
    return false;
}

QList<ITestConfiguration *> TestTreeModel::getAllTestCases() const
{
    QList<ITestConfiguration *> result;
    forItemsAtLevel<1>([&result](ITestTreeItem *testRoot) {
        result.append(testRoot->getAllTestConfigurations());
    });
    return result;
}

QList<ITestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<ITestConfiguration *> result;
    forItemsAtLevel<1>([&result](ITestTreeItem *testRoot) {
        result.append(testRoot->getSelectedTestConfigurations());
    });
    return result;
}

QList<ITestConfiguration *> TestTreeModel::getFailedTests() const
{
    QList<ITestConfiguration *> result;
    forItemsAtLevel<1>([&result](ITestTreeItem *testRoot) {
        result.append(testRoot->getFailedTestConfigurations());
    });
    return result;
}

QList<ITestConfiguration *> TestTreeModel::getTestsForFile(const FilePath &fileName) const
{
    QList<ITestConfiguration *> result;
    forItemsAtLevel<1>([&result, &fileName](ITestTreeItem *testRoot) {
        if (testRoot->testBase()->type() == ITestBase::Framework)
            result.append(static_cast<TestTreeItem *>(testRoot)->getTestConfigurationsForFile(fileName));
    });
    return result;
}

static QList<ITestTreeItem *> testItemsByName(TestTreeItem *root, const QString &testName)
{
    QList<ITestTreeItem *> result;

    root->forFirstLevelChildItems([&testName, &result](TestTreeItem *node) {
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

void TestTreeModel::onTargetChanged(Target *target)
{
    if (target && target->buildSystem()) {
        const Target *topLevelTarget = ProjectManager::startupProject()->targets().first();
        connect(topLevelTarget->buildSystem(), &BuildSystem::testInformationUpdated,
                this, &TestTreeModel::onBuildSystemTestsUpdated, Qt::UniqueConnection);
        disconnect(target->project(), &Project::activeTargetChanged,
                   this, &TestTreeModel::onTargetChanged);
    }
}

void TestTreeModel::onBuildSystemTestsUpdated()
{
    const BuildSystem *bs = ProjectManager::startupBuildSystem();
    if (!bs || !bs->project())
        return;

    QTC_ASSERT(m_checkStateCache, return);
    m_checkStateCache->evolve(ITestBase::Tool);

    ITestTool *testTool = TestFrameworkManager::testToolForBuildSystemId(bs->project()->id());
    if (!testTool)
        return;
    // FIXME
    const TestProjectSettings *projectSettings = AutotestPlugin::projectSettings(bs->project());
    if ((projectSettings->useGlobalSettings() && !testTool->active())
            || !projectSettings->activeTestTools().contains(testTool)) {
        return;
    }

    ITestTreeItem *rootNode = testTool->rootNode();
    QTC_ASSERT(rootNode, return);
    rootNode->removeChildren();
    for (const auto &tci : bs->testcasesInfo()) {
        ITestTreeItem *item = testTool->createItemFromTestCaseInfo(tci);
        QTC_ASSERT(item, continue);
        if (std::optional<Qt::CheckState> cached = m_checkStateCache->get(item))
            item->setData(0, cached.value(), Qt::CheckStateRole);
        m_checkStateCache->insert(item, item->checked());
        rootNode->appendChild(item);
    }
    revalidateCheckState(rootNode);
    emit testTreeModelChanged();
}

const QList<TestTreeItem *> TestTreeModel::frameworkRootNodes() const
{
    QList<TestTreeItem *> result;
    forItemsAtLevel<1>([&result](ITestTreeItem *rootNode) {
        if (auto framework = rootNode->testBase()->asFramework())
            result.append(framework->rootNode());
    });
    return result;
}

const QList<ITestTreeItem *> TestTreeModel::testToolRootNodes() const
{
    QList<ITestTreeItem *> result;
    forItemsAtLevel<1>([&result](ITestTreeItem *rootNode) {
        if (auto testTool = rootNode->testBase()->asTestTool())
            result.append(testTool->rootNode());
    });
    return result;
}

QList<ITestTreeItem *> TestTreeModel::testItemsByName(const QString &testName)
{
    QList<ITestTreeItem *> result;
    for (TestTreeItem *frameworkRoot : frameworkRootNodes())
        result << Autotest::testItemsByName(frameworkRoot, testName);

    return result;
}

void TestTreeModel::synchronizeTestFrameworks()
{
    const TestFrameworks sorted = AutotestPlugin::activeTestFrameworks();
    qCDebug(LOG) << "Active frameworks sorted by priority" << sorted;
    const auto sortedParsers = Utils::transform(sorted, &ITestFramework::testParser);
    // pre-check to avoid further processing when frameworks are unchanged
    TreeItem *invisibleRoot = rootItem();
    QSet<ITestParser *> newlyAdded;
    QList<ITestTreeItem *> oldFrameworkRoots;
    for (TreeItem *oldFrameworkRoot : *invisibleRoot)
        oldFrameworkRoots.append(static_cast<ITestTreeItem *>(oldFrameworkRoot));

    for (ITestTreeItem *oldFrameworkRoot : oldFrameworkRoots)
        takeItem(oldFrameworkRoot);  // do NOT delete the ptr is still held by TestFrameworkManager

    for (ITestParser *parser : sortedParsers) {
        TestTreeItem *frameworkRootNode = parser->framework()->rootNode();
        invisibleRoot->appendChild(frameworkRootNode);
        if (!oldFrameworkRoots.removeOne(frameworkRootNode))
            newlyAdded.insert(parser);
    }
    for (ITestTreeItem *oldFrameworkRoot : oldFrameworkRoots) {
        if (oldFrameworkRoot->testBase()->type() == ITestBase::Framework)
            oldFrameworkRoot->removeChildren();
        else // re-add the test tools - they are handled separately
            invisibleRoot->appendChild(oldFrameworkRoot);
    }

    m_parser->syncTestFrameworks(sortedParsers);
    if (!newlyAdded.isEmpty())
        m_parser->updateTestTree(newlyAdded);
    emit updatedActiveFrameworks(invisibleRoot->childCount());
}

void TestTreeModel::synchronizeTestTools()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    TestTools tools;
    if (!project || AutotestPlugin::projectSettings(project)->useGlobalSettings()) {
        tools = Utils::filtered(TestFrameworkManager::registeredTestTools(),
                                &ITestFramework::active);
        qCDebug(LOG) << "Active test tools" << tools; // FIXME tools aren't sorted
    } else { // we've got custom project settings
        const TestProjectSettings *settings = AutotestPlugin::projectSettings(project);
        const QHash<ITestTool *, bool> active = settings->activeTestTools();
        tools = Utils::filtered(TestFrameworkManager::registeredTestTools(),
                                [active](ITestTool *testTool) {
            return active.value(testTool, false);
        });
    }

    // pre-check to avoid further processing when test tools are unchanged
    TreeItem *invisibleRoot = rootItem();
    QSet<ITestTool *> newlyAdded;
    QList<ITestTreeItem *> oldFrameworkRoots;
    for (TreeItem *oldFrameworkRoot : *invisibleRoot) {
        auto item = static_cast<ITestTreeItem *>(oldFrameworkRoot);
        if (item->testBase()->type() == ITestBase::Tool)
            oldFrameworkRoots.append(item);
    }

    for (ITestTreeItem *oldFrameworkRoot : oldFrameworkRoots)
        takeItem(oldFrameworkRoot);  // do NOT delete the ptr is still held by TestFrameworkManager

    for (ITestTool *testTool : std::as_const(tools)) {
        ITestTreeItem *testToolRootNode = testTool->rootNode();
        invisibleRoot->appendChild(testToolRootNode);
        if (!oldFrameworkRoots.removeOne(testToolRootNode))
            newlyAdded.insert(testTool);
    }

    if (project) {
        const QList<Target *> &allTargets = project->targets();
        auto target = allTargets.empty() ? nullptr : allTargets.first();
        if (target) {
            auto bs = target->buildSystem();
            for (ITestTool *testTool : newlyAdded) {
                ITestTreeItem *rootNode = testTool->rootNode();
                QTC_ASSERT(rootNode, return);
                rootNode->removeChildren();
                for (const auto &tci : bs->testcasesInfo()) {
                    ITestTreeItem *item = testTool->createItemFromTestCaseInfo(tci);
                    QTC_ASSERT(item, continue);
                    if (std::optional<Qt::CheckState> cached = m_checkStateCache->get(item))
                        item->setData(0, cached.value(), Qt::CheckStateRole);
                    m_checkStateCache->insert(item, item->checked());
                    rootNode->appendChild(item);
                }
                revalidateCheckState(rootNode);
            }
        }
    }
    emit updatedActiveFrameworks(invisibleRoot->childCount());
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

void TestTreeModel::rebuild(const QList<Id> &frameworkIds)
{
    for (const Id &id : frameworkIds) {
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
    m_checkStateCache->evolve(ITestBase::Framework);

    for (TestTreeItem *rootNode : frameworkRootNodes()) {
        rootNode->forAllChildItems([this](TestTreeItem *childItem) {
            m_checkStateCache->insert(childItem, childItem->checked());
        });
    }
}

bool TestTreeModel::hasFailedTests() const
{
    auto failedItem = rootItem()->findAnyChild([](TreeItem *it) {
        return it->data(0, FailedRole).toBool();
    });
    return failedItem != nullptr;
}

void TestTreeModel::clearFailedMarks()
{
    for (TreeItem *rootNode : *rootItem()) {
        rootNode->forAllChildren([](TreeItem *child) {
            child->setData(0, false, FailedRole);
        });
    }
    m_failedStateCache.clear();
}

void TestTreeModel::markAllFrameworkItemsForRemoval()
{
    for (TestTreeItem *frameworkRoot : frameworkRootNodes()) {
        frameworkRoot->forFirstLevelChildItems([](TestTreeItem *child) {
            child->markForRemovalRecursively(true);
        });
    }
}

void TestTreeModel::markForRemoval(const QSet<Utils::FilePath> &filePaths)
{
    for (TestTreeItem *frameworkRoot : frameworkRootNodes()) {
        for (int childRow = frameworkRoot->childCount() - 1; childRow >= 0; --childRow) {
            TestTreeItem *child = frameworkRoot->childItem(childRow);
            child->markForRemovalRecursively(filePaths);
        }
    }
}

void TestTreeModel::sweep()
{
    for (TestTreeItem *frameworkRoot : frameworkRootNodes()) {
        if (frameworkRoot->m_status == TestTreeItem::ForcedRootRemoval) {
            frameworkRoot->framework()->resetRootNode();
            continue;
        }
        sweepChildren(frameworkRoot);
        revalidateCheckState(frameworkRoot);
    }
    // even if nothing has changed by the sweeping we might had parse which added or modified items
    emit testTreeModelChanged();
#ifdef WITH_TESTS
    if (m_parser->state() == TestCodeParser::Idle && !m_parser->furtherParsingExpected())
        emit sweepingDone();
#endif
}

QString TestTreeModel::report(bool full) const
{
    QString result;
    int items = 0;
    QString tree;
    for (TestTreeItem *rootNode : frameworkRootNodes()) {
        int itemsPerRoot = 0;
        result.append("\n");
        result += rootNode->name();
        result.append(" > ");

        if (full) {
            TestTreeSortFilterModel sortFilterModel(const_cast<TestTreeModel *>(this));
            sortFilterModel.setDynamicSortFilter(true);
            sortFilterModel.sort(0);
            tree = "\n" + sortFilterModel.report();
            rootNode->forAllChildren([&itemsPerRoot](TreeItem *) {
                ++itemsPerRoot;
            });

        } else {
            rootNode->forAllChildren([&itemsPerRoot](TreeItem *) {
                ++itemsPerRoot;
            });
        }
        result.append(QString::number(itemsPerRoot));
        items += itemsPerRoot;
    }
    result.append("\nItems: " + QString::number(items));
    if (full)
        return tree + '\n' + result;
    return result;
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
        newItem->forAllChildren([checkState](TreeItem *it) {
            it->setData(0, checkState, Qt::CheckStateRole);
        });
    }
}

void TestTreeModel::insertItemInParent(TestTreeItem *item, TestTreeItem *root, bool groupingEnabled)
{
    TestTreeItem *parentNode = root;
    if (groupingEnabled && item->isGroupable()) {
        parentNode = root->findFirstLevelChildItem([item](const TestTreeItem *it) {
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
        std::optional<Qt::CheckState> cached = m_checkStateCache ? m_checkStateCache->get(item)
                                                                 : std::optional<Qt::CheckState>{};
        if (cached.has_value())
            item->setData(0, cached.value(), Qt::CheckStateRole);
        else
            applyParentCheckState(parentNode, item);
        // ..and the failed state if available
        std::optional<bool> failed = m_failedStateCache.get(item);
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

        if (foundPartiallyChecked || (foundChecked && foundUnchecked)) {
            newState = Qt::PartiallyChecked;
            return;
        }
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
    ITestFramework *framework = result->framework;
    QTC_ASSERT(framework, return);
    TestTreeItem *rootNode = framework->rootNode();
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

    if (!m_checkStateCache) // dataChanged() may be triggered by closing a project
        return;
    for (int row = topLeft.row(), endRow = bottomRight.row(); row <= endRow; ++row) {
        if (auto item = static_cast<ITestTreeItem *>(itemForIndex(index(row, 0, parent))))
            m_checkStateCache->insert(item, item->checked());
    }
}

void TestTreeModel::handleParseResult(const TestParseResult *result, TestTreeItem *parentNode)
{
    const bool groupingEnabled = result->framework->grouping();
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
        if (!m_checkStateCache) // parse results may arrive after session switch / project close
            return;
        std::optional<Qt::CheckState> cached = m_checkStateCache->get(childItem);
        if (cached.has_value())
            childItem->setData(0, cached.value(), Qt::CheckStateRole);
        std::optional<bool> failed = m_failedStateCache.get(childItem);
        if (failed.has_value())
            childItem->setData(0, *failed, FailedRole);
    });
    // it might be necessary to "split" created item
    filterAndInsert(newItem, parentNode, groupingEnabled);
}

void TestTreeModel::removeAllTestItems()
{
    for (TestTreeItem *item : frameworkRootNodes()) {
        item->removeChildren();
        if (item->checked() == Qt::PartiallyChecked)
            item->setData(0, Qt::Checked, Qt::CheckStateRole);
    }
    emit testTreeModelChanged();
}

void TestTreeModel::removeAllTestToolItems()
{
    for (ITestTreeItem *item : testToolRootNodes()) {
        item->removeChildren();
        if (item->checked() == Qt::PartiallyChecked)
            item->setData(0, Qt::Checked, Qt::CheckStateRole);
    }
    emit testTreeModelChanged();
}

#ifdef WITH_TESTS
// we're inside tests - so use some internal knowledge to make testing easier
static TestTreeItem *qtRootNode()
{
    const Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix("QtTest");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

static TestTreeItem *quickRootNode()
{
    const Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix("QtQuickTest");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

static TestTreeItem *gtestRootNode()
{
    const Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix("GTest");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

static TestTreeItem *boostTestRootNode()
{
    const Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix("Boost");
    return TestFrameworkManager::frameworkForId(id)->rootNode();
}

int TestTreeModel::autoTestsCount() const
{
    TestTreeItem *rootNode = qtRootNode();
    return rootNode ? rootNode->childCount() : 0;
}

bool TestTreeModel::hasUnnamedQuickTests(const ITestTreeItem* rootNode) const
{
    for (int row = 0, count = rootNode->childCount(); row < count; ++row) {
        if (rootNode->childAt(row)->name().isEmpty())
            return true;
    }
    return false;
}

ITestTreeItem *TestTreeModel::unnamedQuickTests() const
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
    if (ITestTreeItem *unnamed = unnamedQuickTests())
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
            result.insert(child->name() + '|' + child->proFile().toString(), child->childCount());
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

void TestTreeSortFilterModel::setSortMode(ITestTreeItem::SortMode sortMode)
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

static QString dumpIndex(const QModelIndex &idx, int level = 0)
{
    QString result;
    result.append(QString(level, ' '));
    result.append(idx.data().toString() + '\n');
    for (int row = 0, end = idx.model()->rowCount(idx); row < end; ++row)
        result.append(dumpIndex(idx.model()->index(row, 0, idx), level + 1));
    return result;
}

QString TestTreeSortFilterModel::report() const
{
    QString result;
    for (int row = 0, end = rowCount(); row < end; ++row) {
        auto idx = index(row, 0);
        result.append(dumpIndex(idx));
    }
    return result;
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
