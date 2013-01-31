/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_CDBENGINE_H
#define DEBUGGER_CDBENGINE_H

#include "debuggerengine.h"
#include "breakpoint.h"
#include "threaddata.h"

#include <QSharedPointer>
#include <QProcess>
#include <QVariantList>
#include <QMap>
#include <QMultiHash>
#include <QTime>
#include <QPair>
#include <QList>

namespace Utils {
class ConsoleProcess;
}
namespace Debugger {
namespace Internal {

class DisassemblerAgent;
struct CdbBuiltinCommand;
struct CdbExtensionCommand;
struct CdbOptions;
struct MemoryViewCookie;
class ByteArrayInputStream;
class GdbMi;

class CdbEngine : public Debugger::DebuggerEngine
{
    Q_OBJECT

public:
    typedef QSharedPointer<CdbOptions> OptionsPtr;

    enum CommandFlags { QuietCommand = 0x1 };
    // Flag bits for a sequence of commands
    enum CommandSequenceFlags {
        CommandListStack = 0x1,
        CommandListThreads = 0x2,
        CommandListRegisters = 0x4,
        CommandListModules = 0x8,
        CommandListBreakPoints = 0x10
    };

    typedef QSharedPointer<CdbBuiltinCommand> CdbBuiltinCommandPtr;
    typedef QSharedPointer<CdbExtensionCommand> CdbExtensionCommandPtr;
    typedef void (CdbEngine::*BuiltinCommandHandler)(const CdbBuiltinCommandPtr &);
    typedef void (CdbEngine::*ExtensionCommandHandler)(const CdbExtensionCommandPtr &);

    CdbEngine(const DebuggerStartParameters &sp, const OptionsPtr &options);
    ~CdbEngine();

    // Factory function that returns 0 if the debug engine library cannot be found.

    virtual bool setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor,
                                      const DebuggerToolTipContext &ctx);
    virtual void setupEngine();
    virtual void setupInferior();
    virtual void runEngine();
    virtual void shutdownInferior();
    virtual void shutdownEngine();
    virtual void detachDebugger();
    virtual void updateWatchData(const WatchData &data,
                                 const WatchUpdateFlags & flags = WatchUpdateFlags());
    virtual bool hasCapability(unsigned cap) const;
    virtual void watchPoint(const QPoint &);
    virtual void setRegisterValue(int regnr, const QString &value);

    virtual void executeStep();
    virtual void executeStepOut();
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();

    virtual void continueInferior();
    virtual void interruptInferior();

    virtual void executeRunToLine(const ContextData &data);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const ContextData &data);
    virtual void assignValueInDebugger(const WatchData *w, const QString &expr, const QVariant &value);
    virtual void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

    virtual void activateFrame(int index);
    virtual void selectThread(ThreadId threadId);

    virtual bool stateAcceptsBreakpointChanges() const;
    virtual bool acceptsBreakpoint(BreakpointModelId id) const;
    virtual void attemptBreakpointSynchronization();

    virtual void fetchDisassembler(DisassemblerAgent *agent);
    virtual void fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length);
    virtual void changeMemory(Internal::MemoryAgent *, QObject *, quint64 addr, const QByteArray &data);

    virtual void reloadModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();

    static QString extensionLibraryName(bool is64Bit);

private slots:
    void readyReadStandardOut();
    void readyReadStandardError();
    void processError();
    void processFinished();
    void postCommand(const QByteArray &cmd, unsigned flags);
    void postBuiltinCommand(const QByteArray &cmd,
                            unsigned flags,
                            BuiltinCommandHandler handler,
                            unsigned nextCommandFlag = 0,
                            const QVariant &cookie = QVariant());

    void postExtensionCommand(const QByteArray &cmd,
                              const QByteArray &arguments,
                              unsigned flags,
                              ExtensionCommandHandler handler,
                              unsigned nextCommandFlag = 0,
                              const QVariant &cookie = QVariant());

    void postCommandSequence(unsigned mask);
    void operateByInstructionTriggered(bool);

    void consoleStubError(const QString &);
    void consoleStubProcessStarted();
    void consoleStubExited();

