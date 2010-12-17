/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "classviewsymbollocation.h"
#include "classviewmanager.h"
#include "classviewnavigationwidgetfactory.h"
#include "classviewparser.h"
#include "classviewutils.h"

#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <texteditor/basetexteditor.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolsconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/itexteditor.h>

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

namespace ClassView {
namespace Internal {

///////////////////////////////// ManagerPrivate //////////////////////////////////

/*!
   \struct ManagerPrivate
   \brief Private class data for \a Manager
   \sa Manager
 */
struct ManagerPrivate
{
    ManagerPrivate() : state(false) {}

    //! instance
    static Manager *instance;

    //! Pointer to widget factory
    QPointer<NavigationWidgetFactory> widgetFactory;

    //! State mutex
    QMutex mutexState;

    //! Internal manager state. \sa Manager::state
    bool state;

    //! code state/changes parser
    Parser parser;

    //! separate thread for the parser
    QThread parserThread;

    //! cpp code model manager
    QPointer<CppTools::CppModelManagerInterface> codeModelManager;

    //! there is some massive operation ongoing so temporary we should wait
    bool disableCodeParser;
};

// static variable initialization
Manager *ManagerPrivate::instance = 0;

///////////////////////////////// Manager //////////////////////////////////

Manager::Manager(QObject *parent)
    : QObject(parent),
    d_ptr(new ManagerPrivate())
{
    d_ptr->widgetFactory = NavigationWidgetFactory::instance();

    // register - to be able send between signal/slots
    qRegisterMetaType<QSharedPointer<QStandardItem> >("QSharedPointer<QStandardItem>");

    initialize();

    // start a separate thread for the parser
    d_ptr->parser.moveToThread(&d_ptr->parserThread);
    d_ptr->parserThread.start();

    // initial setup
    onProjectListChanged();
}

Manager::~Manager()
{
    d_ptr->parserThread.quit();
    d_ptr->parserThread.wait();
}

Manager *Manager::instance(QObject *parent)
{
    if (!ManagerPrivate::instance)
        ManagerPrivate::instance = new Manager(parent);
    return ManagerPrivate::instance;
}

bool Manager::canFetchMore(QStandardItem *item) const
{
    return d_ptr->parser.canFetchMore(item);
}

void Manager::fetchMore(QStandardItem *item, bool skipRoot)
{
    d_ptr->parser.fetchMore(item, skipRoot);
}

void Manager::initialize()
{
    // use Qt::QueuedConnection everywhere

    // widget factory signals
    connect(d_ptr->widgetFactory, SIGNAL(widgetIsCreated()),
            SLOT(onWidgetIsCreated()), Qt::QueuedConnection);

    // internal manager state is changed
    connect(this, SIGNAL(stateChanged(bool)), SLOT(onStateChanged(bool)), Qt::QueuedConnection);

    // connections to enable/disable navi widget factory
    ProjectExplorer::SessionManager *sessionManager =
        ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            SLOT(onProjectListChanged()), Qt::QueuedConnection);
    connect(sessionManager, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            SLOT(onProjectListChanged()), Qt::QueuedConnection);

    Core::ICore *core = Core::ICore::instance();

    // connect to the progress manager for signals about Parsing tasks
    connect(core->progressManager(), SIGNAL(taskStarted(QString)),
            SLOT(onTaskStarted(QString)), Qt::QueuedConnection);
    connect(core->progressManager(), SIGNAL(allTasksFinished(QString)),
            SLOT(onAllTasksFinished(QString)), Qt::QueuedConnection);

    // connect to the cpp model manager for signals about document updates
    d_ptr->codeModelManager = CppTools::CppModelManagerInterface::instance();

    // when code manager signals that document is updated - handle it by ourselves
    connect(d_ptr->codeModelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)), Qt::QueuedConnection);

    // when we signals that really document is updated - sent it to the parser
    connect(this, SIGNAL(requestDocumentUpdated(CPlusPlus::Document::Ptr)),
            &d_ptr->parser, SLOT(parseDocument(CPlusPlus::Document::Ptr)), Qt::QueuedConnection);

    //
    connect(d_ptr->codeModelManager, SIGNAL(aboutToRemoveFiles(QStringList)),
            &d_ptr->parser, SLOT(removeFiles(QStringList)), Qt::QueuedConnection);

    // translate data update from the parser to listeners
    connect(&d_ptr->parser, SIGNAL(treeDataUpdate(QSharedPointer<QStandardItem>)),
            this, SLOT(onTreeDataUpdate(QSharedPointer<QStandardItem>)), Qt::QueuedConnection);

    // requet current state - immediately after a notification
    connect(this, SIGNAL(requestTreeDataUpdate()),
            &d_ptr->parser, SLOT(requestCurrentState()), Qt::QueuedConnection);

    // full reset request to parser
    connect(this, SIGNAL(requestResetCurrentState()),
            &d_ptr->parser, SLOT(resetDataToCurrentState()), Qt::QueuedConnection);

    // clear cache request
    connect(this, SIGNAL(requestClearCache()),
            &d_ptr->parser, SLOT(clearCache()), Qt::QueuedConnection);

    // clear full cache request
    connect(this, SIGNAL(requestClearCacheAll()),
            &d_ptr->parser, SLOT(clearCacheAll()), Qt::QueuedConnection);

    // flat mode request
    connect(this, SIGNAL(requestSetFlatMode(bool)),
            &d_ptr->parser, SLOT(setFlatMode(bool)), Qt::QueuedConnection);
}

