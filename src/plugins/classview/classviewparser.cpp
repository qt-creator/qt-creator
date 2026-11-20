// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewparser.h"

#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cppprojectfile.h>

#include <QElapsedTimer>
#include <QDebug>
#include <QHash>
#include <QSet>

#ifdef WITH_TESTS
#include "classviewutils.h"

#include <cppeditor/cpptoolstestcase.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>

#include <utils/algorithm.h>

#include <QSignalSpy>
#include <QStandardItem>
#include <QTest>
#endif

enum { debug = false };

using namespace ProjectExplorer;
using namespace Utils;

namespace ClassView::Internal {

// ----------------------------- ParserPrivate ---------------------------------

/*!
   \class ParserPrivate
   \brief The ParserPrivate class defines private class data for the Parser
   class.
   \sa Parser
 */

/*!
   \class Parser
   \brief The Parser class parses C++ information. Multithreading is supported.
*/

class ParserPrivate
{
public:
    unsigned updateCount(const FilePath &filePath) const
    {
        return m_updateCount.value(filePath, 0);
    }

    struct DocumentCache {
        unsigned builtAtUpdateCount = 0;
        ParserTreeItem::ConstPtr tree;
    };
    struct ProjectCache {
        unsigned builtAtUpdateCount = 0;
        ParserTreeItem::ConstPtr tree;
        QString projectName;
        QSet<FilePath> fileNames;
    };

    // Project file path to its cached data
    QHash<FilePath, DocumentCache> m_documentCache;
    // Project file path to its cached data
    QHash<FilePath, ProjectCache> m_projectCache;
    // How often the code model has reported a new document for each file. Used instead
    // of the document's revision: a file the snapshot lacks has no document, and every
    // document read on request has the same revision.
    QHash<FilePath, unsigned> m_updateCount;

    //! Flat mode
    bool flatMode = false;

