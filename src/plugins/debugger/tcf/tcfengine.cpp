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

#include "tcfengine.h"

#include "debuggerconstants.h"
#include "debuggerdialogs.h"
#include "debuggerstringutils.h"
#include "json.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
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

#define DEBUG_TCF 1
#if DEBUG_TCF
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

#define CB(callback) &TcfEngine::callback, STRINGIFY(callback)

//#define USE_CONGESTION_CONTROL

static QByteArray C(const QByteArray &ba1,
    const QByteArray &ba2 = QByteArray(),
    const QByteArray &ba3 = QByteArray(),
    const QByteArray &ba4 = QByteArray(),
    const QByteArray &ba5 = QByteArray())
{
    QByteArray result = ba1;
    if (!ba2.isEmpty()) { result += '\0'; result += ba2; }
    if (!ba3.isEmpty()) { result += '\0'; result += ba3; }
    if (!ba4.isEmpty()) { result += '\0'; result += ba4; }
    if (!ba5.isEmpty()) { result += '\0'; result += ba5; }
    return result;
}

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// TcfCommand
//
///////////////////////////////////////////////////////////////////////


QString TcfEngine::TcfCommand::toString() const
{
    return quoteUnprintableLatin1(command);
}


///////////////////////////////////////////////////////////////////////
//
// TcfEngine
//
///////////////////////////////////////////////////////////////////////

TcfEngine::TcfEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("TcfEngine"));
    m_congestion = 0;
    m_inAir = 0;

    m_sendTimer.setSingleShot(true);
    m_sendTimer.setInterval(100); // ms
    connect(&m_sendTimer, SIGNAL(timeout()), this, SLOT(handleSendTimer()));

    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(socketError(QAbstractSocket::SocketError)));

    //void aboutToClose ()
    //void bytesWritten ( qint64 bytes )
    //void readChannelFinished ()
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));

    //connect(m_socket, SIGNAL(hostFound())
    //connect(m_socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy, QAuthenticator *)))
    //connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
    //    thism SLOT(socketStateChanged(QAbstractSocket::SocketState)));
}

TcfEngine::~TcfEngine()
{
}

void TcfEngine::socketReadyRead()
{
    //XSDEBUG("TcfEngine::socketReadyRead()");
    m_inbuffer.append(m_socket->readAll());
    int pos = 0;
    while (1) {
        // the  "\3" is followed by either "\1" or "\2"
        int next = m_inbuffer.indexOf("\3", pos);
        //qDebug() << "pos: " << pos << "next: " << next;
        if (next == -1)
            break;
        handleResponse(m_inbuffer.mid(pos, next - pos));
        pos = next + 2; 
    }
    m_inbuffer.clear();
}

void TcfEngine::socketConnected()
{
    showStatusMessage("Socket connected.");
}

void TcfEngine::socketDisconnected()
{
    showStatusMessage("Socket disconnected.");
    XSDEBUG("FIXME:  TcfEngine::socketDisconnected()");
}

void TcfEngine::socketError(QAbstractSocket::SocketError)
{
    QString msg = tr("%1.").arg(m_socket->errorString());
    //QMessageBox::critical(q->mainWindow(), tr("Error"), msg);
    showStatusMessage(msg);
    notifyEngineIll();
}

void TcfEngine::executeDebuggerCommand(const QString &command)
{
    QByteArray cmd = command.toUtf8();
    cmd = cmd.mid(cmd.indexOf(' ') + 1);
    QByteArray null;
    null.append('\0');
    // FIXME: works for single-digit escapes only
    cmd.replace("\\0", null);
    cmd.replace("\\1", "\1");
    cmd.replace("\\3", "\3");
    TcfCommand tcf;
    tcf.command = cmd;
    enqueueCommand(tcf);
}

void TcfEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showStatusMessage(tr("Running requested..."), 5000);
    const DebuggerStartParameters &sp = startParameters();
    const int pos = sp.remoteChannel.indexOf(QLatin1Char(':'));
    const QString host = sp.remoteChannel.left(pos);
    const quint16 port = sp.remoteChannel.mid(pos + 1).toInt();
    m_socket->connectToHost(host, port);
    if (m_socket->waitForConnected())
        notifyEngineSetupOk();
    else
        notifyEngineSetupFailed();
}

void TcfEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    notifyInferiorSetupOk();
}

void TcfEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    //notifyEngineRunOk(); 
}

void TcfEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    notifyInferiorShutdownOk();
}

void TcfEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_congestion = 0;
    m_inAir = 0;
    m_services.clear();
    notifyEngineShutdownOk();
}

void TcfEngine::continueInferior()
{
    SDEBUG("TcfEngine::continueInferior()");
}

