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

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
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
    connect(&m_futureWatcher, &QFutureWatcher<TestParseResultPtr>::started,
            this, &TestCodeParser::parsingStarted);
    connect(&m_futureWatcher, &QFutureWatcher<TestParseResultPtr>::finished,
            this, &TestCodeParser::onFinished);
    connect(&m_futureWatcher, &QFutureWatcher<TestParseResultPtr>::resultReadyAt,
            this, [this] (int index) {
        emit testParseResultReady(m_futureWatcher.resultAt(index));
    });

    m_testCodeParsers.append(new QtTestParser);
    m_testCodeParsers.append(new QuickTestParser);
    m_testCodeParsers.append(new GoogleTestParser);
}

TestCodeParser::~TestCodeParser()
{
    qDeleteAll(m_testCodeParsers);
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

static bool includesQtTest(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot)
{
    static QStringList expectedHeaderPrefixes
            = Utils::HostOsInfo::isMacHost()
            ? QStringList({ QLatin1String("QtTest.framework/Headers"), QLatin1String("QtTest") })
            : QStringList({ QLatin1String("QtTest") });

    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (const CPlusPlus::Document::Include &inc, includes) {
        // TODO this short cut works only for #include <QtTest>
        // bad, as there could be much more different approaches
        if (inc.unresolvedFileName() == QLatin1String("QtTest")) {
            foreach (const QString &prefix, expectedHeaderPrefixes) {
                if (inc.resolvedFileName().endsWith(QString::fromLatin1("%1/QtTest").arg(prefix)))
                    return true;
            }
        }
    }

    const QSet<QString> allIncludes = snapshot.allIncludesForDocument(doc->fileName());
    foreach (const QString &include, allIncludes) {
        foreach (const QString &prefix, expectedHeaderPrefixes) {
        if (include.endsWith(QString::fromLatin1("%1/qtest.h").arg(prefix)))
            return true;
        }
    }
    return false;
}

static bool includesQtQuickTest(const CPlusPlus::Document::Ptr &doc,
                                const CPlusPlus::Snapshot &snapshot)
{
    static QStringList expectedHeaderPrefixes
            = Utils::HostOsInfo::isMacHost()
            ? QStringList({ QLatin1String("QtQuickTest.framework/Headers"),
                            QLatin1String("QtQuickTest") })
            : QStringList({ QLatin1String("QtQuickTest") });

    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (const CPlusPlus::Document::Include &inc, includes) {
        if (inc.unresolvedFileName() == QLatin1String("QtQuickTest/quicktest.h")) {
            foreach (const QString &prefix, expectedHeaderPrefixes) {
                if (inc.resolvedFileName().endsWith(
                            QString::fromLatin1("%1/quicktest.h").arg(prefix))) {
                    return true;
                }
            }
        }
    }

    foreach (const QString &include, snapshot.allIncludesForDocument(doc->fileName())) {
        foreach (const QString &prefix, expectedHeaderPrefixes) {
            if (include.endsWith(QString::fromLatin1("%1/quicktest.h").arg(prefix)))
                return true;
        }
    }
    return false;
}

static bool includesGTest(const CPlusPlus::Document::Ptr &doc,
                          const CPlusPlus::Snapshot &snapshot)
{
    const QString gtestH = QLatin1String("gtest/gtest.h");
    foreach (const CPlusPlus::Document::Include &inc, doc->resolvedIncludes()) {
        if (inc.resolvedFileName().endsWith(gtestH))
            return true;
    }

    foreach (const QString &include, snapshot.allIncludesForDocument(doc->fileName())) {
        if (include.endsWith(gtestH))
            return true;
    }

    return false;
}

static bool qtTestLibDefined(const QString &fileName)
{
    const QList<CppTools::ProjectPart::Ptr> parts =
            CppTools::CppModelManager::instance()->projectPart(fileName);
    if (parts.size() > 0)
        return parts.at(0)->projectDefines.contains("#define QT_TESTLIB_LIB");
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
                         const CPlusPlus::Snapshot &snapshot, const QString &fileName)
{
    const QByteArray &fileContent = getFileContent(fileName);
    CPlusPlus::Document::Ptr document = modelManager->document(fileName);
    if (document.isNull())
        return QString();

    const QList<CPlusPlus::Document::MacroUse> macros = document->macroUses();

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (TestUtils::isQTestMacro(name)) {
            const CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(fileContent.mid(arg.bytesBegin(),
                                                 arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    // check if one has used a self-defined macro or QTest::qExec() directly
    document = snapshot.preprocessedDocument(fileContent, fileName);
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

static bool hasGTestNames(const CPlusPlus::Document::Ptr &document)
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
                                                  const CPlusPlus::Snapshot &snapshot,
                                                  const QString &testCaseName,
                                                  unsigned *line, unsigned *column)
{
    CPlusPlus::Document::Ptr declaringDoc = doc;
    CPlusPlus::TypeOfExpression typeOfExpr;
    typeOfExpr.init(doc, snapshot);

    QList<CPlusPlus::LookupItem> lookupItems = typeOfExpr(testCaseName.toUtf8(),
                                                          doc->globalNamespace());
    if (lookupItems.size()) {
        if (CPlusPlus::Symbol *symbol = lookupItems.first().declaration()) {
            if (CPlusPlus::Class *toeClass = symbol->asClass()) {
                const QString declFileName = QLatin1String(toeClass->fileId()->chars(),
                                                           toeClass->fileId()->size());
                declaringDoc = snapshot.document(declFileName);
                *line = toeClass->line();
                *column = toeClass->column() - 1;
            }
        }
    }
    return declaringDoc;
}

static QSet<QString> filesWithDataFunctionDefinitions(
            const QMap<QString, TestCodeLocationAndType> &testFunctions)
{
    QSet<QString> result;
    QMap<QString, TestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
    const QMap<QString, TestCodeLocationAndType>::ConstIterator end = testFunctions.end();

    for ( ; it != end; ++it) {
        const QString &key = it.key();
        if (key.endsWith(QLatin1String("_data")) && testFunctions.contains(key.left(key.size() - 5)))
            result.insert(it.value().m_name);
    }
    return result;
}

static QMap<QString, TestCodeLocationList> checkForDataTags(const QString &fileName,
            const CPlusPlus::Snapshot &snapshot)
{
    const QByteArray fileContent = getFileContent(fileName);
    CPlusPlus::Document::Ptr document = snapshot.preprocessedDocument(fileContent, fileName);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    TestDataFunctionVisitor visitor(document);
    visitor.accept(ast);
    return visitor.dataTags();
}

/****** end of helpers ******/

static bool checkQmlDocumentForQuickTestCode(QFutureInterface<TestParseResultPtr> futureInterface,
                                             const QmlJS::Document::Ptr &qmlJSDoc,
                                             const QString &proFile = QString())
{
    if (qmlJSDoc.isNull())
        return false;
    QmlJS::AST::Node *ast = qmlJSDoc->ast();
    QTC_ASSERT(ast, return false);
    TestQmlVisitor qmlVisitor(qmlJSDoc);
    QmlJS::AST::Node::accept(ast, &qmlVisitor);

    const QString testCaseName = qmlVisitor.testCaseName();
    const TestCodeLocationAndType tcLocationAndType = qmlVisitor.testCaseLocation();
    const QMap<QString, TestCodeLocationAndType> &testFunctions = qmlVisitor.testFunctions();

    QuickTestParseResult *parseResult = new QuickTestParseResult;
    parseResult->proFile = proFile;
    parseResult->itemType = TestTreeItem::TestCase;
    QMap<QString, TestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
    const QMap<QString, TestCodeLocationAndType>::ConstIterator end = testFunctions.end();
    for ( ; it != end; ++it) {
        const TestCodeLocationAndType &loc = it.value();
        QuickTestParseResult *funcResult = new QuickTestParseResult;
        funcResult->name = it.key();
        funcResult->displayName = it.key();
        funcResult->itemType = loc.m_type;
        funcResult->fileName = loc.m_name;
        funcResult->line = loc.m_line;
        funcResult->column = loc.m_column;
        funcResult->proFile = proFile;

        parseResult->children.append(funcResult);
    }
    if (!testCaseName.isEmpty()) {
        parseResult->fileName = tcLocationAndType.m_name;
        parseResult->name = testCaseName;
        parseResult->line = tcLocationAndType.m_line;
        parseResult->column = tcLocationAndType.m_column;
    }
    futureInterface.reportResult(TestParseResultPtr(parseResult));
    return true;
}

static bool handleQtTest(QFutureInterface<TestParseResultPtr> futureInterface,
                         CPlusPlus::Document::Ptr document,
                         const CPlusPlus::Snapshot &snapshot,
                         const QString &oldTestCaseName)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &fileName = document->fileName();
    QString testCaseName(testClass(modelManager, snapshot, fileName));
    // we might be in a reparse without the original entry point with the QTest::qExec()
    if (testCaseName.isEmpty())
        testCaseName = oldTestCaseName;
    if (!testCaseName.isEmpty()) {
        unsigned line = 0;
        unsigned column = 0;
        CPlusPlus::Document::Ptr declaringDoc = declaringDocument(document, snapshot, testCaseName,
                                                                  &line, &column);
        if (declaringDoc.isNull())
            return false;

        TestVisitor visitor(testCaseName);
        visitor.accept(declaringDoc->globalNamespace());
        if (!visitor.resultValid())
            return false;

        const QMap<QString, TestCodeLocationAndType> &testFunctions = visitor.privateSlots();
        const QSet<QString> &files = filesWithDataFunctionDefinitions(testFunctions);

        // TODO: change to QHash<>
        QMap<QString, TestCodeLocationList> dataTags;
        foreach (const QString &file, files)
            dataTags.unite(checkForDataTags(file, snapshot));

        QtTestParseResult *parseResult = new QtTestParseResult;
        parseResult->itemType = TestTreeItem::TestCase;
        parseResult->fileName = declaringDoc->fileName();
        parseResult->name = testCaseName;
        parseResult->displayName = testCaseName;
        parseResult->line = line;
        parseResult->column = column;
        parseResult->proFile = modelManager->projectPart(fileName).first()->projectFile;
        QMap<QString, TestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
        const QMap<QString, TestCodeLocationAndType>::ConstIterator end = testFunctions.end();
        for ( ; it != end; ++it) {
            const TestCodeLocationAndType &location = it.value();
            QtTestParseResult *func = new QtTestParseResult;
            func->itemType = location.m_type;
            func->name = testCaseName + QLatin1String("::") + it.key();
            func->displayName = it.key();
            func->fileName = location.m_name;
            func->line = location.m_line;
            func->column = location.m_column;

            foreach (const TestCodeLocationAndType &tag, dataTags.value(func->name)) {
                QtTestParseResult *dataTag = new QtTestParseResult;
                dataTag->itemType = tag.m_type;
                dataTag->name = tag.m_name;
                dataTag->displayName = tag.m_name;
                dataTag->fileName = testFunctions.value(it.key() + QLatin1String("_data")).m_name;
                dataTag->line = tag.m_line;
                dataTag->column = tag.m_column;

                func->children.append(dataTag);
            }
            parseResult->children.append(func);
        }

        futureInterface.reportResult(TestParseResultPtr(parseResult));
        return true;
    }
    return false;
}

static bool handleQtQuickTest(QFutureInterface<TestParseResultPtr> futureInterface,
                              CPlusPlus::Document::Ptr document)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    if (quickTestName(document).isEmpty())
        return false;

    const QString cppFileName = document->fileName();
    QList<CppTools::ProjectPart::Ptr> ppList = modelManager->projectPart(cppFileName);
    QTC_ASSERT(!ppList.isEmpty(), return false);
    const QString &proFile = ppList.at(0)->projectFile;

    const QString srcDir = quickTestSrcDir(modelManager, cppFileName);
    if (srcDir.isEmpty())
        return false;

    const QList<QmlJS::Document::Ptr> qmlDocs = scanDirectoryForQuickTestQmlFiles(srcDir);
    bool result = false;
    foreach (const QmlJS::Document::Ptr &qmlJSDoc, qmlDocs)
        result |= checkQmlDocumentForQuickTestCode(futureInterface, qmlJSDoc, proFile);
    return result;
}

static bool handleGTest(QFutureInterface<TestParseResultPtr> futureInterface,
                        const CPlusPlus::Document::Ptr &doc,
                        const CPlusPlus::Snapshot &snapshot)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &filePath = doc->fileName();
    const QByteArray &fileContent = getFileContent(filePath);
    CPlusPlus::Document::Ptr document = snapshot.preprocessedDocument(fileContent, filePath);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    GTestVisitor visitor(document);
    visitor.accept(ast);

    QMap<GTestCaseSpec, GTestCodeLocationList> result = visitor.gtestFunctions();
    QString proFile;
    const QList<CppTools::ProjectPart::Ptr> &ppList = modelManager->projectPart(filePath);
    if (ppList.size())
        proFile = ppList.first()->projectFile;

    foreach (const GTestCaseSpec &testSpec, result.keys()) {
        GoogleTestParseResult *parseResult = new GoogleTestParseResult;
        parseResult->itemType = TestTreeItem::TestCase;
        parseResult->fileName = filePath;
        parseResult->name = testSpec.testCaseName;
        parseResult->parameterized = testSpec.parameterized;
        parseResult->typed = testSpec.typed;
        parseResult->disabled = testSpec.disabled;
        parseResult->proFile = proFile;

        foreach (const GTestCodeLocationAndType &location, result.value(testSpec)) {
            GoogleTestParseResult *testSet = new GoogleTestParseResult;
            testSet->name = location.m_name;
            testSet->fileName = filePath;
            testSet->line = location.m_line;
            testSet->column = location.m_column;
            testSet->disabled = location.m_state & GoogleTestTreeItem::Disabled;
            testSet->itemType = location.m_type;
            testSet->proFile = proFile;

            parseResult->children.append(testSet);
        }

        futureInterface.reportResult(TestParseResultPtr(parseResult));
    }
    return !result.keys().isEmpty();
}

// used internally to indicate a parse that failed due to having triggered a parse for a file that
// is not (yet) part of the CppModelManager's snapshot
static bool parsingHasFailed;

static bool checkDocumentForTestCode(QFutureInterface<TestParseResultPtr> &futureInterface,
                                     const QString &fileName,
                                     const QVector<ITestParser *> &parsers)
{
    foreach (ITestParser *currentParser, parsers) {
        if (currentParser->processDocument(futureInterface, fileName))
            return true;
    }
    return false;
}

static void performParse(QFutureInterface<TestParseResultPtr> &futureInterface,
                         const QStringList &list, const QVector<ITestParser *> &parsers)
{
    int progressValue = 0;
    futureInterface.setProgressRange(0, list.size());
    futureInterface.setProgressValue(progressValue);

    foreach (const QString &file, list) {
        if (futureInterface.isCanceled())
            return;
        futureInterface.setProgressValue(++progressValue);
        if (!checkDocumentForTestCode(futureInterface, file, parsers)) {
            parsingHasFailed |= !CppTools::CppModelManager::instance()->snapshot().contains(file)
                && (CppTools::ProjectFile::classify(file) != CppTools::ProjectFile::Unclassified);
        }
    }
    futureInterface.setProgressValue(list.size());
}

/****** threaded parsing stuff *******/

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
    if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName))
        return;

    qCDebug(LOG) << "calling scanForTests (onCppDocumentUpdated)";
    scanForTests(QStringList(fileName));
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &document)
{
    const QString &fileName = document->fileName();
    if (m_codeModelParsing) {
        if (!m_fullUpdatePostponed) {
            m_partialUpdatePostponed = true;
            m_postponedFiles.insert(fileName);
        }
        return;
    }

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;
    if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        // what if the file is not listed inside the pro file, but will be used anyway?
        return;
    }

    scanForTests(QStringList(fileName));
}

