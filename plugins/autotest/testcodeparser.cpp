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
#include "testinfo.h"
#include "testvisitor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
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

#include <utils/multitask.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>

#include <QFuture>
#include <QFutureInterface>

namespace Autotest {
namespace Internal {

TestCodeParser::TestCodeParser(TestTreeModel *parent)
    : QObject(parent),
      m_model(parent),
      m_parserEnabled(true),
      m_pendingUpdate(false),
      m_fullUpdatePostPoned(false),
      m_partialUpdatePostPoned(false),
      m_parserState(Idle)
{
    // connect to ProgressManager to post-pone test parsing when CppModelManager is parsing
    auto progressManager = qobject_cast<Core::ProgressManager *>(Core::ProgressManager::instance());
    connect(progressManager, &Core::ProgressManager::taskStarted,
            this, &TestCodeParser::onTaskStarted);
    connect(progressManager, &Core::ProgressManager::allTasksFinished,
            this, &TestCodeParser::onAllTasksFinished);
    connect(this, &TestCodeParser::partialParsingFinished,
            this, &TestCodeParser::onPartialParsingFinished);
}

TestCodeParser::~TestCodeParser()
{
    clearCache();
}

void TestCodeParser::setState(State state)
{
    m_parserState = state;
    if (m_parserState == Disabled) {
        m_pendingUpdate = m_fullUpdatePostPoned = m_partialUpdatePostPoned = false;
        m_postPonedFiles.clear();
    }
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
        return;
    }

    if (ProjectExplorer::Project *project = currentProject()) {
        if (auto qmakeProject = qobject_cast<QmakeProjectManager::QmakeProject *>(project)) {
            if (qmakeProject->asyncUpdateState() != QmakeProjectManager::QmakeProject::Base) {
                m_pendingUpdate = true;
                return;
            }
            connect(qmakeProject, &QmakeProjectManager::QmakeProject::proFilesEvaluated,
                    this, &TestCodeParser::onProFileEvaluated, Qt::UniqueConnection);
        }
    } else
        return;

    m_pendingUpdate = false;

    clearCache();
    emit cacheCleared();
    scanForTests();
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

static TestTreeItem constructTestTreeItem(const QString &fileName,
                                          const QString &mainFile,  // used for Quick Tests only
                                          const QString &testCaseName,
                                          int line, int column,
                                          const QMap<QString, TestCodeLocationAndType> functions)
{
    TestTreeItem treeItem(testCaseName, fileName, TestTreeItem::TEST_CLASS);
    treeItem.setMainFile(mainFile); // used for Quick Tests only
    treeItem.setLine(line);
    treeItem.setColumn(column);

    foreach (const QString &functionName, functions.keys()) {
        const TestCodeLocationAndType locationAndType = functions.value(functionName);
        TestTreeItem *treeItemChild = new TestTreeItem(functionName, locationAndType.m_fileName,
                                                       locationAndType.m_type, &treeItem);
        treeItemChild->setLine(locationAndType.m_line);
        treeItemChild->setColumn(locationAndType.m_column);
        treeItem.appendChild(treeItemChild);
    }
    return treeItem;
}

/****** end of helpers ******/

void performParse(QFutureInterface<void> &futureInterface, QStringList list,
                  TestCodeParser *testCodeParser)
{
    int progressValue = 0;
    futureInterface.setProgressRange(0, list.size());
    futureInterface.setProgressValue(progressValue);
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snapshot = cppMM->snapshot();

    foreach (const QString &file, list) {
        if (snapshot.contains(file)) {
            CPlusPlus::Document::Ptr doc = snapshot.find(file).value();
            futureInterface.setProgressValue(++progressValue);
            testCodeParser->checkDocumentForTestCode(doc);
        }
    }
    futureInterface.setProgressValue(list.size());
}

/****** threaded parsing stuff *******/
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

            TestTreeItem item = constructTestTreeItem(declaringDoc->fileName(), QString(),
                                                      testCaseName, line, column, testFunctions);
            updateModelAndCppDocMap(document, declaringDoc->fileName(), item);
            return;
        }
    }
    // could not find the class to test, or QTest is not included and QT_TESTLIB_LIB defined
    // maybe file is only a referenced file
    if (m_cppDocMap.contains(fileName)) {
        const TestInfo info = m_cppDocMap[fileName];
        CPlusPlus::Snapshot snapshot = modelManager->snapshot();
        if (snapshot.contains(info.referencingFile())) {
            checkDocumentForTestCode(snapshot.find(info.referencingFile()).value());
        } else { // no referencing file too, so this test case is no more a test case
            m_cppDocMap.remove(fileName);
            emit testItemsRemoved(fileName, TestTreeModel::AutoTest);
        }
    }
}

