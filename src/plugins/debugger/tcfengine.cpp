/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "tcfengine.h"

#include "debuggerdialogs.h"
#include "breakhandler.h"
#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "disassemblerhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "moduleshandler.h"

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


using namespace Debugger;
using namespace Debugger::Internal;
using namespace Debugger::Constants;

#define DEBUG_TCF 1
#if DEBUG_TCF
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)
#define CB(callback) &TcfEngine::callback, STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// TcfEngine
//
///////////////////////////////////////////////////////////////////////

TcfEngine::TcfEngine(DebuggerManager *parent)
{
    q = parent;
    qq = parent->engineInterface();
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

    connect(this, SIGNAL(tcfOutputAvailable(QString,QString)),
        q, SLOT(showDebuggerOutput(QString,QString)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(tcfInputAvailable(QString,QString)),
        q, SLOT(showDebuggerInput(QString,QString)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(applicationOutputAvailable(QString)),
        q, SLOT(showApplicationOutput(QString)),
        Qt::QueuedConnection);
}

TcfEngine::~TcfEngine()
{
}

void TcfEngine::socketReadyRead()
{
    //XSDEBUG("TcfEngine::socketReadyRead()");
    m_inbuffer.append(m_socket->readAll());
    //handleResponse(QByteArray::fromRawData(m_inbuffer.constData() + start, end - start));
    handleResponse(m_inbuffer);
    m_inbuffer.clear();
}

void TcfEngine::socketConnected()
{
    q->showStatusMessage("Socket connected.");
    m_socket->waitForConnected(2000);
    //sendCommand("Locator", "redirect", "ID");
}

void TcfEngine::socketDisconnected()
{
    XSDEBUG("FIXME:  TcfEngine::socketDisconnected()");
}

void TcfEngine::socketError(QAbstractSocket::SocketError)
{
    QString msg = tr("Socket error: %1").arg(m_socket->errorString());
    QMessageBox::critical(q->mainWindow(), tr("Error"), msg);
    q->showStatusMessage(msg);
    qq->notifyInferiorExited();
}

void TcfEngine::executeDebuggerCommand(const QString &command)
{
    Q_UNUSED(command);
    XSDEBUG("FIXME:  TcfEngine::executeDebuggerCommand()");
}

void TcfEngine::shutdown()
{
    exitDebugger(); 
}

void TcfEngine::exitDebugger()
{
    SDEBUG("TcfEngine::exitDebugger()");
    qq->notifyInferiorExited();
}

bool TcfEngine::startDebugger()
{
    qq->notifyInferiorRunningRequested();
    int pos = q->m_remoteChannel.indexOf(':');
    QString host = q->m_remoteChannel.left(pos);
    quint16 port = q->m_remoteChannel.mid(pos + 1).toInt();
    //QTimer::singleShot(0, this, SLOT(runInferior()));
    m_socket->connectToHost(host, port);
    return true;
}

void TcfEngine::continueInferior()
{
    SDEBUG("TcfEngine::continueInferior()");
}

void TcfEngine::runInferior()
{
}

void TcfEngine::interruptInferior()
{
    XSDEBUG("TcfEngine::interruptInferior()");
}

void TcfEngine::stepExec()
{
    //SDEBUG("TcfEngine::stepExec()");
}

void TcfEngine::stepIExec()
{
    //SDEBUG("TcfEngine::stepIExec()");
}

void TcfEngine::stepOutExec()
{
    //SDEBUG("TcfEngine::stepOutExec()");
}

void TcfEngine::nextExec()
{
    //SDEBUG("TcfEngine::nextExec()");
}

void TcfEngine::nextIExec()
{
    //SDEBUG("TcfEngine::nextIExec()");
}

void TcfEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName);
    Q_UNUSED(lineNumber);
    SDEBUG("FIXME:  TcfEngine::runToLineExec()");
}

void TcfEngine::runToFunctionExec(const QString &functionName)
{
    Q_UNUSED(functionName);
    XSDEBUG("FIXME:  TcfEngine::runToFunctionExec()");
}

void TcfEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName);
    Q_UNUSED(lineNumber);
    XSDEBUG("FIXME:  TcfEngine::jumpToLineExec()");
}

void TcfEngine::activateFrame(int index)
{
    Q_UNUSED(index);
}

void TcfEngine::selectThread(int index)
{
    Q_UNUSED(index);
}

void TcfEngine::attemptBreakpointSynchronization()
{
}

void TcfEngine::reloadDisassembler()
{
}

void TcfEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName);
}

void TcfEngine::loadAllSymbols()
{
}

void TcfEngine::reloadModules()
{
}

QList<Symbol> TcfEngine::moduleSymbols(const QString & /*moduleName*/)
{
    return QList<Symbol>();
}


void TcfEngine::handleResponse(const QByteArray &buf)
{
    static QTime lastTime;

    //emit tcfOutputAvailable(_("            "), currentTime());
    TcfResponse response;
    QList<QByteArray> parts = buf.split('\0');
    int n = parts.size();
    if (n >= 1)
        response.tag = parts.at(0);
    if (n >= 2)
        response.service = parts.at(1);
    if (n >= 3)
        response.cmd = parts.at(2);
    if (n >= 4)
        response.data = parts.at(3);
    if (response.cmd != "peerHeartBeat")
        emit tcfOutputAvailable(_("\ntcf:"), quoteUnprintableLatin1(buf));
    //emit tcfOutputAvailable(_("\ntcf:"), response.toString());
    qDebug() << response.toString();

    if (response.service == "Locator" && response.cmd == "Hello") {
        postCommand('C', CB(handleRunControlSuspend),
            "RunControl", "suspend", "\"Thread1\"");
        //postCommand('F', "0", "", "");
        //postCommand('E', "Locator", "Hello", "");
        //postCommand('C', "Locator", "sync", "");
        //postCommand("Locator", "redirect", "ID");
        return;
    }

    TcfCommand tcf = m_cookieForToken[1];
    if (tcf.callback)
        (this->*(tcf.callback))(response, tcf.cookie);
}

void TcfEngine::postCommand(char tag,
    TcfCommandCallback callback,
    const char *callbackName,
    const QByteArray &service,
    const QByteArray &cmd,
    const QByteArray &args)
{
    static int token;
    ++token;
    
    const char delim = 0;
    const char marker_eom = -1;
    const char marker_eos = -2;
    const char marker_null = -3;

    QByteArray ba;
    ba.append(tag);
    ba.append(delim);
    ba.append(QString::number(token).toLatin1());
    ba.append(delim);
    ba.append(service);
    ba.append(delim);
    ba.append(cmd);
    ba.append(delim);
    ba.append(args);
    ba.append(delim);
    ba.append(3);
    ba.append(1);

    TcfCommand tcf;
    tcf.command = ba;
    tcf.callback = callback;

    m_cookieForToken[token] = tcf;

    emit tcfInputAvailable("send", quoteUnprintableLatin1(ba));
    int result = m_socket->write(tcf.command);
    m_socket->flush();
    emit tcfInputAvailable("send", QString::number(result));
}

void TcfEngine::handleRunControlSuspend(const TcfResponse &response, const QVariant &)
{
    qDebug() << "HANDLE RESULT";
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

void TcfEngine::setToolTipExpression(const QPoint &pos, const QString &exp0)
{
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void TcfEngine::assignValueInDebugger(const QString &expression,
    const QString &value)
{
    XSDEBUG("ASSIGNING: " << expression + '=' + value);
    updateLocals();
}

void TcfEngine::updateLocals()
{
}

void TcfEngine::updateWatchModel()
{
    qq->watchHandler()->rebuildModel();
    q->showStatusMessage(tr("Stopped."), 5000);
}

void TcfEngine::updateSubItem(const WatchData &data0)
{
    QTC_ASSERT(false, return);
}

IDebuggerEngine *createTcfEngine(DebuggerManager *parent, QList<Core::IOptionsPage*> *)
{
    return new TcfEngine(parent);
}

