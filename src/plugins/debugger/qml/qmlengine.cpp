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
        : QDeclarativeDebugClient(QLatin1String("QDeclarativeEngine"), connection)
        , m_connection(connection), m_engine(engine)
    {
        setEnabled(true);
    }

    void sendMessage(const QByteArray &msg)
    {
        QTC_ASSERT(isConnected(), /**/);
        qDebug() << "SENDING: " << quoteUnprintableLatin1(msg);
        QDeclarativeDebugClient::sendMessage(msg);
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
    m_engineDebugInterface = 0;
    m_frameRate = 0;

    /*
    m_watchTableModel = new Internal::WatchTableModel(0, this);

    m_objectTreeWidget = new Internal::ObjectTree;
    m_propertiesWidget = new Internal::ObjectPropertiesView(m_watchTableModel);
    m_watchTableView = new Internal::WatchTableView(m_watchTableModel);
    m_expressionWidget = new Internal::ExpressionQueryWidget(Internal::ExpressionQueryWidget::SeparateEntryMode);
//    m_frameRateWidget = new Internal::CanvasFrameRate;
//    m_frameRateWidget->setObjectName(QLatin1String("QmlDebugFrameRate"));

    connect(Debugger::DebuggerPlugin::instance(),
        SIGNAL(stateChanged(int)), this, SLOT(debuggerStateChanged(int)));

    m_editablePropertyTypes = QStringList() << "qreal" << "bool" << "QString"
                                            << "int" << "QVariant" << "QUrl" << "QColor";

    connect(m_connectionTimer, SIGNAL(timeout()), SLOT(pollInspector()));
    */
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

    //m_objectTreeWidget->saveSettings(m_settings);
    //m_propertiesWidget->saveSettings(m_settings);
    //m_settings.saveSettings(Core::ICore::instance()->settings());
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
        setState(EngineStartFailed);
        startFailed();
        return;
    }
    setState(EngineStarted);
    startSuccessful();
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


    QTC_ASSERT(m_engineDebugInterface == 0, /**/);
    m_engineDebugInterface = new QDeclarativeEngineDebug(m_conn, this);

    //m_objectTreeWidget->setEngineDebug(m_engineDebugInterface);
    //m_propertiesWidget->setEngineDebug(m_engineDebugInterface);
    //m_watchTableModel->setEngineDebug(m_engineDebugInterface);
    //m_expressionWidget->setEngineDebug(m_engineDebugInterface);
    //resetViews();
    //m_frameRateWidget->reset(m_conn);

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
    setState(InferiorRunningRequested);
    setState(InferiorRunning);

    reloadEngines();
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
    qDebug() << "UPDATE LOCALS";
}

void QmlEngine::updateWatchData(const WatchData &data)
{
    qDebug() << "UPDATE WATCH DATA" << data.toString();
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

    showMessage(_("RECEIVED RESPONSE: ") + quoteUnprintableLatin1(message));
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

        foreach (QDeclarativeDebugWatch *watch, m_watches) {
            qDebug() << "WATCH"
                << watch->objectName()
                << watch->queryId()
                << watch->state()
                << watch->objectDebugId();
        }

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
    QString msg = QString::fromUtf8(m_proc.readAllStandardError());
    if (!m_conn)
        setupConnection();
    qDebug() << "STD ERR" << msg;
    showMessage(msg, AppError);
}

void QmlEngine::readProcStandardOutput()
{
    QString msg = QString::fromUtf8(m_proc.readAllStandardOutput());
    qDebug() << "STD OUT" << msg;
    showMessage(msg, AppOutput);
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


#if 0
class EngineComboBox : public QComboBox
{
    Q_OBJECT
public:
    struct EngineInfo
    {
        QString name;
        int id;
    };

    EngineComboBox(QWidget *parent = 0);

    void addEngine(int engine, const QString &name);
    void clearEngines();

protected:

private:
    QList<EngineInfo> m_engines;
};

EngineComboBox::EngineComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setEnabled(false);
    setEditable(false);
}

void EngineComboBox::addEngine(int engine, const QString &name)
{
    EngineInfo info;
    info.id = engine;
    if (name.isEmpty())
        info.name = tr("Engine %1", "engine number").arg(engine);
    else
        info.name = name;
    m_engines << info;

    addItem(info.name);
}

void EngineComboBox::clearEngines()
{
    m_engines.clear();
    clear();
}

} // Internal


