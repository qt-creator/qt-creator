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

#include "idebuggerengine.h"
#include "debuggermanager.h"

namespace Debugger {
namespace Internal {

class ScriptAgent;
class WatchData;
class QmlResponse;

class QmlEngine : public IDebuggerEngine
{
    Q_OBJECT

public:
    explicit QmlEngine(DebuggerManager *parent);
    ~QmlEngine();

private:
    // IDebuggerEngine implementation
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void shutdown();
    void setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);
    void startDebugger(const DebuggerStartParametersPtr &sp);
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

private:
    Q_SLOT void startDebugging();

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
    void debugMessage(const QString &msg);

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
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_QMLENGINE_H
