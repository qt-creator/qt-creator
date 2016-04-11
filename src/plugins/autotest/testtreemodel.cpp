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
    m_autoTestRootItem(new AutoTestTreeItem(tr("Auto Tests"), QString(), TestTreeItem::Root)),
    m_quickTestRootItem(new QuickTestTreeItem(tr("Qt Quick Tests"), QString(), TestTreeItem::Root)),
    m_googleTestRootItem(new GoogleTestTreeItem(tr("Google Tests"), QString(), TestTreeItem::Root)),
    m_parser(new TestCodeParser(this)),
    m_connectionsInitialized(false)
{
    rootItem()->appendChild(m_autoTestRootItem);
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
    switch(item->type()) {
    case TestTreeItem::TestCase:
        if (item->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
    case TestTreeItem::TestFunctionOrSet:
        if (item->parentItem()->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    case TestTreeItem::Root:
        return Qt::ItemIsEnabled;
    case TestTreeItem::TestDataFunction:
    case TestTreeItem::TestSpecialFunction:
    case TestTreeItem::TestDataTag:
    default:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
}

bool TestTreeModel::hasTests() const
{
    return m_autoTestRootItem->childCount() > 0 || m_quickTestRootItem->childCount() > 0
            || m_googleTestRootItem->childCount() > 0;
}

QList<TestConfiguration *> TestTreeModel::getAllTestCases() const
{
    QList<TestConfiguration *> result;

    result.append(m_autoTestRootItem->getAllTestConfigurations());
    result.append(m_quickTestRootItem->getAllTestConfigurations());
    result.append(m_googleTestRootItem->getAllTestConfigurations());
    return result;
}

QList<TestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<TestConfiguration *> result;
    result.append(m_autoTestRootItem->getSelectedTestConfigurations());
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
    foreach (Utils::TreeItem *item, m_autoTestRootItem->children())
        static_cast<TestTreeItem *>(item)->markForRemovalRecursively(true);

    foreach (Utils::TreeItem *item, m_quickTestRootItem->children())
        static_cast<TestTreeItem *>(item)->markForRemovalRecursively(true);

    foreach (Utils::TreeItem *item, m_googleTestRootItem->children())
        static_cast<TestTreeItem *>(item)->markForRemovalRecursively(true);
}

void TestTreeModel::markForRemoval(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    Type types[] = { AutoTest, QuickTest, GoogleTest };
    for (Type type : types) {
        TestTreeItem *root = rootItemForType(type);
        for (int childRow = root->childCount() - 1; childRow >= 0; --childRow) {
            TestTreeItem *child = root->childItem(childRow);
            // Qt + named Quick Tests
            if (child->filePath() == filePath) {
                child->markForRemovalRecursively(true);
            } else {
                // unnamed Quick Tests and GTest and Qt Tests with separated source/header
                int grandChildRow = child->childCount() - 1;
                for ( ; grandChildRow >= 0; --grandChildRow) {
                    TestTreeItem *grandChild = child->childItem(grandChildRow);
                    if (grandChild->filePath() == filePath) {
                        grandChild->markForRemovalRecursively(true);
                    }
                }
            }
        }
    }
}

void TestTreeModel::sweep()
{
    Type types[] = { AutoTest, QuickTest, GoogleTest };
    for (Type type : types) {
        TestTreeItem *root = rootItemForType(type);
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
    if (!m_autoTestRootItem)
        return result;

    for (int row = 0, count = m_autoTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_autoTestRootItem->childItem(row);
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
    switch (result->type) {
    case AutoTest:
        handleQtParseResult(result);
        break;
    case QuickTest:
        handleQuickParseResult(result);
        break;
    case GoogleTest:
        handleGTestParseResult(result);
        break;
    case Invalid:
        QTC_ASSERT(false, qWarning("TestParseResult of type Invalid unexpected."));
        break;
    }
}

void TestTreeModel::handleQtParseResult(const TestParseResultPtr result)
{
    TestTreeItem *toBeModified = m_autoTestRootItem->findChildByFile(result->fileName);
    // if there's no matching item, add the new one
    if (!toBeModified) {
        m_autoTestRootItem->appendChild(AutoTestTreeItem::createTestItem(*result));
        return;
    }
    // else we have to check level by level.. first the current level...
    bool changed = toBeModified->modifyTestCaseContent(result->testCaseName, result->line,
                                                       result->column);
    toBeModified->markForRemoval(false);
    if (changed)
        emit dataChanged(indexForItem(toBeModified), indexForItem(toBeModified));
    // ...now the functions
    const QtTestParseResult &parseResult = static_cast<const QtTestParseResult &>(*result);
    foreach (const QString &func, parseResult.functions.keys()) {
        TestTreeItem *functionItem = toBeModified->findChildByName(func);
        if (!functionItem) {
            // if there's no function matching, add the new one
            const QString qualifiedName = parseResult.testCaseName + QLatin1String("::") + func;
            toBeModified->appendChild(AutoTestTreeItem::createFunctionItem(
                                          func, parseResult.functions.value(func),
                                          parseResult.dataTags.value(qualifiedName)));
            continue;
        }
        // else we have to check level by level.. first the current level...
        changed = functionItem->modifyTestFunctionContent(parseResult.functions.value(func));
        functionItem->markForRemoval(false);
        if (changed)
            emit dataChanged(indexForItem(functionItem), indexForItem(functionItem));
        // ...now the data tags
        const QString &funcFileName = parseResult.functions.value(func).m_name;
        const QString qualifiedFunctionName = parseResult.testCaseName + QLatin1String("::") + func;
        foreach (const TestCodeLocationAndType &location, parseResult.dataTags.value(qualifiedFunctionName)) {
            TestTreeItem *dataTagItem = functionItem->findChildByName(location.m_name);
            if (!dataTagItem) {
                functionItem->appendChild(AutoTestTreeItem::createDataTagItem(funcFileName, location));
                continue;
            }
            changed = dataTagItem->modifyDataTagContent(funcFileName, location);
            dataTagItem->markForRemoval(false);
            if (changed)
                emit dataChanged(indexForItem(dataTagItem), indexForItem(dataTagItem));
        }
    }
}

void TestTreeModel::handleQuickParseResult(const TestParseResultPtr result)
{
    if (result->testCaseName.isEmpty()) {
        handleUnnamedQuickParseResult(result);
        return;
    }

    TestTreeItem *toBeModified = m_quickTestRootItem->findChildByFile(result->fileName);
    // if there's no matching item, add the new one
    if (!toBeModified) {
        m_quickTestRootItem->appendChild(QuickTestTreeItem::createTestItem(*result));
        return;
    }
    // else we have to check level by level.. first the current level...
    bool changed = toBeModified->modifyTestCaseContent(result->testCaseName, result->line,
                                                       result->column);
    toBeModified->markForRemoval(false);
    if (changed)
        emit dataChanged(indexForItem(toBeModified), indexForItem(toBeModified));
    // ...now the functions

    const QuickTestParseResult &parseResult = static_cast<const QuickTestParseResult &>(*result);
    foreach (const QString &func, parseResult.functions.keys()) {
        TestTreeItem *functionItem = toBeModified->findChildByName(func);
        if (!functionItem) {
            // if there's no matching, add the new one
            toBeModified->appendChild(QuickTestTreeItem::createFunctionItem(
                                          func, parseResult.functions.value(func)));
            continue;
        }
        // else we have to modify..
        changed = functionItem->modifyTestFunctionContent(parseResult.functions.value(func));
        functionItem->markForRemoval(false);
        if (changed)
            emit dataChanged(indexForItem(functionItem), indexForItem(functionItem));
    }
}

void TestTreeModel::handleUnnamedQuickParseResult(const TestParseResultPtr result)
{
    const QuickTestParseResult &parseResult = static_cast<const QuickTestParseResult &>(*result);
    TestTreeItem *toBeModified = unnamedQuickTests();
    if (!toBeModified) {
        m_quickTestRootItem->appendChild(QuickTestTreeItem::createUnnamedQuickTestItem(parseResult));
        return;
    }
    // if we have already Unnamed Quick tests we might update them..
    foreach (const QString &func, parseResult.functions.keys()) {
        const TestCodeLocationAndType &location = parseResult.functions.value(func);
        TestTreeItem *functionItem = toBeModified->findChildByNameAndFile(func, location.m_name);
        if (!functionItem) {
            toBeModified->appendChild(QuickTestTreeItem::createUnnamedQuickFunctionItem(
                                          func, parseResult));
            continue;
        }
        functionItem->modifyLineAndColumn(location);
        functionItem->markForRemoval(false);
    }
}

void TestTreeModel::handleGTestParseResult(const TestParseResultPtr result)
{
    const GoogleTestParseResult &parseResult = static_cast<const GoogleTestParseResult &>(*result);
    QTC_ASSERT(!parseResult.testSets.isEmpty(), return);

    GoogleTestTreeItem::TestStates states = GoogleTestTreeItem::Enabled;
    if (parseResult.parameterized)
        states |= GoogleTestTreeItem::Parameterized;
    if (parseResult.typed)
        states |= GoogleTestTreeItem::Typed;
    TestTreeItem *toBeModified = m_googleTestRootItem->findChildByNameStateAndFile(
                parseResult.testCaseName, states, parseResult.proFile);
    if (!toBeModified) {
        m_googleTestRootItem->appendChild(GoogleTestTreeItem::createTestItem(parseResult));
        return;
    }
    // if found nothing has to be updated as all relevant members are used to find the item
    foreach (const TestCodeLocationAndType &location , parseResult.testSets) {
        TestTreeItem *testSetItem = toBeModified->findChildByNameAndFile(location.m_name,
                                                                         parseResult.fileName);
        if (!testSetItem) {
            toBeModified->appendChild(GoogleTestTreeItem::createTestSetItem(parseResult, location));
            continue;
        }
        bool changed = static_cast<GoogleTestTreeItem *>(testSetItem)->modifyTestSetContent(
                    parseResult.fileName, location);
        testSetItem->markForRemoval(false);
        if (changed)
            emit dataChanged(indexForItem(testSetItem), indexForItem(testSetItem));
    }
}

void TestTreeModel::removeAllTestItems()
{
    m_autoTestRootItem->removeChildren();
    m_quickTestRootItem->removeChildren();
    m_googleTestRootItem->removeChildren();
    emit testTreeModelChanged();
}

TestTreeItem *TestTreeModel::rootItemForType(TestTreeModel::Type type)
{
    switch (type) {
    case AutoTest:
        return m_autoTestRootItem;
    case QuickTest:
        return m_quickTestRootItem;
    case GoogleTest:
        return m_googleTestRootItem;
    case Invalid:
        break;
    }
    QTC_ASSERT(false, return 0);
}

#ifdef WITH_TESTS
int TestTreeModel::autoTestsCount() const
{
    return m_autoTestRootItem ? m_autoTestRootItem->childCount() : 0;
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
    foreach (Utils::TreeItem *item, m_autoTestRootItem->children()) {
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
      m_sortMode(Alphabetically),
      m_filterMode(Basic)
{
    setSourceModel(sourceModel);
}

void TestTreeSortFilterModel::setSortMode(SortMode sortMode)
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
    // root items keep the intended order: 1st Auto Tests, 2nd Quick Tests
    const TestTreeItem *leftItem = static_cast<TestTreeItem *>(left.internalPointer());
    if (leftItem->type() == TestTreeItem::Root)
        return left.row() > right.row();

    const QString leftVal = m_sourceModel->data(left, Qt::DisplayRole).toString();
    const QString rightVal = m_sourceModel->data(right, Qt::DisplayRole).toString();

    // unnamed Quick Tests will always be listed first
    if (leftVal == tr(Constants::UNNAMED_QUICKTESTS))
        return false;
    if (rightVal == tr(Constants::UNNAMED_QUICKTESTS))
        return true;

    switch (m_sortMode) {
    case Alphabetically:
        if (leftVal == rightVal)
            return left.row() > right.row();
        return leftVal > rightVal;
    case Naturally: {
        const TextEditor::TextEditorWidget::Link leftLink =
                m_sourceModel->data(left, LinkRole).value<TextEditor::TextEditorWidget::Link>();
        const TextEditor::TextEditorWidget::Link rightLink =
                m_sourceModel->data(right, LinkRole).value<TextEditor::TextEditorWidget::Link>();

        if (leftLink.targetFileName == rightLink.targetFileName) {
            return leftLink.targetLine == rightLink.targetLine
                    ? leftLink.targetColumn > rightLink.targetColumn
                    : leftLink.targetLine > rightLink.targetLine;
        } else {
            return leftLink.targetFileName > rightLink.targetFileName;
        }
    }
    default:
        return true;
    }
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