bool QmlEngine::setDebugConfigurationDataFromProject(ProjectExplorer::Project *projectToDebug)
{
    if (!projectToDebug) {
        emit statusMessage(tr("Invalid project, debugging canceled."));
        return false;
    }

    QmlProjectManager::QmlProjectRunConfiguration* config =
            qobject_cast<QmlProjectManager::QmlProjectRunConfiguration*>(projectToDebug->activeTarget()->activeRunConfiguration());
    if (!config) {
        emit statusMessage(tr("Cannot find project run configuration, debugging canceled."));
        return false;
    }
    m_runConfigurationDebugData.serverAddress = config->debugServerAddress();
    m_runConfigurationDebugData.serverPort = config->debugServerPort();
    m_connectionTimer->setInterval(ConnectionAttemptDefaultInterval);

    return true;
}

void QmlEngine::startQmlProjectDebugger()
{
    m_simultaneousCppAndQmlDebugMode = false;
    m_connectionTimer->start();
}

bool QmlEngine::connectToViewer()
{
    if (m_conn && m_conn->state() != QAbstractSocket::UnconnectedState)
        return false;

    delete m_engineDebugInterface; m_engineDebugInterface = 0;

    if (m_conn) {
        m_conn->disconnectFromHost();
        delete m_conn;
        m_conn = 0;
    }

    QString host = m_runConfigurationDebugData.serverAddress;
    quint16 port = quint16(m_runConfigurationDebugData.serverPort);

    m_conn = new QDeclarativeDebugConnection(this);
    connect(m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionError()));

    emit statusMessage(tr("[Inspector] set to connect to debug server %1:%2").arg(host).arg(port));
    m_conn->connectToHost(host, port);
    // blocks until connected; if no connection is available, will fail immediately

    if (!m_conn->waitForConnected())
        return false;

    QTC_ASSERT(m_debuggerRunControl, return false);
    QmlEngine *engine = qobject_cast<QmlEngine *>(m_debuggerRunControl->engine());
    QTC_ASSERT(engine, return false);

    (void) new DebuggerClient(m_conn, engine);

    return true;
}

void QmlEngine::disconnectFromViewer()
{
    m_conn->disconnectFromHost();
    updateMenuActions();
}

void QmlEngine::connectionStateChanged()
{
    switch (m_conn->state()) {
        case QAbstractSocket::UnconnectedState:
        {
            emit statusMessage(tr("[Inspector] disconnected.\n\n"));
            resetViews();
            updateMenuActions();
            break;
        }
        case QAbstractSocket::HostLookupState:
            emit statusMessage(tr("[Inspector] resolving host..."));
            break;
        case QAbstractSocket::ConnectingState:
            emit statusMessage(tr("[Inspector] connecting to debug server..."));
            break;
        case QAbstractSocket::ConnectedState:
        {
            emit statusMessage(tr("[Inspector] connected.\n"));

            resetViews();
//            m_frameRateWidget->reset(m_conn);

            break;
        }
        case QAbstractSocket::ClosingState:
            emit statusMessage(tr("[Inspector] closing..."));
            break;
        case QAbstractSocket::BoundState:
        case QAbstractSocket::ListeningState:
            break;
    }
}

void QmlEngine::resetViews()
{
    m_objectTreeWidget->cleanup();
    m_propertiesWidget->clear();
    m_expressionWidget->clear();
    m_watchTableModel->removeAllWatches();
}

void QmlEngine::createDockWidgets()
{

    m_engineComboBox = new Internal::EngineComboBox;
    m_engineComboBox->setEnabled(false);
    connect(m_engineComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(queryEngineContext(int)));

    // FancyMainWindow uses widgets' window titles for tab labels
//    m_frameRateWidget->setWindowTitle(tr("Frame rate"));

    Utils::StyledBar *treeOptionBar = new Utils::StyledBar;
    QHBoxLayout *treeOptionBarLayout = new QHBoxLayout(treeOptionBar);
    treeOptionBarLayout->setContentsMargins(5, 0, 5, 0);
    treeOptionBarLayout->setSpacing(5);
    treeOptionBarLayout->addWidget(new QLabel(tr("QML engine:")));
    treeOptionBarLayout->addWidget(m_engineComboBox);

    QWidget *treeWindow = new QWidget;
    treeWindow->setObjectName(QLatin1String("QmlDebugTree"));
    treeWindow->setWindowTitle(tr("Object Tree"));
    QVBoxLayout *treeWindowLayout = new QVBoxLayout(treeWindow);
    treeWindowLayout->setMargin(0);
    treeWindowLayout->setSpacing(0);
    treeWindowLayout->setContentsMargins(0,0,0,0);
    treeWindowLayout->addWidget(treeOptionBar);
    treeWindowLayout->addWidget(m_objectTreeWidget);


    m_watchTableView->setModel(m_watchTableModel);
    Internal::WatchTableHeaderView *header = new Internal::WatchTableHeaderView(m_watchTableModel);
    m_watchTableView->setHorizontalHeader(header);

    connect(m_objectTreeWidget, SIGNAL(activated(QDeclarativeDebugObjectReference)),
            this, SLOT(treeObjectActivated(QDeclarativeDebugObjectReference)));

    connect(m_objectTreeWidget, SIGNAL(currentObjectChanged(QDeclarativeDebugObjectReference)),
            m_propertiesWidget, SLOT(reload(QDeclarativeDebugObjectReference)));

    connect(m_objectTreeWidget, SIGNAL(expressionWatchRequested(QDeclarativeDebugObjectReference,QString)),
            m_watchTableModel, SLOT(expressionWatchRequested(QDeclarativeDebugObjectReference,QString)));

    connect(m_propertiesWidget, SIGNAL(watchToggleRequested(QDeclarativeDebugObjectReference,QDeclarativeDebugPropertyReference)),
            m_watchTableModel, SLOT(togglePropertyWatch(QDeclarativeDebugObjectReference,QDeclarativeDebugPropertyReference)));

    connect(m_watchTableModel, SIGNAL(watchCreated(QDeclarativeDebugWatch*)),
            m_propertiesWidget, SLOT(watchCreated(QDeclarativeDebugWatch*)));

    connect(m_watchTableModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            m_watchTableView, SLOT(scrollToBottom()));

    connect(m_watchTableView, SIGNAL(objectActivated(int)),
            m_objectTreeWidget, SLOT(setCurrentObject(int)));

    connect(m_objectTreeWidget, SIGNAL(currentObjectChanged(QDeclarativeDebugObjectReference)),
            m_expressionWidget, SLOT(setCurrentObject(QDeclarativeDebugObjectReference)));


    Core::MiniSplitter *propSplitter = new Core::MiniSplitter(Qt::Horizontal);
    Core::MiniSplitter *propWatcherSplitter = new Core::MiniSplitter(Qt::Vertical);
    propWatcherSplitter->addWidget(m_propertiesWidget);
    propWatcherSplitter->addWidget(m_watchTableView);
    propWatcherSplitter->setStretchFactor(0, 2);
    propWatcherSplitter->setStretchFactor(1, 1);
    propWatcherSplitter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);

    propSplitter->setWindowTitle(tr("Properties and Watchers"));
    propSplitter->setObjectName(QLatin1String("QmlDebugProperties"));
    propSplitter->addWidget(m_objectTreeWidget);
    propSplitter->addWidget(propWatcherSplitter);
    propSplitter->setStretchFactor(0, 1);
    propSplitter->setStretchFactor(1, 3);

    InspectorOutputWidget *inspectorOutput = new InspectorOutputWidget();
    inspectorOutput->setObjectName(QLatin1String("QmlDebugInspectorOutput"));
    connect(this, SIGNAL(statusMessage(QString)),
            inspectorOutput, SLOT(addInspectorStatus(QString)));

    Debugger::DebuggerUISwitcher *uiSwitcher = Debugger::DebuggerUISwitcher::instance();

    m_watchTableView->hide();
//    m_objectTreeDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
//                                                            treeWindow, Qt::BottomDockWidgetArea);
//    m_frameRateDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
//                                                            m_frameRateWidget, Qt::BottomDockWidgetArea);
    m_propertyWatcherDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
                                                            propSplitter, Qt::BottomDockWidgetArea);
    m_inspectorOutputDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
                                                            inspectorOutput, Qt::BottomDockWidgetArea);

    m_expressionWidget->setWindowTitle(tr("Script Console"));
    m_expressionQueryDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
                                                         m_expressionWidget, Qt::BottomDockWidgetArea);

    m_inspectorOutputDock->setToolTip(tr("Output of the QML inspector, such as information on connecting to the server."));

    m_dockWidgets << /*m_objectTreeDock << *//*m_frameRateDock << */ m_propertyWatcherDock
                  << m_inspectorOutputDock << m_expressionQueryDock;

    m_context = new Internal::InspectorContext(m_objectTreeWidget);
    m_propWatcherContext = new Internal::InspectorContext(m_propertyWatcherDock);

    Core::ICore *core = Core::ICore::instance();
    core->addContextObject(m_propWatcherContext);
    core->addContextObject(m_context);

    m_simultaneousDebugAction = new QAction(this);
    m_simultaneousDebugAction->setText(tr("Start Debugging C++ and QML Simultaneously..."));
    connect(m_simultaneousDebugAction, SIGNAL(triggered()),
        this, SLOT(simultaneouslyDebugQmlCppApplication()));

    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *mstart = am->actionContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING);
    Core::Command *cmd = am->registerAction(m_simultaneousDebugAction, Constants::M_DEBUG_SIMULTANEOUSLY,
                                            m_context->context());
    cmd->setAttribute(Core::Command::CA_Hide);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    m_settings.readSettings(core->settings());
    m_objectTreeWidget->readSettings(m_settings);
    m_propertiesWidget->readSettings(m_settings);

    connect(m_objectTreeWidget, SIGNAL(contextHelpIdChanged(QString)), m_context,
            SLOT(setContextHelpId(QString)));
    connect(m_watchTableView, SIGNAL(contextHelpIdChanged(QString)), m_propWatcherContext,
            SLOT(setContextHelpId(QString)));
    connect(m_propertiesWidget, SIGNAL(contextHelpIdChanged(QString)), m_propWatcherContext,
            SLOT(setContextHelpId(QString)));
    connect(m_expressionWidget, SIGNAL(contextHelpIdChanged(QString)), m_propWatcherContext,
            SLOT(setContextHelpId(QString)));
}

