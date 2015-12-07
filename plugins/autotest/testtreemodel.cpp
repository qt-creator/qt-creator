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

#include "autotestconstants.h"
#include "testcodeparser.h"
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
    m_autoTestRootItem(new TestTreeItem(tr("Auto Tests"), QString(), TestTreeItem::Root)),
    m_quickTestRootItem(new TestTreeItem(tr("Qt Quick Tests"), QString(), TestTreeItem::Root)),
    m_parser(new TestCodeParser(this)),
    m_connectionsInitialized(false)
{
    rootItem()->appendChild(m_autoTestRootItem);
    rootItem()->appendChild(m_quickTestRootItem);

    connect(m_parser, &TestCodeParser::cacheCleared, this,
            &TestTreeModel::removeAllTestItems, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testItemCreated,
            this, &TestTreeModel::addTestTreeItem, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testItemModified,
            this, &TestTreeModel::modifyTestTreeItem, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testItemsRemoved,
            this, &TestTreeModel::removeTestTreeItems, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::unnamedQuickTestsUpdated,
            this, &TestTreeModel::updateUnnamedQuickTest, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::unnamedQuickTestsRemoved,
            this, &TestTreeModel::removeUnnamedQuickTests, Qt::QueuedConnection);

//    CppTools::CppModelManagerInterface *cppMM = CppTools::CppModelManagerInterface::instance();
//    if (cppMM) {
//        // replace later on by
//        // cppMM->registerAstProcessor([this](const CplusPlus::Document::Ptr &doc,
//        //                             const CPlusPlus::Snapshot &snapshot) {
//        //        checkForQtTestStuff(doc, snapshot);
//        //      });
//        connect(cppMM, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
//                this, SLOT(checkForQtTestStuff(CPlusPlus::Document::Ptr)),
//                Qt::DirectConnection);

//    }
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
            m_parser, &TestCodeParser::removeFiles, Qt::QueuedConnection);
    connect(cppMM, &CppTools::CppModelManager::projectPartsUpdated,
            m_parser, &TestCodeParser::onProjectPartsUpdated);

    QmlJS::ModelManagerInterface *qmlJsMM = QmlJS::ModelManagerInterface::instance();
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::documentUpdated,
            m_parser, &TestCodeParser::onQmlDocumentUpdated, Qt::QueuedConnection);
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::aboutToRemoveFiles,
            m_parser, &TestCodeParser::removeFiles, Qt::QueuedConnection);
    m_connectionsInitialized = true;
}

