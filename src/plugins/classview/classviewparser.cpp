/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "classviewparser.h"
#include "classviewconstants.h"
#include "classviewutils.h"

// cplusplus shared library. the same folder (cplusplus)
#include <Symbol.h>
#include <Symbols.h>
#include <Scope.h>
#include <Name.h>

// other
#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Icons.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <coreplugin/idocument.h>
#include <utils/qtcassert.h>

#include <QStandardItem>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>

enum { debug = false };

namespace ClassView {
namespace Internal {

// ----------------------------- ParserPrivate ---------------------------------

/*!
   \struct ParserPrivate
   \brief Private class data for \a Parser
   \sa Parser
 */
class ParserPrivate
{
public:
    //! Constructor
    ParserPrivate() : flatMode(false) {}

    //! Get document from documentList
    CPlusPlus::Document::Ptr document(const QString &fileName) const;

    CPlusPlus::Overview overview;

    //! timer
    QPointer<QTimer> timer;

    // documents
    //! Documents read write lock
    QReadWriteLock docLocker;

    //! Current document list
    QHash<QString, CPlusPlus::Document::Ptr> documentList;

    //! Parsed documents' revision - to speed up computations
    QHash<QString, unsigned> cachedDocTreesRevision;

    //! Parsed documents - to speed up computations
    QHash<QString, ParserTreeItem::ConstPtr> cachedDocTrees;

    // project trees
    //! Projects read write lock
    QReadWriteLock prjLocker;

    //! Parsed projects' revision - to speed up computations
    QHash<QString, unsigned> cachedPrjTreesRevision;

    //! Merged trees for projects. Not const - projects might be substracted/added
    QHash<QString, ParserTreeItem::Ptr> cachedPrjTrees;

    //! Cached file lists for projects (non-flat mode)
    QHash<QString, QStringList> cachedPrjFileLists;

    // other
    //! List for files which has to be parsed
    QSet<QString> fileList;

    //! Root item read write lock
    QReadWriteLock rootItemLocker;

    //! Parsed root item
    ParserTreeItem::ConstPtr rootItem;

    //! Flat mode
    bool flatMode;
};

CPlusPlus::Document::Ptr ParserPrivate::document(const QString &fileName) const
{
    if (!documentList.contains(fileName))
        return CPlusPlus::Document::Ptr();
    return documentList[fileName];
}

// ----------------------------- Parser ---------------------------------

Parser::Parser(QObject *parent)
    : QObject(parent),
    d(new ParserPrivate())
{
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);

    // connect signal/slots
    // internal data reset
    connect(this, SIGNAL(resetDataDone()), SLOT(onResetDataDone()), Qt::QueuedConnection);