    //! Whether a tree is worth building. \sa Parser::setEnabled
    bool enabled = false;
};

static QSet<FilePath> filesToRead(const FilePaths &filesInProject)
{
    QSet<FilePath> files;
    files.reserve(filesInProject.size());
    for (const FilePath &filePath : filesInProject) {
        switch (CppEditor::ProjectFile::classify(filePath)) {
        case CppEditor::ProjectFile::CHeader:
        case CppEditor::ProjectFile::CSource:
        case CppEditor::ProjectFile::CXXHeader:
        case CppEditor::ProjectFile::CXXSource:
        case CppEditor::ProjectFile::ObjCHeader:
        case CppEditor::ProjectFile::ObjCSource:
        case CppEditor::ProjectFile::ObjCXXHeader:
        case CppEditor::ProjectFile::ObjCXXSource:
        case CppEditor::ProjectFile::CudaSource:
        case CppEditor::ProjectFile::OpenCLSource:
        case CppEditor::ProjectFile::AmbiguousHeader:
            files.insert(filePath);
            break;
        case CppEditor::ProjectFile::Unclassified:
        case CppEditor::ProjectFile::Unsupported:
            break;
        }
    }
    return files;
}

// ----------------------------- Parser ---------------------------------

/*!
    Constructs the parser object.
*/

Parser::Parser(QObject *parent)
    : QObject(parent),
    d(new ParserPrivate())
{
}

/*!
    Destructs the parser object.
*/

Parser::~Parser()
{
    delete d;
}

/*!
    Switches to flat mode (without subprojects) if \a flat returns \c true.
*/

void Parser::setFlatMode(bool flatMode)
{
    if (flatMode == d->flatMode)
        return;

    // change internal
    d->flatMode = flatMode;

    // regenerate and resend current tree
    requestCurrentState();
}

/*!
    Parses the class and produces a new tree.

    \sa addProject
*/

ParserTreeItem::ConstPtr Parser::parse()
{
    QScopedPointer<QElapsedTimer> timer;
    if (debug) {
        timer.reset(new QElapsedTimer());
        timer->start();
    }

    QHash<SymbolInformation, ParserTreeItem::ConstPtr> projectTrees;

    for (auto it = d->m_projectCache.cbegin(); it != d->m_projectCache.cend(); ++it) {
        const ParserPrivate::ProjectCache &projectCache = it.value();
        const FilePath projectPath = it.key();
        const SymbolInformation projectInfo = {projectCache.projectName, projectPath.path()};
        ParserTreeItem::ConstPtr item = getCachedOrParseProjectTree(projectPath, projectCache.fileNames);
        if (!item)
            continue;
        projectTrees.insert(projectInfo, item);
    }

    ParserTreeItem::ConstPtr rootItem(new ParserTreeItem(projectTrees));

    if (debug) {
        qDebug() << "Class View:" << QDateTime::currentDateTime().toString()
                 << "Parsed in " << timer->elapsed() << "msecs.";
    }

    return rootItem;
}

/*!
    Parses the project with the \a projectId and adds the documents from the
    \a fileList to the project. Updates the internal cached tree for this
    project.
*/

ParserTreeItem::ConstPtr Parser::getParseProjectTree(const FilePath &projectPath,
                                                     const QSet<FilePath> &filesInProject)
{
    //! \todo Way to optimize - for documentUpdate - use old cached project and subtract
    //! changed files only (old edition), and add curent editions

    QList<ParserTreeItem::ConstPtr> docTrees;
    unsigned updateCount = 0;
    for (const FilePath &fileInProject : filesInProject) {
        updateCount += d->updateCount(fileInProject);

        const ParserTreeItem::ConstPtr docTree = getCachedOrParseDocumentTree(fileInProject);
        if (!docTree)
            continue;
        docTrees.append(docTree);
    }

    ParserTreeItem::ConstPtr item = ParserTreeItem::mergeTrees(projectPath, docTrees);

    // update the cache
    if (!projectPath.isEmpty()) {
        ParserPrivate::ProjectCache &projectCache = d->m_projectCache[projectPath];
        projectCache.tree = item;
        projectCache.builtAtUpdateCount = updateCount;
    }
    return item;
}

/*!
    Gets the project with \a projectId from the cache if it is valid or parses
    the project and adds the documents from the \a fileList to the project.
    Updates the internal cached tree for this project.
*/

ParserTreeItem::ConstPtr Parser::getCachedOrParseProjectTree(const FilePath &projectPath,
                                                             const QSet<FilePath> &filesInProject)
{
    const auto it = d->m_projectCache.constFind(projectPath);
    if (it != d->m_projectCache.constEnd() && it.value().tree) {
        // calculate project's update count
        unsigned updateCount = 0;
        for (const FilePath &fileInProject : filesInProject)
            updateCount += d->updateCount(fileInProject);

        // if even update count is the same, return cached project
        if (updateCount == it.value().builtAtUpdateCount)
            return it.value().tree;
    }

    return getParseProjectTree(projectPath, filesInProject);
}

/*!
    Parses the document \a doc if it is in the project files and adds a tree to
    the internal storage. Updates the internal cached tree for this document.

    \sa parseDocument
*/

ParserTreeItem::ConstPtr Parser::getParseDocumentTree(const FilePath &filePath)
{
    if (filePath.isEmpty())
        return ParserTreeItem::ConstPtr();

    const CPlusPlus::Document::Ptr doc
        = CppEditor::CppModelManager::document(filePath, CppEditor::DocumentLookup::ReadIfNeeded);
    if (doc.isNull())
        return ParserTreeItem::ConstPtr();

    ParserTreeItem::ConstPtr itemPtr = ParserTreeItem::parseDocument(doc);

    d->m_documentCache.insert(filePath, { d->updateCount(filePath), itemPtr } );
    return itemPtr;
}

/*!
    Gets the document \a doc from the cache or parses it if it is in the project
    files and adds a tree to the internal storage.

    \sa parseDocument
*/

ParserTreeItem::ConstPtr Parser::getCachedOrParseDocumentTree(const FilePath &filePath)
{
    if (filePath.isEmpty())
        return ParserTreeItem::ConstPtr();

    const auto it = d->m_documentCache.constFind(filePath);
    if (it != d->m_documentCache.constEnd() && it.value().tree
            && it.value().builtAtUpdateCount == d->updateCount(filePath)) {
        return it.value().tree;
    }
    return getParseDocumentTree(filePath);
}

/*!
    Parses the document list \a docs if they are in the project files and adds a tree to
    the internal storage.
*/

void Parser::updateDocuments(const QSet<FilePath> &documentPaths)
{
    for (const FilePath &documentPath : documentPaths)
        ++d->m_updateCount[documentPath];

    for (const FilePath &documentPath : documentPaths)
        getParseDocumentTree(documentPath);
    requestCurrentState();
}

/*!
    Removes the files defined in the \a fileList from the parsing.
*/

void Parser::removeFiles(const FilePaths &fileList)
{
    if (fileList.isEmpty())
        return;

    for (const FilePath &filePath : fileList) {
        d->m_documentCache.remove(filePath);
        d->m_updateCount.remove(filePath);
        d->m_projectCache.remove(filePath);
        // The update counts need not change, so the cached tree is dropped explicitly.
        for (auto it = d->m_projectCache.begin(); it != d->m_projectCache.end(); ++it) {
            if (it.value().fileNames.remove(filePath))
                it.value().tree.reset();
        }
    }
    requestCurrentState();
}

/*!
    Fully resets the internal state of the code parser to \a snapshot.
*/
void Parser::resetData(const QHash<FilePath, QPair<QString, FilePaths>> &projects)
{
    d->m_projectCache.clear();
    d->m_documentCache.clear();

    for (auto it = projects.cbegin(); it != projects.cend(); ++it) {
        const auto projectData = it.value();
        d->m_projectCache.insert(
            it.key(), {0, nullptr, projectData.first, filesToRead(projectData.second)});
    }

    requestCurrentState();
}

void Parser::addProject(const FilePath &projectPath, const QString &projectName,
                        const FilePaths &filesInProject)
{
    // Files are read only when the pane asks for a tree.
    d->m_projectCache.insert(projectPath, {0, nullptr, projectName, filesToRead(filesInProject)});
    requestCurrentState();
}

void Parser::removeProject(const FilePath &projectPath)
{
    auto it = d->m_projectCache.find(projectPath);
    if (it == d->m_projectCache.end())
        return;

    const QSet<FilePath> &filesInProject = it.value().fileNames;
    for (const FilePath &fileInProject : filesInProject)
        d->m_documentCache.remove(fileInProject);

    d->m_projectCache.erase(it);

    requestCurrentState();
}

void Parser::setEnabled(bool enabled)
{
    d->enabled = enabled;
}

/*!
    Requests to emit a signal with the current tree state.
*/
void Parser::requestCurrentState()
{
    // Building a tree may read every unread project file, which is worth it only
    // while the pane is shown.
    if (!d->enabled)
        return;

    emit treeRegenerated(parse());
}

} // namespace ClassView::Internal

