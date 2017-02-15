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
#include <cplusplus/Symbols.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Name.h>

// other
#include <cpptools/cppmodelmanager.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Icons.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
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

using namespace ProjectExplorer;

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

/*!
    \fn void Parser::resetDataDone()

    Emits a signal that internal data is reset.

    \sa resetData, resetDataToCurrentState
*/

class ParserPrivate
{
public:
    typedef QHash<QString, CPlusPlus::Document::Ptr>::const_iterator CitDocumentList;

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
    CitDocumentList cit = documentList.find(fileName);
    if (cit == documentList.end())
        return CPlusPlus::Document::Ptr();
    return cit.value();
}

// ----------------------------- Parser ---------------------------------

/*!
    Constructs the parser object.
*/

Parser::Parser(QObject *parent)
    : QObject(parent),
    d(new ParserPrivate())
{
    d->timer = new QTimer(this);
    d->timer->setObjectName(QLatin1String("ClassViewParser::timer"));
    d->timer->setSingleShot(true);

    // connect signal/slots
    // internal data reset
    connect(this, &Parser::resetDataDone, this, &Parser::onResetDataDone, Qt::QueuedConnection);

    // timer for emitting changes
    connect(d->timer.data(), &QTimer::timeout, this, &Parser::requestCurrentState, Qt::QueuedConnection);
}

/*!
    Destructs the parser object.
*/

Parser::~Parser()
{
    delete d;
}

/*!
    Checks \a item for lazy data population of a QStandardItemModel.
*/

bool Parser::canFetchMore(QStandardItem *item, bool skipRoot) const
{
    ParserTreeItem::ConstPtr ptr = findItemByRoot(item, skipRoot);
    if (ptr.isNull())
        return false;
    return ptr->canFetchMore(item);
}

/*!
    Checks \a item for lazy data population of a QStandardItemModel.
    \a skipRoot skips the root item.
*/

void Parser::fetchMore(QStandardItem *item, bool skipRoot) const
{
    ParserTreeItem::ConstPtr ptr = findItemByRoot(item, skipRoot);
    if (ptr.isNull())
        return;
    ptr->fetchMore(item);
}

bool Parser::hasChildren(QStandardItem *item) const
{
    ParserTreeItem::ConstPtr ptr = findItemByRoot(item);
    if (ptr.isNull())
        return false;
    return ptr->childCount() != 0;
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
    emitCurrentTree();
}

/*!
    Returns the internal tree item for \a item. \a skipRoot skips the root
    item.
*/

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

/*!
    Parses the class and produces a new tree.

    \sa addProject
*/

ParserTreeItem::ConstPtr Parser::parse()
{
    QTime time;
    if (debug)
        time.start();

    ParserTreeItem::Ptr rootItem(new ParserTreeItem());

    // check all projects
    foreach (const Project *prj, SessionManager::projects()) {
        if (!prj)
            continue;

        ParserTreeItem::Ptr item;
        QString prjName(prj->displayName());
        QString prjType(prjName);
        if (prj->document())
            prjType = prj->projectFilePath().toString();
        SymbolInformation inf(prjName, prjType);
        item = ParserTreeItem::Ptr(new ParserTreeItem());

        if (d->flatMode)
            addFlatTree(item, prj->rootProjectNode());
        else
            addProjectNode(item, prj->rootProjectNode());

        item->setIcon(prj->rootProjectNode()->icon());
        rootItem->appendChild(item, inf);
    }

    if (debug)
        qDebug() << "Class View:" << QDateTime::currentDateTime().toString()
            << "Parsed in " << time.elapsed() << "msecs.";

    return rootItem;
}

/*!
    Parses the project with the \a projectId and adds the documents
    from the \a fileList to the tree item \a item.
*/

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

/*!
    Parses \a symbol and adds the results to \a item (as a parent).
*/