    // timer for emitting changes
    connect(d->timer, SIGNAL(timeout()), SLOT(requestCurrentState()), Qt::QueuedConnection);
}

Parser::~Parser()
{
    delete d;
}

bool Parser::canFetchMore(QStandardItem *item) const
{
    ParserTreeItem::ConstPtr ptr = findItemByRoot(item);
    if (ptr.isNull())
        return false;
    return ptr->canFetchMore(item);
}

void Parser::fetchMore(QStandardItem *item, bool skipRoot) const
{
    ParserTreeItem::ConstPtr ptr = findItemByRoot(item, skipRoot);
    if (ptr.isNull())
        return;
    ptr->fetchMore(item);
}

void Parser::setFlatMode(bool flatMode)
{
    if (flatMode == d->flatMode)
        return;

    // change internal
    d->flatMode = flatMode;

    // regenerate and resend current tree
    emitCurrentTree();
}

ParserTreeItem::ConstPtr Parser::findItemByRoot(const QStandardItem *item, bool skipRoot) const
{
    if (!item)
        return ParserTreeItem::ConstPtr();

    // go item by item to the root
    QList<const QStandardItem *> uiList;
    const QStandardItem *cur = item;
    while (cur) {
        uiList.append(cur);
        cur = cur->parent();
    }

    if (skipRoot && uiList.count() > 0)
        uiList.removeLast();

    QReadLocker locker(&d->rootItemLocker);

    // using internal root - search correct item
    ParserTreeItem::ConstPtr internal = d->rootItem;

    while (uiList.count() > 0) {
        cur = uiList.last();
        uiList.removeLast();
        const SymbolInformation &inf = Utils::symbolInformationFromItem(cur);
        internal = internal->child(inf);
        if (internal.isNull())
            break;
    }

    return internal;
}

ParserTreeItem::ConstPtr Parser::parse()
{
    QTime time;
    if (debug)
        time.start();

    ParserTreeItem::Ptr rootItem(new ParserTreeItem());

    // check all projects
    QList<ProjectExplorer::Project *> projects = getProjectList();
    foreach (const ProjectExplorer::Project *prj, projects) {
        if (!prj)
            continue;

        ParserTreeItem::Ptr item;
        if (!d->flatMode)
            item = ParserTreeItem::Ptr(new ParserTreeItem());

        QString prjName(prj->displayName());
        QString prjType(prjName);
        if (prj->document())
            prjType = prj->document()->fileName();
        SymbolInformation inf(prjName, prjType);

        QStringList projectList = addProjectNode(item, prj->rootProjectNode());

        if (d->flatMode) {
            // use prj path (prjType) as a project id
//            addProject(item, prj->files(ProjectExplorer::Project::ExcludeGeneratedFiles), prjType);
            //! \todo return back, works too long
            ParserTreeItem::Ptr flatItem = createFlatTree(projectList);
            item.swap(flatItem);
        }
        item->setIcon(prj->rootProjectNode()->icon());
        rootItem->appendChild(item, inf);
    }

    if (debug)
        qDebug() << "Class View:" << QDateTime::currentDateTime().toString()
            << "Parsed in " << time.elapsed() << "msecs.";

    return rootItem;
}

void Parser::addProject(const ParserTreeItem::Ptr &item, const QStringList &fileList,
                                 const QString &projectId)
{
    // recalculate cache tree if needed
    ParserTreeItem::Ptr prj(getCachedOrParseProjectTree(fileList, projectId));
    if (item.isNull())
        return;

    // if there is an item - copy project tree to that item
    item->copy(prj);
}

void Parser::addSymbol(const ParserTreeItem::Ptr &item, const CPlusPlus::Symbol *symbol)
{
    if (item.isNull() || !symbol)
        return;

    // easy solution - lets add any scoped symbol and
    // any symbol which does not contain :: in the name

//    if (symbol->isDeclaration())
//        return;

    //! \todo collect statistics and reorder to optimize
    if (symbol->isForwardClassDeclaration()
        || symbol->isExtern()
        || symbol->isFriend()
        || symbol->isGenerated()
        || symbol->isUsingNamespaceDirective()
        || symbol->isUsingDeclaration()
        )
        return;

    // skip static local functions
//    if ((!symbol->scope() || symbol->scope()->isClass())
//        && symbol->isStatic() && symbol->isFunction())
//        return;


    const CPlusPlus::Name *symbolName = symbol->name();
    if (symbolName && symbolName->isQualifiedNameId())
        return;

    QString name = d->overview.prettyName(symbol->name()).trimmed();
    QString type = d->overview.prettyType(symbol->type()).trimmed();
    int iconType = CPlusPlus::Icons::iconTypeForSymbol(symbol);

    SymbolInformation information(name, type, iconType);

    ParserTreeItem::Ptr itemAdd;

    // If next line will be removed, 5% speed up for the initial parsing.
    // But there might be a problem for some files ???
    // Better to improve qHash timing
    itemAdd = item->child(information);

    if (itemAdd.isNull())
        itemAdd = ParserTreeItem::Ptr(new ParserTreeItem());

    // locations are 1-based in Symbol, start with 0 for the editor
    SymbolLocation location(QString::fromUtf8(symbol->fileName() , symbol->fileNameLength()),
                            symbol->line(), symbol->column() - 1);
    itemAdd->addSymbolLocation(location);

    // prevent showing a content of the functions
    if (!symbol->isFunction()) {
        const CPlusPlus::Scope *scope = symbol->asScope();
        if (scope) {
            CPlusPlus::Scope::iterator cur = scope->firstMember();
            while (cur != scope->lastMember()) {
                const CPlusPlus::Symbol *curSymbol = *cur;
                ++cur;
                if (!curSymbol)
                    continue;

                //                if (!symbol->isClass() && curSymbol->isStatic() && curSymbol->isFunction())
                //                        return;

                addSymbol(itemAdd, curSymbol);
            }
        }
    }

    bool appendChild = true;

    // if item is empty and has not to be added
    if (symbol->isNamespace() && itemAdd->childCount() == 0)
        appendChild = false;

    if (appendChild)
        item->appendChild(itemAdd, information);
}

ParserTreeItem::Ptr Parser::createFlatTree(const QStringList &projectList)
{
    QReadLocker locker(&d->prjLocker);

    ParserTreeItem::Ptr item(new ParserTreeItem());
    foreach (const QString &project, projectList) {
        if (!d->cachedPrjTrees.contains(project))
            continue;
        ParserTreeItem::ConstPtr list = d->cachedPrjTrees[project];
        item->add(list);
    }
    return item;
}

ParserTreeItem::Ptr Parser::getParseProjectTree(const QStringList &fileList,
                                                const QString &projectId)
{
    //! \todo Way to optimize - for documentUpdate - use old cached project and subtract
    //! changed files only (old edition), and add curent editions
    ParserTreeItem::Ptr item(new ParserTreeItem());
    unsigned revision = 0;
    foreach (const QString &file, fileList) {
        // ? locker for document?..
        const CPlusPlus::Document::Ptr &doc = d->document(file);
        if (doc.isNull())
            continue;

        revision += doc->revision();

        ParserTreeItem::ConstPtr list = getCachedOrParseDocumentTree(doc);
        if (list.isNull())
            continue;

        // add list to out document
        item->add(list);
    }

    // update the cache
    if (!projectId.isEmpty()) {
        QWriteLocker locker(&d->prjLocker);

        d->cachedPrjTrees[projectId] = item;
        d->cachedPrjTreesRevision[projectId] = revision;
    }
    return item;
}

ParserTreeItem::Ptr Parser::getCachedOrParseProjectTree(const QStringList &fileList,
                                                const QString &projectId)
{
    d->prjLocker.lockForRead();

    // calculate current revision
    if (!projectId.isEmpty() && d->cachedPrjTrees.contains(projectId)) {
        // calculate project's revision
        unsigned revision = 0;
        foreach (const QString &file, fileList) {
            const CPlusPlus::Document::Ptr &doc = d->document(file);
            if (doc.isNull())
                continue;
            revision += doc->revision();
        }

        // if even revision is the same, return cached project
        if (revision == d->cachedPrjTreesRevision[projectId]) {
            d->prjLocker.unlock();
            return d->cachedPrjTrees[projectId];
        }
    }

    d->prjLocker.unlock();
    return getParseProjectTree(fileList, projectId);
}

ParserTreeItem::ConstPtr Parser::getParseDocumentTree(const CPlusPlus::Document::Ptr &doc)
{
    if (doc.isNull())
        return ParserTreeItem::ConstPtr();

    const QString &fileName = doc->fileName();
    if (!d->fileList.contains(fileName))
        return ParserTreeItem::ConstPtr();

    ParserTreeItem::Ptr itemPtr(new ParserTreeItem());

    unsigned total = doc->globalSymbolCount();
    for (unsigned i = 0; i < total; ++i)
        addSymbol(itemPtr, doc->globalSymbolAt(i));

    QWriteLocker locker(&d->docLocker);

    d->cachedDocTrees[fileName] = itemPtr;
    d->cachedDocTreesRevision[fileName] = doc->revision();
    d->documentList[fileName] = doc;

    return itemPtr;
}

ParserTreeItem::ConstPtr Parser::getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc)
{
    if (doc.isNull())
        return ParserTreeItem::ConstPtr();

    const QString &fileName = doc->fileName();
    d->docLocker.lockForRead();
    if (d->cachedDocTrees.contains(fileName)
        && d->cachedDocTreesRevision.contains(fileName)
        && d->cachedDocTreesRevision[fileName] == doc->revision()) {
        d->docLocker.unlock();
        return d->cachedDocTrees[fileName];
    }
    d->docLocker.unlock();
    return getParseDocumentTree(doc);
}

