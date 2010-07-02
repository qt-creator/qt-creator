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

#include "debuggerconstants.h"
#include "debuggerplugin.h"
#include "debuggerdialogs.h"
#include "debuggerstringutils.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include "canvasframerate.h"

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

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

#include <private/qdeclarativedebug_p.h>
#include <private/qdeclarativedebugclient_p.h>

#define DEBUG_QML 1
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

#define CB(callback) &QmlEngine::callback, STRINGIFY(callback)

//#define USE_CONGESTION_CONTROL


namespace Debugger {
namespace Internal {


class QmlResponse
{
public:
    QmlResponse() {}
    QmlResponse(const QByteArray &data_) : data(data_) {}

    QString toString() const { return data; }

    QByteArray data;
};


///////////////////////////////////////////////////////////////////////
//
// QmlDebuggerClient
//
///////////////////////////////////////////////////////////////////////

class QmlDebuggerClient : public QDeclarativeDebugClient
{
    Q_OBJECT

public:
    QmlDebuggerClient(QDeclarativeDebugConnection *connection, QmlEngine *engine)
        : QDeclarativeDebugClient(QLatin1String("Debugger"), connection)
        , m_connection(connection), m_engine(engine)
    {
        setEnabled(true);
    }

    void messageReceived(const QByteArray &data)
    {
        m_engine->messageReceived(data);
    }


    QDeclarativeDebugConnection *m_connection;
    QmlEngine *m_engine;
};


class QmlFrameRateClient : public QDeclarativeDebugClient
{
    Q_OBJECT

public:
    QmlFrameRateClient(QDeclarativeDebugConnection *connection, QmlEngine *engine)
        : QDeclarativeDebugClient(QLatin1String("CanvasFrameRate"), connection)
        , m_connection(connection), m_engine(engine)
    {
        setEnabled(true);
    }

    void messageReceived(const QByteArray &data)
    {
        Q_UNUSED(data);
        // FIXME
        //qDebug() << "CANVAS FRAME RATE: " << data.size();
        //m_engine->messageReceived(data);
    }


    QDeclarativeDebugConnection *m_connection;
    QmlEngine *m_engine;
};



///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    m_conn = 0;
    m_client = 0;
    m_engineQuery = 0;
    m_contextQuery = 0;

    m_frameRate = 0;
}

QmlEngine::~QmlEngine()
{
}

void QmlEngine::executeDebuggerCommand(const QString &command)
{
    QByteArray cmd = command.toUtf8();
    cmd = cmd.mid(cmd.indexOf(' ') + 1);
    QByteArray null;
    null.append('\0');
    // FIXME: works for single-digit escapes only
    cmd.replace("\\0", null);
    cmd.replace("\\1", "\1");
    cmd.replace("\\3", "\3");
    //QmlCommand tcf;
    //tcf.command = cmd;
    //enqueueCommand(tcf);
}

void QmlEngine::shutdown()
{
    exitDebugger();
}

void QmlEngine::exitDebugger()
{
    SDEBUG("QmlEngine::exitDebugger()");
}

const int serverPort = 3768;

void QmlEngine::startDebugger()
{
    QTC_ASSERT(state() == DebuggerNotReady, setState(DebuggerNotReady));
    setState(EngineStarting);
    const DebuggerStartParameters &sp = startParameters();
    const int pos = sp.remoteChannel.indexOf(QLatin1Char(':'));
    const QString host = sp.remoteChannel.left(pos);
    const quint16 port = sp.remoteChannel.mid(pos + 1).toInt();
    qDebug() << "STARTING QML ENGINE" <<  host << port << sp.remoteChannel
        << sp.executable << sp.processArgs << sp.workingDirectory;
  
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment(); // empty env by default
    env.set("QML_DEBUG_SERVER_PORT", QString::number(serverPort));

    connect(&m_proc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleProcError(QProcess::ProcessError)));
    connect(&m_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(handleProcFinished(int, QProcess::ExitStatus)));
    connect(&m_proc, SIGNAL(readyReadStandardOutput()),
        SLOT(readProcStandardOutput()));
    connect(&m_proc, SIGNAL(readyReadStandardError()),
        SLOT(readProcStandardError()));

    setState(AdapterStarting);
    m_proc.setEnvironment(env.toStringList());
    m_proc.setWorkingDirectory(sp.workingDirectory);
    m_proc.start(sp.executable, sp.processArgs);

    if (!m_proc.waitForStarted()) {
        setState(AdapterStartFailed);
        startFailed();
        return;
    }
    setState(AdapterStarted);
    setState(InferiorStarting);

    //m_frameRate = new CanvasFrameRate(0);
    //m_frameRate->show();
}

