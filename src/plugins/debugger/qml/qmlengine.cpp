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

#include "debuggertooltip.h"
#include "debuggerconstants.h"
#include "debuggerplugin.h"
#include "debuggerdialogs.h"
#include "debuggerstringutils.h"
#include "debuggeruiswitcher.h"
#include "debuggerrunner.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/applicationlauncher.h>

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
    data.setType(type.toUtf8(), false);
    data.setValue(value);
    data.setHasChildren(hasChildren);
    data.setAllUnneeded();
    return s;
}

} // namespace Internal

struct QmlEnginePrivate {
    explicit QmlEnginePrivate(QmlEngine *q);

    int m_ping;
    QmlAdapter *m_adapter;
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    bool m_addedAdapterToObjectPool;
    bool m_attachToRunningExternalApp;
    bool m_hasShutdown;
};

QmlEnginePrivate::QmlEnginePrivate(QmlEngine *q) :
  m_ping(0)
, m_adapter(new QmlAdapter(q))
, m_addedAdapterToObjectPool(false)
, m_attachToRunningExternalApp(false)
, m_hasShutdown(false)
{
}

///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters), d(new QmlEnginePrivate(this))
{
    setObjectName(QLatin1String("QmlEngine"));
}

QmlEngine::~QmlEngine()
{
}

void QmlEngine::setAttachToRunningExternalApp(bool value)
{
    d->m_attachToRunningExternalApp = value;
}

void QmlEngine::pauseConnection()
{
    d->m_adapter->pauseConnection();
}

void QmlEngine::gotoLocation(const QString &fileName, int lineNumber, bool setMarker)
{
    QString processedFilename = fileName;

    if (isShadowBuildProject())
        processedFilename = fromShadowBuildFilename(fileName);

    DebuggerEngine::gotoLocation(processedFilename, lineNumber, setMarker);
}

void QmlEngine::gotoLocation(const Internal::StackFrame &frame, bool setMarker)
{
    Internal::StackFrame adjustedFrame = frame;
    if (isShadowBuildProject())
        adjustedFrame.file = fromShadowBuildFilename(frame.file);

    DebuggerEngine::gotoLocation(adjustedFrame, setMarker);
}

void QmlEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    if (startParameters().startMode == AttachToRemote) {
        emit remoteStartupRequested();
    } else {
        connect(&d->m_applicationLauncher, SIGNAL(processExited(int)),
                this, SLOT(disconnected()));
        connect(&d->m_applicationLauncher, SIGNAL(appendMessage(QString,bool)),
                runControl(), SLOT(emitAppendMessage(QString,bool)));
        connect(&d->m_applicationLauncher, SIGNAL(appendOutput(QString, bool)),
                runControl(), SLOT(emitAddToOutputWindow(QString, bool)));
        connect(&d->m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
                runControl(), SLOT(bringApplicationToForeground(qint64)));

        d->m_applicationLauncher.setEnvironment(startParameters().environment);
        d->m_applicationLauncher.setWorkingDirectory(startParameters().workingDirectory);

        notifyInferiorSetupOk();
    }
}

void QmlEngine::connectionEstablished()
{
    attemptBreakpointSynchronization();

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(d->m_adapter);
    pluginManager->addObject(this);
    d->m_addedAdapterToObjectPool = true;

    plugin()->showMessage(tr("QML Debugger connected."), StatusBar);

    notifyEngineRunAndInferiorRunOk();
}

void QmlEngine::connectionStartupFailed()
{
    QMessageBox::critical(0,
                          tr("Failed to connect to debugger"),
                          tr("Could not connect to QML debugger server at %1:%2.")
                          .arg(startParameters().qmlServerAddress)
                          .arg(startParameters().qmlServerPort));
    notifyEngineRunFailed();
}

void QmlEngine::connectionError(QAbstractSocket::SocketError socketError)
{
    if (socketError ==QAbstractSocket::RemoteHostClosedError)
        plugin()->showMessage(tr("QML Debugger: Remote host closed connection."), StatusBar);
}

void QmlEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    if (!d->m_attachToRunningExternalApp) {
        d->m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui,
                                    startParameters().executable,
                                    startParameters().processArgs);
    }

    d->m_adapter->beginConnection();
    plugin()->showMessage(tr("QML Debugger connecting..."), StatusBar);
}

void QmlEngine::handleRemoteSetupDone()
{
    notifyInferiorSetupOk();
}

