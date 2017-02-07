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

#include "quicktestparser.h"
#include "quicktesttreeitem.h"
#include "quicktestvisitors.h"
#include "quicktest_utils.h"
#include "../testcodeparser.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpart.h>
#include <projectexplorer/session.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdialect.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <utils/hostosinfo.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

TestTreeItem *QuickTestParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root || itemType == TestTreeItem::TestDataTag)
        return nullptr;

    QuickTestTreeItem *item = new QuickTestTreeItem(name, fileName, itemType);
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
    static QStringList expectedHeaderPrefixes
            = Utils::HostOsInfo::isMacHost()
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

    for (const QString &include : snapshot.allIncludesForDocument(doc->fileName())) {
        for (const QString &prefix : expectedHeaderPrefixes) {
            if (include.endsWith(QString("%1/quicktest.h").arg(prefix)))
                return true;
        }
    }
    return false;
}

static QString quickTestSrcDir(const CppTools::CppModelManager *cppMM,
                               const QString &fileName)
{
    const QList<CppTools::ProjectPart::Ptr> parts = cppMM->projectPart(fileName);
    if (parts.size() > 0) {
        const ProjectExplorer::Macros &macros = parts.at(0)->projectMacros;
        auto found = std::find_if(
                    macros.begin(),
                    macros.end(),
                    [] (const ProjectExplorer::Macro &macro) { return macro.key == "QUICK_TEST_SOURCE_DIR"; });
       if (found != macros.end())  {
           QByteArray result = found->value;
           if (result.startsWith('"'))
               result.remove(result.length() - 1, 1).remove(0, 1);
           if (result.startsWith("\\\""))
               result.remove(result.length() - 2, 2).remove(0, 2);
           return QLatin1String(result);
       }
    }
    return QString();
}

