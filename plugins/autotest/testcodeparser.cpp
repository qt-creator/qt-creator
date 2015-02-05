/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "autotestconstants.h"
#include "testcodeparser.h"
#include "testinfo.h"
#include "testtreeitem.h"
#include "testtreemodel.h"
#include "testvisitor.h"

#include <coreplugin/progressmanager/progressmanager.h>

#include <cplusplus/LookupContext.h>
#include <cplusplus/TypeOfExpression.h>

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>

#include <projectexplorer/session.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdialect.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <utils/textfileformat.h>

namespace Autotest {
namespace Internal {

TestCodeParser::TestCodeParser(TestTreeModel *parent)
    : QObject(parent),
      m_model(parent),
      m_parserEnabled(true),
      m_pendingUpdate(false)
{
    // connect to ProgressManager to post-pone test parsing when CppModelManager is parsing
    auto progressManager = qobject_cast<Core::ProgressManager *>(Core::ProgressManager::instance());
    connect(progressManager, &Core::ProgressManager::taskStarted,
            this, &TestCodeParser::onTaskStarted);
    connect(progressManager, &Core::ProgressManager::allTasksFinished,
            this, &TestCodeParser::onAllTasksFinished);
}

TestCodeParser::~TestCodeParser()
{
    clearMaps();
}

void TestCodeParser::emitUpdateTestTree()
{
    QTimer::singleShot(1000, this, SLOT(updateTestTree()));
}

ProjectExplorer::Project *currentProject()
{
    const ProjectExplorer::SessionManager *session = ProjectExplorer::SessionManager::instance();
    if (!session || !session->hasProjects())
        return 0;
    return session->startupProject();
}

void TestCodeParser::updateTestTree()
{
    if (!m_parserEnabled) {
        m_pendingUpdate = true;
        qDebug() << "Skipped update due to running parser or pro file evaluate";
        return;
    }

    qDebug("updating TestTreeModel");

    clearMaps();
    m_model->removeAllAutoTests();
    m_model->removeAllQuickTests();

    if (ProjectExplorer::Project *project = currentProject()) {
        if (auto qmakeProject = qobject_cast<QmakeProjectManager::QmakeProject *>(project)) {
            connect(qmakeProject, &QmakeProjectManager::QmakeProject::proFilesEvaluated,
                    this, &TestCodeParser::onProFileEvaluated, Qt::UniqueConnection);
        }
    } else
        return;

    scanForTests();
    m_pendingUpdate = false;
}

/****** scan for QTest related stuff helpers ******/

static QByteArray getFileContent(QString filePath)
{
    QByteArray fileContent;
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CppTools::WorkingCopy wc = cppMM->workingCopy();
    if (wc.contains(filePath)) {
        fileContent = wc.source(filePath);
    } else {
        QString error;
        const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
        if (Utils::TextFileFormat::readFileUTF8(filePath, codec, &fileContent, &error)
                != Utils::TextFileFormat::ReadSuccess) {
            qDebug() << "Failed to read file" << filePath << ":" << error;
        }
    }
    return fileContent;
}

static bool includesQtTest(const CPlusPlus::Document::Ptr &doc,
                           const CppTools::CppModelManager *cppMM)
{
    static QString expectedHeaderPrefix
            = Utils::HostOsInfo::isMacHost()
            ? QLatin1String("QtTest.framework/Headers")
            : QLatin1String("QtTest");

    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (const CPlusPlus::Document::Include &inc, includes) {
        // TODO this short cut works only for #include <QtTest>
        // bad, as there could be much more different approaches
        if (inc.unresolvedFileName() == QLatin1String("QtTest")
                && inc.resolvedFileName().endsWith(
                    QString::fromLatin1("%1/QtTest").arg(expectedHeaderPrefix))) {
            return true;
        }
    }

    if (cppMM) {
        CPlusPlus::Snapshot snapshot = cppMM->snapshot();
        const QSet<QString> allIncludes = snapshot.allIncludesForDocument(doc->fileName());
        foreach (const QString &include, allIncludes) {

            if (include.endsWith(QString::fromLatin1("%1/qtest.h").arg(expectedHeaderPrefix))) {
                return true;
            }
        }
    }
    return false;
}

static bool includesQtQuickTest(const CPlusPlus::Document::Ptr &doc,
                                const CppTools::CppModelManager *cppMM)
{
    static QString expectedHeaderPrefix
            = Utils::HostOsInfo::isMacHost()
            ? QLatin1String("QtQuickTest.framework/Headers")
            : QLatin1String("QtQuickTest");

    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (const CPlusPlus::Document::Include &inc, includes) {
        if (inc.unresolvedFileName() == QLatin1String("QtQuickTest/quicktest.h")
                && inc.resolvedFileName().endsWith(
                    QString::fromLatin1("%1/quicktest.h").arg(expectedHeaderPrefix))) {
            return true;
        }
    }

    if (cppMM) {
        foreach (const QString &include, cppMM->snapshot().allIncludesForDocument(doc->fileName())) {
            if (include.endsWith(QString::fromLatin1("%1/quicktest.h").arg(expectedHeaderPrefix)))
                return true;
        }
    }
    return false;
}

static bool qtTestLibDefined(const CppTools::CppModelManager *cppMM,
                             const QString &fileName)
{
    const QList<CppTools::ProjectPart::Ptr> parts = cppMM->projectPart(fileName);
    if (parts.size() > 0)
        return parts.at(0)->projectDefines.contains("#define QT_TESTLIB_LIB 1");
    return false;
}

static QString quickTestSrcDir(const CppTools::CppModelManager *cppMM,
                               const QString &fileName)
{
    static const QByteArray qtsd(" QUICK_TEST_SOURCE_DIR ");
    const QList<CppTools::ProjectPart::Ptr> parts = cppMM->projectPart(fileName);
    if (parts.size() > 0) {
        QByteArray projDefines(parts.at(0)->projectDefines);
        foreach (const QByteArray &line, projDefines.split('\n')) {
            if (line.contains(qtsd)) {
                QByteArray result = line.mid(line.indexOf(qtsd) + qtsd.length());
                if (result.startsWith('"'))
                    result.remove(result.length() - 1, 1).remove(0, 1);
                if (result.startsWith("\\\""))
                    result.remove(result.length() - 2, 2).remove(0, 2);
                return QLatin1String(result);
            }
        }
    }
    return QString();
}

static QString testClass(const CppTools::CppModelManager *modelManager,
                         CPlusPlus::Document::Ptr &document)
{
    static const QByteArray qtTestMacros[] = {"QTEST_MAIN", "QTEST_APPLESS_MAIN", "QTEST_GUILESS_MAIN"};
    const QList<CPlusPlus::Document::MacroUse> macros = document->macroUses();

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (name == qtTestMacros[0] || name == qtTestMacros[1] || name == qtTestMacros[2]) {
            const CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(getFileContent(document->fileName())
                                 .mid(arg.bytesBegin(), arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    // check if one has used a self-defined macro or QTest::qExec() directly
    const CPlusPlus::Snapshot snapshot = modelManager->snapshot();
    const QByteArray fileContent = getFileContent(document->fileName());
    document = snapshot.preprocessedDocument(fileContent, document->fileName());
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    TestAstVisitor astVisitor(document);
    astVisitor.accept(ast);
    return astVisitor.className();
}

static QString quickTestName(const CPlusPlus::Document::Ptr &doc)
{
    static const QByteArray qtTestMacros[] = {"QUICK_TEST_MAIN", "QUICK_TEST_OPENGL_MAIN"};
    const QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (name == qtTestMacros[0] || name == qtTestMacros[1] || name == qtTestMacros[2]) {
            CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(getFileContent(doc->fileName())
                                 .mid(arg.bytesBegin(), arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    return QString();
}

static QList<QmlJS::Document::Ptr> scanDirectoryForQuickTestQmlFiles(const QString &srcDir)
{
    QStringList dirs(srcDir);
    QmlJS::ModelManagerInterface *qmlJsMM = QmlJSTools::Internal::ModelManager::instance();
    // make sure even files not listed in pro file are available inside the snapshot
    QFutureInterface<void> future;
    QmlJS::PathsAndLanguages paths;
    paths.maybeInsert(Utils::FileName::fromString(srcDir), QmlJS::Dialect::Qml);
    const bool dontEmitDocumentChanges = false;
    const bool notOnlyTheLib = false;
    QmlJS::ModelManagerInterface::importScan(future, qmlJsMM->workingCopy(), paths, qmlJsMM,
        dontEmitDocumentChanges, notOnlyTheLib);

    const QmlJS::Snapshot snapshot = QmlJSTools::Internal::ModelManager::instance()->snapshot();
    QDirIterator it(srcDir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi(it.fileInfo().canonicalFilePath());
        dirs << fi.filePath();
    }
    QList<QmlJS::Document::Ptr> foundDocs;

    foreach (const QString &path, dirs) {
        const QList<QmlJS::Document::Ptr> docs = snapshot.documentsInDirectory(path);
        foreach (const QmlJS::Document::Ptr &doc, docs) {
            const QString fileName(QFileInfo(doc->fileName()).fileName());
            if (fileName.startsWith(QLatin1String("tst_")) && fileName.endsWith(QLatin1String(".qml")))
                foundDocs << doc;
        }
    }

    return foundDocs;
}

static CPlusPlus::Document::Ptr declaringDocument(CPlusPlus::Document::Ptr doc,
                                                  const QString &testCaseName,
                                                  unsigned *line, unsigned *column)
{
    CPlusPlus::Document::Ptr declaringDoc = doc;
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CPlusPlus::TypeOfExpression typeOfExpr;
    typeOfExpr.init(doc, cppMM->snapshot());

    auto  lookupItems = typeOfExpr(testCaseName.toUtf8(), doc->globalNamespace());
    if (lookupItems.size()) {
        CPlusPlus::Class *toeClass = lookupItems.first().declaration()->asClass();
        if (toeClass) {
            const QString declFileName = QLatin1String(toeClass->fileId()->chars(),
                                                       toeClass->fileId()->size());
            declaringDoc = cppMM->snapshot().document(declFileName);
            *line = toeClass->line();
            *column = toeClass->column() - 1;
        }
    }
    return declaringDoc;
}

static TestTreeItem *constructTestTreeItem(const QString &fileName,
                                           const QString &mainFile,  // used for Quick Tests only
                                           const QString &testCaseName,
                                           int line, int column,
                                           const QMap<QString, TestCodeLocationAndType> functions,
                                           TestTreeItem *rootItem)
{
    TestTreeItem *treeItem = new TestTreeItem(testCaseName, fileName, TestTreeItem::TEST_CLASS, rootItem);
    treeItem->setMainFile(mainFile); // used for Quick Tests only
    treeItem->setLine(line);
    treeItem->setColumn(column);

    foreach (const QString &functionName, functions.keys()) {
        const TestCodeLocationAndType locationAndType = functions.value(functionName);
        TestTreeItem *treeItemChild = new TestTreeItem(functionName, locationAndType.m_fileName,
                                                       locationAndType.m_type, treeItem);
        treeItemChild->setLine(locationAndType.m_line);
        treeItemChild->setColumn(locationAndType.m_column);
        treeItem->appendChild(treeItemChild);
    }
    return treeItem;
}

/****** end of helpers ******/

void TestCodeParser::checkDocumentForTestCode(CPlusPlus::Document::Ptr document)
{
    const QString fileName = document->fileName();
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    QList<CppTools::ProjectPart::Ptr> projParts = modelManager->projectPart(fileName);
    if (projParts.size())
        if (!projParts.at(0)->selectedForBuilding) {
            removeTestsIfNecessary(fileName);
            return;
        }

    if (includesQtQuickTest(document, modelManager)) {
        handleQtQuickTest(document);
        return;
    }

    if (includesQtTest(document, modelManager) && qtTestLibDefined(modelManager, fileName)) {
        QString testCaseName(testClass(modelManager, document));
        if (!testCaseName.isEmpty()) {
            unsigned line = 0;
            unsigned column = 0;
            CPlusPlus::Document::Ptr declaringDoc = declaringDocument(document, testCaseName,
                                                                      &line, &column);
            if (declaringDoc.isNull())
                return;

            TestVisitor visitor(testCaseName);
            visitor.accept(declaringDoc->globalNamespace());
            const QMap<QString, TestCodeLocationAndType> testFunctions = visitor.privateSlots();

            const QModelIndex autoTestRootIndex = m_model->index(0, 0);
            TestTreeItem *autoTestRootItem = static_cast<TestTreeItem *>(autoTestRootIndex.internalPointer());

            TestTreeItem *ttItem = constructTestTreeItem(declaringDoc->fileName(), QString(),
                                                         testCaseName, line, column, testFunctions,
                                                         autoTestRootItem);
            updateModelAndCppDocMap(document, declaringDoc->fileName(), ttItem, autoTestRootItem);
        } else {
            // could not find the class to test, but QTest is included and QT_TESTLIB_LIB defined
            // maybe file is only a referenced file
            if (m_cppDocMap.contains(fileName)) {
                const TestInfo info = m_cppDocMap[fileName];
                CPlusPlus::Snapshot snapshot = modelManager->snapshot();
                if (snapshot.contains(info.referencingFile())) {
                    checkDocumentForTestCode(snapshot.find(info.referencingFile()).value());
                } else { // no referencing file too, so this test case is no more a test case
                    m_model->removeAutoTestSubtreeByFilePath(fileName);
                    m_cppDocMap.remove(fileName);
                }
            }
        }
    }
}

void TestCodeParser::handleQtQuickTest(CPlusPlus::Document::Ptr document)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    if (quickTestName(document).isEmpty())
        return;

    const QString fileName = document->fileName();
    const QString srcDir = quickTestSrcDir(modelManager, fileName);
    if (srcDir.isEmpty())
        return;

    const QModelIndex quickTestRootIndex = m_model->index(1, 0);
    TestTreeItem *quickTestRootItem = static_cast<TestTreeItem *>(quickTestRootIndex.internalPointer());

    const QList<QmlJS::Document::Ptr> qmlDocs = scanDirectoryForQuickTestQmlFiles(srcDir);
    foreach (const QmlJS::Document::Ptr &qmlJSDoc, qmlDocs) {
        QmlJS::AST::Node *ast = qmlJSDoc->ast();
        if (!ast) {
            qDebug() << "ast is zero pointer" << qmlJSDoc->fileName(); // should not happen
            continue;
        }
        TestQmlVisitor qmlVisitor(qmlJSDoc);
        QmlJS::AST::Node::accept(ast, &qmlVisitor);

        const QString testCaseName = qmlVisitor.testCaseName();
        const TestCodeLocationAndType tcLocationAndType = qmlVisitor.testCaseLocation();
        const QMap<QString, TestCodeLocationAndType> testFunctions = qmlVisitor.testFunctions();

        if (testCaseName.isEmpty()) {
            // remove found test functions before re-adding them
            removeUnnamedQuickTests(qmlJSDoc->fileName(), testFunctions.keys());
            // re-create TestTreeItem for unnamed Quick Tests
            recreateUnnamedQuickTest(testFunctions, fileName, quickTestRootItem);
            continue;
        } // end of handling test cases without name property

        // construct new/modified TestTreeItem
        TestTreeItem *testTreeItem
                = constructTestTreeItem(tcLocationAndType.m_fileName, fileName, testCaseName,
                                        tcLocationAndType.m_line, tcLocationAndType.m_column,
                                        testFunctions, quickTestRootItem);

        // update model and internal map
        const QmlJS::Document::Ptr qmlDoc =
                QmlJSTools::Internal::ModelManager::instance()->snapshot().document(tcLocationAndType.m_fileName);
        updateModelAndQuickDocMap(qmlDoc, qmlJSDoc->fileName(), fileName, testTreeItem,
                                  quickTestRootItem);
    }
}

void TestCodeParser::onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    ProjectExplorer::Project *project = currentProject();
    if (!project)
        return;
    const QString fileName = document->fileName();
    if (m_cppDocMap.contains(fileName)) {
        if (m_cppDocMap[fileName].revision() == document->revision()
                && m_cppDocMap[fileName].editorRevision() == document->editorRevision()) {
            qDebug("Skipped due revision equality"); // added to verify if this ever happens..
            return;
        }
    } else if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        return;
    }
    checkDocumentForTestCode(document);
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &document)
{
    ProjectExplorer::Project *project = currentProject();
    if (!project)
        return;
    const QString fileName = document->fileName();
    if (m_quickDocMap.contains(fileName)) {
        if ((int)m_quickDocMap[fileName].editorRevision() == document->editorRevision()) {
            qDebug("Skipped due revision equality (QML)"); // added to verify this ever happens....
            return;
        }
    } else if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        // what if the file is not listed inside the pro file, but will be used anyway?
        return;
    }
    const CPlusPlus::Snapshot snapshot = CppTools::CppModelManager::instance()->snapshot();
    if (m_quickDocMap.contains(fileName)
            && snapshot.contains(m_quickDocMap[fileName].referencingFile())) {
        checkDocumentForTestCode(snapshot.document(m_quickDocMap[fileName].referencingFile()));
    }
    if (!m_quickDocMap.contains(tr(Constants::UNNAMED_QUICKTESTS)))
        return;

    // special case of having unnamed TestCases
    TestTreeItem *unnamed = m_model->unnamedQuickTests();
    for (int row = 0, count = unnamed->childCount(); row < count; ++row) {
        const TestTreeItem *child = unnamed->child(row);
        if (fileName == child->filePath()) {
            if (snapshot.contains(child->mainFile()))
                checkDocumentForTestCode(snapshot.document(child->mainFile()));
            break;
        }
    }
}

void TestCodeParser::removeFiles(const QStringList &files)
{
    foreach (const QString &file, files)
        removeTestsIfNecessary(file);
}

void TestCodeParser::scanForTests(const QStringList &fileList)
{
    QStringList list;
    if (fileList.isEmpty()) {
        list = currentProject()->files(ProjectExplorer::Project::AllFiles);
    } else {
        list << fileList;
    }

    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snapshot = cppMM->snapshot();

    foreach (const QString &file, list) {
        if (snapshot.contains(file)) {
            CPlusPlus::Document::Ptr doc = snapshot.find(file).value();
            checkDocumentForTestCode(doc);
        }
    }
}

void TestCodeParser::clearMaps()
{
    m_cppDocMap.clear();
    m_quickDocMap.clear();
}

void TestCodeParser::removeTestsIfNecessary(const QString &fileName)
{
    // check if this file was listed before and remove if necessary (switched config,...)
    if (m_cppDocMap.contains(fileName)) {
        m_cppDocMap.remove(fileName);
        m_model->removeAutoTestSubtreeByFilePath(fileName);
    } else { // handle Qt Quick Tests
        QList<QString> toBeRemoved;
        foreach (const QString &file, m_quickDocMap.keys()) {
            if (file == fileName) {
                toBeRemoved.append(file);
                continue;
            }
            const TestInfo info = m_quickDocMap.value(file);
            if (info.referencingFile() == fileName)
                toBeRemoved.append(file);
        }
        foreach (const QString &file, toBeRemoved) {
            m_quickDocMap.remove(file);
            m_model->removeQuickTestSubtreeByFilePath(file);
        }
        // unnamed Quick Tests must be handled separately
        QSet<QString> filePaths;
        QList<QString> functionNames;
        if (TestTreeItem *unnamedQT = m_model->unnamedQuickTests()) {
            for (int i = 0; i < unnamedQT->childCount(); ++i) {
                const TestTreeItem *child = unnamedQT->child(i);
                if (child->mainFile() == fileName) {
                    filePaths.insert(child->filePath());
                    functionNames.append(child->name());
                }
            }
            foreach (const QString &file, filePaths)
                m_model->removeUnnamedQuickTests(file);
            // update info map
            TestInfo unnamedInfo = m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)];
            QStringList functions = unnamedInfo.testFunctions();
            foreach (const QString &func, functionNames)
                functions.removeOne(func);
            unnamedInfo.setTestFunctions(functions);
            if (functions.size() == 0)
                m_quickDocMap.remove(tr(Constants::UNNAMED_QUICKTESTS));
            else
                m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), unnamedInfo);
        }
    }
}

