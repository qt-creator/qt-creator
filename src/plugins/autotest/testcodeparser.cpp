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
#include "autotest_utils.h"
#include "testcodeparser.h"
#include "testvisitor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cplusplus/LookupContext.h>
#include <cplusplus/TypeOfExpression.h>

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdialect.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <utils/multitask.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>

#include <QDirIterator>
#include <QFuture>
#include <QFutureInterface>
#include <QLoggingCategory>
#include <QTimer>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.testcodeparser")

namespace Autotest {
namespace Internal {

TestCodeParser::TestCodeParser(TestTreeModel *parent)
    : QObject(parent),
      m_model(parent),
      m_codeModelParsing(false),
      m_fullUpdatePostponed(false),
      m_partialUpdatePostponed(false),
      m_dirty(false),
      m_singleShotScheduled(false),
      m_parserState(Disabled)
{
    // connect to ProgressManager to postpone test parsing when CppModelManager is parsing
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
}

void TestCodeParser::setState(State state)
{
    qCDebug(LOG) << "setState(" << state << "), currentState:" << m_parserState;
    // avoid triggering parse before code model parsing has finished, but mark as dirty
    if (m_codeModelParsing) {
        m_dirty = true;
        qCDebug(LOG) << "Not setting new state - code model parsing is running, just marking dirty";
        return;
    }

    if ((state == Disabled || state == Idle)
            && (m_parserState == PartialParse || m_parserState == FullParse)) {
        qCDebug(LOG) << "Not setting state, parse is running";
        return;
    }
    m_parserState = state;

    if (m_parserState == Disabled) {
        m_fullUpdatePostponed = m_partialUpdatePostponed = false;
        m_postponedFiles.clear();
    } else if (m_parserState == Idle && ProjectExplorer::SessionManager::startupProject()) {
        if (m_fullUpdatePostponed || m_dirty) {
            emitUpdateTestTree();
        } else if (m_partialUpdatePostponed) {
            m_partialUpdatePostponed = false;
            qCDebug(LOG) << "calling scanForTests with postponed files (setState)";
            scanForTests(m_postponedFiles.toList());
        }
    }
}

void TestCodeParser::emitUpdateTestTree()
{
    if (m_singleShotScheduled) {
        qCDebug(LOG) << "not scheduling another updateTestTree";
        return;
    }

    qCDebug(LOG) << "adding singleShot";
    m_singleShotScheduled = true;
    QTimer::singleShot(1000, this, SLOT(updateTestTree()));
}

void TestCodeParser::updateTestTree()
{
    m_singleShotScheduled = false;
    if (m_codeModelParsing) {
        m_fullUpdatePostponed = true;
        m_partialUpdatePostponed = false;
        m_postponedFiles.clear();
        return;
    }

    if (!ProjectExplorer::SessionManager::startupProject())
        return;

    m_fullUpdatePostponed = false;

    emit aboutToPerformFullParse();
    qCDebug(LOG) << "calling scanForTests (updateTestTree)";
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

static bool includesGTest(const CPlusPlus::Document::Ptr &doc,
                          const CppTools::CppModelManager *cppMM)
{
    const QString gtestH = QLatin1String("gtest/gtest.h");
    foreach (const CPlusPlus::Document::Include &inc, doc->resolvedIncludes()) {
        if (inc.resolvedFileName().endsWith(gtestH))
            return true;
    }

    if (cppMM) {
        const CPlusPlus::Snapshot snapshot = cppMM->snapshot();
        foreach (const QString &include, snapshot.allIncludesForDocument(doc->fileName())) {
            if (include.endsWith(gtestH))
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
    const QList<CPlusPlus::Document::MacroUse> macros = document->macroUses();

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (TestUtils::isQTestMacro(name)) {
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
    const QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (TestUtils::isQuickTestMacro(name)) {
            CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(getFileContent(doc->fileName())
                                 .mid(arg.bytesBegin(), arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    return QString();
}

static bool hasGTestNames(CPlusPlus::Document::Ptr &document)
{
    foreach (const CPlusPlus::Document::MacroUse &macro, document->macroUses()) {
        if (!macro.isFunctionLike())
            continue;
        if (TestUtils::isGTestMacro(QLatin1String(macro.macro().name()))) {
            const QVector<CPlusPlus::Document::Block> args = macro.arguments();
            if (args.size() != 2)
                continue;
            return true;
        }
    }
    return false;
}

static QList<QmlJS::Document::Ptr> scanDirectoryForQuickTestQmlFiles(const QString &srcDir)
{
    QStringList dirs(srcDir);
    QmlJS::ModelManagerInterface *qmlJsMM = QmlJSTools::Internal::ModelManager::instance();
    // make sure even files not listed in pro file are available inside the snapshot
    QFutureInterface<void> future;
    QmlJS::PathsAndLanguages paths;
    paths.maybeInsert(Utils::FileName::fromString(srcDir), QmlJS::Dialect::Qml);
    const bool emitDocumentChanges = false;
    const bool onlyTheLib = false;
    QmlJS::ModelManagerInterface::importScan(future, qmlJsMM->workingCopy(), paths, qmlJsMM,
        emitDocumentChanges, onlyTheLib);

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

static bool hasFunctionWithDataTagUsage(const QMap<QString, TestCodeLocationAndType> &testFunctions)
{
    foreach (const QString &functionName, testFunctions.keys()) {
        if (functionName.endsWith(QLatin1String("_data")) &&
                testFunctions.contains(functionName.left(functionName.size() - 5))) {
            return true;
        }
    }
    return false;
}

static QMap<QString, TestCodeLocationList> checkForDataTags(const QString &fileName,
            const QMap<QString, TestCodeLocationAndType> &testFunctions)
{
    if (hasFunctionWithDataTagUsage(testFunctions)) {
        const CPlusPlus::Snapshot snapshot = CPlusPlus::CppModelManagerBase::instance()->snapshot();
        const QByteArray fileContent = getFileContent(fileName);
        CPlusPlus::Document::Ptr document = snapshot.preprocessedDocument(fileContent, fileName);
        document->check();
        CPlusPlus::AST *ast = document->translationUnit()->ast();
        TestDataFunctionVisitor visitor(document);
        visitor.accept(ast);
        return visitor.dataTags();
    }
    return QMap<QString, TestCodeLocationList>();
}

static TestTreeItem *constructTestTreeItem(const QString &fileName,
                                           const QString &mainFile,  // used for Quick Tests only
                                           const QString &testCaseName,
                                           int line, int column,
                                           const QMap<QString, TestCodeLocationAndType> &functions,
                                           const QMap<QString, TestCodeLocationList> dataTags = QMap<QString, TestCodeLocationList>())
{
    TestTreeItem *treeItem = new TestTreeItem(testCaseName, fileName, TestTreeItem::TestClass);
    treeItem->setMainFile(mainFile); // used for Quick Tests only
    treeItem->setLine(line);
    treeItem->setColumn(column);

    foreach (const QString &functionName, functions.keys()) {
        const TestCodeLocationAndType locationAndType = functions.value(functionName);
        TestTreeItem *treeItemChild = new TestTreeItem(functionName, locationAndType.m_name,
                                                       locationAndType.m_type);
        treeItemChild->setLine(locationAndType.m_line);
        treeItemChild->setColumn(locationAndType.m_column);
        treeItemChild->setState(locationAndType.m_state);

        // check for data tags and if there are any for this function add them
        const QString qualifiedFunctionName = testCaseName + QLatin1String("::") + functionName;
        if (dataTags.contains(qualifiedFunctionName)) {
            const TestCodeLocationList &tags = dataTags.value(qualifiedFunctionName);
            foreach (const TestCodeLocationAndType &tagLocation, tags) {
                TestTreeItem *tagTreeItem = new TestTreeItem(tagLocation.m_name,
                                                             locationAndType.m_name,
                                                             tagLocation.m_type);
                tagTreeItem->setLine(tagLocation.m_line);
                tagTreeItem->setColumn(tagLocation.m_column);
                tagTreeItem->setState(tagLocation.m_state);
                treeItemChild->appendChild(tagTreeItem);
            }
        }

        treeItem->appendChild(treeItemChild);
    }
    return treeItem;
}

static TestTreeItem *constructGTestTreeItem(const QString &filePath, const GTestCaseSpec &caseSpec,
                                            const QString &proFile,
                                            const TestCodeLocationList &testSets)
{
    TestTreeItem *item = new TestTreeItem(caseSpec.testCaseName, QString(),
            caseSpec.parameterized ? TestTreeItem::GTestCaseParameterized
                                   : TestTreeItem::GTestCase);
    foreach (const TestCodeLocationAndType &locationAndType, testSets) {
        TestTreeItem *treeItemChild = new TestTreeItem(locationAndType.m_name, filePath,
                                                       locationAndType.m_type);
        treeItemChild->setState(locationAndType.m_state);
        treeItemChild->setLine(locationAndType.m_line);
        treeItemChild->setColumn(locationAndType.m_column);
        treeItemChild->setMainFile(proFile);
        item->appendChild(treeItemChild);
    }
    return item;
}

/****** end of helpers ******/

// used internally to indicate a parse that failed due to having triggered a parse for a file that
// is not (yet) part of the CppModelManager's snapshot
static bool parsingHasFailed;

void performParse(QFutureInterface<void> &futureInterface, const QStringList &list,
                  const QMap<QString, QString> &referencingFiles,
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
            const QString &referencingFile = referencingFiles.value(file);
            testCodeParser->checkDocumentForTestCode(doc,
                    list.contains(referencingFile) ? QString() : referencingFile);
        } else {
            parsingHasFailed |= (CppTools::ProjectFile::classify(file)
                                 != CppTools::ProjectFile::Unclassified);
        }
    }
    futureInterface.setProgressValue(list.size());
}

/****** threaded parsing stuff *******/
void TestCodeParser::checkDocumentForTestCode(CPlusPlus::Document::Ptr document,
                                              const QString &referencingFile)
{
    const QString fileName = document->fileName();
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    QList<CppTools::ProjectPart::Ptr> projParts = modelManager->projectPart(fileName);
    if (projParts.size())
        if (!projParts.at(0)->selectedForBuilding)
            return;

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

            const bool hasReferencingFile = declaringDoc->fileName() != document->fileName();

            TestVisitor visitor(testCaseName);
            visitor.accept(declaringDoc->globalNamespace());
            const QMap<QString, TestCodeLocationAndType> testFunctions = visitor.privateSlots();

            QMap<QString, TestCodeLocationList> dataTags =
                    checkForDataTags(declaringDoc->fileName(), testFunctions);

            if (hasReferencingFile)
                dataTags.unite(checkForDataTags(document->fileName(), testFunctions));

            TestTreeItem *item = constructTestTreeItem(declaringDoc->fileName(), QString(),
                                                       testCaseName, line, column, testFunctions,
                                                       dataTags);
            if (hasReferencingFile)
                item->setReferencingFile(fileName);

            emit testItemCreated(item, TestTreeModel::AutoTest);
            return;
        }
    }

    if (includesGTest(document, modelManager)) {
        if (hasGTestNames(document)) {
            handleGTest(document->fileName());
            return;
        }
    }

    // could not find the class to test, or QTest is not included and QT_TESTLIB_LIB defined
    // maybe file is only a referenced file
    if (!referencingFile.isEmpty()) {
        CPlusPlus::Snapshot snapshot = modelManager->snapshot();
        if (snapshot.contains(referencingFile))
            checkDocumentForTestCode(snapshot.find(referencingFile).value());
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

        TestTreeItem *testTreeItem;
        if (testCaseName.isEmpty()) {
            testTreeItem = new TestTreeItem(QString(), QString(), TestTreeItem::TestClass);

            foreach (const QString &functionName, testFunctions.keys()) {
                const TestCodeLocationAndType locationAndType = testFunctions.value(functionName);
                TestTreeItem *testFunction = new TestTreeItem(functionName, locationAndType.m_name,
                                                              locationAndType.m_type);
                testFunction->setLine(locationAndType.m_line);
                testFunction->setColumn(locationAndType.m_column);
                testFunction->setMainFile(cppFileName);
                testFunction->setReferencingFile(cppFileName);
                testTreeItem->appendChild(testFunction);
            }

        } else {
            testTreeItem = constructTestTreeItem(tcLocationAndType.m_name, cppFileName,
                                                 testCaseName, tcLocationAndType.m_line,
                                                 tcLocationAndType.m_column, testFunctions);
            testTreeItem->setReferencingFile(cppFileName);
        }
        emit testItemCreated(testTreeItem, TestTreeModel::QuickTest);
    }
}

void TestCodeParser::handleGTest(const QString &filePath)
{
    const QByteArray &fileContent = getFileContent(filePath);
    const CPlusPlus::Snapshot snapshot = CPlusPlus::CppModelManagerBase::instance()->snapshot();
    CPlusPlus::Document::Ptr document = snapshot.preprocessedDocument(fileContent, filePath);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    GTestVisitor visitor(document);
    visitor.accept(ast);

    QMap<GTestCaseSpec, TestCodeLocationList> result = visitor.gtestFunctions();
    QString proFile;
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(filePath);
    if (ppList.size())
        proFile = ppList.at(0)->projectFile;

    foreach (const GTestCaseSpec &testSpec, result.keys()) {
        TestTreeItem *item = constructGTestTreeItem(filePath, testSpec, proFile,
                                                    result.value(testSpec));
        emit testItemCreated(item, TestTreeModel::GoogleTest);
    }
}

void TestCodeParser::onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    if (m_codeModelParsing) {
        if (!m_fullUpdatePostponed) {
            m_partialUpdatePostponed = true;
            m_postponedFiles.insert(document->fileName());
        }
        return;
    }

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;
    const QString fileName = document->fileName();
    if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        return;
    }
    qCDebug(LOG) << "calling scanForTests (onCppDocumentUpdated)";
    scanForTests(QStringList(fileName));
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &document)
{
    if (m_codeModelParsing) {
        if (!m_fullUpdatePostponed) {
            m_partialUpdatePostponed = true;
            m_postponedFiles.insert(document->fileName());
        }
        return;
    }

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;
    const QString fileName = document->fileName();
    if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        // what if the file is not listed inside the pro file, but will be used anyway?
        return;
    }
    const CPlusPlus::Snapshot snapshot = CppTools::CppModelManager::instance()->snapshot();
    QMap<QString, QString> referencingFiles = m_model->referencingFiles();
    if (referencingFiles.contains(fileName)) {
        const QString &referencingFile = referencingFiles.value(fileName);
        if (!referencingFile.isEmpty() && snapshot.contains(referencingFile)) {
            qCDebug(LOG) << "calling scanForTests with cached referencing files"
                         << "(onQmlDocumentUpdated)";
            scanForTests(QStringList(referencingFile));
        }
    }
}

void TestCodeParser::onStartupProjectChanged(ProjectExplorer::Project *)
{
    if (m_parserState == FullParse || m_parserState == PartialParse) {
        Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
    } else {
        emit aboutToPerformFullParse();
        emitUpdateTestTree();
    }
}

void TestCodeParser::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    if (project != ProjectExplorer::SessionManager::startupProject())
        return;
    if (m_codeModelParsing || m_parserState == Disabled)
        m_fullUpdatePostponed = true;
    else
        emitUpdateTestTree();
}

bool TestCodeParser::postponed(const QStringList &fileList)
{
    switch (m_parserState) {
    case Idle:
        return false;
    case PartialParse:
    case FullParse:
        // parse is running, postponing a full parse
        if (fileList.isEmpty()) {
            m_partialUpdatePostponed = false;
            m_postponedFiles.clear();
            m_fullUpdatePostponed = true;
        } else {
            // partial parse triggered, but full parse is postponed already, ignoring this
            if (m_fullUpdatePostponed)
                return true;
            // partial parse triggered, postpone or add current files to already postponed partial
            foreach (const QString &file, fileList)
                m_postponedFiles.insert(file);
            m_partialUpdatePostponed = true;
        }
        return true;
    case Disabled:
        break;
    }
    QTC_ASSERT(false, return false); // should not happen at all
}

void TestCodeParser::scanForTests(const QStringList &fileList)
{
    if (m_parserState == Disabled) {
        m_dirty = true;
        if (fileList.isEmpty()) {
            m_fullUpdatePostponed = true;
            m_partialUpdatePostponed = false;
            m_postponedFiles.clear();
        } else {
            if (!m_fullUpdatePostponed) {
                m_partialUpdatePostponed = true;
                foreach (const QString &file, fileList)
                    m_postponedFiles.insert(file);
            }
        }
        return;
    }

    if (postponed(fileList))
        return;

    m_postponedFiles.clear();
    bool isFullParse = fileList.isEmpty();
    bool isSmallChange = !isFullParse && fileList.size() < 6;
    QStringList list;
    if (isFullParse) {
        list = ProjectExplorer::SessionManager::startupProject()->files(ProjectExplorer::Project::AllFiles);
        if (list.isEmpty())
            return;
        qCDebug(LOG) << "setting state to FullParse (scanForTests)";
        m_parserState = FullParse;
    } else {
        list << fileList;
        qCDebug(LOG) << "setting state to PartialParse (scanForTests)";
        m_parserState = PartialParse;
    }

    parsingHasFailed = false;

    QMap<QString, QString> referencingFiles = m_model->referencingFiles();
    if (isSmallChange) { // no need to do this async or should we do this always async?
        foreach (const QString &filePath, list)
            m_model->markForRemoval(filePath);

        CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
        CPlusPlus::Snapshot snapshot = cppMM->snapshot();
        foreach (const QString &file, list) {
            if (snapshot.contains(file)) {
                CPlusPlus::Document::Ptr doc = snapshot.find(file).value();
                const QString &referencingFile = referencingFiles.value(file);
                checkDocumentForTestCode(doc, list.contains(referencingFile) ? QString()
                                                                             : referencingFile);
            } else {
                parsingHasFailed |= (CppTools::ProjectFile::classify(file)
                                     != CppTools::ProjectFile::Unclassified);
            }
        }
        onFinished();
        return;
    }

    m_model->markAllForRemoval();

    QFuture<void> future = Utils::runAsync<void>(&performParse, list, referencingFiles, this);
    Core::FutureProgress *progress
            = Core::ProgressManager::addTask(future, isFullParse ? tr("Scanning for Tests")
                                                                 : tr("Refreshing Tests List"),
                                             Autotest::Constants::TASK_PARSE);
    connect(progress, &Core::FutureProgress::finished,
            this, &TestCodeParser::onFinished);

    emit parsingStarted();
}

void TestCodeParser::onTaskStarted(Core::Id type)
{
    if (type == CppTools::Constants::TASK_INDEX)
        m_codeModelParsing = true;
}

void TestCodeParser::onAllTasksFinished(Core::Id type)
{
    // only CPP parsing is relevant as we trigger Qml parsing internally anyway
    if (type != CppTools::Constants::TASK_INDEX)
        return;
    m_codeModelParsing = false;

    // avoid illegal parser state if respective widgets became hidden while parsing
    setState(Idle);
}

void TestCodeParser::onFinished()
{
    switch (m_parserState) {
    case PartialParse:
        qCDebug(LOG) << "setting state to Idle (onFinished, PartialParse)";
        m_parserState = Idle;
        emit partialParsingFinished();
        break;
    case FullParse:
        qCDebug(LOG) << "setting state to Idle (onFinished, FullParse)";
        m_parserState = Idle;
        m_dirty = parsingHasFailed;
        if (m_partialUpdatePostponed || m_fullUpdatePostponed || parsingHasFailed) {
            emit partialParsingFinished();
        } else {
            qCDebug(LOG) << "emitting parsingFinished"
                         << "(onFinished, FullParse, nothing postponed, parsing succeeded)";
            emit parsingFinished();
        }
        m_dirty = false;
        break;
    case Disabled: // can happen if all Test related widgets become hidden while parsing
        qCDebug(LOG) << "emitting parsingFinished (onFinished, Disabled)";
        emit parsingFinished();
        break;
    default:
        qWarning("I should not be here... State: %d", m_parserState);
        break;
    }
}

void TestCodeParser::onPartialParsingFinished()
{
    QTC_ASSERT(m_fullUpdatePostponed != m_partialUpdatePostponed
            || ((m_fullUpdatePostponed || m_partialUpdatePostponed) == false),
               m_partialUpdatePostponed = false;m_postponedFiles.clear(););
    if (m_fullUpdatePostponed) {
        m_fullUpdatePostponed = false;
        qCDebug(LOG) << "calling updateTestTree (onPartialParsingFinished)";
        updateTestTree();
    } else if (m_partialUpdatePostponed) {
        m_partialUpdatePostponed = false;
        qCDebug(LOG) << "calling scanForTests with postponed files (onPartialParsingFinished)";
        scanForTests(m_postponedFiles.toList());
    } else {
        m_dirty |= m_codeModelParsing;
        if (m_dirty) {
            emit parsingFailed();
        } else if (!m_singleShotScheduled) {
            qCDebug(LOG) << "emitting parsingFinished"
                         << "(onPartialParsingFinished, nothing postponed, not dirty)";
            emit parsingFinished();
        } else {
            qCDebug(LOG) << "not emitting parsingFinished"
                         << "(on PartialParsingFinished, singleshot scheduled)";
        }
    }
}

} // namespace Internal
} // namespace Autotest