#ifdef WITH_TESTS

namespace ClassView::Internal {

static QStringList rowsUnder(const ParserTreeItem::ConstPtr &item)
{
    if (!item)
        return {};

    QStandardItem root;
    item->fetchMore(&root);

    QStringList names;
    for (int row = 0; row < root.rowCount(); ++row)
        names << symbolInformationFromItem(root.child(row)).name();
    return names;
}

static ParserTreeItem::ConstPtr rowNamed(const ParserTreeItem::ConstPtr &item, const QString &name)
{
    QStandardItem root;
    item->fetchMore(&root);

    for (int row = 0; row < root.rowCount(); ++row) {
        const SymbolInformation information = symbolInformationFromItem(root.child(row));
        if (information.name() == name)
            return item->child(information);
    }
    return {};
}

// The parser on the test's thread, so that a request and its tree are one step.
class DrivenParser
{
public:
    DrivenParser()
    {
        QObject::connect(&parser, &Parser::treeRegenerated,
                         [this](const ParserTreeItem::ConstPtr &root) { tree = root; });
        // A disabled parser builds no tree.
        parser.setEnabled(true);
    }

    Parser parser;
    ParserTreeItem::ConstPtr tree;
};

class ClassViewParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void testASecondSessionShowsTheSameClasses();
    void testClassesOfAFileTheIndexerDoesNotDeliverAreShown();
    void testADisabledParserReadsNoFile();
    void testTheClassesOfARemovedFileAreGone();
};

