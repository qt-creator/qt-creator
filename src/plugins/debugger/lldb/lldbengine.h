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

#ifndef DEBUGGER_LLDBENGINE
#define DEBUGGER_LLDBENGINE

#include <debugger/debuggerengine.h>
#include <debugger/disassembleragent.h>
#include <debugger/memoryagent.h>
#include <debugger/watchhandler.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggerprotocol.h>

#include <utils/consoleprocess.h>

#include <QPointer>
#include <QProcess>
#include <QQueue>
#include <QMap>
#include <QStack>
#include <QVariant>


namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;

/* A debugger engine interfacing the LLDB debugger
 * using its Python interface.
 */

class LldbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit LldbEngine(const DebuggerStartParameters &startParameters);
    ~LldbEngine();

private:
    // DebuggerEngine implementation
    DebuggerEngine *cppEngine() { return this; }

    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void setupEngine();
    void startLldb();
    void startLldbStage2();
    void setupInferior();
    void setupInferiorStage2();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();
    void abortDebugger();

    bool setToolTipExpression(const DebuggerToolTipContext &);

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);

    void activateFrame(int index);
    void selectThread(ThreadId threadId);

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const;
    bool acceptsBreakpoint(Breakpoint bp) const;
    void insertBreakpoint(Breakpoint bp);
    void removeBreakpoint(Breakpoint bp);
    void changeBreakpoint(Breakpoint bp);

    void assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value);
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters();
    void reloadSourceFiles() {}
    void reloadFullStack();
    void reloadDebuggingHelpers();
    void fetchDisassembler(Internal::DisassemblerAgent *);
    void refreshDisassembly(const GdbMi &data);

    bool supportsThreads() const { return true; }
    bool isSynchronous() const { return true; }
    void updateWatchItem(WatchItem *item);
    void setRegisterValue(const QByteArray &name, const QString &value);

    void fetchMemory(Internal::MemoryAgent *, QObject *, quint64 addr, quint64 length);
    void changeMemory(Internal::MemoryAgent *, QObject *, quint64 addr, const QByteArray &data);
    void refreshMemory(const GdbMi &data);

signals:
    void outputReady(const QByteArray &data);

private:
    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const;

    void handleLldbFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleLldbError(QProcess::ProcessError error);
    void readLldbStandardOutput();
    void readLldbStandardError();
    void handleResponse(const QByteArray &data);
    void updateAll();
    void updateLocals();
    void createFullBacktrace();
    void doUpdateLocals(UpdateParameters params);
    void handleContinuation(const GdbMi &data);

    void refreshAll(const GdbMi &all);
    void refreshThreads(const GdbMi &threads);
    void refreshCurrentThread(const GdbMi &data);
    void refreshStack(const GdbMi &stack);
    void refreshRegisters(const GdbMi &registers);
    void refreshTypeInfo(const GdbMi &typeInfo);
    void refreshState(const GdbMi &state);
    void refreshLocation(const GdbMi &location);
    void refreshModules(const GdbMi &modules);
    void refreshSymbols(const GdbMi &symbols);
    void refreshOutput(const GdbMi &output);
    void refreshAddedBreakpoint(const GdbMi &bkpts);
    void refreshChangedBreakpoint(const GdbMi &bkpts);
    void refreshRemovedBreakpoint(const GdbMi &bkpts);
    void showFullBacktrace(const GdbMi &data);

    typedef void (LldbEngine::*LldbCommandContinuation)();

    void handleListLocals(const QByteArray &response);
    void handleListModules(const QByteArray &response);
    void handleListSymbols(const QByteArray &response);
    void handleBreakpointsSynchronized(const QByteArray &response);
    void updateBreakpointData(const GdbMi &bkpt, bool added);
    void handleUpdateStack(const QByteArray &response);
    void handleUpdateThreads(const QByteArray &response);

    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);

    void handleChildren(const WatchData &data0, const GdbMi &item,
        QList<WatchData> *list);

    void runCommand(const DebuggerCommand &cmd);
    void debugLastCommand();
    DebuggerCommand m_lastDebuggableCommand;

    QByteArray m_inbuffer;
    QString m_scriptFileName;
    QProcess m_lldbProc;
    QString m_lldbCmd;

    // FIXME: Make generic.
    int m_lastAgentId;
    int m_continueAtNextSpontaneousStop;
    QMap<QPointer<DisassemblerAgent>, int> m_disassemblerAgents;
    QMap<QPointer<MemoryAgent>, int> m_memoryAgents;
    QHash<int, QPointer<QObject> > m_memoryAgentTokens;

    // Console handling.
    Q_SLOT void stubError(const QString &msg);
    Q_SLOT void stubExited();
    Q_SLOT void stubStarted();
    bool prepareCommand();
    Utils::ConsoleProcess m_stubProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE
