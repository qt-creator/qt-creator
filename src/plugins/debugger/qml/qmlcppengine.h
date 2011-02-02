#ifndef QMLGDBENGINE_H
#define QMLGDBENGINE_H

#include "debuggerengine.h"

#include <QtCore/QScopedPointer>

namespace Debugger {
namespace Internal {

class QmlCppEnginePrivate;

class DEBUGGER_EXPORT QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit QmlCppEngine(const DebuggerStartParameters &sp);
    ~QmlCppEngine();

    void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor * editor, int cursorPos);
    void updateWatchData(const WatchData &data,
        const WatchUpdateFlags &flags);

    void watchPoint(const QPoint &);
    void fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length);
    void fetchDisassembler(DisassemblerAgent *);
    void activateFrame(int index);

    void reloadModules();
    void examineModules();
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);

    void reloadRegisters();
    void reloadSourceFiles();
    void reloadFullStack();

    void setRegisterValue(int regnr, const QString &value);
    unsigned debuggerCapabilities() const;

    bool isSynchronous() const;
    QByteArray qtNamespace() const;

    void createSnapshot();
    void updateAll();

    void attemptBreakpointSynchronization();
    bool acceptsBreakpoint(BreakpointId id) const;
    void selectThread(int index);

    void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);

    DebuggerEngine *cppEngine() const;
    void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    void handleRemoteSetupFailed(const QString &message);

protected:
    void detachDebugger();
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();
    void executeReturn();
    void continueInferior();
    void interruptInferior();
    void requestInterruptInferior();

    void executeRunToLine(const QString &fileName, int lineNumber);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const QString &fileName, int lineNumber);
    void executeDebuggerCommand(const QString &command);

    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();

    void notifyInferiorRunOk();
    void notifyInferiorSpontaneousStop();
    void notifyEngineRunAndInferiorRunOk();
    void notifyInferiorShutdownOk();

private:
    void engineStateChanged(DebuggerState newState);
    void setState(DebuggerState newState, bool forced = false);
    void slaveEngineStateChanged(DebuggerEngine *slaveEngine, DebuggerState state);

    void readyToExecuteQmlStep();

private:
    QScopedPointer<QmlCppEnginePrivate> d;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLGDBENGINE_H
