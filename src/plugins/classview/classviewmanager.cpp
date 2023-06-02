// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewmanager.h"

#include "classviewparser.h"
#include "classviewutils.h"

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>

#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/projectmanager.h>

#include <texteditor/texteditor.h>

#include <QThread>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClassView::Internal {

///////////////////////////////// ManagerPrivate //////////////////////////////////

// static variable initialization
static Manager *managerInstance = nullptr;

/*!
    \class ClassView::Internal::Manager

    \brief The Manager class implements a class view manager that interacts with
    other \QC plugins and acts as a proxy between them and the parser.

    The parser is moved to a separate thread and is connected to the manager by
    using signals and slots. Manager's signals starting with 'request' are for
    the parser.
*/

/*!
    \fn explicit ClassView::Internal::Manager(QObject *parent = 0)

    Creates a shared instance of a \a parent object.
*/

/*!
    \fn void ClassView::Internal::Manager::treeDataUpdate(QSharedPointer<QStandardItem> result)

    Emits a signal about a tree data update (to tree view). \a result holds the
    item with the current tree.
*/

/*!
   \class ManagerPrivate
   \internal
   \brief The ManagerPrivate class contains private class data for the Manager
    class.
   \sa Manager
*/

class ManagerPrivate
{
public:
    Parser *m_parser = nullptr;
    QThread m_parserThread;
    ParserTreeItem::ConstPtr m_root;

    QTimer m_timer;
    QSet<FilePath> m_awaitingDocuments;

    //! Internal manager state. \sa Manager::state
    bool state = false;

    //! there is some massive operation ongoing so temporary we should wait
    bool disableCodeParser = false;

    void cancelScheduledUpdate();
    void resetParser();
    ParserTreeItem::ConstPtr findItemByRoot(const QStandardItem *item, bool skipRoot = false) const;
};

void ManagerPrivate::cancelScheduledUpdate()
{
    m_timer.stop();
    m_awaitingDocuments.clear();
}

void ManagerPrivate::resetParser()
{
    cancelScheduledUpdate();

    QHash<FilePath, QPair<QString, FilePaths>> projectData;
    for (const Project *project : ProjectManager::projects()) {
        projectData.insert(project->projectFilePath(),
                           {project->displayName(), project->files(Project::SourceFiles)});
    }
    QMetaObject::invokeMethod(m_parser, [this, projectData]() {
        m_parser->resetData(projectData);
    }, Qt::QueuedConnection);
}

/*!
    Returns the internal tree item for \a item. \a skipRoot skips the root
    item.
*/
ParserTreeItem::ConstPtr ManagerPrivate::findItemByRoot(const QStandardItem *item, bool skipRoot) const
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

    ParserTreeItem::ConstPtr internal = m_root;
    while (uiList.count() > 0) {
        cur = uiList.last();
        uiList.removeLast();
        const SymbolInformation &inf = Internal::symbolInformationFromItem(cur);
        internal = internal->child(inf);
        if (internal.isNull())
            break;
    }

    return internal;
}

///////////////////////////////// Manager //////////////////////////////////

Manager::Manager(QObject *parent)
    : QObject(parent),
    d(new ManagerPrivate())
{
    d->m_parser = new Parser();
    d->m_parser->moveToThread(&d->m_parserThread);
    connect(&d->m_parserThread, &QThread::finished, d->m_parser, &QObject::deleteLater);
    managerInstance = this;

    // register - to be able send between signal/slots
    qRegisterMetaType<QSharedPointer<QStandardItem> >("QSharedPointer<QStandardItem>");

    initialize();

    // start a separate thread for the parser
    d->m_parserThread.start();
}

Manager::~Manager()
{
    d->m_parserThread.quit();
    d->m_parserThread.wait();
    delete d;
    managerInstance = nullptr;
}

Manager *Manager::instance()
{
    return managerInstance;
}

/*!
    Checks \a item for lazy data population of a QStandardItemModel.
*/
bool Manager::canFetchMore(QStandardItem *item, bool skipRoot) const
{
    ParserTreeItem::ConstPtr ptr = d->findItemByRoot(item, skipRoot);
    if (ptr.isNull())
        return false;
    return ptr->canFetchMore(item);
}

/*!
    Checks \a item for lazy data population of a QStandardItemModel.
    \a skipRoot item is needed for the manual update, call not from model.
*/
void Manager::fetchMore(QStandardItem *item, bool skipRoot)
{
    ParserTreeItem::ConstPtr ptr = d->findItemByRoot(item, skipRoot);
    if (ptr.isNull())
        return;
    ptr->fetchMore(item);
}

bool Manager::hasChildren(QStandardItem *item) const
{
    ParserTreeItem::ConstPtr ptr = d->findItemByRoot(item);
    if (ptr.isNull())
        return false;
    return ptr->childCount();
}