void TcfEngine::interruptInferior()
{
    XSDEBUG("TcfEngine::interruptInferior()");
}

void TcfEngine::executeStep()
{
    //SDEBUG("TcfEngine::executeStep()");
}

void TcfEngine::executeStepI()
{
    //SDEBUG("TcfEngine::executeStepI()");
}

void TcfEngine::executeStepOut()
{
    //SDEBUG("TcfEngine::executeStepOut()");
}

void TcfEngine::executeNext()
{
    //SDEBUG("TcfEngine::nextExec()");
}

void TcfEngine::executeNextI()
{
    //SDEBUG("TcfEngine::executeNextI()");
}

void TcfEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    SDEBUG("FIXME:  TcfEngine::executeRunToLine()");
}

void TcfEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  TcfEngine::executeRunToFunction()");
}

void TcfEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    XSDEBUG("FIXME:  TcfEngine::executeJumpToLine()");
}

void TcfEngine::activateFrame(int index)
{
    Q_UNUSED(index)
}

void TcfEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

void TcfEngine::attemptBreakpointSynchronization()
{
}

void TcfEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void TcfEngine::loadAllSymbols()
{
}

void TcfEngine::reloadModules()
{
}

void TcfEngine::requestModuleSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}


void TcfEngine::handleResponse(const QByteArray &response)
{
    static QTime lastTime;

    //showMessage(_("            "), currentTime(), LogTime);
    QList<QByteArray> parts = response.split('\0');
    if (parts.size() < 2 || !parts.last().isEmpty()) {
        SDEBUG("WRONG RESPONSE PACKET LAYOUT" << parts);
        //if (response.isEmpty())
            acknowledgeResult();
        return;
    }
    parts.removeLast(); // always empty
    QByteArray tag = parts.at(0);
    int n = parts.size();
    if (n == 2 && tag == "N") { // unidentified command
        int token = parts.at(1).toInt();
        TcfCommand tcf = m_cookieForToken[token];
        SDEBUG("COMMAND NOT RECOGNIZED FOR TOKEN" << token << tcf.toString());
        showMessage(QString::number(token) + "^"
               + "NOT RECOQNIZED: " + quoteUnprintableLatin1(response),
                LogOutput);
        acknowledgeResult();
    } else if (n == 2 && tag == "F") { // flow control
        m_congestion = parts.at(1).toInt();
        SDEBUG("CONGESTION: " << m_congestion);
    } else if (n == 4 && tag == "R") { // result data
        acknowledgeResult();
        int token = parts.at(1).toInt();
        QByteArray message = parts.at(2);
        JsonValue data(parts.at(3));
        showMessage(QString("%1^%2%3").arg(token)
            .arg(quoteUnprintableLatin1(response))
            .arg(QString::fromUtf8(data.toString())), LogOutput);
        TcfCommand tcf = m_cookieForToken[token];
        JsonValue result(data);
        SDEBUG("GOOD RESPONSE: " << quoteUnprintableLatin1(response));
        if (tcf.callback)
            (this->*(tcf.callback))(result, tcf.cookie);
    } else if (n == 3 && tag == "P") { // progress data (partial result)
        //int token = parts.at(1).toInt();
        QByteArray data = parts.at(2);
        SDEBUG(_("\nTCF PARTIAL:") << quoteUnprintableLatin1(response));
    } else if (n == 4 && tag == "E") { // an event
        QByteArray service = parts.at(1);
        QByteArray eventName = parts.at(2);
        JsonValue data(parts.at(3));
        if (eventName != "peerHeartBeat")
            SDEBUG(_("\nTCF EVENT:") << quoteUnprintableLatin1(response)
                << data.toString());
        if (service == "Locator" && eventName == "Hello") {
            m_services.clear();
            foreach (const JsonValue &service, data.children())
                m_services.append(service.data());
            QTimer::singleShot(0, this, SLOT(startDebugging()));
        }
    } else {
        SDEBUG("UNKNOWN RESPONSE PACKET:"
            << quoteUnprintableLatin1(response) << parts);
    }
}