void QmlEngine::simultaneouslyDebugQmlCppApplication()
{
    QString errorMessage;
    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *project = pex->startupProject();

    if (!project)
         errorMessage = QString(tr("No project was found."));
    else {
        if (project->id() == "QmlProjectManager.QmlProject")
            errorMessage = attachToQmlViewerAsExternalApp(project);
        else {
            errorMessage = attachToExternalCppAppWithQml(project);
        }
    }

    if (!errorMessage.isEmpty())
        QMessageBox::warning(Core::ICore::instance()->mainWindow(), "Failed to debug C++ and QML", errorMessage);
}

QString QmlEngine::attachToQmlViewerAsExternalApp(ProjectExplorer::Project *project)
{
    m_debugMode = QmlProjectWithCppPlugins;

    QmlProjectManager::QmlProjectRunConfiguration* runConfig =
                qobject_cast<QmlProjectManager::QmlProjectRunConfiguration*>(project->activeTarget()->activeRunConfiguration());

    if (!runConfig)
        return QString(tr("No run configurations were found for the project '%1'.").arg(project->displayName()));

    Internal::StartExternalQmlDialog dlg(Debugger::DebuggerUISwitcher::instance()->mainWindow());

    QString importPathArgument = "-I";
    QString execArgs;
    if (runConfig->viewerArguments().contains(importPathArgument))
        execArgs = runConfig->viewerArguments().join(" ");
    else {
        QFileInfo qmlFileInfo(runConfig->viewerArguments().last());
        importPathArgument.append(" " + qmlFileInfo.absolutePath() + " ");
        execArgs = importPathArgument + runConfig->viewerArguments().join(" ");
    }


    dlg.setPort(runConfig->debugServerPort());
    dlg.setDebuggerUrl(runConfig->debugServerAddress());
    dlg.setProjectDisplayName(project->displayName());
    dlg.setDebugMode(Internal::StartExternalQmlDialog::QmlProjectWithCppPlugins);
    dlg.setQmlViewerArguments(execArgs);
    dlg.setQmlViewerPath(runConfig->viewerPath());

    if (dlg.exec() != QDialog::Accepted)
        return QString();

    m_runConfigurationDebugData.serverAddress = dlg.debuggerUrl();
    m_runConfigurationDebugData.serverPort = dlg.port();
    m_settings.setExternalPort(dlg.port());
    m_settings.setExternalUrl(dlg.debuggerUrl());

    ProjectExplorer::Environment customEnv = ProjectExplorer::Environment::systemEnvironment(); // empty env by default
    customEnv.set(QmlProjectManager::Constants::E_QML_DEBUG_SERVER_PORT, QString::number(m_settings.externalPort()));

    Debugger::DebuggerRunControl *debuggableRunControl =
     createDebuggerRunControl(runConfig, dlg.qmlViewerPath(), dlg.qmlViewerArguments());

    return executeDebuggerRunControl(debuggableRunControl, &customEnv);
}