void TestCodeParser::removeTestsIfNecessaryByProFile(const QString &proFile)
{
    QList<QString> fList;
    foreach (const QString &fileName, m_cppDocMap.keys()) {
        if (m_cppDocMap[fileName].proFile() == proFile)
            fList.append(fileName);
    }
    foreach (const QString &fileName, fList) {
        m_cppDocMap.remove(fileName);
        m_model->removeAutoTestSubtreeByFilePath(fileName);
    }
    fList.clear();
    foreach (const QString &fileName, m_quickDocMap.keys()) {
       if (m_quickDocMap[fileName].proFile() == proFile)
           fList.append(fileName);
    }
    foreach (const QString &fileName, fList) {
        m_quickDocMap.remove(fileName);
        m_model->removeQuickTestSubtreeByFilePath(fileName);
    }
    // handle unnamed Quick Tests
    fList.clear(); // will now be re-used as function names storage
    QSet<QString> filePaths;
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    if (TestTreeItem *unnamedQT = m_model->unnamedQuickTests()) {
        for (int i = 0; i < unnamedQT->childCount(); ++i) {
            const TestTreeItem *child = unnamedQT->child(i);
            QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(child->mainFile());
            if (ppList.size() && ppList.at(0)->projectFile == proFile) {
                filePaths.insert(child->filePath());
                fList.append(child->name());
            }
        }
    }
    foreach (const QString &filePath, filePaths) {
        m_model->removeUnnamedQuickTests(filePath);
    }
    // update info map
    TestInfo unnamedInfo = m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)];
    QStringList functions = unnamedInfo.testFunctions();
    foreach (const QString &function, fList)
        functions.removeOne(function);
    unnamedInfo.setTestFunctions(functions);
    if (functions.size() == 0)
        m_quickDocMap.remove(tr(Constants::UNNAMED_QUICKTESTS));
    else
        m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), unnamedInfo);
}