void Parser::parseDocument(const CPlusPlus::Document::Ptr &doc)
{
    if (doc.isNull())
        return;

    const QString &name = doc->fileName();

    // if it is external file (not in any of our projects)
    if (!d->fileList.contains(name))
        return;

    getParseDocumentTree(doc);

    QTC_ASSERT(d->timer, return);

    if (!d->timer->isActive())
        d->timer->start(400); //! Delay in msecs before an update
    return;
}

void Parser::clearCacheAll()
{
    d->docLocker.lockForWrite();

    d->cachedDocTrees.clear();
    d->cachedDocTreesRevision.clear();
    d->documentList.clear();

    d->docLocker.unlock();

    clearCache();
}

void Parser::clearCache()
{
    QWriteLocker locker(&d->prjLocker);

    // remove cached trees
    d->cachedPrjFileLists.clear();

    //! \todo where better to clear project's trees?
    //! When file is add/removed from a particular project?..
    d->cachedPrjTrees.clear();
    d->cachedPrjTreesRevision.clear();
}

void Parser::setFileList(const QStringList &fileList)
{
    d->fileList.clear();
    d->fileList = QSet<QString>::fromList(fileList);
}

void Parser::removeFiles(const QStringList &fileList)
{
    if (fileList.count() == 0)
        return;

    QWriteLocker lockerPrj(&d->prjLocker);
    QWriteLocker lockerDoc(&d->docLocker);
    foreach (const QString &name, fileList) {
        d->fileList.remove(name);
        d->cachedDocTrees.remove(name);
        d->cachedDocTreesRevision.remove(name);
        d->documentList.remove(name);
        d->cachedPrjTrees.remove(name);
        d->cachedPrjFileLists.clear();
    }

    emit filesAreRemoved();
}