void QmlEngine::setupConnection()
{
    QTC_ASSERT(m_conn == 0, /**/);
    m_conn = new QDeclarativeDebugConnection(this);

    connect(m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionError()));
    connect(m_conn, SIGNAL(connected()),
            SLOT(connectionConnected()));

    QTC_ASSERT(m_client == 0, /**/);
    m_client = new QmlDebuggerClient(m_conn, this);
    (void) new QmlFrameRateClient(m_conn, this);

    //m_objectTreeWidget->setEngineDebug(m_client);
    //m_propertiesWidget->setEngineDebug(m_client);
    //m_watchTableModel->setEngineDebug(m_client);
    //m_expressionWidget->setEngineDebug(m_client);

    //   resetViews();
    //   m_frameRateWidget->reset(m_conn);
    //   reloadEngines();

    QHostAddress ha(QHostAddress::LocalHost);

    qDebug() << "CONNECTING TO " << ha.toString() << serverPort;
    m_conn->connectToHost(ha, serverPort);

    if (!m_conn->waitForConnected()) {
        qDebug() << "CONNECTION FAILED";
        setState(InferiorStartFailed);
        startFailed();
        return;
    }

    qDebug() << "CONNECTION SUCCESSFUL";
    setState(InferiorRunning);
    startSuccessful();
}

void QmlEngine::continueInferior()
{
    SDEBUG("QmlEngine::continueInferior()");
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("CONTINUE");
    sendMessage(reply);
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
}

void QmlEngine::runInferior()
{
}

void QmlEngine::interruptInferior()
{
    qDebug() << "INTERRUPT";
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("INTERRUPT");
    sendMessage(reply);
}

void QmlEngine::executeStep()
{
    SDEBUG("QmlEngine::executeStep()");
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPINTO");
    sendMessage(reply);
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
}

void QmlEngine::executeStepI()
{
    SDEBUG("QmlEngine::executeStepI()");
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPINTO");
    sendMessage(reply);
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
}

void QmlEngine::executeStepOut()
{
    SDEBUG("QmlEngine::executeStepOut()");
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPOUT");
    sendMessage(reply);
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
}

void QmlEngine::executeNext()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STEPOVER");
    sendMessage(reply);
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    SDEBUG("QmlEngine::nextExec()");
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
    qDebug() << Q_FUNC_INFO << index;
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

void QmlEngine::sendMessage(const QByteArray &msg)
{
    QTC_ASSERT(m_client, return);
    m_client->sendMessage(msg);
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
    XSDEBUG("ASSIGNING: " << expression + '=' + value);
    updateLocals();
}

void QmlEngine::updateLocals()
{
}

void QmlEngine::updateWatchData(const WatchData &data)
{
    //watchHandler()->rebuildModel();
    showStatusMessage(tr("Stopped."), 5000);

    if (!data.name.isEmpty()) {
        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("EXEC");
        rs << data.iname << data.name;
        sendMessage(reply);
    }

    {
        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_EXPRESSIONS");
        rs << watchHandler()->watchedExpressions();
        sendMessage(reply);
    }
}

void QmlEngine::updateSubItem(const WatchData &data0)
{
    Q_UNUSED(data0)
    QTC_ASSERT(false, return);
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

static void updateWatchDataFromVariant(const QVariant &value,  WatchData &data)
{
    switch (value.userType()) {
        case QVariant::Bool:
            data.setType(QLatin1String("Bool"), false);
            data.setValue(value.toBool() ? QLatin1String("true") : QLatin1String("false"));
            data.setHasChildren(false);
            break;
        case QVariant::Date:
        case QVariant::DateTime:
        case QVariant::Time:
            data.setType(QLatin1String("Date"), false);
            data.setValue(value.toDateTime().toString());
            data.setHasChildren(false);
            break;
        /*} else if (ob.isError()) {
            data.setType(QLatin1String("Error"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isFunction()) {
            data.setType(QLatin1String("Function"), false);
            data.setValue(QString(QLatin1Char(' ')));*/
        case QVariant::Invalid:{
            const QString nullValue = QLatin1String("<null>");
            data.setType(nullValue, false);
            data.setValue(nullValue);
            break;}
        case QVariant::UInt:
        case QVariant::Int:
        case QVariant::Double:
        //FIXME FLOAT
            data.setType(QLatin1String("Number"), false);
            data.setValue(QString::number(value.toDouble()));
            data.setHasChildren(false);
            break;
/*                } else if (ob.isObject()) {
            data.setType(QLatin1String("Object"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isQMetaObject()) {
            data.setType(QLatin1String("QMetaObject"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isQObject()) {
            data.setType(QLatin1String("QObject"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isRegExp()) {
            data.setType(QLatin1String("RegExp"), false);
            data.setValue(ob.toRegExp().pattern());
        } else if (ob.isString()) {*/
        case QVariant::String:
            data.setType(QLatin1String("String"), false);
            data.setValue(value.toString());
/*                } else if (ob.isVariant()) {
            data.setType(QLatin1String("Variant"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isUndefined()) {
            data.setType(QLatin1String("<undefined>"), false);
            data.setValue(QLatin1String("<unknown>"));
            data.setHasChildren(false);
        } else {*/
        default:{
            const QString unknown = QLatin1String("<unknown>");
            data.setType(unknown, false);
            data.setValue(unknown);
            data.setHasChildren(false);
        }
    }
}

void QmlEngine::messageReceived(const QByteArray &message)
{
    QByteArray rwData = message;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    QByteArray command;
    stream >> command;

    qDebug() << "RECEIVED COMMAND: " << command;

    showMessage(_("RECEIVED RESPONSE" + command));
    if (command == "STOPPED") {
        setState(InferiorStopping);
        setState(InferiorStopped);

        QList<QPair<QString, QPair<QString, qint32> > > backtrace;
        QList<QPair<QString, QVariant> > watches;
        stream >> backtrace >> watches;

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

        typedef QPair<QString, QVariant > Iterator2;
        foreach (const Iterator2 &it, watches) {
            WatchData data;
            data.name = it.first;
            data.exp = it.first.toUtf8();
            data.iname = watchHandler()->watcherName(data.exp);
            updateWatchDataFromVariant(it.second,  data);
            watchHandler()->insertData(data);
        }

        watchHandler()->endCycle();

    } else if (command == "RESULT") {
        WatchData data;
        QVariant variant;
        stream >> data.iname >> data.name >> variant;
        data.exp = data.name.toUtf8();
        updateWatchDataFromVariant(variant,  data);
        qDebug() << Q_FUNC_INFO << this << data.name << data.iname << variant;
        watchHandler()->insertData(data);
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown command: " << command;
    }

}

QString QmlEngine::errorMessage(QProcess::ProcessError error)
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Gdb process failed to start. Either the "
                "invoked program is missing, or you may have insufficient "
                "permissions to invoke the program.");
        case QProcess::Crashed:
            return tr("The Gdb process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the Gdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Gdb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the Gdb process occurred. ");
    }
}