void TestCodeParser::handleQtQuickTest(CPlusPlus::Document::Ptr document)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    if (quickTestName(document).isEmpty())
        return;

    const QString cppFileName = document->fileName();
    const QString srcDir = quickTestSrcDir(modelManager, cppFileName);
    if (srcDir.isEmpty())
        return;

    const QList<QmlJS::Document::Ptr> qmlDocs = scanDirectoryForQuickTestQmlFiles(srcDir);
    foreach (const QmlJS::Document::Ptr &qmlJSDoc, qmlDocs) {
        QmlJS::AST::Node *ast = qmlJSDoc->ast();
        QTC_ASSERT(ast, continue);
        TestQmlVisitor qmlVisitor(qmlJSDoc);
        QmlJS::AST::Node::accept(ast, &qmlVisitor);

        const QString testCaseName = qmlVisitor.testCaseName();
        const TestCodeLocationAndType tcLocationAndType = qmlVisitor.testCaseLocation();
        const QMap<QString, TestCodeLocationAndType> testFunctions = qmlVisitor.testFunctions();

        if (testCaseName.isEmpty()) {
            updateUnnamedQuickTests(qmlJSDoc->fileName(), cppFileName, testFunctions);
            continue;
        } // end of handling test cases without name property

        // construct new/modified TestTreeItem
        TestTreeItem testTreeItem
                = constructTestTreeItem(tcLocationAndType.m_fileName, cppFileName, testCaseName,
                                        tcLocationAndType.m_line, tcLocationAndType.m_column,
                                        testFunctions);

        // update model and internal map
        updateModelAndQuickDocMap(qmlJSDoc, cppFileName, testTreeItem);
    }
}

void TestCodeParser::onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    if (!m_parserEnabled) {
        if (!m_pendingUpdate) {
            m_partialUpdatePostPoned = true;
            m_postPonedFiles.insert(document->fileName());
        }
        return;
    }

    ProjectExplorer::Project *project = currentProject();
    if (!project)
        return;
    const QString fileName = document->fileName();
    if (m_cppDocMap.contains(fileName)) {
        if (m_cppDocMap[fileName].revision() == document->revision()
                && m_cppDocMap[fileName].editorRevision() == document->editorRevision()) {
            return;
        }
    } else if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        return;
    }
    scanForTests(QStringList(fileName));
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &document)
{
    if (!m_parserEnabled) {
        if (!m_pendingUpdate) {
            m_partialUpdatePostPoned = true;
            m_postPonedFiles.insert(document->fileName());
        }
        return;
    }

    ProjectExplorer::Project *project = currentProject();
    if (!project)
        return;
    const QString fileName = document->fileName();
    if (m_quickDocMap.contains(fileName)) {
        if ((int)m_quickDocMap[fileName].editorRevision() == document->editorRevision()) {
            return;
        }
    } else if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        // what if the file is not listed inside the pro file, but will be used anyway?
        return;
    }
    const CPlusPlus::Snapshot snapshot = CppTools::CppModelManager::instance()->snapshot();
    if (m_quickDocMap.contains(fileName)
            && snapshot.contains(m_quickDocMap[fileName].referencingFile())) {
            if (!m_quickDocMap[fileName].referencingFile().isEmpty())
                scanForTests(QStringList(m_quickDocMap[fileName].referencingFile()));
    }
    if (m_unnamedQuickDocList.size() == 0)
        return;

    // special case of having unnamed TestCases
    const QString &mainFile = m_model->getMainFileForUnnamedQuickTest(fileName);
    if (!mainFile.isEmpty() && snapshot.contains(mainFile)) {
        scanForTests(QStringList(mainFile));
    }
}

void TestCodeParser::removeFiles(const QStringList &files)
{
    foreach (const QString &file, files)
        removeTestsIfNecessary(file);
}

bool TestCodeParser::postponed(const QStringList &fileList)
{
    switch (m_parserState) {
    case Idle:
        return false;
    case PartialParse:
        // partial is running, postponing a full parse
        if (fileList.isEmpty()) {
            m_partialUpdatePostPoned = false;
            m_postPonedFiles.clear();
            m_fullUpdatePostPoned = true;
        } else {
            // partial parse triggered, but full parse is postponed already, ignoring this
            if (m_fullUpdatePostPoned)
                return true;
            // partial parse triggered, postpone or add current files to already postponed partial
            foreach (const QString &file, fileList)
                    m_postPonedFiles.insert(file);
            m_partialUpdatePostPoned = true;
        }
        return true;
    case FullParse:
        // full parse is running, postponing another full parse
        if (fileList.isEmpty()) {
            m_partialUpdatePostPoned = false;
            m_postPonedFiles.clear();
            m_fullUpdatePostPoned = true;
        } else {
            // full parse already postponed, ignoring triggering a partial parse
            if (m_fullUpdatePostPoned) {
                return true;
            }
            // partial parse triggered, postpone or add current files to already postponed partial
            foreach (const QString &file, fileList)
                m_postPonedFiles.insert(file);
            m_partialUpdatePostPoned = true;
        }
        return true;
    case Disabled:
        qWarning("Checking for postponing but being disabled...");
        return false;
    }
    QTC_ASSERT(false, return false); // should not happen at all
}

