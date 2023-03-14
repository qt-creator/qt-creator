// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

#include <qmldebug/qdebugmessageclient.h>
#include <qmldebug/qmloutputparser.h>
#include <qmljs/qmljsdocument.h>

namespace Debugger::Internal {

class QmlEnginePrivate;
class QmlInspectorAgent;

class QmlEngine : public DebuggerEngine
{
public:
    QmlEngine();
    ~QmlEngine() override;

    void logServiceStateChange(const QString &service, float version,
                               QmlDebug::QmlDebugClient::State newState);
    void logServiceActivity(const QString &service, const QString &logMessage);

    void expressionEvaluated(quint32 queryId, const QVariant &result);
    Utils::FilePath toFileInProject(const QUrl &fileUrl);

private:
    void disconnected();
    void errorMessageBoxFinished(int result);
    void updateCurrentContext();

    void tryToConnect();
    void beginConnection();
    void handleLauncherStarted();
    void connectionEstablished();
    void connectionStartupFailed();
    void appStartupFailed(const QString &errorMessage);

    void setState(DebuggerState state, bool forced) override;

    void gotoLocation(const Internal::Location &location) override;

    bool canDisplayTooltip() const override { return false; }

    void resetLocation() override;

    void executeStepOver(bool) override;
    void executeStepIn(bool) override;
    void executeStepOut() override;

    void setupEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;

    bool canHandleToolTip(const DebuggerToolTipContext &) const override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;

    void activateFrame(int index) override;
    void selectThread(const Thread &thread) override;

    bool acceptsBreakpoint(const BreakpointParameters &bp) const final;
    void insertBreakpoint(const Breakpoint &bp) final;
    void removeBreakpoint(const Breakpoint &bp) final;
    void updateBreakpoint(const Breakpoint &bp) final;

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value) override;

    void loadSymbols(const Utils::FilePath &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const Utils::FilePath &moduleName) override;
    void reloadModules() override;
    void reloadRegisters() override {}
    void reloadSourceFiles() override;
    void reloadFullStack() override {}

    void updateAll() override;
    void updateItem(const QString &iname) override;
    void expandItem(const QString &iname) override;
    void selectWatchData(const QString &iname) override;
    void executeDebuggerCommand(const QString &command) override;

    bool companionPreventsActions() const override;
    bool hasCapability(unsigned) const override;
    void quitDebugger() override;

    void doUpdateLocals(const UpdateParameters &params) override;
    Core::Context languageContext() const override;

    void closeConnection();
    void startProcess();
    void stopProcess();

    void connectionFailed();

    void checkConnectionState();
    void showConnectionStateMessage(const QString &message);
    bool isConnected() const;

private:
    friend class QmlEnginePrivate;
    friend class QmlInspectorAgent;
    QmlEnginePrivate *d;
};

} // Debugger::Internal
