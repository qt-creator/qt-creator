/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#include "classviewparser.h"
#include "classviewconstants.h"
#include "classviewutils.h"

// cplusplus shared library. the same folder (cplusplus)
#include <cplusplus/Symbol.h>

// other
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QStandardItem>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QElapsedTimer>

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

/*!
    \fn void Parser::treeDataUpdate(QSharedPointer<QStandardItem> result)

    Emits a signal about a tree data update.
*/

class ParserPrivate
{
public:
    //! Get document from documentList
    CPlusPlus::Document::Ptr document(const QString &fileName) const;

    struct DocumentCache {
        unsigned treeRevision = 0;
        ParserTreeItem::ConstPtr tree;
        CPlusPlus::Document::Ptr document;
    };
    struct ProjectCache {
        unsigned treeRevision = 0;
        ParserTreeItem::ConstPtr tree;
        QStringList fileList;
    };

    // Project file path to its cached data
    QHash<QString, DocumentCache> m_documentCache;
    // Project file path to its cached data
    QHash<QString, ProjectCache> m_projectCache;

    // other
    //! List for files which has to be parsed
    QSet<QString> fileList;

    //! Flat mode
    bool flatMode = false;
};

CPlusPlus::Document::Ptr ParserPrivate::document(const QString &fileName) const
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

    // TODO: move a call to SessionManager::projects() out of this thread
    for (const Project *prj : SessionManager::projects()) {
        const QString prjName(prj->displayName());
        const QString prjType = prj->projectFilePath().toString();
        const SymbolInformation inf(prjName, prjType);

        ParserTreeItem::ConstPtr item = addFlatTree(prj);
        if (item.isNull())
            continue;
        projectTrees.insert(inf, item);
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

ParserTreeItem::ConstPtr Parser::getParseProjectTree(const QStringList &fileList,
                                                const QString &projectId)
{
    //! \todo Way to optimize - for documentUpdate - use old cached project and subtract
    //! changed files only (old edition), and add curent editions

    QList<ParserTreeItem::ConstPtr> docTrees;
    unsigned revision = 0;
    for (const QString &file : fileList) {
        const CPlusPlus::Document::Ptr &doc = d->document(file);
        if (doc.isNull())
            continue;

        revision += doc->revision();

        const ParserTreeItem::ConstPtr docTree = getCachedOrParseDocumentTree(doc);
        if (docTree.isNull())
            continue;
        docTrees.append(docTree);
    }

    ParserTreeItem::ConstPtr item = ParserTreeItem::mergeTrees(Utils::FilePath::fromString(projectId), docTrees);

    // update the cache
    if (!projectId.isEmpty()) {
        ParserPrivate::ProjectCache &projectCache = d->m_projectCache[projectId];
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

ParserTreeItem::ConstPtr Parser::getCachedOrParseProjectTree(const QStringList &fileList,
                                                        const QString &projectId)
{
    const auto it = d->m_projectCache.constFind(projectId);
    if (it != d->m_projectCache.constEnd() && !it.value().tree.isNull()) {
        // calculate project's revision
        unsigned revision = 0;
        for (const QString &file : fileList) {
            const CPlusPlus::Document::Ptr &doc = d->document(file);
            if (doc.isNull())
                continue;
            revision += doc->revision();
        }

        // if even revision is the same, return cached project
        if (revision == it.value().treeRevision)
            return it.value().tree;
    }

    return getParseProjectTree(fileList, projectId);
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

    const QString &fileName = doc->fileName();
    if (!d->fileList.contains(fileName))
        return ParserTreeItem::ConstPtr();

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

    const QString &fileName = doc->fileName();
    const auto it = d->m_documentCache.constFind(fileName);
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

void Parser::updateDocuments(const QList<CPlusPlus::Document::Ptr> &docs)
{
    for (const CPlusPlus::Document::Ptr &doc: docs) {
        const QString &name = doc->fileName();

        // if it is external file (not in any of our projects)
        if (!d->fileList.contains(name))
            continue;

        getParseDocumentTree(doc);
    }
    requestCurrentState();
}

/*!
    Specifies the files that must be allowed for the parsing as a \a fileList.
    Files outside of this list will not be in any tree.
*/

void Parser::setFileList(const QStringList &fileList)
{
    d->fileList = Utils::toSet(fileList);
}

/*!
    Removes the files defined in the \a fileList from the parsing.
*/

void Parser::removeFiles(const QStringList &fileList)
{
    if (fileList.isEmpty())
        return;

    for (const QString &name : fileList) {
        d->fileList.remove(name);
        d->m_documentCache.remove(name);
        d->m_projectCache.remove(name);
        for (auto it = d->m_projectCache.begin(); it != d->m_projectCache.end(); ++it)
            it.value().fileList.removeOne(name);
    }
    requestCurrentState();
}

/*!
    Fully resets the internal state of the code parser to \a snapshot.
*/
void Parser::resetData(const CPlusPlus::Snapshot &snapshot)
{
    d->m_projectCache.clear();
    d->m_documentCache.clear();
    for (auto it = snapshot.begin(); it != snapshot.end(); ++it)
        d->m_documentCache[it.key().toString()].document = it.value();

    // recalculate file list
    FilePaths fileList;

    // TODO: move a call to SessionManager::projects() out of this thread
    for (const Project *prj : SessionManager::projects())
        fileList += prj->files(Project::SourceFiles);
    setFileList(Utils::transform(fileList, &FilePath::toString));

    requestCurrentState();
}

/*!
    Fully resets the internal state of the code parser to the current state.

    \sa resetData
*/

void Parser::resetDataToCurrentState()
{
    // get latest data
    resetData(CppTools::CppModelManager::instance()->snapshot());
}

/*!
    Requests to emit a signal with the current tree state.
*/

void Parser::requestCurrentState()
{
    // TODO: we need to have a fresh SessionManager data here, which we could pass to parse()
    emit treeRegenerated(parse());
}

// TODO: don't use Project class in this thread
QStringList Parser::getAllFiles(const Project *project)
{
    if (!project)
        return {};

    const QString projectPath = project->projectFilePath().toString();
    const auto it = d->m_projectCache.constFind(projectPath);
    if (it != d->m_projectCache.constEnd())
        return it.value().fileList;

    const QStringList fileList = Utils::transform(project->files(Project::SourceFiles),
                                                  &FilePath::toString);
    d->m_projectCache[projectPath].fileList = fileList;
    return fileList;
}

// TODO: don't use Project class in this thread
ParserTreeItem::ConstPtr Parser::addFlatTree(const Project *project)
{
    if (!project)
        return {};

    const QStringList fileList = getAllFiles(project);
    if (fileList.isEmpty())
        return {};

    return getCachedOrParseProjectTree(fileList, project->projectFilePath().toString());
}

} // namespace Internal
} // namespace ClassView