static QString quickTestName(const CPlusPlus::Document::Ptr &doc)
{
    const QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();

    for (const CPlusPlus::Document::MacroUse &macro : macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (QuickTestUtils::isQuickTestMacro(name)) {
            CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(CppParser::getFileContent(doc->fileName())
                                 .mid(arg.bytesBegin(), arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    return QString();
}

QList<QmlJS::Document::Ptr> QuickTestParser::scanDirectoryForQuickTestQmlFiles(const QString &srcDir) const
{
    QStringList dirs(srcDir);
    QmlJS::ModelManagerInterface *qmlJsMM = QmlJSTools::Internal::ModelManager::instance();
    // make sure even files not listed in pro file are available inside the snapshot
    QFutureInterface<void> future;
    QmlJS::PathsAndLanguages paths;
    paths.maybeInsert(Utils::FileName::fromString(srcDir), QmlJS::Dialect::Qml);
    QmlJS::ModelManagerInterface::importScan(future, qmlJsMM->workingCopy(), paths, qmlJsMM,
        false /*emitDocumentChanges*/, false /*onlyTheLib*/, true /*forceRescan*/ );

    const QmlJS::Snapshot snapshot = QmlJSTools::Internal::ModelManager::instance()->snapshot();
    QDirIterator it(srcDir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi(it.fileInfo().canonicalFilePath());
        dirs.append(fi.filePath());
    }
    emit updateWatchPaths(dirs);

    QList<QmlJS::Document::Ptr> foundDocs;

    for (const QString &path : dirs) {
        const QList<QmlJS::Document::Ptr> docs = snapshot.documentsInDirectory(path);
        for (const QmlJS::Document::Ptr &doc : docs) {
            const QFileInfo fi(doc->fileName());
            // using working copy above might provide no more existing files
            if (!fi.exists())
                continue;
            const QString fileName(fi.fileName());
            if (fileName.startsWith("tst_") && fileName.endsWith(".qml"))
                foundDocs << doc;
        }
    }

    return foundDocs;
}

static bool checkQmlDocumentForQuickTestCode(QFutureInterface<TestParseResultPtr> futureInterface,
                                             const QmlJS::Document::Ptr &qmlJSDoc,
                                             const Core::Id &id,
                                             const QString &proFile = QString())
{
    if (qmlJSDoc.isNull())
        return false;
    QmlJS::AST::Node *ast = qmlJSDoc->ast();
    QTC_ASSERT(ast, return false);
    QmlJS::Snapshot snapshot = QmlJS::ModelManagerInterface::instance()->snapshot();
    TestQmlVisitor qmlVisitor(qmlJSDoc, snapshot);
    QmlJS::AST::Node::accept(ast, &qmlVisitor);
    if (!qmlVisitor.isValid())
        return false;

    const QString testCaseName = qmlVisitor.testCaseName();
    const TestCodeLocationAndType tcLocationAndType = qmlVisitor.testCaseLocation();
    const QMap<QString, TestCodeLocationAndType> &testFunctions = qmlVisitor.testFunctions();

    QuickTestParseResult *parseResult = new QuickTestParseResult(id);
    parseResult->proFile = proFile;
    parseResult->itemType = TestTreeItem::TestCase;
    QMap<QString, TestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
    const QMap<QString, TestCodeLocationAndType>::ConstIterator end = testFunctions.end();
    for ( ; it != end; ++it) {
        const TestCodeLocationAndType &loc = it.value();
        QuickTestParseResult *funcResult = new QuickTestParseResult(id);
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

bool QuickTestParser::handleQtQuickTest(QFutureInterface<TestParseResultPtr> futureInterface,
                                        CPlusPlus::Document::Ptr document,
                                        const Core::Id &id) const
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    if (quickTestName(document).isEmpty())
        return false;

    const QString cppFileName = document->fileName();
    QList<CppTools::ProjectPart::Ptr> ppList = modelManager->projectPart(cppFileName);
    if (ppList.isEmpty()) // happens if shutting down while parsing
        return false;
    const QString &proFile = ppList.at(0)->projectFile;

    const QString srcDir = quickTestSrcDir(modelManager, cppFileName);
    if (srcDir.isEmpty())
        return false;

    if (futureInterface.isCanceled())
        return false;
    const QList<QmlJS::Document::Ptr> qmlDocs = scanDirectoryForQuickTestQmlFiles(srcDir);
    bool result = false;
    for (const QmlJS::Document::Ptr &qmlJSDoc : qmlDocs) {
        if (futureInterface.isCanceled())
            break;
        result |= checkQmlDocumentForQuickTestCode(futureInterface, qmlJSDoc, id, proFile);
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
    const QStringList &keys = watched.keys();
    if (filesAndDates.keys() != keys) { // removed or added files
        m_watchedFiles[directory] = filesAndDates;
        TestTreeModel::instance()->parser()->emitUpdateTestTree(this);
    } else { // we might still have different timestamps
        const bool timestampChanged = Utils::anyOf(keys, [&](const QString &file) {
            return filesAndDates.value(file) != watched.value(file);
        });
        if (timestampChanged) {
            QmlJS::PathsAndLanguages paths;
            paths.maybeInsert(Utils::FileName::fromString(directory), QmlJS::Dialect::Qml);
            QFutureInterface<void> future;
            QmlJS::ModelManagerInterface *qmlJsMM = QmlJS::ModelManagerInterface::instance();
            QmlJS::ModelManagerInterface::importScan(future, qmlJsMM->workingCopy(), paths, qmlJsMM,
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

QuickTestParser::QuickTestParser()
    : CppParser()
{
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged, [this] {
        const QStringList &dirs = m_directoryWatcher.directories();
        if (!dirs.isEmpty())
            m_directoryWatcher.removePaths(dirs);
        m_watchedFiles.clear();
    });
    connect(&m_directoryWatcher, &QFileSystemWatcher::directoryChanged,
            this, &QuickTestParser::handleDirectoryChanged);
    connect(this, &QuickTestParser::updateWatchPaths,
            this, &QuickTestParser::doUpdateWatchPaths, Qt::QueuedConnection);
}

QuickTestParser::~QuickTestParser()
{
}

void QuickTestParser::init(const QStringList &filesToParse)
{
    m_qmlSnapshot = QmlJSTools::Internal::ModelManager::instance()->snapshot();
    m_proFilesForQmlFiles = QuickTestUtils::proFilesForQmlFiles(id(), filesToParse);
    CppParser::init(filesToParse);
}

void QuickTestParser::release()
{
    m_qmlSnapshot = QmlJS::Snapshot();
    m_proFilesForQmlFiles.clear();
    CppParser::release();
}

bool QuickTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                      const QString &fileName)
{
    if (fileName.endsWith(".qml")) {
        const QString &proFile = m_proFilesForQmlFiles.value(fileName);
        if (proFile.isEmpty())
            return false;
        QmlJS::Document::Ptr qmlJSDoc = m_qmlSnapshot.document(fileName);
        return checkQmlDocumentForQuickTestCode(futureInterface, qmlJSDoc, id(), proFile);
    }
    if (!m_cppSnapshot.contains(fileName) || !selectedForBuilding(fileName))
        return false;
    CPlusPlus::Document::Ptr document = m_cppSnapshot.find(fileName).value();
    if (!includesQtQuickTest(document, m_cppSnapshot))
        return false;
    return handleQtQuickTest(futureInterface, document, id());
}

} // namespace Internal
} // namespace Autotest
