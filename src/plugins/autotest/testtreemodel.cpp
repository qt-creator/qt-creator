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
#include <cpptools/projectpart.h>

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

static QString getCMakeDisplayNameIfNecessary(const QString &filePath, const QString &proFile)
{
    static const QString CMAKE_LISTS = QLatin1String("CMakeLists.txt");
    if (!proFile.endsWith(CMAKE_LISTS))
        return QString();

    const QList<CppTools::ProjectPart::Ptr> &projectParts
            = CppTools::CppModelManager::instance()->projectPart(filePath);
    if (projectParts.size())
        return projectParts.first()->displayName;

    return QString();
}

// used as key inside getAllTestCases()/getSelectedTestCases() for Google Tests
class ProFileWithDisplayName
{
public:
    ProFileWithDisplayName(const QString &file, const QString &name)
        : proFile(file), displayName(name) {}
    QString proFile;
    QString displayName;

    bool operator==(const ProFileWithDisplayName &rhs) const
    {
        return proFile == rhs.proFile && displayName == rhs.displayName;
    }
};

// needed as ProFileWithDisplayName is used as key inside a QHash
bool operator<(const ProFileWithDisplayName &lhs, const ProFileWithDisplayName &rhs)
{
    return lhs.proFile == rhs.proFile ? lhs.displayName < rhs.displayName
                                      : lhs.proFile < rhs.proFile;
}

// needed as ProFileWithDisplayName is used as a key inside a QHash
uint qHash(const ProFileWithDisplayName &lhs)
{
    return ::qHash(lhs.proFile) ^ ::qHash(lhs.displayName);
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
        tc->setProFile(child->proFile());
        tc->setProject(project);
        tc->setDisplayName(getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
        result << tc;
    }

    // get all Quick Tests
    QHash<QString, int> foundProFiles;
    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_quickTestRootItem->childItem(row);
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            for (int childRow = 0, ccount = child->childCount(); childRow < ccount; ++ childRow) {
                const TestTreeItem *grandChild = child->childItem(childRow);
                const QString &proFile = grandChild->proFile();
                foundProFiles.insert(proFile, foundProFiles[proFile] + 1);
            }
            continue;
        }
        // named Quick Test
        const QString &proFile = child->proFile();
        foundProFiles.insert(proFile, foundProFiles[proFile] + child->childCount());
    }
    // create TestConfiguration for each project file
    {
        QHash<QString, int>::ConstIterator it = foundProFiles.begin();
        QHash<QString, int>::ConstIterator end = foundProFiles.end();
        for ( ; it != end; ++it) {
            TestConfiguration *tc = new TestConfiguration(QString(), QStringList(), it.value());
            tc->setProFile(it.key());
            tc->setProject(project);
            result << tc;
        }
    }

    foundProFiles.clear();

    // get all Google Tests
    QHash<ProFileWithDisplayName, int> proFilesWithTestSets;
    for (int row = 0, count = m_googleTestRootItem->childCount(); row < count; ++row) {
        const GoogleTestTreeItem *child = m_googleTestRootItem->childItem(row)->asGoogleTestTreeItem();

        const int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                                           getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                                          grandChild->proFile()));

                proFilesWithTestSets.insert(key, proFilesWithTestSets[key] + 1);
            }
        }
    }

    {
        QHash<ProFileWithDisplayName, int>::ConstIterator it = proFilesWithTestSets.begin();
        QHash<ProFileWithDisplayName, int>::ConstIterator end = proFilesWithTestSets.end();
        for ( ; it != end; ++it) {
            const ProFileWithDisplayName &key = it.key();
            TestConfiguration *tc = new TestConfiguration(QString(), QStringList(), it.value());
            tc->setTestType(TestTypeGTest);
            tc->setProFile(key.proFile);
            tc->setDisplayName(key.displayName);
            tc->setProject(project);
            result << tc;
        }
    }

    return result;
}

