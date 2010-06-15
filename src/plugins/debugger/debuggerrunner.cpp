/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "debuggerrunner.h"
#include "debuggermanager.h"
#include "debuggeroutputwindow.h"

#include "idebuggerengine.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "snapshothandler.h"
#include "stackhandler.h"
#include "stackframe.h"
#include "threadshandler.h"
#include "watchhandler.h"

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtGui/QAbstractItemView>
#include <QtGui/QTextDocument>
#include <QtGui/QTreeWidget>

using namespace ProjectExplorer;
using namespace Debugger::Internal;


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

namespace Debugger {

// A factory to create DebuggerRunControls
DebuggerRunControlFactory::DebuggerRunControlFactory(DebuggerManager *manager)
    : m_manager(manager)
{}

bool DebuggerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
//    return mode == ProjectExplorer::Constants::DEBUGMODE;
    return mode == ProjectExplorer::Constants::DEBUGMODE
            && qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
}

QString DebuggerRunControlFactory::displayName() const
{
    return tr("Debug");
}

static DebuggerStartParameters localStartParameters(RunConfiguration *runConfiguration)
{
    DebuggerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    LocalApplicationRunConfiguration *rc =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartInternal;
    sp.executable = rc->executable();
    sp.environment = rc->environment().toStringList();
    sp.workingDirectory = rc->workingDirectory();
    sp.processArgs = rc->commandLineArguments();
    sp.toolChainType = rc->toolChainType();
    sp.useTerminal = rc->runMode() == LocalApplicationRunConfiguration::Console;
    sp.dumperLibrary = rc->dumperLibrary();
    sp.dumperLibraryLocations = rc->dumperLibraryLocations();
    sp.displayName = rc->displayName();

    // Find qtInstallPath.
    QString qmakePath = DebuggingHelperLibrary::findSystemQt(rc->environment());
    if (!qmakePath.isEmpty()) {
        QProcess proc;
        QStringList args;
        args.append(QLatin1String("-query"));
        args.append(QLatin1String("QT_INSTALL_HEADERS"));
        proc.start(qmakePath, args);
        proc.waitForFinished();
        QByteArray ba = proc.readAllStandardOutput().trimmed();
        QFileInfo fi(QString::fromLocal8Bit(ba) + "/..");
        sp.qtInstallPath = fi.absoluteFilePath();
    }
    return sp;
}

RunControl *DebuggerRunControlFactory::create
    (RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(mode == ProjectExplorer::Constants::DEBUGMODE, return 0);
    DebuggerStartParameters sp = localStartParameters(runConfiguration);
    return new DebuggerRunControl(m_manager, sp);
}

RunControl *DebuggerRunControlFactory::create(const DebuggerStartParameters &sp)
{
    return new DebuggerRunControl(m_manager, sp);
}

QWidget *DebuggerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    // NBS TODO: Add GDB-specific configuration widget
    Q_UNUSED(runConfiguration)
    return 0;
}


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControl::Private
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunControl::Private
{
public:
    Private(DebuggerRunControl *parent, DebuggerManager *manager,
        const DebuggerStartParameters &startParameters);
    ~Private();

public:
    DebuggerRunControl *q;

    DebuggerStartParameters m_startParameters;
    DebuggerManager *m_manager;
    Internal::IDebuggerEngine *m_engine;
    bool m_running;

    ModulesHandler *m_modulesHandler;
    RegisterHandler *m_registerHandler;
/*
    // FIXME: Move from DebuggerManager
    BreakHandler *m_breakHandler;
    SnapshotHandler *m_snapshotHandler;
    StackHandler *m_stackHandler;
    ThreadsHandler *m_threadsHandler;
    WatchHandler *m_watchHandler;
*/
};

DebuggerRunControl::Private::Private(DebuggerRunControl *parent,
        DebuggerManager *manager,
        const DebuggerStartParameters &startParameters)
  : q(parent),
    m_startParameters(startParameters),
    m_manager(manager),
    m_engine(0)
{
    m_running = false;
    m_modulesHandler = new ModulesHandler(q);
    m_registerHandler = new RegisterHandler();
}

DebuggerRunControl::Private::~Private()
{
#define doDelete(ptr) delete ptr; ptr = 0
    doDelete(m_modulesHandler);
    doDelete(m_registerHandler);
#undef doDelete
}


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControl
//
////////////////////////////////////////////////////////////////////////

