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

class ReferencingFilesFinder : public Utils::TreeItemVisitor
{
public:
    ReferencingFilesFinder() {}

    bool preVisit(Utils::TreeItem *item) override
    {
        // 0 = invisible root, 1 = main categories, 2 = test cases, 3 = test functions
        return item->level() < 4;
    }

    void visit(Utils::TreeItem *item) override
    {
        // skip invisible root item
        if (!item->parent())
            return;

        if (auto testItem = static_cast<TestTreeItem *>(item)) {
            if (!testItem->filePath().isEmpty() && !testItem->referencingFile().isEmpty())
                m_referencingFiles.insert(testItem->filePath(), testItem->referencingFile());
        }
    }

    QMap<QString, QString> referencingFiles() const { return m_referencingFiles; }

private:
    QMap<QString, QString> m_referencingFiles;

};

/***********************************************************************************************/

TestTreeModel::TestTreeModel(QObject *parent) :
    TreeModel(parent),
    m_autoTestRootItem(new TestTreeItem(tr("Auto Tests"), QString(), TestTreeItem::Root)),
    m_quickTestRootItem(new TestTreeItem(tr("Qt Quick Tests"), QString(), TestTreeItem::Root)),
    m_googleTestRootItem(new TestTreeItem(tr("Google Tests"), QString(), TestTreeItem::Root)),
    m_parser(new TestCodeParser(this)),
    m_connectionsInitialized(false)
{
    rootItem()->appendChild(m_autoTestRootItem);
    rootItem()->appendChild(m_quickTestRootItem);
    rootItem()->appendChild(m_googleTestRootItem);

    connect(m_parser, &TestCodeParser::aboutToPerformFullParse, this,
            &TestTreeModel::removeAllTestItems, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testItemCreated,
            this, &TestTreeModel::addTestTreeItem, Qt::QueuedConnection);
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
            case TestTreeItem::TestClass:
            case TestTreeItem::GTestCase:
            case TestTreeItem::GTestCaseParameterized:
                if (item->childCount() > 0)
                    emit dataChanged(index.child(0, 0), index.child(item->childCount() - 1, 0));
                break;
            case TestTreeItem::TestFunction:
            case TestTreeItem::GTestName:
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
    case TestTreeItem::GTestCase:
    case TestTreeItem::GTestCaseParameterized:
        if (item->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
    case TestTreeItem::TestFunction:
    case TestTreeItem::GTestName:
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

    foundMains.clear();

    // get all Google Tests
    for (int row = 0, count = m_googleTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_googleTestRootItem->childItem(row);
        for (int childRow = 0, childCount = child->childCount(); childRow < childCount; ++childRow) {
            const QString &proFilePath = child->childItem(childRow)->mainFile();
            foundMains.insert(proFilePath, foundMains.contains(proFilePath)
                              ? foundMains.value(proFilePath) + 1 : 1);
        }
    }

    foreach (const QString &proFile, foundMains.keys()) {
        TestConfiguration *tc = new TestConfiguration(QString(), QStringList(),
                                                      foundMains.value(proFile));
        tc->setProFile(proFile);
        tc->setProject(project);
        tc->setTestType(TestTypeGTest);
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

    // get selected Google Tests
    QMap<QString, QStringList> proFilesWithEnabledTestSets;

    for (int row = 0, count = m_googleTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_googleTestRootItem->childItem(row);
        if (child->checked() == Qt::Unchecked) // add this test name to disabled list ?
            continue;

        int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            const QString &proFile = grandChild->mainFile();
            QStringList enabled = proFilesWithEnabledTestSets.value(proFile);
            if (grandChild->checked() == Qt::Checked) {
                QString testSpecifier = child->name() + QLatin1Char('.') + grandChild->name();
                if (child->type() == TestTreeItem::GTestCaseParameterized) {
                    testSpecifier.prepend(QLatin1String("*/"));
                    testSpecifier.append(QLatin1String("/*"));
                }
                enabled << testSpecifier;
            }
            proFilesWithEnabledTestSets.insert(proFile, enabled);
        }
    }

    foreach (const QString &proFile, proFilesWithEnabledTestSets.keys()) {
        TestConfiguration *tc = new TestConfiguration(QString(),
                                                      proFilesWithEnabledTestSets.value(proFile));
        tc->setTestType(TestTypeGTest);
        tc->setProFile(proFile);
        tc->setProject(project);
        result << tc;
    }

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
    case TestTreeItem::GTestCase:
    case TestTreeItem::GTestCaseParameterized: {
        QString testSpecifier = item->name() + QLatin1String(".*");
        if (item->type() == TestTreeItem::GTestCaseParameterized)
            testSpecifier.prepend(QLatin1String("*/"));

        if (int childCount = item->childCount()) {
            config = new TestConfiguration(QString(), QStringList(testSpecifier));
            config->setTestCaseCount(childCount);
            config->setProFile(item->childItem(0)->mainFile());
            config->setProject(project);
            config->setTestType(TestTypeGTest);
        }
        break;
    }
    case TestTreeItem::GTestName: {
        const TestTreeItem *parent = item->parentItem();
        QString testSpecifier = parent->name() + QLatin1Char('.') + item->name();

        if (parent->type() == TestTreeItem::GTestCaseParameterized) {
            testSpecifier.prepend(QLatin1String("*/"));
            testSpecifier.append(QLatin1String("/*"));
        }
        config = new TestConfiguration(QString(), QStringList(testSpecifier));
        config->setProFile(item->mainFile());
        config->setProject(project);
        config->setTestType(TestTypeGTest);
        break;
    }
    // not supported items
    default:
        return 0;
    }
    return config;
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
            if (child->markedForRemoval())
                continue;
            // Qt + named Quick Tests
            if (child->filePath() == filePath || child->referencingFile() == filePath) {
                child->markForRemovalRecursively(true);
            } else {
                // unnamed Quick Tests and GTest and Qt Tests with separated source/header
                int grandChildRow = child->childCount() - 1;
                for ( ; grandChildRow >= 0; --grandChildRow) {
                    TestTreeItem *grandChild = child->childItem(grandChildRow);
                    if (grandChild->filePath() == filePath
                            || grandChild->referencingFile() == filePath) {
                        grandChild->markForRemovalRecursively(true);
                    }
                }
            }
        }
    }
}

