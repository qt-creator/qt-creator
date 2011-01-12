/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlengine.h"
#include "qmladapter.h"

#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggermainwindow.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggertooltip.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/applicationlauncher.h>

#include <utils/environment.h>
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

struct JSAgentBreakpointData
{
    QByteArray functionName;
    QByteArray fileName;
    qint32 lineNumber;
};

uint qHash(const JSAgentBreakpointData &b)
{
    return b.lineNumber ^ qHash(b.fileName);
}

QDataStream &operator<<(QDataStream &s, const JSAgentBreakpointData &data)
{
    return s << data.functionName << data.fileName << data.lineNumber;
}

QDataStream &operator>>(QDataStream &s, JSAgentBreakpointData &data)
{
    return s >> data.functionName >> data.fileName >> data.lineNumber;
}

bool operator==(const JSAgentBreakpointData &b1, const JSAgentBreakpointData &b2)
{
    return b1.lineNumber == b2.lineNumber && b1.fileName == b2.fileName;
}

typedef QSet<JSAgentBreakpointData> JSAgentBreakpoints;


static QDataStream &operator>>(QDataStream &s, WatchData &data)
{
    data = WatchData();
    QByteArray name;
    QByteArray value;
    QByteArray type;
    bool hasChildren = false;
    s >> data.exp >> name >> value >> type >> hasChildren >> data.id;
    data.name = QString::fromUtf8(name);
    data.setType(type, false);
    data.setValue(QString::fromUtf8(value));
    data.setHasChildren(hasChildren);
    data.setAllUnneeded();
    return s;
}

static QDataStream &operator>>(QDataStream &s, StackFrame &frame)
{
    frame = StackFrame();
    QByteArray function;
    QByteArray file;
    s >> function >> file >> frame.line;
    frame.function = QString::fromUtf8(function);
    frame.file = QString::fromUtf8(file);
    frame.usable = QFileInfo(frame.file).isReadable();
    return s;
}


class QmlEnginePrivate
{
public:
    explicit QmlEnginePrivate(QmlEngine *q);
    ~QmlEnginePrivate() { delete m_adapter; }

private:
    friend class QmlEngine;
    int m_ping;
    QmlAdapter *m_adapter;
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
};

QmlEnginePrivate::QmlEnginePrivate(QmlEngine *q)
    : m_ping(0), m_adapter(new QmlAdapter(q))
{}


///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine)
  : DebuggerEngine(startParameters, masterEngine),
    d(new QmlEnginePrivate(this))
{
    setObjectName(QLatin1String("QmlEngine"));
}

QmlEngine::~QmlEngine()
{}

void QmlEngine::gotoLocation(const Location &loc0)
{
    Location loc = loc0;
    if (isShadowBuildProject())
        loc.setFileName(fromShadowBuildFilename(loc0.fileName()));
    DebuggerEngine::gotoLocation(loc);
}

void QmlEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    if (startParameters().startMode == AttachToRemote) {
        requestRemoteSetup();
    } else {
        connect(&d->m_applicationLauncher, SIGNAL(processExited(int)),
                SLOT(disconnected()));
        connect(&d->m_applicationLauncher, SIGNAL(appendMessage(QString,ProjectExplorer::OutputFormat)),
                SLOT(appendMessage(QString,ProjectExplorer::OutputFormat)));
        connect(&d->m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
                runControl(), SLOT(bringApplicationToForeground(qint64)));

        d->m_applicationLauncher.setEnvironment(startParameters().environment);
        d->m_applicationLauncher.setWorkingDirectory(startParameters().workingDirectory);

        notifyInferiorSetupOk();
    }
}

void QmlEngine::appendMessage(const QString &msg, ProjectExplorer::OutputFormat /* format */)
{
    showMessage(msg, AppStuff); // FIXME: Redirect to RunControl
}

void QmlEngine::connectionEstablished()
{
    attemptBreakpointSynchronization();

    ExtensionSystem::PluginManager *pluginManager =
        ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(d->m_adapter);
    pluginManager->addObject(this);

    showMessage(tr("QML Debugger connected."), StatusBar);

    notifyEngineRunAndInferiorRunOk();

}