void QmlEngine::handleRemoteSetupFailed(const QString &message)
{
    QMessageBox::critical(0,tr("Failed to start application"),
        tr("Application startup failed: %1").arg(message));
    notifyInferiorSetupFailed();
}

void QmlEngine::shutdownInferiorAsSlave()
{
    resetLocation();

    // This can be issued in almost any state. We assume, though,
    // that at this point of time the inferior is not running anymore,
    // even if stop notification were not issued or got lost.
    if (state() == InferiorRunOk) {
        setState(InferiorStopRequested);
        setState(InferiorStopOk);
    }
    setState(InferiorShutdownRequested);
    setState(InferiorShutdownOk);
}

void QmlEngine::shutdownEngineAsSlave()
{
    if (d->m_hasShutdown)
        return;

    disconnect(d->m_adapter, SIGNAL(connectionStartupFailed()), this, SLOT(connectionStartupFailed()));
    d->m_adapter->closeConnection();

    if (d->m_addedAdapterToObjectPool) {
        ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
        pluginManager->removeObject(d->m_adapter);
        pluginManager->removeObject(this);
    }

    if (d->m_attachToRunningExternalApp) {
        setState(EngineShutdownRequested, true);
        setState(EngineShutdownOk, true);
        setState(DebuggerFinished, true);
    } else {
        if (d->m_applicationLauncher.isRunning()) {
            // should only happen if engine is ill
            disconnect(&d->m_applicationLauncher, SIGNAL(processExited(int)), this, SLOT(disconnected()));
            d->m_applicationLauncher.stop();
        }
    }
    d->m_hasShutdown = true;
}

void QmlEngine::shutdownInferior()
{
    // don't do normal shutdown if running as slave engine
    if (d->m_attachToRunningExternalApp)
        return;

    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    if (!d->m_applicationLauncher.isRunning()) {
        showMessage(tr("Trying to stop while process is no longer running."), LogError);
    } else {
        disconnect(&d->m_applicationLauncher, SIGNAL(processExited(int)), this, SLOT(disconnected()));
        if (!d->m_attachToRunningExternalApp)
            d->m_applicationLauncher.stop();
    }
    notifyInferiorShutdownOk();
}

void QmlEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());

    shutdownEngineAsSlave();

    notifyEngineShutdownOk();
    plugin()->showMessage(QString(), StatusBar);
}