void TestCodeParser::scanForTests(const QStringList &fileList)
{
    if (postponed(fileList))
        return;

    bool isFullParse = fileList.isEmpty();
    bool isSmallChange = !isFullParse && fileList.size() < 6;
    QStringList list;
    if (isFullParse) {
        list = currentProject()->files(ProjectExplorer::Project::AllFiles);
        if (list.isEmpty())
            return;
        m_parserState = FullParse;
    } else {
        list << fileList;
        m_parserState = PartialParse;
    }

    if (isSmallChange) { // no need to do this async or should we do this always async?
        CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
        CPlusPlus::Snapshot snapshot = cppMM->snapshot();
        foreach (const QString &file, list) {
            if (snapshot.contains(file)) {
                CPlusPlus::Document::Ptr doc = snapshot.find(file).value();
                checkDocumentForTestCode(doc);
            }
        }
        emit onFinished();
        return;
    }

    QFuture<void> future = QtConcurrent::run(&performParse, list, this);
    Core::FutureProgress *progress
            = Core::ProgressManager::addTask(future, isFullParse ? tr("Scanning for Tests")
                                                                 : tr("Updating Tests"),
                                             Autotest::Constants::TASK_PARSE);
    connect(progress, &Core::FutureProgress::finished,
            this, &TestCodeParser::onFinished);

    emit parsingStarted();
}

void TestCodeParser::clearCache()
{
    m_cppDocMap.clear();
    m_quickDocMap.clear();
    m_unnamedQuickDocList.clear();
}

void TestCodeParser::removeTestsIfNecessary(const QString &fileName)
{
    // check if this file was listed before and remove if necessary (switched config,...)
    if (m_cppDocMap.contains(fileName)) {
        m_cppDocMap.remove(fileName);
        emit testItemsRemoved(fileName, TestTreeModel::AutoTest);
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
            emit testItemsRemoved(file, TestTreeModel::QuickTest);
        }
        // unnamed Quick Tests must be handled separately
        removeUnnamedQuickTestsByName(fileName);

        QSet<QString> filePaths;
        QList<QString> functionNames;
        if (m_model->hasUnnamedQuickTests()) {
            m_model->qmlFilesAndFunctionNamesForMainFile(fileName, &filePaths, &functionNames);
            foreach (const QString &file, filePaths)
                emit testItemsRemoved(file, TestTreeModel::QuickTest);
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
        emit testItemsRemoved(fileName, TestTreeModel::AutoTest);
    }
    fList.clear();
    foreach (const QString &fileName, m_quickDocMap.keys()) {
       if (m_quickDocMap[fileName].proFile() == proFile)
           fList.append(fileName);
    }
    foreach (const QString &fileName, fList) {
        m_quickDocMap.remove(fileName);
        emit testItemsRemoved(fileName, TestTreeModel::QuickTest);
    }
    // handle unnamed Quick Tests
    const QSet<QString> &filePaths = m_model->qmlFilesForProFile(proFile);
    foreach (const QString &fileName, filePaths) {
        removeUnnamedQuickTestsByName(fileName);
        emit unnamedQuickTestsRemoved(fileName);
    }
}

void TestCodeParser::onTaskStarted(Core::Id type)
{
    if (type != CppTools::Constants::TASK_INDEX)
        return;
    m_parserEnabled = false;
}

void TestCodeParser::onAllTasksFinished(Core::Id type)
{
    // only CPP parsing is relevant as we trigger Qml parsing internally anyway
    if (type != CppTools::Constants::TASK_INDEX)
        return;
    m_parserEnabled = true;
    if (m_pendingUpdate)
        updateTestTree();
    else if (m_partialUpdatePostPoned) {
        m_partialUpdatePostPoned = false;
        QStringList tmp;
        foreach (const QString &file, m_postPonedFiles)
            tmp << file;
        m_postPonedFiles.clear();
        scanForTests(tmp);
    }
}

void TestCodeParser::onFinished()
{
    switch (m_parserState) {
    case PartialParse:
        m_parserState = Idle;
        emit partialParsingFinished();
        break;
    case FullParse:
        m_parserState = Idle;
        emit parsingFinished();
        break;
    default:
        qWarning("I should not be here...");
        break;
    }
}

