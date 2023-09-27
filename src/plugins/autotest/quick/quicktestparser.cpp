// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quicktestparser.h"

#include "quicktesttreeitem.h"
#include "quicktestvisitors.h"
#include "quicktest_utils.h"
#include "../testcodeparser.h"
#include "../testtreemodel.h"
#include "../qtest/qttestframework.h"

#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/projectpart.h>

#include <projectexplorer/projectmanager.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdialect.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <utils/hostosinfo.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QPromise>

using namespace QmlJS;
using namespace Utils;

namespace Autotest::Internal {

TestTreeItem *QuickTestParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root || itemType == TestTreeItem::TestDataTag)
        return nullptr;

    QuickTestTreeItem *item = new QuickTestTreeItem(framework, name, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);
    for (const TestParseResult *funcResult : children)
        item->appendChild(funcResult->createTestTreeItem());
    return item;
}

static bool includesQtQuickTest(const CPlusPlus::Document::Ptr &doc,
                                const CPlusPlus::Snapshot &snapshot)
{
    static QStringList expectedHeaderPrefixes = HostOsInfo::isMacHost()
            ? QStringList({"QtQuickTest.framework/Headers", "QtQuickTest"})
            : QStringList({"QtQuickTest"});

    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    for (const CPlusPlus::Document::Include &inc : includes) {
        if (inc.unresolvedFileName() == "QtQuickTest/quicktest.h") {
            for (const QString &prefix : expectedHeaderPrefixes) {
                if (inc.resolvedFileName().endsWith(
                            QString("%1/quicktest.h").arg(prefix))) {
                    return true;
                }
            }
        }
    }

    for (const FilePath &include : snapshot.allIncludesForDocument(doc->filePath())) {
        for (const QString &prefix : expectedHeaderPrefixes) {
            if (include.pathView().endsWith(QString("%1/quicktest.h").arg(prefix)))
                return true;
        }
    }

    for (const QString &prefix : expectedHeaderPrefixes) {
        if (CppParser::precompiledHeaderContains(snapshot,
                                                 doc->filePath(),
                                                 QString("%1/quicktest.h").arg(prefix))) {
            return true;
        }
    }
    return false;
}

static QString quickTestSrcDir(const FilePath &fileName)
{
    const QList<CppEditor::ProjectPart::ConstPtr> parts =
        CppEditor::CppModelManager::projectPart(fileName);
    if (parts.size() > 0) {
        const ProjectExplorer::Macros &macros = parts.at(0)->projectMacros;
        auto found = std::find_if(macros.cbegin(), macros.cend(),
                                  [](const ProjectExplorer::Macro &macro) {
            return macro.key == "QUICK_TEST_SOURCE_DIR";
        });
        if (found != macros.cend())  {
            QByteArray result = found->value;
            if (result.startsWith('"'))
                result.remove(result.length() - 1, 1).remove(0, 1);
            if (result.startsWith("\\\""))
                result.remove(result.length() - 2, 2).remove(0, 2);
            return QLatin1String(result);
        }
    }
    return {};
}

QString QuickTestParser::quickTestName(const CPlusPlus::Document::Ptr &doc) const
{
    const QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();
    const FilePath filePath = doc->filePath();

    for (const CPlusPlus::Document::MacroUse &macro : macros) {
        if (!macro.isFunctionLike() || macro.arguments().isEmpty())
            continue;
        const QByteArray name = macro.macro().name();
        if (QuickTestUtils::isQuickTestMacro(name)) {
            CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(getFileContent(filePath)
                                 .mid(int(arg.bytesBegin()), int(arg.bytesEnd() - arg.bytesBegin())));
        }
    }


    const QByteArray fileContent = getFileContent(filePath);
    // check for using quick_test_main() directly
    CPlusPlus::Document::Ptr document = m_cppSnapshot.preprocessedDocument(fileContent, filePath);
    if (document.isNull())
        return {};
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    QuickTestAstVisitor astVisitor(document);
    astVisitor.accept(ast);
    if (!astVisitor.testBaseName().isEmpty())
        return astVisitor.testBaseName();

    // check for precompiled headers
    static QStringList expectedHeaderPrefixes = HostOsInfo::isMacHost()
            ? QStringList({"QtQuickTest.framework/Headers", "QtQuickTest"})
            : QStringList({"QtQuickTest"});
    bool pchIncludes = false;
    for (const QString &prefix : expectedHeaderPrefixes) {
        if (CppParser::precompiledHeaderContains(m_cppSnapshot, filePath,
                                                 QString("%1/quicktest.h").arg(prefix))) {
            pchIncludes = true;
            break;
        }
    }

    if (pchIncludes) {
        static const QRegularExpression regex("\\bQUICK_TEST_(MAIN|OPENGL_MAIN|MAIN_WITH_SETUP)");
        const QRegularExpressionMatch match = regex.match(QString::fromUtf8(fileContent));
        if (match.hasMatch())
            return match.captured(); // we do not care for the name, just return something non-empty
    }
    return {};
}