bool Manager::state() const
{
    return d_ptr->state;
}

void Manager::setState(bool state)
{
    QMutexLocker locker(&d_ptr->mutexState);

    // boolean comparsion - should be done correctly by any compiler
    if (state == d_ptr->state)
        return;

    d_ptr->state = state;

    emit stateChanged(d_ptr->state);
}

void Manager::onWidgetIsCreated()
{
    // do nothing - continue to sleep
}

void Manager::onWidgetVisibilityIsChanged(bool visibility)
{
    // activate data handling - when 1st time 'Class View' will be open
    if (visibility)
        setState(true);
}

void Manager::onStateChanged(bool state)
{
    if (state) {
        // enabled - request a current snapshots etc?..
        emit requestResetCurrentState();
    } else {
        // disabled - clean parsers internal cache
        emit requestClearCacheAll();
    }
}

void Manager::onProjectListChanged()
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    // update to the latest state
    requestTreeDataUpdate();
}

void Manager::onTaskStarted(const QString &type)
{
    if (type != CppTools::Constants::TASK_INDEX)
        return;

    // disable tree updates to speed up
    d_ptr->disableCodeParser = true;
}

void Manager::onAllTasksFinished(const QString &type)
{
    if (type != CppTools::Constants::TASK_INDEX)
        return;

    // parsing is finished, enable tree updates
    d_ptr->disableCodeParser = false;

    // do nothing if Manager is disabled
    if (!state())
        return;

    // any document might be changed, emit signal to clear cache
    emit requestClearCache();

    // request to update a tree to the current state
    emit requestResetCurrentState();
}

void Manager::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    // do nothing if updates are disabled
    if (d_ptr->disableCodeParser)
        return;

    emit requestDocumentUpdated(doc);
}

void Manager::gotoLocation(const QString &fileName, int line, int column)
{
    bool newEditor = false;
    TextEditor::BaseTextEditor::openEditorAt(fileName, line, column, QString(),
                                             Core::EditorManager::IgnoreNavigationHistory,
                                             &newEditor);
}

void Manager::gotoLocations(const QList<QVariant> &list)
{
    QSet<SymbolLocation> locations = Utils::roleToLocations(list);

    if (locations.count() == 0)
        return;

    QString fileName;
    int line = 0;
    int column = 0;
    bool currentPositionAvailable = false;

    // what is open now?
    Core::IEditor *editor = Core::EditorManager::instance()->currentEditor();
    if (editor) {
        // get current file name
        Core::IFile *file = editor->file();
        if (file)
            fileName = file->fileName();

        // if text file - what is current position?
        TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
        if (textEditor) {
            // there is open currently text editor
            int position = textEditor->position();
            textEditor->convertPosition(position, &line, &column);
            currentPositionAvailable = true;
        }
    }

    // if there is something open - try to check, is it currently activated symbol?
    if (currentPositionAvailable) {
        SymbolLocation current(fileName, line, column);
        QSet<SymbolLocation>::const_iterator it = locations.find(current);
        QSet<SymbolLocation>::const_iterator end = locations.constEnd();
        // is it known location?
        if (it != end) {
            // found - do one additional step
            ++it;
            if (it == end)
                it = locations.begin();
            const SymbolLocation &found = *it;
            gotoLocation(found.fileName(), found.line(), found.column());
            return;
        }
    }

    // no success - open first item in the list
    const SymbolLocation loc = *locations.constBegin();

    gotoLocation(loc.fileName(), loc.line(), loc.column());
}

void Manager::onRequestTreeDataUpdate()
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    emit requestTreeDataUpdate();
}

void Manager::setFlatMode(bool flat)
{
    emit requestSetFlatMode(flat);
}

void Manager::onTreeDataUpdate(QSharedPointer<QStandardItem> result)
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    emit treeDataUpdate(result);
}

} // namespace Internal
} // namespace ClassView