void TestCodeParser::onStartupProjectChanged(ProjectExplorer::Project *project)
{
    if (m_parserState == FullParse || m_parserState == PartialParse)
        Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
    else if (project)
        emitUpdateTestTree();
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
            Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
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
        } else if (!m_fullUpdatePostponed) {
            m_partialUpdatePostponed = true;
            foreach (const QString &file, fileList)
                m_postponedFiles.insert(file);
        }
        return;
    }

    if (postponed(fileList))
        return;

    m_postponedFiles.clear();
    bool isFullParse = fileList.isEmpty();
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

    if (isFullParse) {
        // remove qml files as they will be found automatically by the referencing cpp file
        list = Utils::filtered(list, [] (const QString &fn) {
            return !fn.endsWith(QLatin1String(".qml"));
        });
        m_model->markAllForRemoval();
    } else {
        foreach (const QString &filePath, list)
            m_model->markForRemoval(filePath);
    }

    foreach (ITestParser *parser, m_testCodeParsers)
        parser->init(list);

    QFuture<TestParseResultPtr> future = Utils::runAsync(&performParse, list, m_testCodeParsers);
    m_futureWatcher.setFuture(future);
    if (list.size() > 5) {
        Core::ProgressManager::addTask(future, tr("Scanning for Tests"),
                                       Autotest::Constants::TASK_PARSE);
    }
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
        onPartialParsingFinished();
        break;
    case FullParse:
        qCDebug(LOG) << "setting state to Idle (onFinished, FullParse)";
        m_parserState = Idle;
        m_dirty = parsingHasFailed;
        if (m_partialUpdatePostponed || m_fullUpdatePostponed || parsingHasFailed) {
            onPartialParsingFinished();
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

TestTreeItem *QtTestParseResult::createTestTreeItem() const
{
    return itemType == TestTreeItem::Root ? 0 : AutoTestTreeItem::createTestItem(this);
}

TestTreeItem *QuickTestParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root || itemType == TestTreeItem::TestDataTag)
        return 0;
    return QuickTestTreeItem::createTestItem(this);
}