void Parser::addSymbol(const ParserTreeItem::Ptr &item, const CPlusPlus::Symbol *symbol)
{
    if (item.isNull() || !symbol)
        return;

    // easy solution - lets add any scoped symbol and
    // any symbol which does not contain :: in the name

    //! \todo collect statistics and reorder to optimize
    if (symbol->isForwardClassDeclaration()
        || symbol->isExtern()
        || symbol->isFriend()
        || symbol->isGenerated()
        || symbol->isUsingNamespaceDirective()
        || symbol->isUsingDeclaration()
        )
        return;

    const CPlusPlus::Name *symbolName = symbol->name();
    if (symbolName && symbolName->isQualifiedNameId())
        return;

    QString name = d->overview.prettyName(symbolName).trimmed();
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
        if (const CPlusPlus::Scope *scope = symbol->asScope()) {
            CPlusPlus::Scope::iterator cur = scope->memberBegin();
            CPlusPlus::Scope::iterator last = scope->memberEnd();
            while (cur != last) {
                const CPlusPlus::Symbol *curSymbol = *cur;
                ++cur;
                if (!curSymbol)
                    continue;

                addSymbol(itemAdd, curSymbol);
            }
        }
    }

    // if item is empty and has not to be added
    if (!(symbol->isNamespace() && itemAdd->childCount() == 0))
        item->appendChild(itemAdd, information);
}

/*!
    Parses the project with the \a projectId and adds the documents from the
    \a fileList to the project. Updates the internal cached tree for this
    project.
*/

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

/*!
    Gets the project with \a projectId from the cache if it is valid or parses
    the project and adds the documents from the \a fileList to the project.
    Updates the internal cached tree for this project.
*/

ParserTreeItem::Ptr Parser::getCachedOrParseProjectTree(const QStringList &fileList,
                                                const QString &projectId)
{
    d->prjLocker.lockForRead();

    ParserTreeItem::Ptr item = d->cachedPrjTrees.value(projectId);
    // calculate current revision
    if (!projectId.isEmpty() && !item.isNull()) {
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
            return item;
        }
    }

    d->prjLocker.unlock();
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

    ParserTreeItem::Ptr itemPtr(new ParserTreeItem());

    const unsigned total = doc->globalSymbolCount();
    for (unsigned i = 0; i < total; ++i)
        addSymbol(itemPtr, doc->globalSymbolAt(i));

    QWriteLocker locker(&d->docLocker);

    d->cachedDocTrees[fileName] = itemPtr;
    d->cachedDocTreesRevision[fileName] = doc->revision();
    d->documentList[fileName] = doc;

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
    d->docLocker.lockForRead();
    ParserTreeItem::ConstPtr item = d->cachedDocTrees.value(fileName);
    CitCachedDocTreeRevision citCachedDocTreeRevision = d->cachedDocTreesRevision.constFind(fileName);
    if (!item.isNull()
        && citCachedDocTreeRevision != d->cachedDocTreesRevision.constEnd()
            && citCachedDocTreeRevision.value() == doc->revision()) {
        d->docLocker.unlock();
        return item;
    }
    d->docLocker.unlock();
    return getParseDocumentTree(doc);
}

/*!
    Parses the document \a doc if it is in the project files and adds a tree to
    the internal storage.
*/

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

/*!
    Requests to clear full internal stored data.
*/

void Parser::clearCacheAll()
{
    d->docLocker.lockForWrite();

    d->cachedDocTrees.clear();
    d->cachedDocTreesRevision.clear();
    d->documentList.clear();

    d->docLocker.unlock();

    clearCache();
}

/*!
    Requests to clear internal stored data. The data has to be regenerated on
    the next request.
*/

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

/*!
    Specifies the files that must be allowed for the parsing as a \a fileList.
    Files outside of this list will not be in any tree.
*/

void Parser::setFileList(const QStringList &fileList)
{
    d->fileList.clear();
    d->fileList = QSet<QString>::fromList(fileList);
}

/*!
    Removes the files defined in the \a fileList from the parsing.
*/

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

/*!
    Fully resets the internal state of the code parser to \a snapshot.
*/

