/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLENGINE_H
#define QMLENGINE_H

#include <debugger/debuggerengine.h>

#include <qmldebug/qdebugmessageclient.h>
#include <qmldebug/qmloutputparser.h>
#include <qmljs/iscriptevaluator.h>
#include <qmljs/qmljsdocument.h>

namespace Debugger {
namespace Internal {

class WatchData;
class WatchItem;
class QmlEnginePrivate;
class QmlInspectorAgent;

class QmlEngine : public DebuggerEngine, QmlJS::IScriptEvaluator
{
    Q_OBJECT

public:
    explicit QmlEngine(const DebuggerRunParameters &runParameters,
                       DebuggerEngine *masterEngine = nullptr);
    ~QmlEngine();

    void filterApplicationMessage(const QString &msg, int channel) const;

    void logServiceStateChange(const QString &service, float version,
                               QmlDebug::QmlDebugClient::State newState);
    void logServiceActivity(const QString &service, const QString &logMessage);

    void expressionEvaluated(quint32 queryId, const QVariant &result);

private slots:
    void disconnected();
    void errorMessageBoxFinished(int result);
    void updateCurrentContext();

    void tryToConnect(quint16 port = 0);
    void beginConnection(quint16 port = 0);
    void connectionEstablished();
    void connectionStartupFailed();
    void appStartupFailed(const QString &errorMessage);
    void appendMessage(const QString &msg, Utils::OutputFormat);

private:
    void notifyEngineRemoteServerRunning(const QByteArray &, int pid) override;
    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result) override;

    void showMessage(const QString &msg, int channel = LogDebug,
                     int timeout = -1) const override;
    void gotoLocation(const Internal::Location &location) override;
    void insertBreakpoint(Breakpoint bp) override;

    bool isSynchronous() const override { return false; }
    bool canDisplayTooltip() const override { return false; }

    void executeStep() override;
    void executeStepOut() override;
    void executeNext() override;
    void executeStepI() override;
    void executeNextI() override;

    void setupEngine() override;
    void setupInferior() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;

    bool canHandleToolTip(const DebuggerToolTipContext &) const override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;

    void activateFrame(int index) override;
    void selectThread(ThreadId threadId) override;

    void attemptBreakpointSynchronization() override;
    void removeBreakpoint(Breakpoint bp) override;
    void changeBreakpoint(Breakpoint bp) override;
    bool acceptsBreakpoint(Breakpoint bp) const override;

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value) override;

    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const QString &moduleName) override;
    void reloadModules() override;
    void reloadRegisters() override {}
    void reloadSourceFiles() override;
    void reloadFullStack() override {}

    void updateAll() override;
    void updateItem(const QByteArray &iname) override;
    void expandItem(const QByteArray &iname) override;
    void selectWatchData(const QByteArray &iname) override;
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) override;
    bool evaluateScript(const QString &expression) override;

    bool hasCapability(unsigned) const override;
    void quitDebugger() override;

    void closeConnection();
    void startApplicationLauncher();
    void stopApplicationLauncher();

    void connectionErrorOccurred(QAbstractSocket::SocketError socketError);
    void connectionStateChanged(QAbstractSocket::SocketState socketState);

    void clientStateChanged(QmlDebug::QmlDebugClient::State state);
    void checkConnectionState();
    void showConnectionStateMessage(const QString &message);
    bool isConnected() const;

private:
    friend class QmlCppEngine;
    friend class QmlEnginePrivate;
    friend class QmlInspectorAgent;
    QmlEnginePrivate *d;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLENGINE_H