void TestTreeModel::disableParsing()
{
    if (!m_refCounter.deref())
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
            case TestTreeItem::TestClass:
                if (item->childCount() > 0)
                    emit dataChanged(index.child(0, 0), index.child(item->childCount() - 1, 0));
                break;
            case TestTreeItem::TestFunction:
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
    case TestTreeItem::TestClass:
        if (item->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
    case TestTreeItem::TestFunction:
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
    return m_autoTestRootItem->childCount() > 0 || m_quickTestRootItem->childCount() > 0;
}

QList<TestConfiguration *> TestTreeModel::getAllTestCases() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return result;

    // get all Auto Tests
    for (int row = 0, count = m_autoTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_autoTestRootItem->childItem(row);

        TestConfiguration *tc = new TestConfiguration(child->name(), QStringList(),
                                                      child->childCount());
        tc->setMainFilePath(child->filePath());
        tc->setProject(project);
        result << tc;
    }

    // get all Quick Tests
    QMap<QString, int> foundMains;
    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_quickTestRootItem->childItem(row);
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            for (int childRow = 0, ccount = child->childCount(); childRow < ccount; ++ childRow) {
                const TestTreeItem *grandChild = child->childItem(childRow);
                const QString mainFile = grandChild->mainFile();
                foundMains.insert(mainFile, foundMains.contains(mainFile)
                                            ? foundMains.value(mainFile) + 1 : 1);
            }
            continue;
        }
        // named Quick Test
        const QString mainFile = child->mainFile();
        foundMains.insert(mainFile, foundMains.contains(mainFile)
                          ? foundMains.value(mainFile) + child->childCount()
                          : child->childCount());
    }
    // create TestConfiguration for each main
    foreach (const QString &mainFile, foundMains.keys()) {
        TestConfiguration *tc = new TestConfiguration(QString(), QStringList(),
                                                      foundMains.value(mainFile));
        tc->setMainFilePath(mainFile);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

QList<TestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return result;

    TestConfiguration *testConfiguration = 0;

    for (int row = 0, count = m_autoTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_autoTestRootItem->childItem(row);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
            testConfiguration = new TestConfiguration(child->name(), QStringList(), child->childCount());
            testConfiguration->setMainFilePath(child->filePath());
            testConfiguration->setProject(project);
            result << testConfiguration;
            continue;
        case Qt::PartiallyChecked:
        default:
            const QString childName = child->name();
            int grandChildCount = child->childCount();
            QStringList testCases;
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->checked() == Qt::Checked)
                    testCases << grandChild->name();
            }

            testConfiguration = new TestConfiguration(childName, testCases);
            testConfiguration->setMainFilePath(child->filePath());
            testConfiguration->setProject(project);
            result << testConfiguration;
        }
    }
    // Quick Tests must be handled differently - need the calling cpp file to use this in
    // addProjectInformation() - additionally this must be unique to not execute the same executable
    // on and on and on...
    // TODO: do this later on for Auto Tests as well to support strange setups? or redo the model

    QMap<QString, TestConfiguration *> foundMains;

    if (TestTreeItem *unnamed = unnamedQuickTests()) {
        for (int childRow = 0, ccount = unnamed->childCount(); childRow < ccount; ++ childRow) {
            const TestTreeItem *grandChild = unnamed->childItem(childRow);
            const QString mainFile = grandChild->mainFile();
            if (foundMains.contains(mainFile)) {
                QTC_ASSERT(testConfiguration,
                           qWarning() << "Illegal state (unnamed Quick Test listed as named)";
                           return QList<TestConfiguration *>());
                foundMains[mainFile]->setTestCaseCount(testConfiguration->testCaseCount() + 1);
            } else {
                testConfiguration = new TestConfiguration(QString(), QStringList());
                testConfiguration->setTestCaseCount(1);
                testConfiguration->setUnnamedOnly(true);
                testConfiguration->setMainFilePath(mainFile);
                testConfiguration->setProject(project);
                foundMains.insert(mainFile, testConfiguration);
            }
        }
    }

    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_quickTestRootItem->childItem(row);
        // unnamed Quick Tests have been handled separately already
        if (child->name().isEmpty())
            continue;

        // named Quick Tests
        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
        case Qt::PartiallyChecked:
        default:
            QStringList testFunctions;
            int grandChildCount = child->childCount();
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->type() != TestTreeItem::TestFunction)
                    continue;
                testFunctions << child->name() + QLatin1String("::") + grandChild->name();
            }
            TestConfiguration *tc;
            if (foundMains.contains(child->mainFile())) {
                tc = foundMains[child->mainFile()];
                QStringList oldFunctions(tc->testCases());
                // if oldFunctions.size() is 0 this test configuration is used for at least one
                // unnamed test case
                if (oldFunctions.size() == 0) {
                    tc->setTestCaseCount(tc->testCaseCount() + testFunctions.size());
                    tc->setUnnamedOnly(false);
                } else {
                    oldFunctions << testFunctions;
                    tc->setTestCases(oldFunctions);
                }
            } else {
                tc = new TestConfiguration(QString(), testFunctions);
                tc->setMainFilePath(child->mainFile());
                tc->setProject(project);
                foundMains.insert(child->mainFile(), tc);
            }
            break;
        }
    }

    foreach (TestConfiguration *config, foundMains.values())
        if (!config->unnamedOnly())
            result << config;

    return result;
}

TestConfiguration *TestTreeModel::getTestConfiguration(const TestTreeItem *item) const
{
    QTC_ASSERT(item != 0, return 0);
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    TestConfiguration *config = 0;
    switch (item->type()) {
    case TestTreeItem::TestClass: {
        if (item->parent() == m_quickTestRootItem) {
            // Quick Test TestCase
            QStringList testFunctions;
            for (int row = 0, count = item->childCount(); row < count; ++row) {
                    testFunctions << item->name() + QLatin1String("::")
                                     + item->childItem(row)->name();
            }
            config = new TestConfiguration(QString(), testFunctions);
            config->setMainFilePath(item->mainFile());
            config->setProject(project);
        } else {
            // normal auto test
            config = new TestConfiguration(item->name(), QStringList(), item->childCount());
            config->setMainFilePath(item->filePath());
            config->setProject(project);
        }
        break;
    }
    case TestTreeItem::TestFunction: {
        const TestTreeItem *parent = item->parentItem();
        if (parent->parent() == m_quickTestRootItem) {
            // it's a Quick Test function of a named TestCase
            QStringList testFunction(parent->name() + QLatin1String("::") + item->name());
            config = new TestConfiguration(QString(), testFunction);
            config->setMainFilePath(parent->mainFile());
            config->setProject(project);
        } else {
            // normal auto test
            config = new TestConfiguration(parent->name(), QStringList() << item->name());
            config->setMainFilePath(parent->filePath());
            config->setProject(project);
        }
        break;
    }
    case TestTreeItem::TestDataTag: {
        const TestTreeItem *function = item->parentItem();
        const TestTreeItem *parent = function ? function->parentItem() : 0;
        if (!parent)
            return 0;
        const QString functionWithTag = function->name() + QLatin1Char(':') + item->name();
        config = new TestConfiguration(parent->name(), QStringList() << functionWithTag);
        config->setMainFilePath(parent->filePath());
        config->setProject(project);
        break;
    }
    // not supported items
    default:
        return 0;
    }
    return config;
}