void TestTreeModel::sweep()
{
    bool hasChanged = false;
    Type types[] = { AutoTest, QuickTest, GoogleTest };
    for (Type type : types) {
        TestTreeItem *root = rootItemForType(type);
        hasChanged |= sweepChildren(root);
    }
    if (hasChanged)
        emit testTreeModelChanged();
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
        child->markForRemoval(false);
    }
    return hasChanged;
}

QMap<QString, QString> TestTreeModel::referencingFiles() const
{
    ReferencingFilesFinder finder;
    rootItem()->walkTree(&finder);
    return finder.referencingFiles();
}

TestTreeItem *TestTreeModel::findTestTreeItemByContent(TestTreeItem *item, TestTreeItem *parent,
                                                       Type type)
{
    for (int row = 0, count = parent->childCount(); row < count; ++row) {
        TestTreeItem *current = parent->childItem(row);
        if (current->name() != item->name())
            continue;

        switch (type) {
        case AutoTest:
            if (current->filePath() == item->filePath())
                return current;
            break;
        case QuickTest:
            if (current->filePath() == item->filePath() && current->mainFile() == item->mainFile())
                return current;
            break;
        case GoogleTest:
            if (current->type() == item->type())
                return current;
            break;
        }
    }
    return 0;
}

void TestTreeModel::addTestTreeItem(TestTreeItem *item, Type type)
{
    TestTreeItem *parent = rootItemForType(type);
    TestTreeItem *toBeUpdated = findTestTreeItemByContent(item, parent, type);
    const int count = item->childCount();
    if (toBeUpdated) {
        if (!toBeUpdated->markedForRemoval()) {
            for (int row = 0; row < count; ++row)
                toBeUpdated->appendChild(new TestTreeItem(*item->childItem(row)));
        } else {
            for (int childRow = count - 1; childRow >= 0; --childRow) {
                TestTreeItem *childItem = item->childItem(childRow);
                TestTreeItem *origChild = findTestTreeItemByContent(childItem, toBeUpdated, type);
                if (origChild) {
                    QModelIndex toBeModifiedIndex = indexForItem(origChild);
                    modifyTestSubtree(toBeModifiedIndex, childItem);
                } else {
                    toBeUpdated->insertChild(qMin(count, toBeUpdated->childCount()),
                                             new TestTreeItem(*childItem));
                }
            }
        }
        delete item;
    } else {
        parent->appendChild(item);
    }
    emit testTreeModelChanged();
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
    }
    QTC_ASSERT(false, return 0);
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
