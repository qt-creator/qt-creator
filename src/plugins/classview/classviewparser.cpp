// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewparser.h"

#include <cppeditor/cppmodelmanager.h>

#include <QElapsedTimer>
#include <QDebug>
#include <QHash>
#include <QSet>

enum { debug = false };

using namespace ProjectExplorer;
using namespace Utils;

namespace ClassView {
namespace Internal {

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
    //! Get document from documentList
    CPlusPlus::Document::Ptr document(const FilePath &fileName) const;

    struct DocumentCache {
        unsigned treeRevision = 0;
        ParserTreeItem::ConstPtr tree;
        CPlusPlus::Document::Ptr document;
    };
    struct ProjectCache {
        unsigned treeRevision = 0;
        ParserTreeItem::ConstPtr tree;
        QString projectName;
        QSet<FilePath> fileNames;
    };

    // Project file path to its cached data
    QHash<FilePath, DocumentCache> m_documentCache;
    // Project file path to its cached data
    QHash<FilePath, ProjectCache> m_projectCache;

    //! Flat mode
    bool flatMode = false;
};

CPlusPlus::Document::Ptr ParserPrivate::document(const FilePath &fileName) const
{
    return m_documentCache.value(fileName).document;
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
        const SymbolInformation projectInfo = { projectCache.projectName, projectPath.toString() };
        ParserTreeItem::ConstPtr item = getCachedOrParseProjectTree(projectPath, projectCache.fileNames);
        if (item.isNull())
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
    unsigned revision = 0;
    for (const FilePath &fileInProject : filesInProject) {
        const CPlusPlus::Document::Ptr &doc = d->document(fileInProject);
        if (doc.isNull())
            continue;

        revision += doc->revision();

        const ParserTreeItem::ConstPtr docTree = getCachedOrParseDocumentTree(doc);
        if (docTree.isNull())
            continue;
        docTrees.append(docTree);
    }

    ParserTreeItem::ConstPtr item = ParserTreeItem::mergeTrees(projectPath, docTrees);

    // update the cache
    if (!projectPath.isEmpty()) {
        ParserPrivate::ProjectCache &projectCache = d->m_projectCache[projectPath];
        projectCache.tree = item;
        projectCache.treeRevision = revision;
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
    if (it != d->m_projectCache.constEnd() && !it.value().tree.isNull()) {
        // calculate project's revision
        unsigned revision = 0;
        for (const FilePath &fileInProject : filesInProject) {
            const CPlusPlus::Document::Ptr &doc = d->document(fileInProject);
            if (doc.isNull())
                continue;
            revision += doc->revision();
        }

        // if even revision is the same, return cached project
        if (revision == it.value().treeRevision)
            return it.value().tree;
    }

    return getParseProjectTree(projectPath, filesInProject);
}

/*!
    Parses the document \a doc if it is in the project files and adds a tree to
    the internal storage. Updates the internal cached tree for this document.

    \sa parseDocument
*/

ParserTreeItem::ConstPtr Parser::getParseDocumentTree(const CPlusPlus::Document::Ptr &doc)
{
    if (doc.isNull())
        return ParserTreeItem::ConstPtr();

    const FilePath fileName = doc->filePath();

    ParserTreeItem::ConstPtr itemPtr = ParserTreeItem::parseDocument(doc);

    d->m_documentCache.insert(fileName, { doc->revision(), itemPtr, doc } );
    return itemPtr;
}

/*!
    Gets the document \a doc from the cache or parses it if it is in the project
    files and adds a tree to the internal storage.

    \sa parseDocument
*/

ParserTreeItem::ConstPtr Parser::getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc)
{
    if (doc.isNull())
        return ParserTreeItem::ConstPtr();

    const auto it = d->m_documentCache.constFind(doc->filePath());
    if (it != d->m_documentCache.constEnd() && !it.value().tree.isNull()
            && it.value().treeRevision == doc->revision()) {
        return it.value().tree;
    }
    return getParseDocumentTree(doc);
}

/*!
    Parses the document list \a docs if they are in the project files and adds a tree to
    the internal storage.
*/

void Parser::updateDocuments(const QSet<FilePath> &documentPaths)
{
    updateDocumentsFromSnapshot(documentPaths, CppEditor::CppModelManager::snapshot());
}

void Parser::updateDocumentsFromSnapshot(const QSet<FilePath> &documentPaths,
                                 const CPlusPlus::Snapshot &snapshot)
{
    for (const FilePath &documentPath : documentPaths) {
        CPlusPlus::Document::Ptr doc = snapshot.document(documentPath);
        if (doc.isNull())
            continue;

        getParseDocumentTree(doc);
    }
    requestCurrentState();
}

/*!
    Removes the files defined in the \a fileList from the parsing.
*/

void Parser::removeFiles(const QStringList &fileList)
{
    if (fileList.isEmpty())
        return;

    for (const QString &name : fileList) {
        const FilePath filePath = FilePath::fromString(name);
        d->m_documentCache.remove(filePath);
        d->m_projectCache.remove(filePath);
        for (auto it = d->m_projectCache.begin(); it != d->m_projectCache.end(); ++it)
            it.value().fileNames.remove(filePath);
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

    const CPlusPlus::Snapshot &snapshot = CppEditor::CppModelManager::snapshot();
    for (auto it = projects.cbegin(); it != projects.cend(); ++it) {
        const auto projectData = it.value();
        QSet<FilePath> commonFiles;
        for (const auto &fileInProject : projectData.second) {
            CPlusPlus::Document::Ptr doc = snapshot.document(fileInProject);
            if (doc.isNull())
                continue;
            commonFiles.insert(fileInProject);
            d->m_documentCache[fileInProject].document = doc;
        }
        d->m_projectCache.insert(it.key(), { 0, nullptr, projectData.first, commonFiles });
    }

    requestCurrentState();
}

void Parser::addProject(const FilePath &projectPath, const QString &projectName,
                        const FilePaths &filesInProject)
{
    const CPlusPlus::Snapshot &snapshot = CppEditor::CppModelManager::snapshot();
    QSet<FilePath> commonFiles;
    for (const auto &fileInProject : filesInProject) {
        CPlusPlus::Document::Ptr doc = snapshot.document(fileInProject);
        if (doc.isNull())
            continue;
        commonFiles.insert(fileInProject);
        d->m_documentCache[fileInProject].document = doc;
    }
    d->m_projectCache.insert(projectPath, { 0, nullptr, projectName, commonFiles });
    updateDocumentsFromSnapshot(commonFiles, snapshot);
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

/*!
    Requests to emit a signal with the current tree state.
*/
void Parser::requestCurrentState()
{
    emit treeRegenerated(parse());
}

} // namespace Internal
} // namespace ClassView