void QmlEngine::connectionStartupFailed()
{
    QMessageBox::critical(0, tr("Failed to connect to debugger"),
        tr("Could not connect to QML debugger server at %1:%2.")
        .arg(startParameters().qmlServerAddress)
        .arg(startParameters().qmlServerPort));
    notifyEngineRunFailed();
}

void QmlEngine::connectionError(QAbstractSocket::SocketError socketError)
{
    if (socketError ==QAbstractSocket::RemoteHostClosedError)
        showMessage(tr("QML Debugger: Remote host closed connection."), StatusBar);
}


void QmlEngine::serviceConnectionError(const QString &serviceName)
{
    showMessage(tr("QML Debugger: Could not connect to service '%1'.")
        .arg(serviceName), StatusBar);
}

void QmlEngine::pauseConnection()
{
    d->m_adapter->pauseConnection();
}

void QmlEngine::closeConnection()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    if (pluginManager->allObjects().contains(this)) {
        disconnect(d->m_adapter, SIGNAL(connectionStartupFailed()), this, SLOT(connectionStartupFailed()));
        d->m_adapter->closeConnection();

        pluginManager->removeObject(d->m_adapter);
        pluginManager->removeObject(this);
    }
}


void QmlEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    if (!isSlaveEngine()) {
        startApplicationLauncher();
    }

    d->m_adapter->beginConnection();
}

void QmlEngine::startApplicationLauncher()
{
    if (!d->m_applicationLauncher.isRunning()) {
        d->m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui,
                                    startParameters().executable,
                                    startParameters().processArgs);
    }
}

void QmlEngine::stopApplicationLauncher()
{
    if (d->m_applicationLauncher.isRunning()) {
        disconnect(&d->m_applicationLauncher, SIGNAL(processExited(int)), this, SLOT(disconnected()));
        d->m_applicationLauncher.stop();
    }
}

void QmlEngine::handleRemoteSetupDone(int port)
{
    if (port != -1)
        startParameters().qmlServerPort = port;
    notifyInferiorSetupOk();
}

void QmlEngine::handleRemoteSetupFailed(const QString &message)
{
    QMessageBox::critical(0,tr("Failed to start application"),
        tr("Application startup failed: %1").arg(message));
    notifyInferiorSetupFailed();
}

void QmlEngine::shutdownInferior()
{
    if (isSlaveEngine()) {
        resetLocation();
    }
    stopApplicationLauncher();
    notifyInferiorShutdownOk();
}

void QmlEngine::shutdownEngine()
{
    closeConnection();

    // double check (ill engine?):
    stopApplicationLauncher();

    notifyEngineShutdownOk();
    if (!isSlaveEngine())
        showMessage(QString(), StatusBar);
}

void QmlEngine::setupEngine()
{
    d->m_adapter->setMaxConnectionAttempts(MaxConnectionAttempts);
    d->m_adapter->setConnectionAttemptInterval(ConnectionAttemptDefaultInterval);
    connect(d->m_adapter, SIGNAL(connectionError(QAbstractSocket::SocketError)),
        SLOT(connectionError(QAbstractSocket::SocketError)));
    connect(d->m_adapter, SIGNAL(serviceConnectionError(QString)),
        SLOT(serviceConnectionError(QString)));
    connect(d->m_adapter, SIGNAL(connected()),
        SLOT(connectionEstablished()));
    connect(d->m_adapter, SIGNAL(connectionStartupFailed()),
        SLOT(connectionStartupFailed()));

    notifyEngineSetupOk();
}

void QmlEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "CONTINUE";
    rs << cmd;
    logMessage(LogSend, cmd);
    sendMessage(reply);
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::interruptInferior()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "INTERRUPT";
    rs << cmd;
    logMessage(LogSend, cmd);
    sendMessage(reply);
    notifyInferiorStopOk();
}

void QmlEngine::executeStep()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPINTO";
    rs << cmd;
    logMessage(LogSend, cmd);
    sendMessage(reply);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepI()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPINTO";
    rs << cmd;
    logMessage(LogSend, cmd);
    sendMessage(reply);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepOut()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPOUT";
    rs << cmd;
    logMessage(LogSend, cmd);
    sendMessage(reply);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNext()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPOVER";
    rs << cmd;
    logMessage(LogSend, cmd);
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
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "ACTIVATE_FRAME";
    rs << cmd
       << index;
    logMessage(LogSend, QString("%1 %2").arg(QString(cmd), QString::number(index)));
    sendMessage(reply);
    gotoLocation(stackHandler()->frames().value(index));
}

void QmlEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

void QmlEngine::attemptBreakpointSynchronization()
{
    BreakHandler *handler = breakHandler();

    foreach (BreakpointId id, handler->unclaimedBreakpointIds()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(id))
            handler->setEngine(id, this);
    }

    JSAgentBreakpoints breakpoints;
    foreach (BreakpointId id, handler->engineBreakpointIds(this)) {
        if (handler->state(id) == BreakpointRemoveRequested) {
            handler->notifyBreakpointRemoveProceeding(id);
            handler->notifyBreakpointRemoveOk(id);
        } else {
            if (handler->state(id) == BreakpointInsertRequested) {
                handler->notifyBreakpointInsertProceeding(id);
            }
            QString processedFilename = handler->fileName(id);
#ifdef Q_OS_MACX
            // Qt Quick Applications by default copy the qml directory to buildDir()/X.app/Contents/Resources
            const QString applicationBundleDir
                    = QFileInfo(startParameters().executable).absolutePath() + "/../..";
            processedFilename = mangleFilenamePaths(handler->fileName(id), startParameters().projectDir, applicationBundleDir + "/Contents/Resources");
#endif
            if (isShadowBuildProject())
                processedFilename = toShadowBuildFilename(processedFilename);
            JSAgentBreakpointData bp;
            bp.fileName = processedFilename.toUtf8();
            bp.lineNumber = handler->lineNumber(id);
            bp.functionName = handler->functionName(id).toUtf8();
            breakpoints.insert(bp);
            if (handler->state(id) == BreakpointInsertProceeding) {
                handler->notifyBreakpointInsertOk(id);
            }
        }
    }

    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "BREAKPOINTS";
    rs << cmd
       << breakpoints;

    QStringList breakPointsStr;
    foreach (const JSAgentBreakpointData &bp, breakpoints) {
        breakPointsStr << QString("('%1' '%2' %3)").arg(QString(bp.functionName),
                                  QString(bp.fileName), QString::number(bp.lineNumber));
    }
    logMessage(LogSend, QString("%1 [%2]").arg(QString(cmd), breakPointsStr.join(", ")));

    sendMessage(reply);
}

bool QmlEngine::acceptsBreakpoint(BreakpointId id) const
{
    return !DebuggerEngine::isCppBreakpoint(breakHandler()->breakpointData(id));
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

void QmlEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, int cursorPos)
{
    // This is processed by QML inspector, which has dependencies to 
    // the qml js editor. Makes life easier.
    emit tooltipRequested(mousePos, editor, cursorPos);
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void QmlEngine::assignValueInDebugger(const WatchData *,
    const QString &expression, const QVariant &valueV)
{
    QRegExp inObject("@([0-9a-fA-F]+)->(.+)");
    if (inObject.exactMatch(expression)) {
        bool ok = false;
        quint64 objectId = inObject.cap(1).toULongLong(&ok, 16);
        QString property = inObject.cap(2);
        if (ok && objectId > 0 && !property.isEmpty()) {
            QByteArray reply;
            QDataStream rs(&reply, QIODevice::WriteOnly);
            QByteArray cmd = "SET_PROPERTY";
            rs << cmd;
            rs << expression.toUtf8() << objectId << property << valueV.toString();
            logMessage(LogSend, QString("%1 %2 %3 %4 %5").arg(
                                 QString(cmd), QString::number(objectId), QString(property),
                                 valueV.toString()));
            sendMessage(reply);
        }
    }
}

void QmlEngine::updateWatchData(const WatchData &data,
    const WatchUpdateFlags &)
{
//    qDebug() << "UPDATE WATCH DATA" << data.toString();
    //watchHandler()->rebuildModel();
    showStatusMessage(tr("Stopped."), 5000);

    if (!data.name.isEmpty() && data.isValueNeeded()) {
        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        QByteArray cmd = "EXEC";
        rs << cmd;
        rs << data.iname << data.name;
        logMessage(LogSend, QString("%1 %2 %3").arg(QString(cmd), QString(data.iname),
                                                          QString(data.name)));
        sendMessage(reply);
    }

    if (!data.name.isEmpty() && data.isChildrenNeeded()
            && watchHandler()->isExpandedIName(data.iname))
        expandObject(data.iname, data.id);

    {
        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        QByteArray cmd = "WATCH_EXPRESSIONS";
        rs << cmd;
        rs << watchHandler()->watchedExpressions();
        logMessage(LogSend, QString("%1 %2").arg(
                             QString(cmd), watchHandler()->watchedExpressions().join(", ")));
        sendMessage(reply);
    }

    if (!data.isSomethingNeeded())
        watchHandler()->insertData(data);
}

void QmlEngine::expandObject(const QByteArray &iname, quint64 objectId)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXPAND";
    rs << cmd;
    rs << iname << objectId;
    logMessage(LogSend, QString("%1 %2 %3").arg(QString(cmd), QString(iname),
                                                      QString::number(objectId)));
    sendMessage(reply);
}