void TestCodeParser::onPartialParsingFinished()
{
    QTC_ASSERT(m_fullUpdatePostPoned != m_partialUpdatePostPoned
            || ((m_fullUpdatePostPoned || m_partialUpdatePostPoned) == false),
               m_partialUpdatePostPoned = false;m_postPonedFiles.clear(););
    if (m_fullUpdatePostPoned) {
        m_fullUpdatePostPoned = false;
        updateTestTree();
    } else if (m_partialUpdatePostPoned) {
        m_partialUpdatePostPoned = false;
        QStringList tmp;
        foreach (const QString &file, m_postPonedFiles)
            tmp << file;
        m_postPonedFiles.clear();
        scanForTests(tmp);
    }
}

void TestCodeParser::updateUnnamedQuickTests(const QString &fileName, const QString &mainFile,
                                             const QMap<QString, TestCodeLocationAndType> &functions)
{
    // if this test case was named before remove it
    m_quickDocMap.remove(fileName);
    emit testItemsRemoved(fileName, TestTreeModel::QuickTest);

    removeUnnamedQuickTestsByName(fileName);
    foreach (const QString &functionName, functions.keys()) {
        UnnamedQuickTestInfo info(functionName, fileName);
        m_unnamedQuickDocList.append(info);
    }

    emit unnamedQuickTestsUpdated(fileName, mainFile, functions);
}

void TestCodeParser::updateModelAndCppDocMap(CPlusPlus::Document::Ptr document,
                                             const QString &declaringFile, TestTreeItem &testItem)
{
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    const QString fileName = document->fileName();
    const QString testCaseName = testItem.name();
    QString proFile;
    const QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(fileName);
    if (ppList.size())
        proFile = ppList.at(0)->projectFile;

    if (m_cppDocMap.contains(fileName)) {
        QStringList files = QStringList() << fileName;
        if (fileName != declaringFile)
            files << declaringFile;
        foreach (const QString &file, files) {
            const bool setReferencingFile = (files.size() == 2 && file == declaringFile);
            emit testItemModified(testItem, TestTreeModel::AutoTest, file);
            TestInfo testInfo(testCaseName, testItem.getChildNames(),
                              document->revision(), document->editorRevision());
            testInfo.setProfile(proFile);
            if (setReferencingFile)
                testInfo.setReferencingFile(fileName);
            m_cppDocMap.insert(file, testInfo);
        }
    } else {
        emit testItemCreated(testItem, TestTreeModel::AutoTest);
        TestInfo ti(testCaseName, testItem.getChildNames(),
                    document->revision(), document->editorRevision());
        ti.setProfile(proFile);
        m_cppDocMap.insert(fileName, ti);
        if (declaringFile != fileName) {
            ti.setReferencingFile(fileName);
            m_cppDocMap.insert(declaringFile, ti);
        }
    }
}

void TestCodeParser::updateModelAndQuickDocMap(QmlJS::Document::Ptr document,
                                               const QString &referencingFile,
                                               TestTreeItem &testItem)
{
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    const QString fileName = document->fileName();
    QString proFile;
    QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(referencingFile);
    if (ppList.size())
        proFile = ppList.at(0)->projectFile;

    if (m_quickDocMap.contains(fileName)) {
        emit testItemModified(testItem, TestTreeModel::QuickTest, fileName);
        TestInfo testInfo(testItem.name(), testItem.getChildNames(), 0, document->editorRevision());
        testInfo.setReferencingFile(referencingFile);
        testInfo.setProfile(proFile);
        m_quickDocMap.insert(fileName, testInfo);
    } else {
        // if it was formerly unnamed remove the respective items
        removeUnnamedQuickTestsByName(fileName);
        emit unnamedQuickTestsRemoved(fileName);

        emit testItemCreated(testItem, TestTreeModel::QuickTest);
        TestInfo testInfo(testItem.name(), testItem.getChildNames(), 0, document->editorRevision());
        testInfo.setReferencingFile(referencingFile);
        testInfo.setProfile(proFile);
        m_quickDocMap.insert(testItem.filePath(), testInfo);
    }
}

void TestCodeParser::removeUnnamedQuickTestsByName(const QString &fileName)
{
    for (int i = m_unnamedQuickDocList.size() - 1; i >= 0; --i) {
        if (m_unnamedQuickDocList.at(i).fileName() == fileName)
            m_unnamedQuickDocList.removeAt(i);
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

#ifdef WITH_TESTS
int TestCodeParser::autoTestsCount() const
{
    int count = 0;
    foreach (const QString &file, m_cppDocMap.keys()) {
        if (m_cppDocMap.value(file).referencingFile().isEmpty())
            ++count;
    }
    return count;
}

int TestCodeParser::namedQuickTestsCount() const
{
    return m_quickDocMap.size();
}

int TestCodeParser::unnamedQuickTestsCount() const
{
    return m_unnamedQuickDocList.size();
}
#endif

} // namespace Internal
} // namespace Autotest