void QmlEngine::handleProcError(QProcess::ProcessError error)
{
    showMessage(_("HANDLE QML ERROR"));
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        m_proc.kill();
        setState(EngineShuttingDown, true);
        plugin()->showMessageBox(QMessageBox::Critical, tr("Gdb I/O Error"),
                       errorMessage(error));
        break;
    }
}

void QmlEngine::handleProcFinished(int code, QProcess::ExitStatus type)
{
    showMessage(_("QML VIEWER PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    setState(DebuggerNotReady, true);
}

void QmlEngine::readProcStandardError()
{
    qDebug() << "STD ERR" << m_proc.readAllStandardError();
    if (!m_conn)
        setupConnection();
}

void QmlEngine::readProcStandardOutput()
{
    qDebug() << "STD ERR" << m_proc.readAllStandardOutput();
}

void QmlEngine::connectionStateChanged()
{
    QTC_ASSERT(m_conn, return);
    QAbstractSocket::SocketState state = m_conn->state();
    qDebug() << "CONNECTION STATE: " << state;
    switch (state) {
        case QAbstractSocket::UnconnectedState:
        {
            showStatusMessage(tr("[QmlEngine] disconnected.\n\n"));

            delete m_engineQuery;
            m_engineQuery = 0;
            delete m_contextQuery;
            m_contextQuery = 0;

//            resetViews();

//            updateMenuActions();

            break;
        }
        case QAbstractSocket::HostLookupState:
            showStatusMessage(tr("[QmlEngine] resolving host..."));
            break;
        case QAbstractSocket::ConnectingState:
            showStatusMessage(tr("[QmlEngine] connecting to debug server..."));
            break;
        case QAbstractSocket::ConnectedState:
            showStatusMessage(tr("[QmlEngine] connected.\n"));
            //setupConnection()
            break;
        case QAbstractSocket::ClosingState:
            showStatusMessage(tr("[QmlEngine] closing..."));
            break;
        case QAbstractSocket::BoundState:
            showStatusMessage(tr("[QmlEngine] bound state"));
            break;
        case QAbstractSocket::ListeningState:
            showStatusMessage(tr("[QmlEngine] listening state"));
            break;
        default:
            showStatusMessage(tr("[QmlEngine] unknown state: %1").arg(state));
            break;
    }
}

void QmlEngine::connectionError()
{
    QTC_ASSERT(m_conn, return);
    showStatusMessage(tr("[QmlEngine] error: (%1) %2", "%1=error code, %2=error message")
            .arg(m_conn->error()).arg(m_conn->errorString()));
}

void QmlEngine::connectionConnected()
{
    QTC_ASSERT(m_conn, return);
    showStatusMessage(tr("[QmlEngine] error: (%1) %2", "%1=error code, %2=error message")
            .arg(m_conn->error()).arg(m_conn->errorString()));
}


} // namespace Internal
} // namespace Debugger

#include "qmlengine.moc"
