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

#ifndef DEBUGGER_TCFENGINE_H
#define DEBUGGER_TCFENGINE_H

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QProcess>
#include <QtCore/QQueue>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <QtNetwork/QAbstractSocket>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

#include "debuggerengine.h"
#include "json.h"

namespace Debugger {
namespace Internal {

class ScriptAgent;
class WatchData;

class TcfEngine : public Debugger::DebuggerEngine
{
    Q_OBJECT

public:
    explicit TcfEngine(const DebuggerStartParameters &startParameters);
    ~TcfEngine();

private:
    // DebuggerEngine implementation
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();

    void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const QString &fileName, int lineNumber);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const QString &fileName, int lineNumber);

    void activateFrame(int index);
    void selectThread(int index);

    void attemptBreakpointSynchronization();
    bool acceptsBreakpoint(BreakpointId) const { return false; }

    void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);
    void executeDebuggerCommand(const QString &command);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return true; }
    void maybeBreakNow(bool byFunction);
    void updateWatchData(const WatchData &data, const WatchUpdateFlags &flags);
    void updateLocals();
    void updateSubItem(const WatchData &data);

    Q_SLOT void socketConnected();
    Q_SLOT void socketDisconnected();
    Q_SLOT void socketError(QAbstractSocket::SocketError);
    Q_SLOT void socketReadyRead();

    void handleResponse(const QByteArray &ba);
    void handleRunControlSuspend(const JsonValue &response, const QVariant &);
    void handleRunControlGetChildren(const JsonValue &response, const QVariant &);
    void handleSysMonitorGetChildren(const JsonValue &response, const QVariant &);

private:
    Q_SLOT void startDebugging();

    typedef void (TcfEngine::*TcfCommandCallback)
        (const JsonValue &record, const QVariant &cookie);

    struct TcfCommand
    {
        TcfCommand() : flags(0), token(-1), callback(0), callbackName(0) {}

        QString toString() const;

        int flags;
        int token;
        TcfCommandCallback callback;
        const char *callbackName;
        QByteArray command;
        QVariant cookie;
    };

    void postCommand(const QByteArray &cmd,
        TcfCommandCallback callback = 0, const char *callbackName = 0);
    void sendCommandNow(const TcfCommand &command);

    QHash<int, TcfCommand> m_cookieForToken;

    QQueue<TcfCommand> m_sendQueue;
    
    // timer based congestion control. does not seem to work well.
    void enqueueCommand(const TcfCommand &command);
    Q_SLOT void handleSendTimer();
    int m_congestion;
    QTimer m_sendTimer;

    // synchrounous communication
    void acknowledgeResult();
    int m_inAir;

    QTcpSocket *m_socket;
    QByteArray m_inbuffer;
    QList<QByteArray> m_services;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_TCFENGINE_H