void Manager::initialize()
{
    d->m_timer.setSingleShot(true);

    // connections to enable/disable navi widget factory
    ProjectManager *sessionManager = ProjectManager::instance();
    connect(sessionManager, &ProjectManager::projectAdded,
            this, [this](Project *project) {
        const FilePath projectPath = project->projectFilePath();
        const QString projectName = project->displayName();
        const FilePaths projectFiles = project->files(Project::SourceFiles);
        QMetaObject::invokeMethod(d->m_parser, [this, projectPath, projectName, projectFiles]() {
            d->m_parser->addProject(projectPath, projectName, projectFiles);
        }, Qt::QueuedConnection);
    });
    connect(sessionManager, &ProjectManager::projectRemoved,
            this, [this](Project *project) {
        const FilePath projectPath = project->projectFilePath();
        QMetaObject::invokeMethod(d->m_parser, [this, projectPath]() {
            d->m_parser->removeProject(projectPath);
        }, Qt::QueuedConnection);
    });

    // connect to the progress manager for signals about Parsing tasks
    connect(ProgressManager::instance(), &ProgressManager::taskStarted,
            this, [this](Id type) {
        if (type != CppEditor::Constants::TASK_INDEX)
            return;

        // disable tree updates to speed up
        d->disableCodeParser = true;
        d->cancelScheduledUpdate();
    });
    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            this, [this](Id type) {
        if (type != CppEditor::Constants::TASK_INDEX)
            return;

        // parsing is finished, enable tree updates
        d->disableCodeParser = false;

        // do nothing if Manager is disabled
        if (!state())
            return;

        // request to update a tree to the current state
        d->resetParser();
    });

    connect(d->m_parser, &Parser::treeRegenerated,
            this, [this](const ParserTreeItem::ConstPtr &root) {
        d->m_root = root;

        if (!state())
            return;

        QSharedPointer<QStandardItem> rootItem(new QStandardItem());
        d->m_root->fetchMore(rootItem.data());
        emit treeDataUpdate(rootItem);
    }, Qt::QueuedConnection);

    // connect to the cpp model manager for signals about document updates
    CppEditor::CppModelManager *codeModelManager = CppEditor::CppModelManager::instance();

    // when code manager signals that document is updated - handle it by ourselves
    connect(codeModelManager, &CppEditor::CppModelManager::documentUpdated,
            this, [this](CPlusPlus::Document::Ptr doc) {
        // do nothing if Manager is disabled
        if (!state())
            return;

        // do nothing if updates are disabled
        if (d->disableCodeParser)
            return;

        if (doc.data() == nullptr)
            return;

        d->m_awaitingDocuments.insert(doc->filePath());
        d->m_timer.start(400); // Accumulate multiple requests into one, restarts the timer
    });

    connect(&d->m_timer, &QTimer::timeout, this, [this]() {
        const QSet<FilePath> docsToBeUpdated = d->m_awaitingDocuments;
        d->cancelScheduledUpdate();
        if (!state() || d->disableCodeParser) // enabling any of them will trigger the total update
            return;
        QMetaObject::invokeMethod(d->m_parser, [this, docsToBeUpdated]() {
            d->m_parser->updateDocuments(docsToBeUpdated);
        }, Qt::QueuedConnection);
    });

    connect(codeModelManager, &CppEditor::CppModelManager::aboutToRemoveFiles,
            d->m_parser, &Parser::removeFiles, Qt::QueuedConnection);
}

/*!
    Gets the internal manager state. If it is disabled, does not emit signals
    about parsing requests. If enabled, does parsing in the background even if
    the navigation pane is not visible. Returns true if the manager is enabled,
    false otherwise.

   \sa setState, stateChanged
*/

bool Manager::state() const
{
    return d->state;
}

/*!
    Sets the internal manager \a state to \c true if the manager has to be
    enabled, otherwise sets it to \c false.

   \sa state, stateChanged
*/

void Manager::setState(bool state)
{
    if (state == d->state)
        return;

    d->state = state;

    if (state) // enabled - request a current snapshots etc?..
        d->resetParser();
}

/*!
    Reacts to the \a visibility of one navigation pane widget being changed
    (there might be a lot of them).

   \sa setState, state
*/

void Manager::onWidgetVisibilityIsChanged(bool visibility)
{
    // activate data handling - when 1st time 'Class View' will be open
    if (!visibility)
        return;
    setState(true);
    // TODO: this one may change into getter (when a new class view widget is being shown)
    QMetaObject::invokeMethod(d->m_parser, &Parser::requestCurrentState, Qt::QueuedConnection);
}

/*!
    Opens the text editor for the file \a fileName on \a line (1-based) and
    \a column (0-based).
*/

void Manager::gotoLocation(const FilePath &filePath, int line, int column)
{
    EditorManager::openEditorAt({filePath, line, column});
}

/*!
    Opens the text editor for any of the symbol locations in the \a list.

   \sa Manager::gotoLocations
*/

void Manager::gotoLocations(const QList<QVariant> &list)
{
    QSet<SymbolLocation> locations = Internal::roleToLocations(list);
    if (locations.size() == 0)
        return;

    // Default to first known location
    auto locationIt = locations.constBegin();
    if (locations.size() > 1) {
        // The symbol has multiple locations. Check if we are already at one location,
        // and if so, cycle to the "next" one
        auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(EditorManager::currentEditor());
        if (textEditor) {
            // check if current cursor position is a known location of the symbol
            const FilePath filePath = textEditor->document()->filePath();
            int line;
            int column;
            textEditor->convertPosition(textEditor->position(), &line, &column);
            const SymbolLocation current(filePath, line, column + 1);
            if (auto it = locations.constFind(current), end = locations.constEnd(); it != end) {
                // we already are at the symbol, cycle to next location
                ++it;
                if (it == end)
                    it = locations.constBegin();
                locationIt = it;
            }
        }
    }
    const SymbolLocation &location = *locationIt;
    // line is 1-based, column is 0-based
    gotoLocation(location.filePath(), location.line(), location.column() - 1);
}

/*!
    Switches to flat mode (without subprojects) if \a flat is set to \c true.
*/

void Manager::setFlatMode(bool flat)
{
    QMetaObject::invokeMethod(d->m_parser, [this, flat]() {
        d->m_parser->setFlatMode(flat);
    }, Qt::QueuedConnection);
}

} // ClassView::Internal
