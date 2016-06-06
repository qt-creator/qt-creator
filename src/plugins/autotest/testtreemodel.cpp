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
#include "testsettings.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

// FIXME
#include "qtest/qttesttreeitem.h"
#include "quick/quicktesttreeitem.h"
#include "gtest/gtesttreeitem.h"
// end of FIXME

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/texteditor.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

TestTreeModel::TestTreeModel(QObject *parent) :
    TreeModel(parent),
    m_qtTestRootItem(new QtTestTreeItem(tr("Qt Tests"), QString(), TestTreeItem::Root)),
    m_quickTestRootItem(new QuickTestTreeItem(tr("Qt Quick Tests"), QString(), TestTreeItem::Root)),
    m_googleTestRootItem(new GTestTreeItem(tr("Google Tests"), QString(), TestTreeItem::Root)),
    m_parser(new TestCodeParser(this)),
    m_connectionsInitialized(false)
{
    rootItem()->appendChild(m_qtTestRootItem);
    rootItem()->appendChild(m_quickTestRootItem);
    rootItem()->appendChild(m_googleTestRootItem);

    connect(m_parser, &TestCodeParser::aboutToPerformFullParse, this,
            &TestTreeModel::removeAllTestItems, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testParseResultReady,
            this, &TestTreeModel::onParseResultReady, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::parsingFinished,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::parsingFailed,
            this, &TestTreeModel::sweep, Qt::QueuedConnection);
}

static TestTreeModel *m_instance = 0;

TestTreeModel *TestTreeModel::instance()
{
    if (!m_instance)
        m_instance = new TestTreeModel;
    return m_instance;
}

TestTreeModel::~TestTreeModel()
{
    m_instance = 0;
}

void TestTreeModel::enableParsing()
{
    m_refCounter.ref();
    setupParsingConnections();
}

void TestTreeModel::enableParsingFromSettings()
{
    setupParsingConnections();
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

void TestTreeModel::disableParsing()
{
    if (!m_refCounter.deref() && !AutotestPlugin::instance()->settings()->alwaysParse)
        m_parser->setState(TestCodeParser::Disabled);
}

void TestTreeModel::disableParsingFromSettings()
{
    if (!m_refCounter.load())
        m_parser->setState(TestCodeParser::Disabled);
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
    return m_qtTestRootItem->childCount() > 0 || m_quickTestRootItem->childCount() > 0
            || m_googleTestRootItem->childCount() > 0;
}

QList<TestConfiguration *> TestTreeModel::getAllTestCases() const
{
    QList<TestConfiguration *> result;

    result.append(m_qtTestRootItem->getAllTestConfigurations());
    result.append(m_quickTestRootItem->getAllTestConfigurations());
    result.append(m_googleTestRootItem->getAllTestConfigurations());
    return result;
}

QList<TestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<TestConfiguration *> result;
    result.append(m_qtTestRootItem->getSelectedTestConfigurations());
    result.append(m_quickTestRootItem->getSelectedTestConfigurations());
    result.append(m_googleTestRootItem->getSelectedTestConfigurations());
    return result;
}

TestConfiguration *TestTreeModel::getTestConfiguration(const TestTreeItem *item) const
{
    QTC_ASSERT(item != 0, return 0);
    return item->testConfiguration();
}

bool TestTreeModel::hasUnnamedQuickTests() const
{
    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row)
        if (m_quickTestRootItem->childItem(row)->name().isEmpty())
            return true;
    return false;
}

TestTreeItem *TestTreeModel::unnamedQuickTests() const
{
    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row) {
        TestTreeItem *child = m_quickTestRootItem->childItem(row);
        if (child->name().isEmpty())
            return child;
    }
    return 0;
}

void TestTreeModel::removeFiles(const QStringList &files)
{
    foreach (const QString &file, files)
        markForRemoval(file);
    sweep();
}

void TestTreeModel::markAllForRemoval()
{
    foreach (Utils::TreeItem *frameworkRoot, rootItem()->children()) {
        foreach (Utils::TreeItem *item, frameworkRoot->children())
            static_cast<TestTreeItem *>(item)->markForRemovalRecursively(true);
    }
}