QList<Document::Ptr> QuickTestParser::scanDirectoryForQuickTestQmlFiles(const FilePath &srcDir)
{
    FilePaths dirs({srcDir});
    QStringList dirsStr({srcDir.toString()});
    ModelManagerInterface *qmlJsMM = QmlJSTools::Internal::ModelManager::instance();
    // make sure even files not listed in pro file are available inside the snapshot
    PathsAndLanguages paths;
    paths.maybeInsert(srcDir, Dialect::Qml);
    ModelManagerInterface::importScan(ModelManagerInterface::workingCopy(), paths, qmlJsMM,
        false /*emitDocumentChanges*/, false /*onlyTheLib*/, true /*forceRescan*/ );

    QDirIterator it(srcDir.toString(),
                    QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        auto subDir = FilePath::fromFileInfo(it.fileInfo()).canonicalPath();
        dirs.append(subDir);
        dirsStr.append(subDir.toString());
    }
    QMetaObject::invokeMethod(
        this,
        [this, dirsStr] { QuickTestParser::doUpdateWatchPaths(dirsStr); },
        Qt::QueuedConnection);

    QList<Document::Ptr> foundDocs;

    const Snapshot snapshot = QmlJSTools::Internal::ModelManager::instance()->snapshot();
    for (const FilePath &path : std::as_const(dirs)) {
        const QList<Document::Ptr> docs = snapshot.documentsInDirectory(path);
        for (const Document::Ptr &doc : docs) {
            const FilePath fi = doc->fileName();
            //const QFileInfo fi(doc->fileName());
            // using working copy above might provide no more existing files
            if (!fi.exists())
                continue;
            const QString fileName = fi.fileName();
            if (fileName.startsWith("tst_") && fileName.endsWith(".qml"))
                foundDocs << doc;
        }
    }

    return foundDocs;
}

static bool checkQmlDocumentForQuickTestCode(QPromise<TestParseResultPtr> &promise,
                                             const Document::Ptr &qmlJSDoc,
                                             ITestFramework *framework,
                                             const FilePath &proFile = {},
                                             bool checkForDerivedTest = false)
{
    if (qmlJSDoc.isNull())
        return false;
    AST::Node *ast = qmlJSDoc->ast();
    QTC_ASSERT(ast, return false);
    Snapshot snapshot = ModelManagerInterface::instance()->snapshot();
    TestQmlVisitor qmlVisitor(qmlJSDoc, snapshot, checkForDerivedTest);
    AST::Node::accept(ast, &qmlVisitor);
    if (!qmlVisitor.isValid())
        return false;

    const QVector<QuickTestCaseSpec> &testCases = qmlVisitor.testCases();

    for (const QuickTestCaseSpec &testCase : testCases) {
        const QString testCaseName = testCase.m_caseName;

        QuickTestParseResult *parseResult = new QuickTestParseResult(framework);
        parseResult->proFile = proFile;
        parseResult->itemType = TestTreeItem::TestCase;
        if (!testCaseName.isEmpty()) {
            parseResult->fileName = testCase.m_locationAndType.m_filePath;
            parseResult->name = testCaseName;
            parseResult->line = testCase.m_locationAndType.m_line;
            parseResult->column = testCase.m_locationAndType.m_column;
        }

        for (const auto &function : testCase.m_functions) {
            QuickTestParseResult *funcResult = new QuickTestParseResult(framework);
            funcResult->name = function.m_name;
            funcResult->displayName = function.m_name;
            funcResult->itemType = function.m_type;
            funcResult->fileName = function.m_filePath;
            funcResult->line = function.m_line;
            funcResult->column = function.m_column;
            funcResult->proFile = proFile;

            parseResult->children.append(funcResult);
        }

        promise.addResult(TestParseResultPtr(parseResult));
    }
    return true;
}

bool QuickTestParser::handleQtQuickTest(QPromise<TestParseResultPtr> &promise,
                                        CPlusPlus::Document::Ptr document,
                                        ITestFramework *framework)
{
    if (quickTestName(document).isEmpty())
        return false;

    QList<CppEditor::ProjectPart::ConstPtr> ppList =
        CppEditor::CppModelManager::projectPart(document->filePath());
    if (ppList.isEmpty()) // happens if shutting down while parsing
        return false;
    const FilePath cppFileName = document->filePath();
    const FilePath proFile = FilePath::fromString(ppList.at(0)->projectFile);
    {
        QWriteLocker lock(&m_parseLock);
        m_mainCppFiles.insert(cppFileName, proFile);
    }
    const FilePath srcDir = FilePath::fromString(quickTestSrcDir(cppFileName));
    if (srcDir.isEmpty())
        return false;

    if (promise.isCanceled())
        return false;
    const QList<Document::Ptr> qmlDocs = scanDirectoryForQuickTestQmlFiles(srcDir);
    bool result = false;
    for (const Document::Ptr &qmlJSDoc : qmlDocs) {
        if (promise.isCanceled())
            break;
        result |= checkQmlDocumentForQuickTestCode(promise, qmlJSDoc, framework, proFile,
                                                   m_checkForDerivedTests);
    }
    return result;
}

