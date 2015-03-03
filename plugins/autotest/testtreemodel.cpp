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

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <texteditor/texteditor.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QIcon>

namespace Autotest {
namespace Internal {

TestTreeModel::TestTreeModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_rootItem(new TestTreeItem(QString(), QString(), TestTreeItem::ROOT)),
    m_autoTestRootItem(new TestTreeItem(tr("Auto Tests"), QString(), TestTreeItem::ROOT, m_rootItem)),
    m_quickTestRootItem(new TestTreeItem(tr("Qt Quick Tests"), QString(), TestTreeItem::ROOT, m_rootItem)),
    m_parser(new TestCodeParser(this)),
    m_connectionsInitialized(false)
{
    m_rootItem->appendChild(m_autoTestRootItem);
    m_rootItem->appendChild(m_quickTestRootItem);

    connect(m_parser, &TestCodeParser::cacheCleared, this,
            &TestTreeModel::removeAllTestItems, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testItemCreated,
            this, &TestTreeModel::addTestTreeItem, Qt::QueuedConnection);
    connect(m_parser, &TestCodeParser::testItemsCreated,
            this, &TestTreeModel::addTestTreeItems, Qt::QueuedConnection);
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
    delete m_rootItem;
    m_instance = 0;
}

void TestTreeModel::enableParsing()
{
    m_parser->setState(TestCodeParser::Idle);
    if (m_connectionsInitialized)
        return;

    ProjectExplorer::SessionManager *sm = ProjectExplorer::SessionManager::instance();
    connect(sm, &ProjectExplorer::SessionManager::startupProjectChanged,
            m_parser, &TestCodeParser::emitUpdateTestTree);

    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    connect(cppMM, &CppTools::CppModelManager::documentUpdated,
            m_parser, &TestCodeParser::onCppDocumentUpdated, Qt::QueuedConnection);
    connect(cppMM, &CppTools::CppModelManager::aboutToRemoveFiles,
            m_parser, &TestCodeParser::removeFiles, Qt::QueuedConnection);

    QmlJS::ModelManagerInterface *qmlJsMM = QmlJS::ModelManagerInterface::instance();
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::documentUpdated,
            m_parser, &TestCodeParser::onQmlDocumentUpdated, Qt::QueuedConnection);
    connect(qmlJsMM, &QmlJS::ModelManagerInterface::aboutToRemoveFiles,
            m_parser, &TestCodeParser::removeFiles, Qt::QueuedConnection);
    m_connectionsInitialized = true;
}

void TestTreeModel::disableParsing()
{
    m_parser->setState(TestCodeParser::Disabled);
}

QModelIndex TestTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TestTreeItem *parentItem = parent.isValid()
            ? static_cast<TestTreeItem *>(parent.internalPointer())
            : m_rootItem;

    TestTreeItem *childItem = parentItem->child(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex TestTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TestTreeItem *childItem = static_cast<TestTreeItem *>(index.internalPointer());
    TestTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool TestTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;

    TestTreeItem *item = static_cast<TestTreeItem *>(parent.internalPointer());
    return item->childCount() > 0;
}

int TestTreeModel::rowCount(const QModelIndex &parent) const
{
    TestTreeItem *parentItem = parent.isValid()
            ? static_cast<TestTreeItem *>(parent.internalPointer()) : m_rootItem;
    return parentItem->childCount();
}

int TestTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[3] = {
        QIcon(),
        QIcon(QLatin1String(":/images/class.png")),
        QIcon(QLatin1String(":/images/func.png"))
    };
    if (type >= 3)
        return icons[2];
    return icons[type];
}