void TestCodeParser::removeUnnamedQuickTests(const QString &fileName,
                                             const QStringList &testFunctions)
{
    // if this test case was named before remove it
    if (m_quickDocMap.contains(fileName)) {
        m_model->removeQuickTestSubtreeByFilePath(fileName);
        m_quickDocMap.remove(fileName);
    }

    if (m_model->unnamedQuickTests()) {
        // remove unnamed quick tests that are already found for this qml file
        if (m_model->removeUnnamedQuickTests(fileName)) {
            // make sure m_quickDocMap does not have a inconsistent state now
            TestInfo testInfo = m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)];
            QStringList testFunctionNames = testInfo.testFunctions();
            foreach (const QString &func, testFunctions)
                testFunctionNames.removeOne(func);
            testInfo.setTestFunctions(testFunctionNames);
            if (testFunctionNames.size() == 0)
                m_quickDocMap.remove(tr(Constants::UNNAMED_QUICKTESTS));
            else
                m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), testInfo);
        }
    }
}

void TestCodeParser::recreateUnnamedQuickTest(const QMap<QString, TestCodeLocationAndType> &testFunctions,
                                              const QString &mainFile, TestTreeItem *rootItem)
{
    TestTreeItem *testTreeItem = new TestTreeItem(QString(), QString(), TestTreeItem::TEST_CLASS,
                                                  rootItem);
    TestTreeItem *unnamedQTItem = m_model->unnamedQuickTests();
    // if there are still other unnamed Quick Tests re-parent them to the new
    if (unnamedQTItem) {
        for (int i = 0, count = unnamedQTItem->childCount(); i < count; ++i) {
            TestTreeItem *child = new TestTreeItem(*unnamedQTItem->child(i));
            child->setParent(testTreeItem);
            testTreeItem->appendChild(child);
        }
    }
    // add test functions of the current
    foreach (const QString &function, testFunctions.keys()) {
        const TestCodeLocationAndType locationAndType = testFunctions.value(function);
        TestTreeItem *testTreeFunction = new TestTreeItem(function, locationAndType.m_fileName,
                                                          locationAndType.m_type, testTreeItem);
        testTreeFunction->setLine(locationAndType.m_line);
        testTreeFunction->setColumn(locationAndType.m_column);
        testTreeFunction->setMainFile(mainFile);
        testTreeItem->appendChild(testTreeFunction);
    }

    TestInfo info = m_quickDocMap.contains(tr(Constants::UNNAMED_QUICKTESTS))
            ? m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)]
            : TestInfo(QString(), QStringList(), 666);
    QStringList originalFunctions(info.testFunctions());
    foreach (const QString &function, testFunctions.keys())
        originalFunctions.append(function);
    info.setTestFunctions(originalFunctions);

    if (unnamedQTItem) {
        m_model->modifyQuickTestSubtree(unnamedQTItem->row(), testTreeItem);
        delete testTreeItem;
    } else {
        m_model->addQuickTest(testTreeItem);
    }
    m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), info);
}

