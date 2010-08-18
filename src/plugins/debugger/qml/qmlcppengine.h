#ifndef QMLGDBENGINE_H
#define QMLGDBENGINE_H

#include "debuggerengine.h"

namespace Core {
class IEditor;
}

namespace Debugger {
namespace Internal {

class GdbEngine;
class QmlEngine;

class QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT
public:
    QmlCppEngine(const DebuggerStartParameters &sp);
    virtual ~QmlCppEngine();

    void setActiveEngine(DebuggerLanguage language);

    virtual void setToolTipExpression(const QPoint & /* mousePos */,
            TextEditor::ITextEditor * /* editor */, int /* cursorPos */);
    virtual void updateWatchData(const WatchData & /* data */);

    virtual void watchPoint(const QPoint &);
    virtual void fetchMemory(MemoryViewAgent *, QObject *,
            quint64 addr, quint64 length);
    virtual void fetchDisassembler(DisassemblerViewAgent *);
    virtual void activateFrame(int index);

    virtual void reloadModules();
    virtual void examineModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();

    virtual void setRegisterValue(int regnr, const QString &value);
    virtual unsigned debuggerCapabilities() const;

    virtual bool isSynchroneous() const;
    virtual QString qtNamespace() const;

    virtual void createSnapshot();
    virtual void updateAll();

    virtual void attemptBreakpointSynchronization();
    virtual void selectThread(int index);

    virtual void assignValueInDebugger(const QString &expr, const QString &value);

    QAbstractItemModel *commandModel() const;
    QAbstractItemModel *modulesModel() const;
    QAbstractItemModel *breakModel() const;
    QAbstractItemModel *registerModel() const;
    QAbstractItemModel *stackModel() const;
    QAbstractItemModel *threadsModel() const;
    QAbstractItemModel *localsModel() const;
    QAbstractItemModel *watchersModel() const;
    QAbstractItemModel *returnModel() const;
    QAbstractItemModel *sourceFilesModel() const;

protected:
    virtual void detachDebugger();
    virtual void executeStep();
    virtual void executeStepOut();
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();
    virtual void executeReturn();
    virtual void continueInferior();
    virtual void interruptInferior();
    virtual void requestInterruptInferior();

    virtual void executeRunToLine(const QString &fileName, int lineNumber);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const QString &fileName, int lineNumber);
    virtual void executeDebuggerCommand(const QString &command);

    virtual void frameUp();
    virtual void frameDown();

    virtual void notifyInferiorRunOk();

protected:
    virtual void setupEngine();
    virtual void setupInferior();
    virtual void runEngine();
    virtual void shutdownInferior();
    virtual void shutdownEngine();

private slots:
    void masterEngineStateChanged(const DebuggerState &state);
    void slaveEngineStateChanged(const DebuggerState &state);
    void setupSlaveEngine();
    void editorChanged(Core::IEditor *editor);

private:
    void setupSlaveEngineOnTimer();
    void finishDebugger();
    void handleSlaveEngineStateChange(const DebuggerState &newState);
    void handleSlaveEngineStateChangeAsActive(const DebuggerState &newState);

private:
    QmlEngine *m_qmlEngine;
    DebuggerEngine *m_cppEngine;
    DebuggerEngine *m_activeEngine;
    bool m_shutdownOk;
    bool m_shutdownDeferred;
    bool m_shutdownDone;
    bool m_isInitialStartup;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLGDBENGINE_H