void TcfEngine::startDebugging()
{
    //foreach (const QByteArray &service, m_services) {
    //    postCommand(CB(handleRunControlGetChildren),
    //        service, "getChildren", "\"\"");
    //}

    postCommand(C("Diagnostics", "getChildren", "\"\""),
        CB(handleRunControlGetChildren));
    postCommand(C("Streams", "getChildren", "\"\""));
    postCommand(C("Expressions", "getChildren", "\"\""));
    postCommand(C("SysMonitor", "getChildren", "\"\""));
    //postCommand(C("FileSystem", "getChildren", "\"\""));
    //postCommand(C("Processes", "getChildren", "\"\""));
    //postCommand(CB(handleRunControlGetChildren), "LineNumbers", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "Symbols", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "StackTrace", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "Registers", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "Memory", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "Breakpoints", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "RunControl", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "Locator", "getChildren");


    //postCommand(CB(handleRunControlSuspend),
    //    "RunControl", "suspend", "\"Thread1\"");
    //postCommand(CB(handleRunControlSuspend),
    //    "RunControl", "getContext", "\"P12318\"");

    //postCommand(C("Locator", "sync"), CB(handleRunControlGetChildren));
    //postCommand("Locator", "redirect", "ID");

    //postCommand(C("FileSystem", "open", "\"/bin/ls\"", "1", "2", "3"),
    //    CB(handleRunControlGetChildren));
    postCommand(C("FileSystem", "stat", "\"/bin/ls\""),
        CB(handleRunControlGetChildren));
}

void TcfEngine::postCommand(const QByteArray &cmd,
    TcfCommandCallback callback, const char *callbackName)
{
    static int token = 20;
    ++token;
    
    //const char marker_eom = -1;
    //const char marker_eos = -2;
    //const char marker_null = -3;

    QByteArray ba = "C";
    ba.append('\0');
    ba.append(QByteArray::number(token));
    ba.append('\0');
    ba.append(cmd);
    ba.append('\0');
    ba.append('\3');
    ba.append('\1');

    TcfCommand tcf;
    tcf.command = ba;
    tcf.callback = callback;
    tcf.callbackName = callbackName;
    tcf.token = token;

    m_cookieForToken[token] = tcf;

    enqueueCommand(tcf);
}

// Congestion control does not seem to work that way. Basically it's
// already too late when we get a flow control packet
void TcfEngine::enqueueCommand(const TcfCommand &cmd)
{
#ifdef USE_CONGESTION_CONTROL
    // congestion controled
    if (m_congestion <= 0 && m_sendQueue.isEmpty()) {
        //SDEBUG("DIRECT SEND" << cmd.toString());
        sendCommandNow(cmd);
    } else {
        SDEBUG("QUEUE " << cmd.toString());
        m_sendQueue.enqueue(cmd);
        m_sendTimer.start();
    }
#else
    // synchrounously
    if (m_inAir == 0)
        sendCommandNow(cmd);
    else
        m_sendQueue.enqueue(cmd);
#endif
}

void TcfEngine::handleSendTimer()
{
    QTC_ASSERT(!m_sendQueue.isEmpty(), return);

    if (m_congestion > 0) {
        // not ready...
        SDEBUG("WAITING FOR CONGESTION TO GO DOWN...");
        m_sendTimer.start();
    } else {
        // go!
        sendCommandNow(m_sendQueue.dequeue());
    }
}

void TcfEngine::sendCommandNow(const TcfCommand &cmd)
{
    ++m_inAir;
    int result = m_socket->write(cmd.command);
    Q_UNUSED(result)
    m_socket->flush();
    showMessage(QString::number(cmd.token) + " " + cmd.toString(), LogInput);
    SDEBUG("SEND " <<  cmd.toString()); //<< " " << QString::number(result));
}

void TcfEngine::acknowledgeResult()
{
#if !defined(USE_CONGESTION_CONTROL)
    QTC_ASSERT(m_inAir == 1, /**/);
    m_inAir = 0;
    if (!m_sendQueue.isEmpty())
        sendCommandNow(m_sendQueue.dequeue());
#endif
}

void TcfEngine::handleRunControlSuspend(const JsonValue &data, const QVariant &)
{
    SDEBUG("HANDLE RESULT" << data.toString());
}

void TcfEngine::handleRunControlGetChildren(const JsonValue &data, const QVariant &)
{
    SDEBUG("HANDLE RUN CONTROL GET CHILDREN" << data.toString());
}

void TcfEngine::handleSysMonitorGetChildren(const JsonValue &data, const QVariant &)
{
    SDEBUG("HANDLE RUN CONTROL GET CHILDREN" << data.toString());
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

void TcfEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
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

void TcfEngine::assignValueInDebugger(const Internal::WatchData *, const QString &expression, const QVariant &value)
{
    XSDEBUG("ASSIGNING: " << expression + '=' + value.toString());
    updateLocals();
}

void TcfEngine::updateLocals()
{
}

void TcfEngine::updateWatchData(const WatchData &, const WatchUpdateFlags &)
{
    //qq->watchHandler()->rebuildModel();
    showStatusMessage(tr("Stopped."), 5000);
}

void TcfEngine::updateSubItem(const WatchData &data0)
{
    Q_UNUSED(data0)
    QTC_ASSERT(false, return);
}

DebuggerEngine *createTcfEngine(const DebuggerStartParameters &sp)
{
    return new TcfEngine(sp);
}

} // namespace Internal
} // namespace Debugger