TestTreeItem *GoogleTestParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::TestCase || itemType == TestTreeItem::TestFunctionOrSet)
        return GoogleTestTreeItem::createTestItem(this);
    return 0;
}

void CppParser::init(const QStringList &filesToParse)
{
    Q_UNUSED(filesToParse);
    m_cppSnapshot = CppTools::CppModelManager::instance()->snapshot();
}

bool CppParser::selectedForBuilding(const QString &fileName)
{
    QList<CppTools::ProjectPart::Ptr> projParts =
            CppTools::CppModelManager::instance()->projectPart(fileName);

    return projParts.size() && projParts.at(0)->selectedForBuilding;
}


void QtTestParser::init(const QStringList &filesToParse)
{
    m_testCaseNames = TestTreeModel::instance()->testCaseNamesForFiles(filesToParse);
    CppParser::init(filesToParse);
}

bool QtTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                   const QString &fileName)
{
    if (!m_cppSnapshot.contains(fileName) || !selectedForBuilding(fileName))
        return false;
    CPlusPlus::Document::Ptr doc = m_cppSnapshot.find(fileName).value();
    const QString &oldName = m_testCaseNames.value(fileName);
    if ((!includesQtTest(doc, m_cppSnapshot) || !qtTestLibDefined(fileName)) && oldName.isEmpty())
        return false;

    return handleQtTest(futureInterface, doc, m_cppSnapshot, oldName);
}

