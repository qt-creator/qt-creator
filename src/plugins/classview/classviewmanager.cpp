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

#include "classviewsymbollocation.h"
#include "classviewmanager.h"
#include "classviewnavigationwidgetfactory.h"
#include "classviewparser.h"
#include "classviewutils.h"

#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <texteditor/basetexteditor.h>
#include <cpptools/ModelManagerInterface.h>
#include <cpptools/cpptoolsconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/itexteditor.h>

#include <QThread>
#include <QMutex>
#include <QMutexLocker>

namespace ClassView {
namespace Internal {

///////////////////////////////// ManagerPrivate //////////////////////////////////

// static variable initialization
static Manager *managerInstance = 0;

/*!
   \struct ManagerPrivate
   \internal
   \brief Private class data for \a Manager
   \sa Manager
 */
class ManagerPrivate
{
public:
    ManagerPrivate() : state(false), disableCodeParser(false) {}

    //! State mutex
    QMutex mutexState;

    //! code state/changes parser
    Parser parser;

    //! separate thread for the parser
    QThread parserThread;

    //! Internal manager state. \sa Manager::state
    bool state;

    //! there is some massive operation ongoing so temporary we should wait
    bool disableCodeParser;
};

///////////////////////////////// Manager //////////////////////////////////

Manager::Manager(QObject *parent)
    : QObject(parent),
    d(new ManagerPrivate())
{
    managerInstance = this;

    // register - to be able send between signal/slots
    qRegisterMetaType<QSharedPointer<QStandardItem> >("QSharedPointer<QStandardItem>");

    initialize();

    // start a separate thread for the parser
    d->parser.moveToThread(&d->parserThread);
    d->parserThread.start();

    // initial setup
    onProjectListChanged();
}

Manager::~Manager()
{
    d->parserThread.quit();
    d->parserThread.wait();
    delete d;
    managerInstance = 0;
}

Manager *Manager::instance()
{
    return managerInstance;
}

bool Manager::canFetchMore(QStandardItem *item) const
{
    return d->parser.canFetchMore(item);
}

void Manager::fetchMore(QStandardItem *item, bool skipRoot)
{
    d->parser.fetchMore(item, skipRoot);
}

void Manager::initialize()
{
    // use Qt::QueuedConnection everywhere

    // widget factory signals
    connect(NavigationWidgetFactory::instance(), SIGNAL(widgetIsCreated()),
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

    // connect to the progress manager for signals about Parsing tasks
    connect(Core::ICore::progressManager(), SIGNAL(taskStarted(QString)),
            SLOT(onTaskStarted(QString)), Qt::QueuedConnection);
    connect(Core::ICore::progressManager(), SIGNAL(allTasksFinished(QString)),
            SLOT(onAllTasksFinished(QString)), Qt::QueuedConnection);

    // when we signals that really document is updated - sent it to the parser
    connect(this, SIGNAL(requestDocumentUpdated(CPlusPlus::Document::Ptr)),
            &d->parser, SLOT(parseDocument(CPlusPlus::Document::Ptr)), Qt::QueuedConnection);

    // translate data update from the parser to listeners
    connect(&d->parser, SIGNAL(treeDataUpdate(QSharedPointer<QStandardItem>)),
            this, SLOT(onTreeDataUpdate(QSharedPointer<QStandardItem>)), Qt::QueuedConnection);

    // requet current state - immediately after a notification
    connect(this, SIGNAL(requestTreeDataUpdate()),
            &d->parser, SLOT(requestCurrentState()), Qt::QueuedConnection);

    // full reset request to parser
    connect(this, SIGNAL(requestResetCurrentState()),
            &d->parser, SLOT(resetDataToCurrentState()), Qt::QueuedConnection);

    // clear cache request
    connect(this, SIGNAL(requestClearCache()),
            &d->parser, SLOT(clearCache()), Qt::QueuedConnection);

    // clear full cache request
    connect(this, SIGNAL(requestClearCacheAll()),
            &d->parser, SLOT(clearCacheAll()), Qt::QueuedConnection);

    // flat mode request
    connect(this, SIGNAL(requestSetFlatMode(bool)),
            &d->parser, SLOT(setFlatMode(bool)), Qt::QueuedConnection);

    // connect to the cpp model manager for signals about document updates
    CPlusPlus::CppModelManagerInterface *codeModelManager
        = CPlusPlus::CppModelManagerInterface::instance();

    // when code manager signals that document is updated - handle it by ourselves
    connect(codeModelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)), Qt::QueuedConnection);
    //
    connect(codeModelManager, SIGNAL(aboutToRemoveFiles(QStringList)),
            &d->parser, SLOT(removeFiles(QStringList)), Qt::QueuedConnection);
}

bool Manager::state() const
{
    return d->state;
}

void Manager::setState(bool state)
{
    QMutexLocker locker(&d->mutexState);

    // boolean comparsion - should be done correctly by any compiler
    if (state == d->state)
        return;

    d->state = state;

    emit stateChanged(d->state);
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
    if (type != QLatin1String(CppTools::Constants::TASK_INDEX))
        return;

    // disable tree updates to speed up
    d->disableCodeParser = true;
}

void Manager::onAllTasksFinished(const QString &type)
{
    if (type != QLatin1String(CppTools::Constants::TASK_INDEX))
        return;

    // parsing is finished, enable tree updates
    d->disableCodeParser = false;

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
    if (d->disableCodeParser)
        return;

    emit requestDocumentUpdated(doc);
}

void Manager::gotoLocation(const QString &fileName, int line, int column)
{
    bool newEditor = false;
    TextEditor::BaseTextEditorWidget::openEditorAt(fileName, line, column, Core::Id(),
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
    if (Core::IEditor *editor = Core::EditorManager::currentEditor()) {
        // get current file name
        if (Core::IDocument *document = editor->document())
            fileName = document->fileName();

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
