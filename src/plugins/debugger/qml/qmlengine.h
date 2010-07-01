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

#ifndef DEBUGGER_QMLENGINE_H
#define DEBUGGER_QMLENGINE_H

#include "debuggerengine.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QProcess>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>

QT_BEGIN_NAMESPACE
class QTcpSocket;
class QDeclarativeDebugConnection;
class QDeclarativeEngineDebug;
class QDeclarativeDebugEnginesQuery;
class QDeclarativeDebugRootContextQuery;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class ScriptAgent;
class WatchData;
class QmlResponse;
class CanvasFrameRate;
class QmlDebuggerClient;

class DEBUGGER_EXPORT QmlEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit QmlEngine(const DebuggerStartParameters &startParameters);
    ~QmlEngine();

    void messageReceived(const QByteArray &message);
    using DebuggerEngine::setState;

private:
    // DebuggerEngine implementation
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void shutdown();
    void setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);
    void startDebugger();
    void exitDebugger();

    void continueInferior();
    Q_SLOT void runInferior();
    void interruptInferior();

    void executeRunToLine(const QString &fileName, int lineNumber);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const QString &fileName, int lineNumber);

    void activateFrame(int index);
    void selectThread(int index);

    void attemptBreakpointSynchronization();

    void assignValueInDebugger(const QString &expr, const QString &value);
    void executeDebuggerCommand(const QString & command);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return true; }
    void maybeBreakNow(bool byFunction);
    void updateWatchData(const WatchData &data);
    void updateLocals();
    void updateSubItem(const WatchData &data);

    Q_SLOT void socketConnected();
    Q_SLOT void socketDisconnected();
    Q_SLOT void socketError(QAbstractSocket::SocketError);
    Q_SLOT void socketReadyRead();

    void handleResponse(const QByteArray &ba);
    void handleRunControlSuspend(const QmlResponse &response, const QVariant &);
    void handleRunControlGetChildren(const QmlResponse &response, const QVariant &);
    void handleSysMonitorGetChildren(const QmlResponse &response, const QVariant &);

    unsigned int debuggerCapabilities() const;

    Q_SLOT void startDebugging();
    void setupConnection();

    void sendMessage(const QByteArray &msg);

private slots:
    void handleProcFinished(int, QProcess::ExitStatus status);
    void handleProcError(QProcess::ProcessError error);
    void readProcStandardOutput();
    void readProcStandardError();

    void connectionError();
    void connectionConnected();
    void connectionStateChanged();

private:
    QString errorMessage(QProcess::ProcessError error);
    typedef void (QmlEngine::*QmlCommandCallback)
        (const QmlResponse &record, const QVariant &cookie);

    struct QmlCommand
    {
        QmlCommand() : flags(0), token(-1), callback(0), callbackName(0) {}

        QString toString() const;

        int flags;
        int token;
        QmlCommandCallback callback;
        const char *callbackName;
        QByteArray command;
        QVariant cookie;
    };

    void postCommand(const QByteArray &cmd,
        QmlCommandCallback callback = 0, const char *callbackName = 0);
    void sendCommandNow(const QmlCommand &command);

    QHash<int, QmlCommand> m_cookieForToken;

    QQueue<QmlCommand> m_sendQueue;
    
    // timer based congestion control. does not seem to work well.
    void enqueueCommand(const QmlCommand &command);
    Q_SLOT void handleSendTimer();
    int m_congestion;
    QTimer m_sendTimer;

    // synchrounous communication
    void acknowledgeResult();
    int m_inAir;

    QTcpSocket *m_socket;
    QByteArray m_inbuffer;
    QList<QByteArray> m_services;
    QProcess m_proc;

    QDeclarativeDebugConnection *m_conn;
    QmlDebuggerClient *m_client;
    QDeclarativeDebugEnginesQuery *m_engineQuery;
    QDeclarativeDebugRootContextQuery *m_contextQuery;
    CanvasFrameRate *m_frameRate;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_QMLENGINE_H