QVariant TestTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
    if (!item)
        return QVariant();

    if (role == Qt::DisplayRole) {
        if ((item == m_autoTestRootItem && m_autoTestRootItem->childCount() == 0)
                || (item == m_quickTestRootItem && m_quickTestRootItem->childCount() == 0)) {
            return QString(item->name() + tr(" (none)"));
        } else {
            if (item->name().isEmpty())
                return tr(Constants::UNNAMED_QUICKTESTS);
            return item->name();
        }

        return QVariant(); // TODO ?
    }
    switch(role) {
    case Qt::ToolTipRole:
        if (item->type() == TestTreeItem::TEST_CLASS && item->name().isEmpty())
            return tr("<p>Unnamed test cases can't be (un)checked - avoid this by assigning a name."
                      "<br/>Having unnamed test cases invalidates the check state of named test "
                      "cases with the same main.cpp when executing selected tests.</p>");
        return item->filePath();
    case Qt::DecorationRole:
        return testTreeIcon(item->type());
    case Qt::CheckStateRole:
        switch (item->type()) {
        case TestTreeItem::ROOT:
        case TestTreeItem::TEST_DATAFUNCTION:
        case TestTreeItem::TEST_SPECIALFUNCTION:
            return QVariant();
        case TestTreeItem::TEST_CLASS:
            if (item->name().isEmpty())
                return QVariant();
            else
                return item->checked();
        case TestTreeItem::TEST_FUNCTION:
            if (TestTreeItem *parent = item->parent())
                return parent->name().isEmpty() ? QVariant() : item->checked();
            else
                return item->checked();
        default:
            return item->checked();
        }
    case LinkRole: {
        QVariant itemLink;
        TextEditor::TextEditorWidget::Link link(item->filePath(), item->line(), item->column());
        itemLink.setValue(link);
        return itemLink;
    }
    case ItalicRole:
        switch (item->type()) {
        case TestTreeItem::TEST_DATAFUNCTION:
        case TestTreeItem::TEST_SPECIALFUNCTION:
            return true;
        case TestTreeItem::TEST_CLASS:
            return item->name().isEmpty();
        case TestTreeItem::TEST_FUNCTION:
            if (TestTreeItem *parent = item->parent())
                return parent->name().isEmpty();
            else
                return false;
        default:
            return false;
        }
    }

    // TODO ?
    return QVariant();
}

bool TestTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (role == Qt::CheckStateRole) {
        TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
        Qt::CheckState old = item->checked();
        item->setChecked((Qt::CheckState)value.toInt());
        if (item->checked() != old) {
            switch(item->type()) {
            case TestTreeItem::TEST_CLASS:
                emit dataChanged(index, index);
                if (item->childCount() > 0) {
                    emit dataChanged(index.child(0, 0), index.child(item->childCount() - 1, 0));
                }
                break;
            case TestTreeItem::TEST_FUNCTION:
                emit dataChanged(index, index);
                emit dataChanged(index.parent(), index.parent());
                break;
            default: // avoid warning regarding unhandled enum member
                break;
            }
            return true;
        }
    }
    return false;
}

Qt::ItemFlags TestTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
    switch(item->type()) {
    case TestTreeItem::TEST_CLASS:
        if (item->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
    case TestTreeItem::TEST_FUNCTION:
        if (item->parent()->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    case TestTreeItem::ROOT:
        return Qt::ItemIsEnabled;
    case TestTreeItem::TEST_DATAFUNCTION:
    case TestTreeItem::TEST_SPECIALFUNCTION:
    default:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
}

bool TestTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!parent.isValid())
        return false;
    TestTreeItem *parentItem = static_cast<TestTreeItem *>(parent.internalPointer());
    if (!parentItem)
        return false;

    bool subItemsSuccess = true;
    bool itemSuccess = true;
    for (int i = row + count - 1;  i >= row; --i) {
        QModelIndex child = index(i, 0, parent);
        subItemsSuccess &= removeRows(0, rowCount(child), child);
        beginRemoveRows(parent, i, i);
        itemSuccess &= parentItem->removeChild(i);
        endRemoveRows();
    }
    return subItemsSuccess && itemSuccess;
}

bool TestTreeModel::hasTests() const
{
    return m_autoTestRootItem->childCount() > 0 || m_quickTestRootItem->childCount() > 0;
}