QString QmlEngine::attachToExternalCppAppWithQml(ProjectExplorer::Project *project)
{
    m_debugMode = CppProjectWithQmlEngines;

    ProjectExplorer::LocalApplicationRunConfiguration* runConfig =
                qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration*>(project->activeTarget()->activeRunConfiguration());

    if (!project->activeTarget() || !project->activeTarget()->activeRunConfiguration())
        return QString(tr("No run configurations were found for the project '%1'.").arg(project->displayName()));
    else if (!runConfig)
        return QString(tr("No valid run configuration was found for the project %1. "
                                  "Only locally runnable configurations are supported.\n"
                                  "Please check your project settings.").arg(project->displayName()));

    Internal::StartExternalQmlDialog dlg(Debugger::DebuggerUISwitcher::instance()->mainWindow());

    dlg.setPort(m_settings.externalPort());
    dlg.setDebuggerUrl(m_settings.externalUrl());
    dlg.setProjectDisplayName(project->displayName());
    dlg.setDebugMode(Internal::StartExternalQmlDialog::CppProjectWithQmlEngine);
    if (dlg.exec() != QDialog::Accepted)
        return QString();

    m_runConfigurationDebugData.serverAddress = dlg.debuggerUrl();
    m_runConfigurationDebugData.serverPort = dlg.port();
    m_settings.setExternalPort(dlg.port());
    m_settings.setExternalUrl(dlg.debuggerUrl());

    ProjectExplorer::Environment customEnv = runConfig->environment();
    customEnv.set(QmlProjectManager::Constants::E_QML_DEBUG_SERVER_PORT, QString::number(m_settings.externalPort()));
    Debugger::DebuggerRunControl *debuggableRunControl = createDebuggerRunControl(runConfig);
    return executeDebuggerRunControl(debuggableRunControl, &customEnv);
}

QString QmlEngine::executeDebuggerRunControl(Debugger::DebuggerRunControl *debuggableRunControl, ProjectExplorer::Environment *environment)
{
    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();

    // to make sure we have a valid, debuggable run control, find the correct factory for it
    if (debuggableRunControl) {

        // modify the env
        debuggableRunControl->setCustomEnvironment(*environment);

        pex->startRunControl(debuggableRunControl, ProjectExplorer::Constants::DEBUGMODE);
        m_simultaneousCppAndQmlDebugMode = true;

        return QString();
    }
    return QString(tr("A valid run control was not registered in Qt Creator for this project run configuration."));;
}

Debugger::DebuggerRunControl *QmlEngine::createDebuggerRunControl(ProjectExplorer::RunConfiguration *runConfig,
                                                                     const QString &executableFile, const QString &executableArguments)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    const QList<Debugger::DebuggerRunControlFactory *> factories = pm->getObjects<Debugger::DebuggerRunControlFactory>();
    ProjectExplorer::RunControl *runControl = 0;

    if (m_debugMode == QmlProjectWithCppPlugins) {
        Debugger::DebuggerStartParameters sp;
        sp.startMode = Debugger::StartExternal;
        sp.executable = executableFile;
        sp.processArgs = executableArguments.split(QLatin1Char(' '));
        runControl = factories.first()->create(sp);
        return qobject_cast<Debugger::DebuggerRunControl *>(runControl);
    }

    if (m_debugMode == CppProjectWithQmlEngines) {
        if (factories.length() && factories.first()->canRun(runConfig, ProjectExplorer::Constants::DEBUGMODE)) {
            runControl = factories.first()->create(runConfig, ProjectExplorer::Constants::DEBUGMODE);
            return qobject_cast<Debugger::DebuggerRunControl *>(runControl);
        }
    }

    return 0;
}

void QmlEngine::updateMenuActions()
{

    bool enabled = true;
    if (m_simultaneousCppAndQmlDebugMode)
        enabled = (m_cppDebuggerState == Debugger::DebuggerNotReady && (!m_conn || m_conn->state() == QAbstractSocket::UnconnectedState));
    else
        enabled = (!m_conn || m_conn->state() == QAbstractSocket::UnconnectedState);

    m_simultaneousDebugAction->setEnabled(enabled);
}


void QmlEngine::debuggerStateChanged(int newState)
{
    if (m_simultaneousCppAndQmlDebugMode) {

        switch(newState) {
        case Debugger::EngineStarting:
            {
                m_connectionInitialized = false;
                break;
            }
        case Debugger::AdapterStartFailed:
        case Debugger::InferiorStartFailed:
            emit statusMessage(QString(tr("Debugging failed: could not start C++ debugger.")));
            break;
        case Debugger::InferiorRunningRequested:
            {
                if (m_cppDebuggerState == Debugger::InferiorStopped) {
                    // re-enable UI again
                    m_objectTreeWidget->setEnabled(true);
                    m_propertiesWidget->setEnabled(true);
                    m_expressionWidget->setEnabled(true);
                }
                break;
            }
        case Debugger::InferiorRunning:
            {
                if (!m_connectionInitialized) {
                    m_connectionInitialized = true;
                    m_connectionTimer->setInterval(ConnectionAttemptSimultaneousInterval);
                    m_connectionTimer->start();
                }
                break;
            }
        case Debugger::InferiorStopped:
            {
                m_objectTreeWidget->setEnabled(false);
                m_propertiesWidget->setEnabled(false);
                m_expressionWidget->setEnabled(false);
                break;
            }
        case Debugger::EngineShuttingDown:
            {
                m_connectionInitialized = false;
                // here it's safe to enable the debugger windows again -
                // disabled ones look ugly.
                m_objectTreeWidget->setEnabled(true);
                m_propertiesWidget->setEnabled(true);
                m_expressionWidget->setEnabled(true);
                m_simultaneousCppAndQmlDebugMode = false;
                break;
            }
        default:
            break;
        }
    }

    m_cppDebuggerState = newState;
    updateMenuActions();
}