void Parser::resetData(const CPlusPlus::Snapshot &snapshot)
{
    // clear internal cache
    clearCache();

    d->docLocker.lockForWrite();

    // copy snapshot's documents
    CPlusPlus::Snapshot::const_iterator cur = snapshot.begin();
    CPlusPlus::Snapshot::const_iterator end = snapshot.end();
    for (; cur != end; cur++)
        d->documentList[cur.key()] = cur.value();

    d->docLocker.unlock();

    // recalculate file list
    QStringList fileList;

    // check all projects
    QList<ProjectExplorer::Project *> projects = getProjectList();
    foreach (const ProjectExplorer::Project *prj, projects) {
        if (prj)
            fileList += prj->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
    }
    setFileList(fileList);

    emit resetDataDone();
}

void Parser::resetDataToCurrentState()
{
    // get latest data
    CPlusPlus::CppModelManagerInterface *codeModel = CPlusPlus::CppModelManagerInterface::instance();
    if (codeModel)
        resetData(codeModel->snapshot());
}

void Parser::onResetDataDone()
{
    // internal data is reset, update a tree and send it back
    emitCurrentTree();
}

void Parser::requestCurrentState()
{
    emitCurrentTree();
}

void Parser::emitCurrentTree()
{
    // stop timer if it is active right now
    d->timer->stop();

    d->rootItemLocker.lockForWrite();
    d->rootItem = parse();
    d->rootItemLocker.unlock();

    // convert
    QSharedPointer<QStandardItem> std(new QStandardItem());

    d->rootItem->convertTo(std.data());

    emit treeDataUpdate(std);
}

QStringList Parser::projectNodeFileList(const ProjectExplorer::FolderNode *node) const
{
    QStringList list;
    if (!node)
        return list;

    QList<ProjectExplorer::FileNode *> fileNodes = node->fileNodes();
    QList<ProjectExplorer::FolderNode *> subFolderNodes = node->subFolderNodes();

    foreach (const ProjectExplorer::FileNode *file, fileNodes) {
        if (file->isGenerated())
            continue;

        list << file->path();
    }

    foreach (const ProjectExplorer::FolderNode *folder, subFolderNodes) {
        if (folder->nodeType() != ProjectExplorer::FolderNodeType
                && folder->nodeType() != ProjectExplorer::VirtualFolderNodeType)
            continue;
        list << projectNodeFileList(folder);
    }

    return list;
}

QStringList Parser::addProjectNode(const ParserTreeItem::Ptr &item,
                                     const ProjectExplorer::ProjectNode *node)
{
    QStringList projectList;
    if (!node)
        return projectList;

    const QString &nodePath = node->path();

    // our own files
    QStringList fileList;

    // try to improve parsing speed by internal cache
    if (d->cachedPrjFileLists.contains(nodePath)) {
        fileList = d->cachedPrjFileLists[nodePath];
    } else {
        fileList = projectNodeFileList(node);
        d->cachedPrjFileLists[nodePath] = fileList;
    }
    if (fileList.count() > 0) {
        addProject(item, fileList, node->path());
        projectList << node->path();
    }

    // subnodes
    QList<ProjectExplorer::ProjectNode *> projectNodes = node->subProjectNodes();

    foreach (const ProjectExplorer::ProjectNode *project, projectNodes) {
        ParserTreeItem::Ptr itemPrj(new ParserTreeItem());
        SymbolInformation information(project->displayName(), project->path());

        projectList += addProjectNode(itemPrj, project);

        itemPrj->setIcon(project->icon());

        // append child if item is not null and there is at least 1 child
        if (!item.isNull() && itemPrj->childCount() > 0)
            item->appendChild(itemPrj, information);
    }

    return projectList;
}

QList<ProjectExplorer::Project *> Parser::getProjectList() const
{
    QList<ProjectExplorer::Project *> list;

    // check all projects
    ProjectExplorer::SessionManager *sessionManager
            = ProjectExplorer::ProjectExplorerPlugin::instance()->session();

    list = sessionManager->projects();

    return list;
}

} // namespace Internal
} // namespace ClassView
