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
#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmloutputparser.h>
#include <qmljs/iscriptevaluator.h>
#include <qmljs/qmljsdocument.h>

namespace Debugger {
namespace Internal {

class WatchData;
class WatchItem;
class QmlEnginePrivate;
class QmlAdapter;

class QmlEngine : public DebuggerEngine, QmlJS::IScriptEvaluator
{
    Q_OBJECT

public:
    explicit QmlEngine(const DebuggerRunParameters &runParameters,
                       DebuggerEngine *masterEngine = 0);
    ~QmlEngine();

    void filterApplicationMessage(const QString &msg, int channel) const;

    void logServiceStateChange(const QString &service, float version,
                               QmlDebug::QmlDebugClient::State newState);
    void logServiceActivity(const QString &service, const QString &logMessage);

private slots:
    void disconnected();
    void documentUpdated(QmlJS::Document::Ptr doc);
    void expressionEvaluated(quint32 queryId, const QVariant &result);

    void errorMessageBoxFinished(int result);
    void updateCurrentContext();

    void tryToConnect(quint16 port = 0);
    void beginConnection(quint16 port = 0);
    void connectionEstablished();
    void connectionStartupFailed();
    void appStartupFailed(const QString &errorMessage);
    void appendMessage(const QString &msg, Utils::OutputFormat);

private:
    void notifyEngineRemoteServerRunning(const QByteArray &, int pid);
    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);

    void showMessage(const QString &msg, int channel = LogDebug,
                     int timeout = -1) const;
    void gotoLocation(const Internal::Location &location);
    void insertBreakpoint(Breakpoint bp);

    bool isSynchronous() const { return false; }
    bool canDisplayTooltip() const { return false; }

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

    bool canHandleToolTip(const DebuggerToolTipContext &) const;

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);

    void activateFrame(int index);
    void selectThread(ThreadId threadId);

    void attemptBreakpointSynchronization();
    void removeBreakpoint(Breakpoint bp);
    void changeBreakpoint(Breakpoint bp);
    bool acceptsBreakpoint(Breakpoint bp) const;

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles();
    void reloadFullStack() {}

    void updateAll();
    void updateItem(const QByteArray &iname);
    void expandItem(const QByteArray &iname);
    void selectWatchData(const QByteArray &iname);
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);
    bool evaluateScript(const QString &expression);

    bool hasCapability(unsigned) const;
    void quitDebugger();

    void closeConnection();
    void startApplicationLauncher();
    void stopApplicationLauncher();

    void connectionErrorOccurred(QDebugSupport::Error socketError);
    void clientStateChanged(QmlDebug::QmlDebugClient::State state);
    void checkConnectionState();
    void showConnectionStateMessage(const QString &message);
    void showConnectionErrorMessage(const QString &message);
    bool isConnected() const;

private:
    friend class QmlCppEngine;
    friend class QmlEnginePrivate;
    QmlEnginePrivate *d;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLENGINE_H