void QmlEngine::setSimpleDockWidgetArrangement()
{
    Utils::FancyMainWindow *mainWindow = Debugger::DebuggerUISwitcher::instance()->mainWindow();

    mainWindow->setTrackingEnabled(false);
    QList<QDockWidget *> dockWidgets = mainWindow->dockWidgets();
    foreach (QDockWidget *dockWidget, dockWidgets) {
        if (m_dockWidgets.contains(dockWidget)) {
            dockWidget->setFloating(false);
            mainWindow->removeDockWidget(dockWidget);
        }
    }

    foreach (QDockWidget *dockWidget, dockWidgets) {
        if (m_dockWidgets.contains(dockWidget)) {
            mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
            dockWidget->show();
        }
    }
    mainWindow->splitDockWidget(mainWindow->toolBarDockWidget(), m_propertyWatcherDock, Qt::Vertical);
    //mainWindow->tabifyDockWidget(m_frameRateDock, m_propertyWatcherDock);
    mainWindow->tabifyDockWidget(m_propertyWatcherDock, m_expressionQueryDock);
    mainWindow->tabifyDockWidget(m_propertyWatcherDock, m_inspectorOutputDock);
    m_propertyWatcherDock->raise();

    m_inspectorOutputDock->setVisible(false);

    mainWindow->setTrackingEnabled(true);
}
#endif

void QmlEngine::reloadEngines()
{
    //m_engineComboBox->setEnabled(false);

    QDeclarativeDebugEnginesQuery *query =
           m_engineDebugInterface->queryAvailableEngines(this);
    if (!query->isWaiting())
        enginesChanged(query);
    else
        QObject::connect(query, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(enginesChanged()));
}

void QmlEngine::enginesChanged()
{
    enginesChanged(qobject_cast<QDeclarativeDebugEnginesQuery *>(sender()));
}

void QmlEngine::enginesChanged(QDeclarativeDebugEnginesQuery *query)
{
    //m_engineComboBox->clearEngines();
    QList<QDeclarativeDebugEngineReference> engines = query->engines();
    if (engines.isEmpty())
        qWarning("QMLDEBUGGER: NO ENGINES FOUND!");

    //m_engineComboBox->setEnabled(true);

    for (int i = 0; i < engines.count(); ++i)
        qDebug() << "ENGINE: "  <<  engines.at(i).debugId() << engines.at(i).name();
    //    m_engineComboBox->addEngine(engines.at(i).debugId(), engines.at(i).name());

    if (engines.count() > 0) {
    //    m_engineComboBox->setCurrentIndex(engines.at(0).debugId());
        queryEngineContext(engines.at(0));
    }
}

void QmlEngine::queryEngineContext(const QDeclarativeDebugEngineReference &engine)
{
    QDeclarativeDebugRootContextQuery *query =
        m_engineDebugInterface->queryRootContexts(engine, this);

    if (!query->isWaiting())
        contextChanged();
    else
        QObject::connect(query, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(contextChanged()));
}

void QmlEngine::contextChanged()
{
    contextChanged(qobject_cast<QDeclarativeDebugRootContextQuery *>(sender()));
}

void QmlEngine::contextChanged(QDeclarativeDebugRootContextQuery *query)
{
    QTC_ASSERT(query, return);
    //dump(query->rootContext(), 0);
    foreach (const QDeclarativeDebugObjectReference &object, query->rootContext().objects())
        reloadObject(object);
}

void QmlEngine::reloadObject(const QDeclarativeDebugObjectReference &object)
{
    qDebug() << "RELOAD OBJECT: " << object.debugId() << object.idString()
            << object.className();
    QDeclarativeDebugObjectQuery *query =
        m_engineDebugInterface->queryObjectRecursive(object, this);
    if (!query->isWaiting())
        objectFetched(query, QDeclarativeDebugQuery::Completed);
    else
        QObject::connect(query, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(objectFetched(QDeclarativeDebugQuery::State)));
}

void QmlEngine::objectFetched(QDeclarativeDebugQuery::State state)
{
    objectFetched(qobject_cast<QDeclarativeDebugObjectQuery *>(sender()), state);
}