DebuggerRunControl::DebuggerRunControl(DebuggerManager *manager,
        const DebuggerStartParameters &startParameters)
    : RunControl(0, ProjectExplorer::Constants::DEBUGMODE),
      d(new Private(this, manager, startParameters))
{
    connect(d->m_manager, SIGNAL(debuggingFinished()),
            this, SLOT(debuggingFinished()),
            Qt::QueuedConnection);
    connect(d->m_manager, SIGNAL(messageAvailable(QString, bool)),
            this, SLOT(slotMessageAvailable(QString, bool)));
    connect(d->m_manager, SIGNAL(inferiorPidChanged(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(stopRequested()),
            d->m_manager, SLOT(exitDebugger()));

    if (d->m_startParameters.environment.empty())
        d->m_startParameters.environment = ProjectExplorer::Environment().toStringList();
    d->m_startParameters.useTerminal = false;

}

DebuggerRunControl::~DebuggerRunControl()
{
    delete d;
}

QString DebuggerRunControl::displayName() const
{
    return d->m_startParameters.displayName;
}

void DebuggerRunControl::setCustomEnvironment(ProjectExplorer::Environment env)
{
    d->m_startParameters.environment = env.toStringList();
}

void DebuggerRunControl::init()
{
}

void DebuggerRunControl::start()
{
    d->m_running = true;
    QString errorMessage;
    QString settingsCategory;
    QString settingsPage;
    if (d->m_manager->checkDebugConfiguration(d->m_startParameters.toolChainType,
            &errorMessage, &settingsCategory, &settingsPage)) {
        d->m_manager->startNewDebugger(this);
        emit started();
    } else {
        appendMessage(this, errorMessage, true);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger"),
            errorMessage, QString(), settingsCategory, settingsPage);
    }
}

void DebuggerRunControl::showMessage(const QString &msg, int channel,
    int timeout)
{
    if (!d->m_manager)
        return;
    DebuggerOutputWindow *ow = d->m_manager->debuggerOutputWindow();
    QTC_ASSERT(ow, return);
    switch (channel) {
        case StatusBar:
            d->m_manager->showStatusMessage(msg, timeout);
            ow->showOutput(LogStatus, msg);
            break;
        case AppOutput:
            emit addToOutputWindowInline(this, msg, false);
            break;
        case AppError:
            emit addToOutputWindowInline(this, msg, true);
            break;
        case LogMiscInput:
            ow->showInput(LogMisc, msg);
            ow->showOutput(LogMisc, msg);
            break;
        case LogInput:
            ow->showInput(channel, msg);
            ow->showOutput(channel, msg);
            break;
        default:
            ow->showOutput(channel, msg);
            break;
    }
}

void DebuggerRunControl::slotMessageAvailable(const QString &data, bool isError)
{
    emit appendMessage(this, data, isError);
}

void DebuggerRunControl::stop()
{
    d->m_running = false;
    emit stopRequested();
}

void DebuggerRunControl::debuggingFinished()
{
    d->m_running = false;
    emit finished();
}

bool DebuggerRunControl::isRunning() const
{
    return d->m_running;
}

const DebuggerStartParameters &DebuggerRunControl::sp() const
{
    return d->m_startParameters;
}

ModulesHandler *DebuggerRunControl::modulesHandler() const
{
    return d->m_modulesHandler;
}

BreakHandler *DebuggerRunControl::breakHandler() const
{
    return d->m_manager->breakHandler();
}

RegisterHandler *DebuggerRunControl::registerHandler() const
{
    return d->m_registerHandler;
}

StackHandler *DebuggerRunControl::stackHandler() const
{
    return d->m_manager->stackHandler();
}

ThreadsHandler *DebuggerRunControl::threadsHandler() const
{
    return d->m_manager->threadsHandler();
}

WatchHandler *DebuggerRunControl::watchHandler() const
{
    return d->m_manager->watchHandler();
}

SnapshotHandler *DebuggerRunControl::snapshotHandler() const
{
    return d->m_manager->snapshotHandler();
}

void DebuggerRunControl::cleanup()
{
    modulesHandler()->removeAll();
}

Internal::IDebuggerEngine *DebuggerRunControl::engine()
{
    QTC_ASSERT(d->m_engine, /**/);
    return d->m_engine;
}

void DebuggerRunControl::startDebugger(IDebuggerEngine *engine)
{
    d->m_engine = engine;
    d->m_engine->setRunControl(this);
    d->m_manager->modulesWindow()->setModel(d->m_modulesHandler->model());
    d->m_manager->registerWindow()->setModel(d->m_registerHandler->model());
    d->m_engine->startDebugger();
}


//////////////////////////////////////////////////////////////////////
//
// AbstractDebuggerEngine
//
//////////////////////////////////////////////////////////////////////

/*
void IDebuggerEngine::showStatusMessage(const QString &msg, int timeout)
{
    m_manager->showStatusMessage(msg, timeout);
}
*/

DebuggerState IDebuggerEngine::state() const
{
    return m_manager->state();
}

void IDebuggerEngine::setState(DebuggerState state, bool forced)
{
    m_manager->setState(state, forced);
}

bool IDebuggerEngine::debuggerActionsEnabled() const
{
    return m_manager->debuggerActionsEnabled();
}

void IDebuggerEngine::showModuleSymbols
    (const QString &moduleName, const Symbols &symbols)
{
    QTreeWidget *w = new QTreeWidget;
    w->setColumnCount(3);
    w->setRootIsDecorated(false);
    w->setAlternatingRowColors(true);
    w->setSortingEnabled(true);
    w->setHeaderLabels(QStringList() << tr("Symbol") << tr("Address") << tr("Code"));
    w->setWindowTitle(tr("Symbols in \"%1\"").arg(moduleName));
    foreach (const Symbol &s, symbols) {
        QTreeWidgetItem *it = new QTreeWidgetItem;
        it->setData(0, Qt::DisplayRole, s.name);
        it->setData(1, Qt::DisplayRole, s.address);
        it->setData(2, Qt::DisplayRole, s.state);
        w->addTopLevelItem(it);
    }
    manager()->createNewDock(w);
}

ModulesHandler *IDebuggerEngine::modulesHandler() const
{
    return runControl()->modulesHandler();
}

BreakHandler *IDebuggerEngine::breakHandler() const
{
    return runControl()->breakHandler();
}

RegisterHandler *IDebuggerEngine::registerHandler() const
{
    return runControl()->registerHandler();
}

StackHandler *IDebuggerEngine::stackHandler() const
{
    return runControl()->stackHandler();
}

ThreadsHandler *IDebuggerEngine::threadsHandler() const
{
    return runControl()->threadsHandler();
}

WatchHandler *IDebuggerEngine::watchHandler() const
{
    return runControl()->watchHandler();
}

SnapshotHandler *IDebuggerEngine::snapshotHandler() const
{
    return runControl()->snapshotHandler();
}

} // namespace Debugger