static void addProjectInformation(TestConfiguration *config, const QString &filePath)
{
    const ProjectExplorer::SessionManager *session = ProjectExplorer::SessionManager::instance();
    if (!session || !session->hasProjects())
        return;

    ProjectExplorer::Project *project = session->startupProject();
    if (!project)
        return;

    QString targetFile;
    QString targetName;
    QString workDir;
    QString proFile;
    QString displayName;
    ProjectExplorer::Project *targetProject = 0;
    Utils::Environment env;
    bool hasDesktopTarget = false;
    bool guessedRunConfiguration = false;
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    QList<CppTools::ProjectPart::Ptr> projParts = cppMM->projectInfo(project).projectParts();

    if (!projParts.empty()) {
        foreach (const CppTools::ProjectPart::Ptr &part, projParts) {
            foreach (const CppTools::ProjectFile currentFile, part->files) {
                if (currentFile.path == filePath) {
                    proFile = part->projectFile;
                    displayName = part->displayName;
                    targetProject = part->project;
                    break;
                }
            }
            if (!proFile.isEmpty()) // maybe better use a goto instead of the break above??
                break;
        }
    }

    if (project) {
        if (auto target = project->activeTarget()) {
            ProjectExplorer::BuildTargetInfoList appTargets = target->applicationTargets();
            foreach (const ProjectExplorer::BuildTargetInfo &bti, appTargets.list) {
                if (bti.isValid() && bti.projectFilePath.toString() == proFile) {
                    targetFile = Utils::HostOsInfo::withExecutableSuffix(bti.targetFilePath.toString());
                    targetName = bti.targetName;
                    break;
                }
            }

            QList<ProjectExplorer::RunConfiguration *> rcs = target->runConfigurations();
            foreach (ProjectExplorer::RunConfiguration *rc, rcs) {
                ProjectExplorer::LocalApplicationRunConfiguration *localRunConfiguration
                    = qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(rc);
                if (localRunConfiguration && localRunConfiguration->executable() == targetFile) {
                    hasDesktopTarget = true;
                    workDir = Utils::FileUtils::normalizePathName(
                                localRunConfiguration->workingDirectory());
                    ProjectExplorer::EnvironmentAspect *envAsp
                            = localRunConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
                    env = envAsp->environment();
                    break;
                }
            }

            // if we could not figure out the run configuration
            // try to use the run configuration of the parent project
            if (!hasDesktopTarget && targetProject && !targetFile.isEmpty()) {
                QList<ProjectExplorer::RunConfiguration *> rcs = target->runConfigurations();
                foreach (ProjectExplorer::RunConfiguration *rc, rcs) {
                    ProjectExplorer::LocalApplicationRunConfiguration *localRunConfiguration
                            = qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(rc);
                    if (localRunConfiguration) {
                        if (ProjectExplorer::ProjectNode *localRootProjectNode = targetProject->rootProjectNode()) {
                            QList<ProjectExplorer::FileNode *> localFileNodes = localRootProjectNode->fileNodes();
                            if (localFileNodes.size()) {
                                if (localFileNodes.at(0)->path()
                                        == targetProject->projectFilePath()) {
                                    hasDesktopTarget = true;
                                    workDir = Utils::FileUtils::normalizePathName(
                                                localRunConfiguration->workingDirectory());
                                    ProjectExplorer::EnvironmentAspect *environmentAspect
                                            = localRunConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
                                    env = environmentAspect->environment();
                                    guessedRunConfiguration = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (hasDesktopTarget) {
        config->setTargetFile(targetFile);
        config->setTargetName(targetName);
        config->setWorkingDirectory(workDir);
        config->setProFile(proFile);
        config->setEnvironment(env);
        config->setProject(project);
        config->setDisplayName(displayName);
        config->setGuessedConfiguration(guessedRunConfiguration);
    } else {
        config->setProFile(proFile);
        config->setDisplayName(displayName);
    }
}

QList<TestConfiguration *> TestTreeModel::getAllTestCases() const
{
    QList<TestConfiguration *> result;

    // get all Auto Tests
    for (int row = 0, count = m_autoTestRootItem->childCount(); row < count; ++row) {
        const TestTreeItem *child = m_autoTestRootItem->child(row);

        TestConfiguration *tc = new TestConfiguration(child->name(), QStringList(),
                                                      child->childCount());
        addProjectInformation(tc, child->filePath());
        result << tc;
    }

    // get all Quick Tests
    QMap<QString, int> foundMains;
    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row) {
        TestTreeItem *child = m_quickTestRootItem->child(row);
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            for (int childRow = 0, ccount = child->childCount(); childRow < ccount; ++ childRow) {
                const TestTreeItem *grandChild = child->child(childRow);
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
        addProjectInformation(tc, mainFile);
        result << tc;
    }

    return result;
}

QList<TestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<TestConfiguration *> result;
    TestConfiguration *testConfiguration = 0;

    for (int row = 0, count = m_autoTestRootItem->childCount(); row < count; ++row) {
        TestTreeItem *child = m_autoTestRootItem->child(row);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
            testConfiguration = new TestConfiguration(child->name(), QStringList(), child->childCount());
            addProjectInformation(testConfiguration, child->filePath());
            result << testConfiguration;
            continue;
        case Qt::PartiallyChecked:
        default:
            const QString childName = child->name();
            int grandChildCount = child->childCount();
            QStringList testCases;
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->child(grandChildRow);
                if (grandChild->checked() == Qt::Checked)
                    testCases << grandChild->name();
            }

            testConfiguration = new TestConfiguration(childName, testCases);
            addProjectInformation(testConfiguration, child->filePath());
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
            const TestTreeItem *grandChild = unnamed->child(childRow);
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
                addProjectInformation(testConfiguration, mainFile);
                foundMains.insert(mainFile, testConfiguration);
            }
        }
    }

    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row) {
        TestTreeItem *child = m_quickTestRootItem->child(row);
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
                const TestTreeItem *grandChild = child->child(grandChildRow);
                if (grandChild->checked() == Qt::Checked)
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
                addProjectInformation(tc, child->mainFile());
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

QString TestTreeModel::getMainFileForUnnamedQuickTest(const QString &qmlFile) const
{
    const TestTreeItem *unnamed = unnamedQuickTests();
    for (int row = 0, count = unnamed->childCount(); row < count; ++row) {
        const TestTreeItem *child = unnamed->child(row);
        if (qmlFile == child->filePath())
            return child->mainFile();
    }
    return QString();
}

void TestTreeModel::qmlFilesAndFunctionNamesForMainFile(const QString &mainFile,
                                                        QSet<QString> *filePaths,
                                                        QList<QString> *functionNames) const
{
    TestTreeItem *unnamed = unnamedQuickTests();
    if (!unnamed)
        return;
    for (int i = 0; i < unnamed->childCount(); ++i) {
        const TestTreeItem *child = unnamed->child(i);
        if (child->mainFile() == mainFile) {
            filePaths->insert(child->filePath());
            functionNames->append(child->name());
        }
    }
}

QList<QString> TestTreeModel::getUnnamedQuickTestFunctions() const
{
    TestTreeItem *unnamed = unnamedQuickTests();
    if (unnamed)
        return unnamed->getChildNames();
    return QList<QString>();
}

QSet<QString> TestTreeModel::qmlFilesForProFile(const QString &proFile) const
{
    QSet<QString> filePaths;
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    if (TestTreeItem *unnamed = unnamedQuickTests()) {
        for (int i = 0; i < unnamed->childCount(); ++i) {
            const TestTreeItem *child = unnamed->child(i);
            QList<CppTools::ProjectPart::Ptr> ppList = modelManager->projectPart(child->mainFile());
            if (ppList.size() && ppList.at(0)->projectFile == proFile)
                filePaths.insert(child->filePath());
        }
    }
    return filePaths;
}

bool TestTreeModel::hasUnnamedQuickTests() const
{
    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row)
        if (m_quickTestRootItem->child(row)->name().isEmpty())
            return true;
    return false;
}

TestTreeItem *TestTreeModel::unnamedQuickTests() const
{
    for (int row = 0, count = m_quickTestRootItem->childCount(); row < count; ++row) {
        TestTreeItem *child = m_quickTestRootItem->child(row);
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

    const QModelIndex unnamedQTIndex = index(1, 0).child(unnamedQT->row(), 0);
    for (int childRow = unnamedQT->childCount() - 1; childRow >= 0; --childRow) {
        const TestTreeItem *child = unnamedQT->child(childRow);
        if (filePath == child->filePath())
            removeRow(childRow, unnamedQTIndex);
    }

    if (unnamedQT->childCount() == 0)
        removeRow(unnamedQT->row(), unnamedQTIndex.parent());
    emit testTreeModelChanged();
}

void TestTreeModel::addTestTreeItem(const TestTreeItem &item, TestTreeModel::Type type)
{
    TestTreeItem *parent = rootItemForType(type);
    QModelIndex index = rootIndexForType(type);
    TestTreeItem *toBeAdded = new TestTreeItem(item);
    toBeAdded->setParent(parent);

    beginInsertRows(index, parent->childCount(), parent->childCount());
    parent->appendChild(toBeAdded);
    endInsertRows();
    emit testTreeModelChanged();
}

void TestTreeModel::addTestTreeItems(const QList<TestTreeItem> &itemList, TestTreeModel::Type type)
{
    TestTreeItem *parent = rootItemForType(type);
    QModelIndex index = rootIndexForType(type);

    beginInsertRows(index, parent->childCount(), parent->childCount() + itemList.size() - 1);
    foreach (const TestTreeItem &item, itemList) {
        TestTreeItem *toBeAdded = new TestTreeItem(item);
        toBeAdded->setParent(parent);
        parent->appendChild(toBeAdded);
    }
    endInsertRows();
    emit testTreeModelChanged();
}

void TestTreeModel::updateUnnamedQuickTest(const QString &fileName, const QString &mainFile,
                                           const QMap<QString, TestCodeLocationAndType> &functions)
{
    removeUnnamedQuickTests(fileName);
    TestTreeItem unnamed = hasUnnamedQuickTests()
            ? TestTreeItem(*unnamedQuickTests())
            : TestTreeItem(QString(), QString(), TestTreeItem::TEST_CLASS, rootItemForType(QuickTest));

    foreach (const QString &functionName, functions.keys()) {
        const TestCodeLocationAndType locationAndType = functions.value(functionName);
        TestTreeItem *testFunction = new TestTreeItem(functionName, locationAndType.m_fileName,
                                                      locationAndType.m_type, &unnamed);
        testFunction->setLine(locationAndType.m_line);
        testFunction->setColumn(locationAndType.m_column);
        testFunction->setMainFile(mainFile);
        unnamed.appendChild(testFunction);
    }
    if (hasUnnamedQuickTests())
        modifyTestTreeItem(unnamed, QuickTest, QString());
    else
        addTestTreeItem(unnamed, QuickTest);
}

void TestTreeModel::modifyTestTreeItem(TestTreeItem item, TestTreeModel::Type type, const QString &file)
{
    QModelIndex index = rootIndexForType(type);
    TestTreeItem *parent = rootItemForType(type);
    item.setParent(parent);
    if (file.isEmpty()) {
        if (TestTreeItem *unnamed = unnamedQuickTests()) {
            index = index.child(unnamed->row(), 0);
            modifyTestSubtree(index, item);
        }
    } else {
        for (int row = 0; row < parent->childCount(); ++row) {
            if (parent->child(row)->filePath() == file) {
                index = index.child(row, 0);
                modifyTestSubtree(index, item);
                break;
            }
        }
    }
}

void TestTreeModel::removeAllTestItems()
{
    beginResetModel();
    m_autoTestRootItem->removeChildren();
    m_quickTestRootItem->removeChildren();
    endResetModel();
    emit testTreeModelChanged();
}

void TestTreeModel::removeTestTreeItems(const QString &filePath, Type type)
{
    bool removed = false;
    const QModelIndex rootIndex = rootIndexForType(type);
    const int count = rowCount(rootIndex);
    for (int row = count - 1; row >= 0; --row) {
        const QModelIndex childIndex = rootIndex.child(row, 0);
        TestTreeItem *childItem = static_cast<TestTreeItem *>(childIndex.internalPointer());
        if (filePath == childItem->filePath())
            removed |= removeRow(row, rootIndex);
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

void TestTreeModel::modifyTestSubtree(QModelIndex &toBeModifiedIndex, const TestTreeItem &newItem)
{
    if (!toBeModifiedIndex.isValid())
        return;

    TestTreeItem *toBeModifiedItem = static_cast<TestTreeItem *>(toBeModifiedIndex.internalPointer());
    if (toBeModifiedItem->modifyContent(&newItem))
        emit dataChanged(toBeModifiedIndex, toBeModifiedIndex,
                         QVector<int>() << Qt::DisplayRole << Qt::ToolTipRole << LinkRole);

    // process sub-items as well...
    const int childCount = toBeModifiedItem->childCount();
    const int newChildCount = newItem.childCount();

    // for keeping the CheckState on modifications
    // TODO might still fail for duplicate entries
    QHash<QString, Qt::CheckState> checkStates;
    for (int row = 0; row < childCount; ++row) {
        const TestTreeItem *child = toBeModifiedItem->child(row);
        checkStates.insert(child->name(), child->checked());
    }

    if (childCount <= newChildCount) {
        processChildren(toBeModifiedIndex, newItem, childCount, checkStates);
        // add additional items
        for (int row = childCount; row < newChildCount; ++row) {
            TestTreeItem *newChild = newItem.child(row);
            TestTreeItem *toBeAdded = new TestTreeItem(*newChild);
            toBeAdded->setParent(toBeModifiedItem);
            if (checkStates.contains(toBeAdded->name())
                    && checkStates.value(toBeAdded->name()) != Qt::Checked)
                toBeAdded->setChecked(checkStates.value(toBeAdded->name()));
            beginInsertRows(toBeModifiedIndex, row, row);
            toBeModifiedItem->appendChild(toBeAdded);
            endInsertRows();
        }
    } else {
        processChildren(toBeModifiedIndex, newItem, newChildCount, checkStates);
        // remove rest of the items
        removeRows(newChildCount, childCount - newChildCount, toBeModifiedIndex);
    }
    emit testTreeModelChanged();
}

void TestTreeModel::processChildren(QModelIndex &parentIndex, const TestTreeItem &newItem,
                                    const int upperBound,
                                    const QHash<QString, Qt::CheckState> &checkStates)
{
    static QVector<int> modificationRoles = QVector<int>() << Qt::DisplayRole
                                                           << Qt::ToolTipRole << LinkRole;
    TestTreeItem *toBeModifiedItem = static_cast<TestTreeItem *>(parentIndex.internalPointer());
    for (int row = 0; row < upperBound; ++row) {
        QModelIndex child = parentIndex.child(row, 0);
        TestTreeItem *toBeModifiedChild = toBeModifiedItem->child(row);
        TestTreeItem *modifiedChild = newItem.child(row);
        if (toBeModifiedChild->modifyContent(modifiedChild))
            emit dataChanged(child, child, modificationRoles);
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
    if (leftItem->type() == TestTreeItem::ROOT)
        return left.row() > right.row();

    const QString leftVal = m_sourceModel->data(left).toString();
    const QString rightVal = m_sourceModel->data(right).toString();

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
    case TestTreeItem::TEST_DATAFUNCTION:
        return m_filterMode & ShowTestData;
    case TestTreeItem::TEST_SPECIALFUNCTION:
        return m_filterMode & ShowInitAndCleanup;
    default:
        return true;
    }
}

} // namespace Internal
} // namespace Autotest