QString TestTreeModel::getMainFileForUnnamedQuickTest(const QString &qmlFile) const
{
    const TestTreeItem *unnamed = unnamedQuickTests();
    const int count = unnamed ? unnamed->childCount() : 0;
    for (int row = 0; row < count; ++row) {
        const TestTreeItem *child = unnamed->childItem(row);
        if (qmlFile == child->filePath())
            return child->mainFile();
    }
    return QString();
}

void TestTreeModel::qmlFilesForMainFile(const QString &mainFile, QSet<QString> *filePaths) const
{
    const TestTreeItem *unnamed = unnamedQuickTests();
    if (!unnamed)
        return;
    for (int row = 0, count = unnamed->childCount(); row < count; ++row) {
        const TestTreeItem *child = unnamed->childItem(row);
        if (child->mainFile() == mainFile)
            filePaths->insert(child->filePath());
    }
}

QList<QString> TestTreeModel::getUnnamedQuickTestFunctions() const
{
    const TestTreeItem *unnamed = unnamedQuickTests();
    if (unnamed)
        return unnamed->getChildNames();
    return QList<QString>();
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

void TestTreeModel::removeUnnamedQuickTests(const QString &filePath)
{
    TestTreeItem *unnamedQT = unnamedQuickTests();
    if (!unnamedQT)
        return;

    for (int childRow = unnamedQT->childCount() - 1; childRow >= 0; --childRow) {
        TestTreeItem *child = unnamedQT->childItem(childRow);
        if (filePath == child->filePath())
            delete takeItem(child);
    }

    if (unnamedQT->childCount() == 0)
        delete takeItem(unnamedQT);
    emit testTreeModelChanged();
}

void TestTreeModel::addTestTreeItem(TestTreeItem *item, TestTreeModel::Type type)
{
    TestTreeItem *parent = rootItemForType(type);
    parent->appendChild(item);
    emit testTreeModelChanged();
}

void TestTreeModel::updateUnnamedQuickTest(const QString &mainFile,
                                           const QMap<QString, TestCodeLocationAndType> &functions)
{
    if (functions.isEmpty())
        return;

    if (!hasUnnamedQuickTests())
        addTestTreeItem(new TestTreeItem(QString(), QString(), TestTreeItem::TestClass), QuickTest);

    TestTreeItem *unnamed = unnamedQuickTests();
    foreach (const QString &functionName, functions.keys()) {
        const TestCodeLocationAndType locationAndType = functions.value(functionName);
        TestTreeItem *testFunction = new TestTreeItem(functionName, locationAndType.m_name,
                                                      locationAndType.m_type);
        testFunction->setLine(locationAndType.m_line);
        testFunction->setColumn(locationAndType.m_column);
        testFunction->setMainFile(mainFile);
        unnamed->appendChild(testFunction);
    }
}

void TestTreeModel::modifyTestTreeItem(TestTreeItem *item, TestTreeModel::Type type, const QStringList &files)
{
    QModelIndex index = rootIndexForType(type);
    TestTreeItem *parent = rootItemForType(type);
    if (files.isEmpty()) {
        if (TestTreeItem *unnamed = unnamedQuickTests()) {
            if (unnamed == item) // no need to update or delete
                return;

            index = indexForItem(unnamed);
            modifyTestSubtree(index, item);
        }
    } else {
        for (int row = 0; row < parent->childCount(); ++row) {
            if (files.contains(parent->childItem(row)->filePath())) {
                index = index.child(row, 0);
                modifyTestSubtree(index, item);
                break;
            }
        }
    }
    // item was created as temporary, destroy it if it won't get destroyed by its parent
    if (!item->parent())
        delete item;
}

void TestTreeModel::removeAllTestItems()
{
    m_autoTestRootItem->removeChildren();
    m_quickTestRootItem->removeChildren();
    emit testTreeModelChanged();
}

void TestTreeModel::removeTestTreeItems(const QString &filePath, Type type)
{
    bool removed = false;
    const TestTreeItem *rootItem = rootItemForType(type);
    for (int row = rootItem->childCount() - 1; row >= 0; --row) {
        TestTreeItem *childItem = rootItem->childItem(row);
        if (filePath == childItem->filePath()) {
            delete takeItem(childItem);
            removed = true;
        }
    }
    if (removed)
        emit testTreeModelChanged();
}

TestTreeItem *TestTreeModel::rootItemForType(TestTreeModel::Type type)
{
    switch (type) {
    case AutoTest:
        return m_autoTestRootItem;
    case QuickTest:
        return m_quickTestRootItem;
    }
    QTC_ASSERT(false, return 0);
}

QModelIndex TestTreeModel::rootIndexForType(TestTreeModel::Type type)
{
    switch (type) {
    case AutoTest:
        return index(0, 0);
    case QuickTest:
        return index(1, 0);
    }
    QTC_ASSERT(false, return QModelIndex());
}

void TestTreeModel::modifyTestSubtree(QModelIndex &toBeModifiedIndex, const TestTreeItem *newItem)
{
    if (!toBeModifiedIndex.isValid())
        return;

    TestTreeItem *toBeModifiedItem = static_cast<TestTreeItem *>(itemForIndex(toBeModifiedIndex));
    if (toBeModifiedItem->modifyContent(newItem))
        emit dataChanged(toBeModifiedIndex, toBeModifiedIndex,
                         QVector<int>() << Qt::DisplayRole << Qt::ToolTipRole << LinkRole);

    // process sub-items as well...
    const int childCount = toBeModifiedItem->childCount();
    const int newChildCount = newItem->childCount();

    // for keeping the CheckState on modifications
    // TODO might still fail for duplicate entries
    QHash<QString, Qt::CheckState> checkStates;
    for (int row = 0; row < childCount; ++row) {
        const TestTreeItem *child = toBeModifiedItem->childItem(row);
        checkStates.insert(child->name(), child->checked());
    }

    if (childCount <= newChildCount) {
        processChildren(toBeModifiedIndex, newItem, childCount, checkStates);
        // add additional items
        for (int row = childCount; row < newChildCount; ++row) {
            const TestTreeItem *newChild = newItem->childItem(row);
            TestTreeItem *toBeAdded = new TestTreeItem(*newChild);
            if (checkStates.contains(toBeAdded->name())
                    && checkStates.value(toBeAdded->name()) != Qt::Checked)
                toBeAdded->setChecked(checkStates.value(toBeAdded->name()));
            toBeModifiedItem->appendChild(toBeAdded);
        }
    } else {
        processChildren(toBeModifiedIndex, newItem, newChildCount, checkStates);
        // remove rest of the items
        for (int row = childCount - 1; row > newChildCount; --row)
            delete takeItem(toBeModifiedItem->childItem(row));
    }
    emit testTreeModelChanged();
}

void TestTreeModel::processChildren(QModelIndex &parentIndex, const TestTreeItem *newItem,
                                    const int upperBound,
                                    const QHash<QString, Qt::CheckState> &checkStates)
{
    static QVector<int> modificationRoles = QVector<int>() << Qt::DisplayRole
                                                           << Qt::ToolTipRole
                                                           << LinkRole;
    TestTreeItem *toBeModifiedItem = static_cast<TestTreeItem *>(itemForIndex(parentIndex));
    for (int row = 0; row < upperBound; ++row) {
        QModelIndex child = parentIndex.child(row, 0);
        TestTreeItem *toBeModifiedChild = toBeModifiedItem->childItem(row);
        const TestTreeItem *modifiedChild = newItem->childItem(row);
        if (toBeModifiedChild->modifyContent(modifiedChild))
            emit dataChanged(child, child, modificationRoles);

        // handle data tags - just remove old and add them
        if (modifiedChild->childCount() || toBeModifiedChild->childCount()) {
            toBeModifiedChild->removeChildren();
            const int count = modifiedChild->childCount();
            for (int childRow = 0; childRow < count; ++childRow)
                toBeModifiedChild->appendChild(new TestTreeItem(*modifiedChild->childItem(childRow)));
        }

        if (checkStates.contains(toBeModifiedChild->name())) {
                Qt::CheckState state = checkStates.value(toBeModifiedChild->name());
                if (state != toBeModifiedChild->checked()) {
                    toBeModifiedChild->setChecked(state);
                    emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
            }
        } else { // newly added (BAD: happens for renaming as well)
            toBeModifiedChild->setChecked(Qt::Checked);
            emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
        }
    }
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