void QmlEngine::objectFetched(QDeclarativeDebugObjectQuery *query,
    QDeclarativeDebugQuery::State state)
{
    QTC_ASSERT(query, return);
    QTC_ASSERT(state == QDeclarativeDebugQuery::Completed, return);
    //dump(m_query->object(), 0);

    m_watches.clear();
    buildTree(query->object(), "local");

    qDebug() << "WATCHES CREATED: " << m_watches.size();
    //watchHandler()->beginCycle();
    //watchHandler()->insertBulkData(list);
    //watchHandler()->endCycle();
    //setCurrentItem(topLevelItem(0));

    // this ugly hack is needed if user wants to see internal structs
    // on startup - debugger does not load them until towards the end,
    // so essentially loading twice gives us the full list as everything
    // is already loaded.
    //if (m_showUninspectableItems && !m_showUninspectableOnInitDone) {
    //    m_showUninspectableOnInitDone = true;
    //    reloadObject(m_currentObjectDebugId);
    //}
}

void QmlEngine::buildTree(const QDeclarativeDebugObjectReference &obj,
    const QByteArray &iname)
{
    //QTC_ASSERT(obj.contextDebugId() >= 0, return);
    WatchData data;
    data.iname = iname;

    if (obj.idString().isEmpty())
        data.name = QString("<%1>").arg(obj.className());
    else
        data.name = obj.idString();

    data.value = "?";
    data.type = "?";
    data.setHasChildren(!obj.children().isEmpty());
    data.setAllUnneeded();
    qDebug() << "CREATED ITEM " << data.iname << data.name;
    m_watches.append(m_engineDebugInterface->addWatch(obj, data.name, 0));
    //QDeclarativeDebugPropertyWatch *QDeclarativeEngineDebug::addWatch(const QDeclarativeDebugPropertyReference &property, QObject *parent)

    //data.userRole = qVariantFromValue(obj);
    /*
    if (parent && obj.contextDebugId() >= 0
            && obj.contextDebugId() != parent->data(0, Qt::UserRole
                    ).value<QDeclarativeDebugObjectReference>().contextDebugId())
    {

        QDeclarativeDebugFileReference source = obj.source();
        if (!source.url().isEmpty()) {
            QString toolTipString = QLatin1String("URL: ") + source.url().toString();
            item->setToolTip(0, toolTipString);
        }

    } else {
        item->setExpanded(true);
    }

    if (obj.contextDebugId() < 0)
        item->setHasValidDebugId(false);
*/

    for (int i = 0; i < obj.children().size(); ++i)
        buildTree(obj.children().at(i), iname + '.' + QByteArray::number(i));
}

#if 0
void QmlEngine::treeObjectActivated(const QDeclarativeDebugObjectReference &obj)
{
    QDeclarativeDebugFileReference source = obj.source();
    QString fileName = source.url().toLocalFile();

    if (source.lineNumber() < 0 || !QFile::exists(fileName))
        return;

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName, QString(), Core::EditorManager::NoModeSwitch);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        editorManager->addCurrentPositionToNavigationHistory();
        textEditor->gotoLine(source.lineNumber());
        textEditor->widget()->setFocus();
    }
}

bool QmlEngine::canEditProperty(const QString &propertyType)
{
    return m_editablePropertyTypes.contains(propertyType);
}

QDeclarativeDebugExpressionQuery *QmlEngine::executeExpression(int objectDebugId, const QString &objectId,
                                                                  const QString &propertyName, const QVariant &value)
{
    //qDebug() << entity.property << entity.title << entity.objectId;
    if (objectId.length()) {

        QString quoteWrappedValue = value.toString();
        if (addQuotesForData(value))
            quoteWrappedValue = QString("'%1'").arg(quoteWrappedValue);

        QString constructedExpression = objectId + "." + propertyName + "=" + quoteWrappedValue;
        //qDebug() << "EXPRESSION:" << constructedExpression;
        return m_client->queryExpressionResult(objectDebugId, constructedExpression, this);
    }

    return 0;
}

bool QmlEngine::addQuotesForData(const QVariant &value) const
{
    switch (value.type()) {
    case QVariant::String:
    case QVariant::Color:
    case QVariant::Date:
        return true;
    default:
        break;
    }

    return false;
}