void QmlEngine::setupEngine()
{
    d->m_adapter->setMaxConnectionAttempts(MaxConnectionAttempts);
    d->m_adapter->setConnectionAttemptInterval(ConnectionAttemptDefaultInterval);
    connect(d->m_adapter, SIGNAL(connectionError(QAbstractSocket::SocketError)),
            SLOT(connectionError(QAbstractSocket::SocketError)));
    connect(d->m_adapter, SIGNAL(connected()), SLOT(connectionEstablished()));
    connect(d->m_adapter, SIGNAL(connectionStartupFailed()), SLOT(connectionStartupFailed()));

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
    Internal::BreakHandler *handler = breakHandler();
    //bool updateNeeded = false;
    QSet< QPair<QString, qint32> > breakList;
    for (int index = 0; index != handler->size(); ++index) {
        Internal::BreakpointData *data = handler->at(index);
        QString processedFilename = data->fileName;
        if (isShadowBuildProject())
            processedFilename = toShadowBuildFilename(data->fileName);
        breakList << qMakePair(processedFilename, data->lineNumber);
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

void QmlEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    // this is processed by QML inspector, which has deps to qml js editor. Makes life easier.
    emit tooltipRequested(mousePos, editor, cursorPos);
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

void QmlEngine::updateWatchData(const Internal::WatchData &data, const Internal::WatchUpdateFlags &)
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
    d->m_ping++;
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("PING");
    rs << d->m_ping;
    sendMessage(reply);
}

namespace Internal {
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &sp)
{
    return new QmlEngine(sp);
}
} // namespace Internal

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

    showMessage(QLatin1String("RECEIVED RESPONSE: ") + Internal::quoteUnprintableLatin1(message));
    if (command == "STOPPED") {
        if (state() == InferiorRunOk) {
            notifyInferiorSpontaneousStop();
        }

        QList<QPair<QString, QPair<QString, qint32> > > backtrace;
        QList<Internal::WatchData> watches;
        QList<Internal::WatchData> locals;
        stream >> backtrace >> watches >> locals;

        Internal::StackFrames stackFrames;
        typedef QPair<QString, QPair<QString, qint32> > Iterator;
        foreach (const Iterator &it, backtrace) {
            Internal::StackFrame frame;
            frame.file = it.second.first;
            frame.line = it.second.second;
            frame.function = it.first;
            stackFrames.append(frame);
        }

        gotoLocation(stackFrames.value(0), true);
        stackHandler()->setFrames(stackFrames);

        watchHandler()->beginCycle();
        bool needPing = false;

        foreach (Internal::WatchData data, watches) {
            data.iname = watchHandler()->watcherName(data.exp);
            watchHandler()->insertData(data);

            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.objectId);
            }
        }

        foreach (Internal::WatchData data, locals) {
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

        bool becauseOfException;
        stream >> becauseOfException;
        if (becauseOfException) {
            QString error;
            stream >> error;

            QString msg =
                tr("<p>An Uncaught Exception occured in <i>%1</i>:</p><p>%2</p>")
                    .arg(stackFrames.value(0).file, Qt::escape(error));
            showMessageBox(QMessageBox::Information, tr("Uncaught Exception"), msg);
        }


    } else if (command == "RESULT") {
        Internal::WatchData data;
        QByteArray iname;
        stream >> iname >> data;
        data.iname = iname;
        if (iname.startsWith("watch.")) {
            watchHandler()->insertData(data);
        } else if(iname == "console") {
            plugin()->showMessage(data.value, ScriptConsoleOutput);
        } else {
            qWarning() << "QmlEngine: Unexcpected result: " << iname << data.value;
        }
    } else if (command == "EXPANDED") {
        QList<Internal::WatchData> result;
        QByteArray iname;
        stream >> iname >> result;
        bool needPing = false;
        foreach (Internal::WatchData data, result) {
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
        QList<Internal::WatchData> locals;
        int frameId;
        stream >> frameId >> locals;
        watchHandler()->beginCycle();
        bool needPing = false;
        foreach (Internal::WatchData data, locals) {
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
        if (ping == d->m_ping)
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

void QmlEngine::executeDebuggerCommand(const QString& command)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("EXEC");
    rs << QByteArray("console") << command;
    sendMessage(reply);
}

bool QmlEngine::isShadowBuildProject() const
{
    if (!startParameters().projectBuildDir.isEmpty()
        && (startParameters().projectDir != startParameters().projectBuildDir))
    {
        return true;
    }
    return false;
}

QString QmlEngine::qmlImportPath() const
{
    QString result;
    const QString qmlImportPathPrefix("QML_IMPORT_PATH=");
    QStringList env = startParameters().environment;
    foreach(const QString &envStr, env) {
        if (envStr.startsWith(qmlImportPathPrefix)) {
            result = envStr.mid(qmlImportPathPrefix.length());
            break;
        }
    }
    return result;
}

QString QmlEngine::toShadowBuildFilename(const QString &filename) const
{
    QString newFilename = filename;
    QString importPath = qmlImportPath();

    newFilename = mangleFilenamePaths(filename, startParameters().projectDir, startParameters().projectBuildDir);
    if (newFilename == filename && !importPath.isEmpty()) {
        newFilename = mangleFilenamePaths(filename, startParameters().projectDir, importPath);
    }

    return newFilename;
}

QString QmlEngine::mangleFilenamePaths(const QString &filename, const QString &oldBasePath, const QString &newBasePath) const
{
    QDir oldBaseDir(oldBasePath);
    QDir newBaseDir(newBasePath);
    QFileInfo fileInfo(filename);

    if (oldBaseDir.exists() && newBaseDir.exists() && fileInfo.exists()) {
        if (fileInfo.absoluteFilePath().startsWith(oldBaseDir.canonicalPath())) {
            QString fileRelativePath = fileInfo.canonicalFilePath().mid(oldBasePath.length());
            QFileInfo projectFile(newBaseDir.canonicalPath() + QLatin1Char('/') + fileRelativePath);

            if (projectFile.exists())
                return projectFile.canonicalFilePath();
        }
    }
    return filename;
}

QString QmlEngine::fromShadowBuildFilename(const QString &filename) const
{
    QString newFilename = filename;
    QString importPath = qmlImportPath();

    newFilename = mangleFilenamePaths(filename, startParameters().projectBuildDir, startParameters().projectDir);
    if (newFilename == filename && !importPath.isEmpty()) {
        newFilename = mangleFilenamePaths(filename, startParameters().projectBuildDir, importPath);
    }

    return newFilename;
}

} // namespace Debugger