void QuickTestParser::init(const QStringList &filesToParse)
{
    m_qmlSnapshot = QmlJSTools::Internal::ModelManager::instance()->snapshot();
    CppParser::init(filesToParse);
}

bool QuickTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                      const QString &fileName)
{
    if (fileName.endsWith(".qml")) {
        QmlJS::Document::Ptr qmlJSDoc = m_qmlSnapshot.document(fileName);
        return checkQmlDocumentForQuickTestCode(futureInterface, qmlJSDoc);
    }
    if (!m_cppSnapshot.contains(fileName) || !selectedForBuilding(fileName))
        return false;
    CPlusPlus::Document::Ptr document = m_cppSnapshot.find(fileName).value();
    if (!includesQtQuickTest(document, m_cppSnapshot))
        return false;
    return handleQtQuickTest(futureInterface, document);
}

bool GoogleTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                       const QString &fileName)
{
    if (!m_cppSnapshot.contains(fileName) || !selectedForBuilding(fileName))
        return false;
    CPlusPlus::Document::Ptr document = m_cppSnapshot.find(fileName).value();
    if (!includesGTest(document, m_cppSnapshot) || !hasGTestNames(document))
        return false;
    return handleGTest(futureInterface, document, m_cppSnapshot);
}

} // namespace Internal
} // namespace Autotest