ObjectTree::ObjectTree(QDeclarativeEngineDebug *client, QWidget *parent)
    : QTreeWidget(parent),
      m_client(client),
      m_query(0), m_clickedItem(0), m_showUninspectableItems(false),
      m_currentObjectDebugId(0), m_showUninspectableOnInitDone(false)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);
    setExpandsOnDoubleClick(false);

    m_addWatchAction = new QAction(tr("Add watch expression..."), this);
    m_toggleUninspectableItemsAction = new QAction(tr("Show uninspectable items"), this);
    m_toggleUninspectableItemsAction->setCheckable(true);
    m_goToFileAction = new QAction(tr("Go to file"), this);
    connect(m_toggleUninspectableItemsAction, SIGNAL(triggered()), SLOT(toggleUninspectableItems()));
    connect(m_addWatchAction, SIGNAL(triggered()), SLOT(addWatch()));
    connect(m_goToFileAction, SIGNAL(triggered()), SLOT(goToFile()));

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            SLOT(currentItemChanged(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
            SLOT(activated(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
}

void ObjectTree::readSettings(const InspectorSettings &settings)
{
    if (settings.showUninspectableItems() != m_showUninspectableItems)
        toggleUninspectableItems();
}
void ObjectTree::saveSettings(InspectorSettings &settings)
{
    settings.setShowUninspectableItems(m_showUninspectableItems);
}

void ObjectTree::setEngineDebug(QDeclarativeEngineDebug *client)
{
    m_client = client;
}

void ObjectTree::toggleUninspectableItems()
{
    m_showUninspectableItems = !m_showUninspectableItems;
    m_toggleUninspectableItemsAction->setChecked(m_showUninspectableItems);
    reload(m_currentObjectDebugId);
}

void ObjectTree::selectionChanged()
{
    if (selectedItems().isEmpty())
        return;

    QTreeWidgetItem *item = selectedItems().first();
    if (item)
        emit contextHelpIdChanged(InspectorContext::contextHelpIdForItem(item->text(0)));
}


void ObjectTree::setCurrentObject(int debugId)
{
    QTreeWidgetItem *item = findItemByObjectId(debugId);
    if (item) {
        setCurrentItem(item);
        scrollToItem(item);
        item->setExpanded(true);
    }


}

{
    if (!item)
        return;

    QDeclarativeDebugObjectReference obj = item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();
    if (obj.debugId() >= 0)
        emit currentObjectChanged(obj);
}

void ObjectTree::activated(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QDeclarativeDebugObjectReference obj = item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();
    if (obj.debugId() >= 0)
        emit activated(obj);
}

void ObjectTree::cleanup()
{
    m_showUninspectableOnInitDone = false;
    clear();
}

void ObjectTree::dump(const QDeclarativeDebugContextReference &ctxt, int ind)
{
    QByteArray indent(ind * 4, ' ');
    qWarning().nospace() << indent.constData() << ctxt.debugId() << " "
                         << qPrintable(ctxt.name());

    for (int ii = 0; ii < ctxt.contexts().count(); ++ii)
        dump(ctxt.contexts().at(ii), ind + 1);

    for (int ii = 0; ii < ctxt.objects().count(); ++ii)
        dump(ctxt.objects().at(ii), ind);
}

void ObjectTree::dump(const QDeclarativeDebugObjectReference &obj, int ind)
{
    QByteArray indent(ind * 4, ' ');
    qWarning().nospace() << indent.constData() << qPrintable(obj.className())
                         << " " << qPrintable(obj.idString()) << " "
                         << obj.debugId();

    for (int ii = 0; ii < obj.children().count(); ++ii)
        dump(obj.children().at(ii), ind + 1);
}

QTreeWidgetItem *ObjectTree::findItemByObjectId(int debugId) const
{
    for (int i=0; i<topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = findItem(topLevelItem(i), debugId);
        if (item)
            return item;
    }

    return 0;
}

QTreeWidgetItem *ObjectTree::findItem(QTreeWidgetItem *item, int debugId) const
{
    if (item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>().debugId() == debugId)
        return item;

    QTreeWidgetItem *child;
    for (int i=0; i<item->childCount(); ++i) {
        child = findItem(item->child(i), debugId);
        if (child)
            return child;
    }

    return 0;
}

void ObjectTree::addWatch()
{
    QDeclarativeDebugObjectReference obj =
            currentItem()->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();

    bool ok = false;
    QString watch = QInputDialog::getText(this, tr("Watch expression"),
            tr("Expression:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !watch.isEmpty())
        emit expressionWatchRequested(obj, watch);

}

void ObjectTree::goToFile()
{
    QDeclarativeDebugObjectReference obj =
            currentItem()->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();

    if (obj.debugId() >= 0)
        emit activated(obj);
}

void ObjectTree::contextMenuEvent(QContextMenuEvent *event)
{

    m_clickedItem = itemAt(QPoint(event->pos().x(),
                                  event->pos().y() ));
    if (!m_clickedItem)
        return;

    QMenu menu;
    menu.addAction(m_addWatchAction);
    menu.addAction(m_goToFileAction);
    if (m_currentObjectDebugId) {
        menu.addSeparator();
        menu.addAction(m_toggleUninspectableItemsAction);
    }

    menu.exec(event->globalPos());
}

} // Internal
} // Qml
#endif

} // namespace Internal
} // namespace Debugger

#include "qmlengine.moc"