static QString gtestFilter(GoogleTestTreeItem::TestStates states)
{
    if ((states & GoogleTestTreeItem::Parameterized) && (states & GoogleTestTreeItem::Typed))
        return QLatin1String("*/%1/*.%2");
    if (states & GoogleTestTreeItem::Parameterized)
        return QLatin1String("*/%1.%2/*");
    if (states & GoogleTestTreeItem::Typed)
        return QLatin1String("%1/*.%2");
    return QLatin1String("%1.%2");
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
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(getCMakeDisplayNameIfNecessary(child->filePath(),
                                                                             child->proFile()));
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
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(getCMakeDisplayNameIfNecessary(child->filePath(),
                                                                             child->proFile()));
            result << testConfiguration;
        }
    }
    // Quick Tests must be handled differently - need the calling cpp file to use this in
    // addProjectInformation() - additionally this must be unique to not execute the same executable
    // on and on and on...
    // TODO: do this later on for Auto Tests as well to support strange setups? or redo the model

    QHash<QString, TestConfiguration *> foundProFiles;

    if (TestTreeItem *unnamed = unnamedQuickTests()) {
        for (int childRow = 0, ccount = unnamed->childCount(); childRow < ccount; ++ childRow) {
            const TestTreeItem *grandChild = unnamed->childItem(childRow);
            const QString &proFile = grandChild->proFile();
            if (foundProFiles.contains(proFile)) {
                QTC_ASSERT(testConfiguration,
                           qWarning() << "Illegal state (unnamed Quick Test listed as named)";
                           return QList<TestConfiguration *>());
                foundProFiles[proFile]->setTestCaseCount(testConfiguration->testCaseCount() + 1);
            } else {
                testConfiguration = new TestConfiguration(QString(), QStringList());
                testConfiguration->setTestCaseCount(1);
                testConfiguration->setUnnamedOnly(true);
                testConfiguration->setProFile(proFile);
                testConfiguration->setProject(project);
                foundProFiles.insert(proFile, testConfiguration);
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
                if (grandChild->type() != TestTreeItem::TestFunctionOrSet)
                    continue;
                testFunctions << child->name() + QLatin1String("::") + grandChild->name();
            }
            TestConfiguration *tc;
            if (foundProFiles.contains(child->proFile())) {
                tc = foundProFiles[child->proFile()];
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
                tc->setProFile(child->proFile());
                tc->setProject(project);
                foundProFiles.insert(child->proFile(), tc);
            }
            break;
        }
    }

    {
        QHash<QString, TestConfiguration *>::ConstIterator it = foundProFiles.begin();
        QHash<QString, TestConfiguration *>::ConstIterator end = foundProFiles.end();
        for ( ; it != end; ++it) {
            TestConfiguration *config = it.value();
            if (!config->unnamedOnly())
                result << config;
            else
                delete config;
        }
    }

    // get selected Google Tests
    QHash<ProFileWithDisplayName, QStringList> proFilesWithCheckedTestSets;
    for (int row = 0, count = m_googleTestRootItem->childCount(); row < count; ++row) {
        const auto child = m_googleTestRootItem->childItem(row)->asGoogleTestTreeItem();
        if (child->checked() == Qt::Unchecked)
            continue;

        int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                                           getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                                          grandChild->proFile()));

                proFilesWithCheckedTestSets[key].append(
                            gtestFilter(child->state()).arg(child->name()).arg(grandChild->name()));
            }
        }
    }

    {
        QHash<ProFileWithDisplayName, QStringList>::ConstIterator it = proFilesWithCheckedTestSets.begin();
        QHash<ProFileWithDisplayName, QStringList>::ConstIterator end = proFilesWithCheckedTestSets.end();
        for ( ; it != end; ++it) {
            const ProFileWithDisplayName &key = it.key();
            TestConfiguration *tc = new TestConfiguration(QString(), it.value());
            tc->setTestType(TestTypeGTest);
            tc->setProFile(key.proFile);
            tc->setDisplayName(key.displayName);
            tc->setProject(project);
            result << tc;
        }
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
    case TestTreeItem::TestCase: {
        if (item->asQuickTestTreeItem()) {
            // Quick Test TestCase
            QStringList testFunctions;
            for (int row = 0, count = item->childCount(); row < count; ++row) {
                    testFunctions << item->name() + QLatin1String("::")
                                     + item->childItem(row)->name();
            }
            config = new TestConfiguration(QString(), testFunctions);
            config->setProFile(item->proFile());
            config->setProject(project);
        } else if (item->asAutoTestTreeItem()) {
            // normal auto test
            config = new TestConfiguration(item->name(), QStringList(), item->childCount());
            config->setProFile(item->proFile());
            config->setProject(project);
            config->setDisplayName(getCMakeDisplayNameIfNecessary(item->filePath(),
                                                                  item->proFile()));
        } else if (auto gtestItem = item->asGoogleTestTreeItem()) {
            const QString &testSpecifier
                    = gtestFilter(gtestItem->state()).arg(item->name()).arg(QLatin1Char('*'));

            if (int childCount = item->childCount()) {
                config = new TestConfiguration(QString(), QStringList(testSpecifier));
                config->setTestCaseCount(childCount);
                config->setProFile(item->proFile());
                config->setProject(project);
                // item has no filePath set - so take it of the first children
                config->setDisplayName(getCMakeDisplayNameIfNecessary(
                                           item->childItem(0)->filePath(), item->proFile()));
                config->setTestType(TestTypeGTest);
            }
        }
        break;
    }
    case TestTreeItem::TestFunctionOrSet: {
        TestTreeItem *parent = item->parentItem();
        if (parent->asQuickTestTreeItem()) {
            // it's a Quick Test function of a named TestCase
            QStringList testFunction(parent->name() + QLatin1String("::") + item->name());
            config = new TestConfiguration(QString(), testFunction);
            config->setProFile(parent->proFile());
            config->setProject(project);
        } else if (parent->asAutoTestTreeItem()){
            // normal auto test
            config = new TestConfiguration(parent->name(), QStringList() << item->name());
            config->setProFile(parent->proFile());
            config->setProject(project);
            config->setDisplayName(getCMakeDisplayNameIfNecessary(item->filePath(),
                                                                  parent->proFile()));
        } else if (auto gtestParent = parent->asGoogleTestTreeItem()) {
            const QString &testSpecifier
                    = gtestFilter(gtestParent->state()).arg(parent->name()).arg(item->name());

            config = new TestConfiguration(QString(), QStringList(testSpecifier));
            config->setProFile(item->proFile());
            config->setProject(project);
            config->setDisplayName(getCMakeDisplayNameIfNecessary(item->filePath(),
                                                                  parent->proFile()));
            config->setTestType(TestTypeGTest);
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
        config->setProFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(getCMakeDisplayNameIfNecessary(item->filePath(), parent->proFile()));
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

void TestTreeModel::onParseResultReady(const TestParseResult &result)
{
    switch (result.type) {
    case AutoTest:
        handleParseResult(result);
        break;
    case QuickTest:
        if (result.testCaseName.isEmpty()) {
            handleUnnamedQuickParseResult(result);
            break;
        }
        handleParseResult(result);
        break;
    case GoogleTest:
        QTC_ASSERT(result.dataTagsOrTestSets.size() == 1, return);
        handleGTestParseResult(result);
        break;
    case Invalid:
        QTC_ASSERT(false, qWarning("TestParseResult of type Invalid unexpected."));
        break;
    }
}

void TestTreeModel::handleParseResult(const TestParseResult &result)
{
    TestTreeItem *root;
    switch (result.type) {
    case AutoTest:
        root = m_autoTestRootItem;
        break;
    case QuickTest:
        root = m_quickTestRootItem;
        break;
    default:
        QTC_ASSERT(false, return); // should never happen, just to avoid warning
    }

    TestTreeItem *toBeModified = root->findChildByFile(result.fileName);
    // if there's no matching item, add the new one
    if (!toBeModified) {
        if (result.type == AutoTest)
            root->appendChild(AutoTestTreeItem::createTestItem(result));
        else
            root->appendChild(QuickTestTreeItem::createTestItem(result));
        return;
    }
    // else we have to check level by level.. first the current level...
    bool changed = toBeModified->modifyTestCaseContent(result.testCaseName, result.line,
                                                       result.column);
    toBeModified->markForRemoval(false);
    if (changed)
        emit dataChanged(indexForItem(toBeModified), indexForItem(toBeModified));
    // ...now the functions
    foreach (const QString &func, result.functions.keys()) {
        TestTreeItem *functionItem = toBeModified->findChildByName(func);
        // if there's no function matching, add the new one
        if (!functionItem) {
            const QString qualifiedName = result.testCaseName + QLatin1String("::") + func;
            if (result.type == AutoTest) {
                toBeModified->appendChild(AutoTestTreeItem::createFunctionItem(
                        func, result.functions.value(func),
                        result.dataTagsOrTestSets.value(qualifiedName)));
            } else {
                toBeModified->appendChild(QuickTestTreeItem::createFunctionItem(
                        func, result.functions.value(func)));
            }
            continue;
        }
        // else we have to check level by level.. first the current level...
        changed = functionItem->modifyTestFunctionContent(result.functions.value(func));
        functionItem->markForRemoval(false);
        if (changed)
            emit dataChanged(indexForItem(functionItem), indexForItem(functionItem));
        // ...now the data tags - actually these are supported only for AutoTestTreeItem
        const QString &funcFileName = result.functions.value(func).m_name;
        const QString qualifiedFunctionName = result.testCaseName + QLatin1String("::") + func;
        foreach (const TestCodeLocationAndType &location, result.dataTagsOrTestSets.value(qualifiedFunctionName)) {
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

void TestTreeModel::handleUnnamedQuickParseResult(const TestParseResult &result)
{
    TestTreeItem *toBeModified = unnamedQuickTests();
    if (!toBeModified) {
        m_quickTestRootItem->appendChild(QuickTestTreeItem::createUnnamedQuickTestItem(result));
        return;
    }
    // if we have already Unnamed Quick tests we might update them..
    foreach (const QString &func, result.functions.keys()) {
        const TestCodeLocationAndType &location = result.functions.value(func);
        TestTreeItem *functionItem = toBeModified->findChildByNameAndFile(func, location.m_name);
        if (!functionItem) {
            toBeModified->appendChild(QuickTestTreeItem::createUnnamedQuickFunctionItem(
                                          func, result));
            continue;
        }
        functionItem->modifyLineAndColumn(location);
        functionItem->markForRemoval(false);
    }
}

void TestTreeModel::handleGTestParseResult(const TestParseResult &result)
{
    GoogleTestTreeItem::TestStates states = GoogleTestTreeItem::Enabled;
    if (result.parameterized)
        states |= GoogleTestTreeItem::Parameterized;
    if (result.typed)
        states |= GoogleTestTreeItem::Typed;
    TestTreeItem *toBeModified = m_googleTestRootItem->findChildByNameStateAndFile(
                result.testCaseName, states, result.proFile);
    if (!toBeModified) {
        m_googleTestRootItem->appendChild(GoogleTestTreeItem::createTestItem(result));
        return;
    }
    // if found nothing has to be updated as all relevant members are used to find the item
    foreach (const TestCodeLocationAndType &location , result.dataTagsOrTestSets.first()) {
        TestTreeItem *testSetItem = toBeModified->findChildByNameAndFile(location.m_name,
                                                                         result.fileName);
        if (!testSetItem) {
            toBeModified->appendChild(GoogleTestTreeItem::createTestSetItem(result, location));
            continue;
        }
        bool changed = testSetItem->asGoogleTestTreeItem()->modifyTestSetContent(
                    result.fileName, location);
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