static QMap<QString, QDateTime> qmlFilesWithMTime(const QString &directory)
{
    const QFileInfoList &qmlFiles = QDir(directory).entryInfoList({ "*.qml" },
                                                                  QDir::Files, QDir::Name);
    QMap<QString, QDateTime> filesAndDates;
    for (const QFileInfo &info : qmlFiles)
        filesAndDates.insert(info.fileName(), info.lastModified());
    return filesAndDates;
}

void QuickTestParser::handleDirectoryChanged(const QString &directory)
{
    const QMap<QString, QDateTime> &filesAndDates = qmlFilesWithMTime(directory);
    const QMap<QString, QDateTime> &watched = m_watchedFiles.value(directory);
    const QList<QString> &keys = watched.keys();
    if (filesAndDates.keys() != keys) { // removed or added files
        m_watchedFiles[directory] = filesAndDates;
        TestTreeModel::instance()->parser()->emitUpdateTestTree(this);
    } else { // we might still have different timestamps
        const bool timestampChanged = Utils::anyOf(keys, [&](const QString &file) {
            return filesAndDates.value(file) != watched.value(file);
        });
        if (timestampChanged) {
            m_watchedFiles[directory] = filesAndDates;
            PathsAndLanguages paths;
            paths.maybeInsert(FilePath::fromString(directory), Dialect::Qml);
            ModelManagerInterface *qmlJsMM = ModelManagerInterface::instance();
            ModelManagerInterface::importScan(ModelManagerInterface::workingCopy(), paths,
                                              qmlJsMM,
                                              true /*emitDocumentChanges*/,
                                              false /*onlyTheLib*/,
                                              true /*forceRescan*/ );
        }
    }
}

void QuickTestParser::doUpdateWatchPaths(const QStringList &directories)
{
    for (const QString &dir : directories) {
        m_directoryWatcher.addPath(dir);
        m_watchedFiles[dir] = qmlFilesWithMTime(dir);
    }
}

QuickTestParser::QuickTestParser(ITestFramework *framework)
    : CppParser(framework)
{
    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::startupProjectChanged, this, [this] {
        const QStringList &dirs = m_directoryWatcher.directories();
        if (!dirs.isEmpty())
            m_directoryWatcher.removePaths(dirs);
        m_watchedFiles.clear();
    });
    connect(&m_directoryWatcher, &QFileSystemWatcher::directoryChanged,
            this, &QuickTestParser::handleDirectoryChanged);
}

void QuickTestParser::init(const QSet<FilePath> &filesToParse, bool fullParse)
{
    m_qmlSnapshot = QmlJSTools::Internal::ModelManager::instance()->snapshot();
    QWriteLocker lock(&m_parseLock); // should not be necessary
    if (!fullParse) {
        // in a full parse we get the correct entry points by the respective main
        m_proFilesForQmlFiles = QuickTestUtils::proFilesForQmlFiles(framework(), filesToParse);
        // get rid of cached main cpp files that are going to get processed anyhow
        for (const FilePath &file : filesToParse) {
            if (m_mainCppFiles.remove(file) == 1) {
                if (m_mainCppFiles.isEmpty())
                    break;
            }
        }
    } else {
        // get rid of all cached main cpp files
        m_mainCppFiles.clear();
    }
    lock.unlock();

    m_checkForDerivedTests = theQtTestFramework().quickCheckForDerivedTests();

    if (std::optional<QSet<Utils::FilePath>> prefiltered = filesContainingMacro("QT_QMLTEST_LIB"))
        m_prefilteredFiles = prefiltered->intersect(filesToParse);
    else
        m_prefilteredFiles = filesToParse;

    CppParser::init(filesToParse, fullParse);
}

void QuickTestParser::release()
{
    m_qmlSnapshot = Snapshot();
    m_proFilesForQmlFiles.clear();
    m_prefilteredFiles.clear();
    CppParser::release();
}

bool QuickTestParser::processDocument(QPromise<TestParseResultPtr> &promise,
                                      const FilePath &fileName)
{
    if (fileName.endsWith(".qml")) {
        const FilePath &proFile = m_proFilesForQmlFiles.value(fileName);
        if (proFile.isEmpty())
            return false;
        Document::Ptr qmlJSDoc = m_qmlSnapshot.document(fileName);
        return checkQmlDocumentForQuickTestCode(promise,
                                                qmlJSDoc,
                                                framework(),
                                                proFile,
                                                m_checkForDerivedTests);
    }

    if (!m_prefilteredFiles.contains(fileName))
        return false;

   CPlusPlus::Document::Ptr cppdoc = document(fileName);
   if (cppdoc.isNull() || !includesQtQuickTest(cppdoc, m_cppSnapshot))
       return false;

   return handleQtQuickTest(promise, cppdoc, framework());
}

FilePath QuickTestParser::projectFileForMainCppFile(const FilePath &fileName)
{
    QReadLocker lock(&m_parseLock);
    return m_mainCppFiles.value(fileName);
}

} // namespace Autotest::Internal