void QmlEngine::sendPing()
{
    d->m_ping++;
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "PING";
    rs << cmd;
    rs << d->m_ping;
    logMessage(LogSend, QString("%1 %2").arg(QString(cmd), QString::number(d->m_ping)));
    sendMessage(reply);
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

    if (command == "STOPPED") {
        if (state() == InferiorRunOk)
            notifyInferiorSpontaneousStop();

        QString logString = QString(command);

        StackFrames stackFrames;
        QList<WatchData> watches;
        QList<WatchData> locals;
        stream >> stackFrames >> watches >> locals;

        logString += QLatin1String(" (") + QString::number(stackFrames.size()) + QLatin1String(" stack frames)");
        logString += QLatin1String(" (") + QString::number(watches.size()) + QLatin1String(" watches)");
        logString += QLatin1String(" (") + QString::number(locals.size()) + QLatin1String(" locals)");

        for (int i = 0; i != stackFrames.size(); ++i)
            stackFrames[i].level = i + 1;

        gotoLocation(stackFrames.value(0));
        stackHandler()->setFrames(stackFrames);

        watchHandler()->beginCycle();
        bool needPing = false;

        foreach (WatchData data, watches) {
            data.iname = watchHandler()->watcherName(data.exp);
            watchHandler()->insertData(data);

            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.id);
            }
        }

        foreach (WatchData data, locals) {
            data.iname = "local." + data.exp;
            watchHandler()->insertData(data);

            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.id);
            }
        }

        if (needPing)
            sendPing();
        else
            watchHandler()->endCycle();

        bool becauseOfException;
        stream >> becauseOfException;

        logString += becauseOfException ? " exception" : " no_exception";

        if (becauseOfException) {
            QString error;
            stream >> error;

            logString += QLatin1String(" ") + error;
            logMessage(LogReceive, logString);

            QString msg =
                tr("<p>An Uncaught Exception occured in <i>%1</i>:</p><p>%2</p>")
                    .arg(stackFrames.value(0).file, Qt::escape(error));
            showMessageBox(QMessageBox::Information, tr("Uncaught Exception"), msg);
        } else {
            //
            // Make breakpoint non-pending
            //
            QString file;
            QString function;
            int line = -1;

            if (!stackFrames.isEmpty()) {
                file = stackFrames.at(0).file;
                line = stackFrames.at(0).line;
                function = stackFrames.at(0).function;

                if (isShadowBuildProject()) {
                    file = fromShadowBuildFilename(file);
                }
            }

            BreakHandler *handler = breakHandler();
            foreach (BreakpointId id, handler->engineBreakpointIds(this)) {
                QString processedFilename = handler->fileName(id);
                if (processedFilename == file && handler->lineNumber(id) == line) {
                    QTC_ASSERT(handler->state(id) == BreakpointInserted,/**/);
                    BreakpointResponse br = handler->response(id);
                    br.fileName = file;
                    br.lineNumber = line;
                    br.functionName = function;
                    handler->setResponse(id, br);
                }
            }

            logMessage(LogReceive, logString);
        }
    } else if (command == "RESULT") {
        WatchData data;
        QByteArray iname;
        stream >> iname >> data;

        logMessage(LogReceive, QString("%1 %2 %3").arg(QString(command),
                                                             QString(iname), QString(data.value)));
        data.iname = iname;
        if (iname.startsWith("watch.")) {
            watchHandler()->insertData(data);
        } else if(iname == "console") {
            showMessage(data.value, ScriptConsoleOutput);
        } else {
            qWarning() << "QmlEngine: Unexcpected result: " << iname << data.value;
        }
    } else if (command == "EXPANDED") {
        QList<WatchData> result;
        QByteArray iname;
        stream >> iname >> result;

        logMessage(LogReceive, QString("%1 %2 (%3 x watchdata)").arg(
                             QString(command), QString(iname), QString::number(result.size())));
        bool needPing = false;
        foreach (WatchData data, result) {
            data.iname = iname + '.' + data.exp;
            watchHandler()->insertData(data);

            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.id);
            }
        }
        if (needPing)
            sendPing();
    } else if (command == "LOCALS") {
        QList<WatchData> locals;
        int frameId;
        stream >> frameId >> locals;

        logMessage(LogReceive, QString("%1 %2 (%3 x locals)").arg(
                             QString(command), QString::number(frameId),
                             QString::number(locals.size())));
        watchHandler()->beginCycle();
        bool needPing = false;
        foreach (WatchData data, locals) {
            data.iname = "local." + data.exp;
            watchHandler()->insertData(data);
            if (watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.id);
            }
        }
        if (needPing)
            sendPing();
        else
            watchHandler()->endCycle();

    } else if (command == "PONG") {
        int ping;
        stream >> ping;

        logMessage(LogReceive, QString("%1 %2").arg(QString(command), QString::number(ping)));

        if (ping == d->m_ping)
            watchHandler()->endCycle();
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown command: " << command;
        logMessage(LogReceive, QString("%1 UNKNOWN COMMAND!!").arg(QString(command)));
    }
}