void ClassViewParserTest::testASecondSessionShowsTheSameClasses()
{
    Kit * const kit = CppEditor::Tests::kitWithQt();
    if (!kit)
        QSKIP("The test requires a valid kit with a C++ compiler and a Qt");

    CppEditor::Tests::TemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const FilePath projectDir = tmpDir.filePath();
    const FilePath header = projectDir / "declared.h";
    const FilePath source = projectDir / "written.cpp";
    QVERIFY(header.writeFileContents("class Declared {};\n"));
    QVERIFY(source.writeFileContents("class Written {};\n"));
    QVERIFY((projectDir / "p.pro").writeFileContents(
        "TEMPLATE = app\nTARGET = p\nHEADERS = declared.h\nSOURCES = written.cpp\n"));

    const FilePath projectFile = projectDir / "p.pro";

    // One parser for both sessions, as in the plugin.
    DrivenParser driven;
    {
        QSignalSpy refreshed(CppEditor::CppModelManager::instance(),
                             &CppEditor::CppModelManager::sourceFilesRefreshed);
        CppEditor::Tests::ProjectOpenerAndCloser projectMgr;
        QVERIFY(projectMgr.open(projectFile, kit));
        QVERIFY(!refreshed.isEmpty() || refreshed.wait(30000));
        driven.tree = {};
        driven.parser.addProject(projectFile, "Test",
                                 projectMgr.projects().first()->files(Project::SourceFiles));
        QVERIFY(driven.tree);
        QCOMPARE(rowsUnder(rowNamed(driven.tree, "Test")), (QStringList{"Declared", "Written"}));
        driven.parser.removeProject(projectFile);
    }

    // The second session starts without the documents of the first one.
    QVERIFY(!CppEditor::CppModelManager::snapshot().contains(source));

    QSignalSpy refreshed(CppEditor::CppModelManager::instance(),
                         &CppEditor::CppModelManager::sourceFilesRefreshed);
    CppEditor::Tests::ProjectOpenerAndCloser projectMgr;
    QVERIFY(projectMgr.open(projectFile, kit));
    QVERIFY(!refreshed.isEmpty() || refreshed.wait(30000));
    driven.tree = {};
    driven.parser.addProject(projectFile, "Test",
                             projectMgr.projects().first()->files(Project::SourceFiles));
    QVERIFY(driven.tree);
    QCOMPARE(rowsUnder(rowNamed(driven.tree, "Test")), (QStringList{"Declared", "Written"}));
}

