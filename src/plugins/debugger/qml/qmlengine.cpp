/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlengine.h"
#include "qmladapter.h"

#include "debuggerconstants.h"
#include "debuggerplugin.h"
#include "debuggerdialogs.h"
#include "debuggerstringutils.h"
#include "debuggeruiswitcher.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/environment.h>

#include <utils/qtcassert.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QToolTip>
#include <QtGui/QTextDocument>

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

#define DEBUG_QML 1
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

enum {
    MaxConnectionAttempts = 50,
    ConnectionAttemptDefaultInterval = 200
};

namespace Debugger {
namespace Internal {

QDataStream& operator>>(QDataStream& s, WatchData &data)
{
    data = WatchData();
    QString value;
    QString type;
    bool hasChildren;
    s >> data.exp >> data.name >> value >> type >> hasChildren >> data.objectId;
    data.setType(type, false);
    data.setValue(value);
    data.setHasChildren(hasChildren);
    data.setAllUnneeded();
    return s;
}

///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
    , m_ping(0)
    , m_adapter(new QmlAdapter(this))
    , m_addedAdapterToObjectPool(false)
{

}

QmlEngine::~QmlEngine()
{
}

void QmlEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    connect(&m_applicationLauncher, SIGNAL(processExited(int)), SLOT(disconnected()));
    m_applicationLauncher.setEnvironment(startParameters().environment);
    m_applicationLauncher.setWorkingDirectory(startParameters().workingDirectory);

    notifyInferiorSetupOk();
}

void QmlEngine::connectionEstablished()
{
    attemptBreakpointSynchronization();

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(m_adapter);
    m_addedAdapterToObjectPool = true;

    plugin()->showMessage(tr("QML Debugger connected."), StatusBar);

    notifyEngineRunAndInferiorRunOk();
}

void QmlEngine::connectionStartupFailed()
{
    QMessageBox::critical(0,
                          tr("Failed to connect to debugger"),
                          tr("Could not connect to debugger server.") );
    notifyEngineRunFailed();
}

void QmlEngine::connectionError()
{
    // do nothing for now - only exit the debugger when inferior exits.
}

void QmlEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    // ### TODO for non-qmlproject apps, start in a different way
    m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui,
                                startParameters().executable,
                                startParameters().processArgs);

    m_adapter->beginConnection();
    plugin()->showMessage(tr("QML Debugger connecting..."), StatusBar);
}

void QmlEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    if (!m_applicationLauncher.isRunning()) {
        showMessage(tr("Trying to stop while process is no longer running."), LogError);
    } else {
        disconnect(&m_applicationLauncher, SIGNAL(processExited(int)), this, SLOT(disconnected()));
        m_applicationLauncher.stop();
    }
    notifyInferiorShutdownOk();
}

void QmlEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());

    if (m_addedAdapterToObjectPool) {
        ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
        pluginManager->removeObject(m_adapter);
    }

    if (m_applicationLauncher.isRunning()) {
        // should only happen if engine is ill
        disconnect(&m_applicationLauncher, SIGNAL(processExited(int)), this, SLOT(disconnected()));
        m_applicationLauncher.stop();
    }

    notifyEngineShutdownOk();
}

void QmlEngine::setupEngine()
{
    m_adapter->setMaxConnectionAttempts(MaxConnectionAttempts);
    m_adapter->setConnectionAttemptInterval(ConnectionAttemptDefaultInterval);
    connect(m_adapter, SIGNAL(connectionError()), SLOT(connectionError()));
    connect(m_adapter, SIGNAL(connected()), SLOT(connectionEstablished()));
    connect(m_adapter, SIGNAL(connectionStartupFailed()), SLOT(connectionStartupFailed()));

    notifyEngineSetupOk();
}

void QmlEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("CONTINUE");
    sendMessage(reply);
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::interruptInferior()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("INTERRUPT");
    sendMessage(reply);
    notifyInferiorStopOk();
}

void QmlEngine::executeStep()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPINTO");
    sendMessage(reply);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepI()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPINTO");
    sendMessage(reply);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepOut()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPOUT");
    sendMessage(reply);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNext()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPOVER");
    sendMessage(reply);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNextI()
{
    SDEBUG("QmlEngine::executeNextI()");
}

void QmlEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    SDEBUG("FIXME:  QmlEngine::executeRunToLine()");
}

void QmlEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  QmlEngine::executeRunToFunction()");
}

void QmlEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    XSDEBUG("FIXME:  QmlEngine::executeJumpToLine()");
}

void QmlEngine::activateFrame(int index)
{
    Q_UNUSED(index)

    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("ACTIVATE_FRAME");
    rs << index;
    sendMessage(reply);

    gotoLocation(stackHandler()->frames().value(index), true);
}

void QmlEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

void QmlEngine::attemptBreakpointSynchronization()
{
    BreakHandler *handler = breakHandler();
    //bool updateNeeded = false;
    QSet< QPair<QString, qint32> > breakList;
    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        breakList << qMakePair(data->fileName, data->lineNumber.toInt());
    }

    {
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("BREAKPOINTS");
    rs << breakList;
    //qDebug() << Q_FUNC_INFO << breakList;
    sendMessage(reply);
    }
}

void QmlEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void QmlEngine::loadAllSymbols()
{
}

void QmlEngine::reloadModules()
{
}

void QmlEngine::requestModuleSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

void QmlEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    Q_UNUSED(mousePos)
    Q_UNUSED(editor)
    Q_UNUSED(cursorPos)
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void QmlEngine::assignValueInDebugger(const QString &expression,
    const QString &value)
{
    QRegExp inObject("@([0-9a-fA-F]+)->(.+)");
    if (inObject.exactMatch(expression)) {
        bool ok = false;
        quint64 objectId = inObject.cap(1).toULongLong(&ok, 16);
        QString property = inObject.cap(2);
        if (ok && objectId > 0 && !property.isEmpty()) {
            QByteArray reply;
            QDataStream rs(&reply, QIODevice::WriteOnly);
            rs << QByteArray("SET_PROPERTY");
            rs << expression.toUtf8() << objectId << property << value;
            sendMessage(reply);
        }
    }
}

void QmlEngine::updateWatchData(const WatchData &data)
{
//    qDebug() << "UPDATE WATCH DATA" << data.toString();
    //watchHandler()->rebuildModel();
    showStatusMessage(tr("Stopped."), 5000);

    if (!data.name.isEmpty() && data.isValueNeeded()) {
        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("EXEC");
        rs << data.iname << data.name;
        sendMessage(reply);
    }

    if (!data.name.isEmpty() && data.isChildrenNeeded()
            && watchHandler()->isExpandedIName(data.iname))
        expandObject(data.iname, data.objectId);

    {
        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_EXPRESSIONS");
        rs << watchHandler()->watchedExpressions();
        sendMessage(reply);
    }

    if (!data.isSomethingNeeded())
        watchHandler()->insertData(data);
}

void QmlEngine::expandObject(const QByteArray& iname, quint64 objectId)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("EXPAND");
    rs << iname << objectId;
    sendMessage(reply);
}

void QmlEngine::sendPing()
{
    m_ping++;
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("PING");
    rs << m_ping;
    sendMessage(reply);
}

DebuggerEngine *createQmlEngine(const DebuggerStartParameters &sp)
{
    return new QmlEngine(sp);
}

unsigned QmlEngine::debuggerCapabilities() const
{
    return AddWatcherCapability;
    /*ReverseSteppingCapability | SnapshotCapability
        | AutoDerefPointersCapability | DisassemblerCapability
        | RegisterCapability | ShowMemoryCapability
        | JumpToLineCapability | ReloadModuleCapability
        | ReloadModuleSymbolsCapability | BreakOnThrowAndCatchCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointCapability
        | AddWatcherCapability;*/
}

void QmlEngine::messageReceived(const QByteArray &message)
{
    QByteArray rwData = message;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    QByteArray command;
    stream >> command;

    showMessage(_("RECEIVED RESPONSE: ") + quoteUnprintableLatin1(message));
    if (command == "STOPPED") {
        notifyInferiorSpontaneousStop();

        QList<QPair<QString, QPair<QString, qint32> > > backtrace;
        QList<WatchData> watches;
        QList<WatchData> locals;
        stream >> backtrace >> watches >> locals;

        StackFrames stackFrames;
        typedef QPair<QString, QPair<QString, qint32> > Iterator;
        foreach (const Iterator &it, backtrace) {
            StackFrame frame;
            frame.file = it.second.first;
            frame.line = it.second.second;
            frame.function = it.first;
            stackFrames.append(frame);
        }

        gotoLocation(stackFrames.value(0), true);
        stackHandler()->setFrames(stackFrames);

        watchHandler()->beginCycle();
        bool needPing = false;

        foreach (WatchData data, watches) {
            data.iname = watchHandler()->watcherName(data.exp);
            watchHandler()->insertData(data);

            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.objectId);
            }
        }

        foreach (WatchData data, locals) {
            data.iname = "local." + data.exp;
            watchHandler()->insertData(data);

            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.objectId);
            }
        }

        if (needPing)
            sendPing();
        else
            watchHandler()->endCycle();

        // Ensure we got the right ui right now.
        Debugger::DebuggerUISwitcher *uiSwitcher
            = Debugger::DebuggerUISwitcher::instance();
        uiSwitcher->setActiveLanguage("C++");

        bool becauseOfexception;
        stream >> becauseOfexception;
        if (becauseOfexception) {
            QString error;
            stream >> error;

            QString msg =
                tr("<p>An Uncaught Exception occured in <i>%1</i>:</p><p>%2</p>")
                    .arg(stackFrames.value(0).file, Qt::escape(error));
            showMessageBox(QMessageBox::Information, tr("Uncaught Exception"), msg);
        }


    } else if (command == "RESULT") {
        WatchData data;
        QByteArray iname;
        stream >> iname >> data;
        data.iname = iname;
        watchHandler()->insertData(data);

    } else if (command == "EXPANDED") {
        QList<WatchData> result;
        QByteArray iname;
        stream >> iname >> result;
        bool needPing = false;
        foreach (WatchData data, result) {
            data.iname = iname + '.' + data.exp;
            watchHandler()->insertData(data);

            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.objectId);
            }
        }
        if (needPing)
            sendPing();
    } else if (command == "LOCALS") {
        QList<WatchData> locals;
        int frameId;
        stream >> frameId >> locals;
        watchHandler()->beginCycle();
        bool needPing = false;
        foreach (WatchData data, locals) {
            data.iname = "local." + data.exp;
            watchHandler()->insertData(data);
            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.objectId);
            }
        }
        if (needPing)
            sendPing();
        else
            watchHandler()->endCycle();

    } else if (command == "PONG") {
        int ping;
        stream >> ping;
        if (ping == m_ping)
            watchHandler()->endCycle();
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown command: " << command;
    }

}

void QmlEngine::disconnected()
{
    plugin()->showMessage(tr("QML Debugger disconnected."), StatusBar);
    notifyInferiorExited();
}


} // namespace Internal
} // namespace Debugger