void TestCodeParser::onTaskStarted(Core::Id type)
{
    if (type != CppTools::Constants::TASK_INDEX
            && type != QmakeProjectManager::Constants::PROFILE_EVALUATE)
        return;
    m_parserEnabled = false;
}

void TestCodeParser::onAllTasksFinished(Core::Id type)
{
    if (type != CppTools::Constants::TASK_INDEX
            && type != QmakeProjectManager::Constants::PROFILE_EVALUATE)
        return;
    m_parserEnabled = true;
    if (m_pendingUpdate)
        updateTestTree();
}

void TestCodeParser::updateModelAndCppDocMap(CPlusPlus::Document::Ptr document,
                                             const QString &declFileName,
                                             TestTreeItem *testItem, TestTreeItem *rootItem)
{
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    const QString fileName = document->fileName();
    const QString testCaseName = testItem->name();
    QString proFile;
    const QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(fileName);
    if (ppList.size())
        proFile = ppList.at(0)->projectFile;

    if (m_cppDocMap.contains(fileName)) {
        QStringList files = QStringList() << fileName;
        if (fileName != declFileName)
            files << declFileName;
        foreach (const QString &file, files) {
            const bool setReferencingFile = (files.size() == 2 && file == declFileName);
            const int count = rootItem->childCount();
            for (int i = 0; i < count; ++i) {
                TestTreeItem *currentItem = rootItem->child(i);
                if (currentItem->filePath() == file) {
                    m_model->modifyAutoTestSubtree(i, testItem);
                    TestInfo testInfo(testCaseName, testItem->getChildNames(),
                                      document->revision(), document->editorRevision());
                    testInfo.setProfile(proFile);
                    if (setReferencingFile)
                        testInfo.setReferencingFile(fileName);
                    m_cppDocMap.insert(file, testInfo);
                    break;
                }
            }
        }
        delete testItem; // this item is no more needed as model updates the original with its content
    } else {
        m_model->addAutoTest(testItem);
        TestInfo ti(testCaseName, testItem->getChildNames(),
                    document->revision(), document->editorRevision());
        ti.setProfile(proFile);
        m_cppDocMap.insert(fileName, ti);
        if (declFileName != fileName) {
            ti.setReferencingFile(fileName);
            m_cppDocMap.insert(declFileName, ti);
        }
    }
}