void TestTreeModel::markForRemoval(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    for (Utils::TreeItem *frameworkRoot : rootItem()->children()) {
        TestTreeItem *root = static_cast<TestTreeItem *>(frameworkRoot);
        for (int childRow = root->childCount() - 1; childRow >= 0; --childRow) {
            TestTreeItem *child = root->childItem(childRow);
            child->markForRemovalRecursively(filePath);
        }
    }
}

void TestTreeModel::sweep()
{
    for (Utils::TreeItem *frameworkRoot : rootItem()->children()) {
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

QHash<QString, QString> TestTreeModel::testCaseNamesForFiles(QStringList files)
{
    QHash<QString, QString> result;
    if (!m_qtTestRootItem)
        return result;

    for (int row = 0, count = m_qtTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_qtTestRootItem->childItem(row);
        if (files.contains(child->filePath())) {
            result.insert(child->filePath(), child->name());
        }
        for (int childRow = 0, children = child->childCount(); childRow < children; ++childRow) {
            const TestTreeItem *grandChild = child->childItem(childRow);
            if (files.contains(grandChild->filePath()))
                result.insert(grandChild->filePath(), child->name());
        }
    }
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

        if (child->parentItem()->type() != TestTreeItem::Root && child->markedForRemoval()) {
            delete takeItem(child);
            hasChanged = true;
            continue;
        }
        if (bool noEndNode = child->hasChildren()) {
            hasChanged |= sweepChildren(child);
            if (noEndNode && child->childCount() == 0) {
                delete takeItem(child);
                hasChanged = true;
                continue;
            }
        }
        hasChanged |= child->newlyAdded();
        child->markForRemoval(false);
    }
    return hasChanged;
}

void TestTreeModel::onParseResultReady(const TestParseResultPtr result)
{
    TestTreeItem *rootNode = rootItemForFramework(result->frameworkId);
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
        foreach (const TestParseResult *child, result->children)
            handleParseResult(child, toBeModified);
        return;
    }
    // if there's no matching item, add the new one
    TestTreeItem *newItem = result->createTestTreeItem();
    QTC_ASSERT(newItem, return);
    parentNode->appendChild(newItem);
}

void TestTreeModel::removeAllTestItems()
{
    m_qtTestRootItem->removeChildren();
    m_quickTestRootItem->removeChildren();
    m_googleTestRootItem->removeChildren();
    emit testTreeModelChanged();
}

TestTreeItem *TestTreeModel::rootItemForFramework(const Core::Id &id)
{
    if (id == Core::Id("QtTest"))
        return m_qtTestRootItem;
    if (id == Core::Id("QtQuickTest"))
        return m_quickTestRootItem;
    if (id == Core::Id("GTest"))
        return m_googleTestRootItem;
    return 0;
}

#ifdef WITH_TESTS
int TestTreeModel::autoTestsCount() const
{
    return m_qtTestRootItem ? m_qtTestRootItem->childCount() : 0;
}

int TestTreeModel::namedQuickTestsCount() const
{
    return m_quickTestRootItem
            ? m_quickTestRootItem->childCount() - (hasUnnamedQuickTests() ? 1 : 0)
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
    int dataTagCount = 0;
    foreach (Utils::TreeItem *item, m_qtTestRootItem->children()) {
        TestTreeItem *classItem = static_cast<TestTreeItem *>(item);
        foreach (Utils::TreeItem *functionItem, classItem->children())
            dataTagCount += functionItem->childCount();
   }
    return dataTagCount;
}

int TestTreeModel::gtestNamesCount() const
{
    return m_googleTestRootItem ? m_googleTestRootItem->childCount() : 0;
}

QMultiMap<QString, int> TestTreeModel::gtestNamesAndSets() const
{
    QMultiMap<QString, int> result;

    if (m_googleTestRootItem) {
        for (int row = 0, count = m_googleTestRootItem->childCount(); row < count; ++row) {
            const TestTreeItem *current = m_googleTestRootItem->childItem(row);
            result.insert(current->name(), current->childCount());
        }
    }
    return result;
}
#endif

/***************************** Sort/Filter Model **********************************/

TestTreeSortFilterModel::TestTreeSortFilterModel(TestTreeModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_sourceModel(sourceModel),
      m_sortMode(TestTreeItem::Alphabetically),
      m_filterMode(Basic)
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