void Parser::resetData(const CPlusPlus::Snapshot &snapshot)
{
    // clear internal cache
    clearCache();

    d->docLocker.lockForWrite();

    // copy snapshot's documents
    CPlusPlus::Snapshot::const_iterator cur = snapshot.begin();
    CPlusPlus::Snapshot::const_iterator end = snapshot.end();
    for (; cur != end; ++cur)
        d->documentList[cur.key().toString()] = cur.value();

    d->docLocker.unlock();

    // recalculate file list
    QStringList fileList;

    // check all projects
    foreach (const Project *prj, SessionManager::projects()) {
        if (prj)
            fileList += prj->files(Project::SourceFiles);
    }
    setFileList(fileList);

    emit resetDataDone();
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
    Regenerates the tree when internal data changes.

    \sa resetDataDone
*/

void Parser::onResetDataDone()
{
    // internal data is reset, update a tree and send it back
    emitCurrentTree();
}

/*!
    Requests to emit a signal with the current tree state.
*/

void Parser::requestCurrentState()
{
    emitCurrentTree();
}

/*!
    Sends the current tree to listeners.
*/

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

/*!
    Generates a project node file list for the root node \a node.
*/

QStringList Parser::projectNodeFileList(const FolderNode *folderNode) const
{
    QStringList list;
    folderNode->forEachNode(
        [&](FileNode *node) {
            if (!node->isGenerated())
                list.append(node->filePath().toString());
        },
        {},
        [&](const FolderNode *node) {
            return node->nodeType() == NodeType::Folder || node->nodeType() == NodeType::VirtualFolder;
        }
    );
    return list;
}

/*!
    Generates projects like the Project Explorer.
    \a item specifies the item and \a node specifies the root node.

    Returns a list of projects which were added to the item.
*/

QStringList Parser::addProjectNode(const ParserTreeItem::Ptr &item, const ProjectNode *node)
{
    QStringList projectList;
    if (!node)
        return projectList;

    const QString nodePath = node->filePath().toString();

    // our own files
    QStringList fileList;

    CitCachedPrjFileLists cit = d->cachedPrjFileLists.constFind(nodePath);
    // try to improve parsing speed by internal cache
    if (cit != d->cachedPrjFileLists.constEnd()) {
        fileList = cit.value();
    } else {
        fileList = projectNodeFileList(node);
        d->cachedPrjFileLists[nodePath] = fileList;
    }
    if (fileList.count() > 0) {
        addProject(item, fileList, node->filePath().toString());
        projectList << node->filePath().toString();
    }

    // subnodes
    for (const Node *n : node->nodes()) {
        if (const ProjectNode *project = n->asProjectNode()) {
            ParserTreeItem::Ptr itemPrj(new ParserTreeItem());
            SymbolInformation information(project->displayName(), project->filePath().toString());

            projectList += addProjectNode(itemPrj, project);

            itemPrj->setIcon(project->icon());

            // append child if item is not null and there is at least 1 child
            if (!item.isNull() && itemPrj->childCount() > 0)
                item->appendChild(itemPrj, information);
        }
    }

    return projectList;
}

QStringList Parser::getAllFiles(const ProjectNode *node)
{
    QStringList fileList;

    if (!node)
        return fileList;

    const QString nodePath = node->filePath().toString();

    CitCachedPrjFileLists cit = d->cachedPrjFileLists.constFind(nodePath);
    // try to improve parsing speed by internal cache
    if (cit != d->cachedPrjFileLists.constEnd()) {
        fileList = cit.value();
    } else {
        fileList = projectNodeFileList(node);
        d->cachedPrjFileLists[nodePath] = fileList;
    }
    // subnodes

    for (const Node *n : node->nodes())
        if (const ProjectNode *project = n->asProjectNode())
            fileList += getAllFiles(project);
    return fileList;
}

void Parser::addFlatTree(const ParserTreeItem::Ptr &item, const ProjectNode *node)
{
    if (!node)
        return;

    QStringList fileList = getAllFiles(node);
    fileList.removeDuplicates();

    if (fileList.count() > 0) {
        addProject(item, fileList, node->filePath().toString());
    }
}

} // namespace Internal
} // namespace ClassView