private:
    typedef QHash<BreakpointModelId, BreakpointResponse> PendingBreakPointMap;
    typedef QPair<QString, QString> SourcePathMapping;
    struct NormalizedSourceFileName // Struct for caching mapped/normalized source files.
    {
        NormalizedSourceFileName(const QString &fn = QString(), bool e = false) : fileName(fn), exists(e) {}

        QString fileName;
        bool exists;
    };

    enum SpecialStopMode
    {
        NoSpecialStop,
        SpecialStopSynchronizeBreakpoints,
        SpecialStopGetWidgetAt,
        CustomSpecialStop // Associated with m_specialStopData, handleCustomSpecialStop()
    };
    enum ParseStackResultFlags // Flags returned by parseStackTrace
    {
        ParseStackStepInto = 1 // Need to execute a step, hit on a call frame in "Step into"
    };


    bool startConsole(const DebuggerStartParameters &sp, QString *errorMessage);
    void init();
    unsigned examineStopReason(const GdbMi &stopReason, QString *message,
                               QString *exceptionBoxMessage,
                               bool conditionalBreakPointTriggered = false);
    void processStop(const GdbMi &stopReason, bool conditionalBreakPointTriggered = false);
    bool commandsPending() const;
    void handleExtensionMessage(char t, int token, const QByteArray &what, const QByteArray &message);
    bool doSetupEngine(QString *errorMessage);
    bool launchCDB(const DebuggerStartParameters &sp, QString *errorMessage);
    void handleSessionAccessible(unsigned long cdbExState);
    void handleSessionInaccessible(unsigned long cdbExState);
    void handleSessionIdle(const QByteArray &message);
    bool doInterruptInferior(SpecialStopMode sm);
    void doInterruptInferiorCustomSpecialStop(const QVariant &v);
    void doContinueInferior();
    inline void parseOutputLine(QByteArray line);
    inline bool isCdbProcessRunning() const { return m_process.state() != QProcess::NotRunning; }
    bool canInterruptInferior() const;
    void syncOperateByInstruction(bool operateByInstruction);
    void postWidgetAtCommand();
    void handleCustomSpecialStop(const QVariant &v);
    void postFetchMemory(const MemoryViewCookie &c);
    inline void postDisassemblerCommand(quint64 address, const QVariant &cookie = QVariant());
    void postDisassemblerCommand(quint64 address, quint64 endAddress,
                                 const QVariant &cookie = QVariant());
    void postResolveSymbol(const QString &module, const QString &function,
                           const QVariant &cookie =  QVariant());
    void evaluateExpression(QByteArray exp, const QVariant &cookie = QVariant());
    // Builtin commands
    void dummyHandler(const CdbBuiltinCommandPtr &);
    void handleStackTrace(const CdbExtensionCommandPtr &);
    void handleRegisters(const CdbBuiltinCommandPtr &);
    void handleDisassembler(const CdbBuiltinCommandPtr &);
    void handleJumpToLineAddressResolution(const CdbBuiltinCommandPtr &);
    void handleExpression(const CdbExtensionCommandPtr &);
    void handleResolveSymbol(const CdbBuiltinCommandPtr &command);
    void handleResolveSymbol(const QList<quint64> &addresses, const QVariant &cookie);
    void jumpToAddress(quint64 address);

    // Extension commands
    void handleThreads(const CdbExtensionCommandPtr &);
    void handlePid(const CdbExtensionCommandPtr &reply);
    void handleLocals(const CdbExtensionCommandPtr &reply);
    void handleAddWatch(const CdbExtensionCommandPtr &reply);
    void handleExpandLocals(const CdbExtensionCommandPtr &reply);
    void handleRegisters(const CdbExtensionCommandPtr &reply);
    void handleModules(const CdbExtensionCommandPtr &reply);
    void handleMemory(const CdbExtensionCommandPtr &);
    void handleWidgetAt(const CdbExtensionCommandPtr &);
    void handleBreakPoints(const CdbExtensionCommandPtr &);
    void handleBreakPoints(const GdbMi &value);
    NormalizedSourceFileName sourceMapNormalizeFileNameFromDebugger(const QString &f);
    void updateLocalVariable(const QByteArray &iname);
    void updateLocals(bool forNewStackFrame = false);
    int elapsedLogTime() const;
    void addLocalsOptions(ByteArrayInputStream &s) const;
    unsigned parseStackTrace(const GdbMi &data, bool sourceStepInto);

    const QByteArray m_creatorExtPrefix;
    const QByteArray m_tokenPrefix;
    const OptionsPtr m_options;

    QProcess m_process;
    QScopedPointer<Utils::ConsoleProcess> m_consoleStub;
    DebuggerStartMode m_effectiveStartMode;
    QByteArray m_outputBuffer;
    //! Debugger accessible (expecting commands)
    bool m_accessible;
    SpecialStopMode m_specialStopMode;
    int m_nextCommandToken;
    QList<CdbBuiltinCommandPtr> m_builtinCommandQueue;
    int m_currentBuiltinCommandIndex; //!< Current command whose output is recorded.
    QList<CdbExtensionCommandPtr> m_extensionCommandQueue;
    QMap<QString, NormalizedSourceFileName> m_normalizedFileCache;
    const QByteArray m_extensionCommandPrefixBA; //!< Library name used as prefix
    bool m_operateByInstructionPending; //!< Creator operate by instruction action changed.
    bool m_operateByInstruction;
    bool m_notifyEngineShutdownOnTermination;
    bool m_hasDebuggee;
    bool m_cdbIs64Bit;
    QTime m_logTime;
    mutable int m_elapsedLogTime;
    QByteArray m_extensionMessageBuffer;
    bool m_sourceStepInto;
    int m_watchPointX;
    int m_watchPointY;
    PendingBreakPointMap m_pendingBreakpointMap;
    QHash<QString, QString> m_fileNameModuleHash;
    QMultiHash<QString, quint64> m_symbolAddressCache;
    QHash<QByteArray, QString> m_watchInameToName;
    bool m_ignoreCdbOutput;
    QVariantList m_customSpecialStopData;
    QList<SourcePathMapping> m_sourcePathMappings;
    QScopedPointer<GdbMi> m_coreStopReason;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINE_H