// Opens a project of a header and a source and removes the source from the snapshot,
// so that only reading it on request can show its class. Returns once reading on
// request works, which it does only after the project is fully indexed.
static bool openProjectWithSourceOutsideSnapshot(
    CppEditor::Tests::ProjectOpenerAndCloser &projectMgr, const FilePath &projectDir,
    FilePath *projectFile, FilePath *source)
{
    Kit * const kit = CppEditor::Tests::kitWithQt();
    if (!kit)
        return false;
    *projectFile = projectDir / "p.pro";
    *source = projectDir / "written.cpp";
    const FilePath probe = projectDir / "probe.cpp";
    if (!(projectDir / "declared.h").writeFileContents("class Declared {};\n")
        || !source->writeFileContents("class Written {};\n")
        || !probe.writeFileContents({})
        || !projectFile->writeFileContents("TEMPLATE = app\nTARGET = p\nHEADERS = declared.h\n"
                                           "SOURCES = written.cpp probe.cpp\n")) {
        return false;
    }
    QSignalSpy refreshed(CppEditor::CppModelManager::instance(),
                         &CppEditor::CppModelManager::sourceFilesRefreshed);
    if (!projectMgr.open(*projectFile, kit) || (refreshed.isEmpty() && !refreshed.wait(30000)))
        return false;
    CppEditor::Tests::removeFromSnapshot({*source, probe});
    return QTest::qWaitFor([&probe] {
        return bool(CppEditor::CppModelManager::document(
            probe, CppEditor::DocumentLookup::ReadIfNeeded));
    });
}

void ClassViewParserTest::testClassesOfAFileTheIndexerDoesNotDeliverAreShown()
{
    if (!CppEditor::Tests::kitWithQt())
        QSKIP("The test requires a valid kit with a C++ compiler and a Qt");

    CppEditor::Tests::TemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    CppEditor::Tests::ProjectOpenerAndCloser projectMgr;
    FilePath projectFile;
    FilePath source;
    QVERIFY(openProjectWithSourceOutsideSnapshot(projectMgr, tmpDir.filePath(), &projectFile,
                                                 &source));

    DrivenParser driven;
    driven.parser.addProject(projectFile, "Test",
                             projectMgr.projects().first()->files(Project::SourceFiles));
    QVERIFY(driven.tree);
    QCOMPARE(rowsUnder(rowNamed(driven.tree, "Test")), (QStringList{"Declared", "Written"}));
}

void ClassViewParserTest::testADisabledParserReadsNoFile()
{
    if (!CppEditor::Tests::kitWithQt())
        QSKIP("The test requires a valid kit with a C++ compiler and a Qt");

    CppEditor::Tests::TemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    CppEditor::Tests::ProjectOpenerAndCloser projectMgr;
    FilePath projectFile;
    FilePath source;
    QVERIFY(openProjectWithSourceOutsideSnapshot(projectMgr, tmpDir.filePath(), &projectFile,
                                                 &source));

    // As while the pane is hidden.
    DrivenParser driven;
    driven.parser.setEnabled(false);
    driven.parser.addProject(projectFile, "Test",
                             projectMgr.projects().first()->files(Project::SourceFiles));
    QVERIFY(!driven.tree);
    QVERIFY(!CppEditor::CppModelManager::document(source,
                                                  CppEditor::DocumentLookup::SnapshotOnly));
}

void ClassViewParserTest::testTheClassesOfARemovedFileAreGone()
{
    if (!CppEditor::Tests::kitWithQt())
        QSKIP("The test requires a valid kit with a C++ compiler and a Qt");

    CppEditor::Tests::TemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    CppEditor::Tests::ProjectOpenerAndCloser projectMgr;
    FilePath projectFile;
    FilePath source;
    QVERIFY(openProjectWithSourceOutsideSnapshot(projectMgr, tmpDir.filePath(), &projectFile,
                                                 &source));

    DrivenParser driven;
    driven.parser.addProject(projectFile, "Test",
                             projectMgr.projects().first()->files(Project::SourceFiles));
    QCOMPARE(rowsUnder(rowNamed(driven.tree, "Test")), (QStringList{"Declared", "Written"}));

    // As when the project drops the file, which no reading of another file follows.
    driven.tree = {};
    driven.parser.removeFiles({source});
    QVERIFY(driven.tree);
    QCOMPARE(rowsUnder(rowNamed(driven.tree, "Test")), (QStringList{"Declared"}));
}

QObject *createClassViewParserTest()
{
    return new ClassViewParserTest;
}

} // namespace ClassView::Internal

#include "classviewparser.moc"

#endif // WITH_TESTS