void QmlEngine::disconnected()
{
    showMessage(tr("QML Debugger disconnected."), StatusBar);
    notifyInferiorExited();
}

void QmlEngine::executeDebuggerCommand(const QString& command)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXEC";
    QByteArray console = "console";
    rs << cmd << console << command;
    logMessage(LogSend, QString("%1 %2 %3").arg(QString(cmd), QString(console),
                                                      QString(command)));
    sendMessage(reply);
}

bool QmlEngine::isShadowBuildProject() const
{
    return !startParameters().projectBuildDir.isEmpty()
        && startParameters().projectDir != startParameters().projectBuildDir;
}

QString QmlEngine::qmlImportPath() const
{
    return startParameters().environment.value("QML_IMPORT_PATH");
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

QString QmlEngine::mangleFilenamePaths(const QString &filename,
    const QString &oldBasePath, const QString &newBasePath) const
{
    QDir oldBaseDir(oldBasePath);
    QDir newBaseDir(newBasePath);
    QFileInfo fileInfo(filename);

    if (oldBaseDir.exists() && newBaseDir.exists() && fileInfo.exists()) {
        if (fileInfo.absoluteFilePath().startsWith(oldBaseDir.canonicalPath())) {
            QString fileRelativePath = fileInfo.canonicalFilePath().mid(oldBaseDir.canonicalPath().length());
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

#ifdef Q_OS_MACX
    // Qt Quick Applications by default copy the qml directory to buildDir()/X.app/Contents/Resources
    const QString applicationBundleDir
                = QFileInfo(startParameters().executable).absolutePath() + "/../..";
    newFilename = mangleFilenamePaths(newFilename, applicationBundleDir + "/Contents/Resources", startParameters().projectDir);
#endif
    newFilename = mangleFilenamePaths(newFilename, startParameters().projectBuildDir, startParameters().projectDir);

    if (newFilename == filename && !importPath.isEmpty()) {
        newFilename = mangleFilenamePaths(filename, startParameters().projectBuildDir, importPath);
    }

    return newFilename;
}

void QmlEngine::logMessage(LogDirection direction, const QString &message)
{
    QString msg = "JSDebugger";
    if (direction == LogSend) {
        msg += " sending ";
    } else {
        msg += " receiving ";
    }
    msg += message;
    showMessage(msg, LogDebug);
}

DebuggerEngine *createQmlEngine(const DebuggerStartParameters &sp,
    DebuggerEngine *masterEngine)
{
    return new QmlEngine(sp, masterEngine);
}

} // namespace Internal
} // namespace Debugger