void TestCodeParser::updateModelAndQuickDocMap(QmlJS::Document::Ptr qmlDoc, const QString &currentQmlJSFile,
                                               const QString &referencingFileName,
                                               TestTreeItem *testItem, TestTreeItem *rootItem)
{
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    const QString fileName = qmlDoc->fileName();
    QString proFile;
    QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(referencingFileName);
    if (ppList.size())
        proFile = ppList.at(0)->projectFile;

    if (m_quickDocMap.contains(fileName)) {
        for (int i = 0; i < rootItem->childCount(); ++i) {
            if (rootItem->child(i)->filePath() == fileName) {
                m_model->modifyQuickTestSubtree(i, testItem);
                TestInfo testInfo(testItem->name(), testItem->getChildNames(), 0, qmlDoc->editorRevision());
                testInfo.setReferencingFile(referencingFileName);
                testInfo.setProfile(proFile);
                m_quickDocMap.insert(fileName, testInfo);
                break;
            }
        }
        delete testItem; // this item is no more needed as model updates the original with its content
    } else {
        // if it was formerly unnamed remove the respective items
        removeUnnamedQuickTests(currentQmlJSFile, testItem->getChildNames());

        m_model->addQuickTest(testItem);
        TestInfo testInfo(testItem->name(), testItem->getChildNames(), 0, qmlDoc->editorRevision());
        testInfo.setReferencingFile(referencingFileName);
        testInfo.setProfile(proFile);
        m_quickDocMap.insert(testItem->filePath(), testInfo);
    }
}

void TestCodeParser::onProFileEvaluated()
{
    ProjectExplorer::Project *project = currentProject();
    if (!project)
        return;

    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QList<CppTools::ProjectPart::Ptr> pp = modelManager->projectInfo(project).projectParts();
    foreach (const CppTools::ProjectPart::Ptr &p, pp) {
        if (!p->selectedForBuilding)
            removeTestsIfNecessaryByProFile(p->projectFile);
        else {
            QStringList files;
            foreach (auto projectFile, p->files)
                files.append(projectFile.path);
            scanForTests(files);
        }
    }
}

} // namespace Internal
} // namespace Autotest
