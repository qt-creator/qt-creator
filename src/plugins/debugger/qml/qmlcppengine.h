#ifndef QMLGDBENGINE_H
#define QMLGDBENGINE_H

#include "debuggerengine.h"

#include <QtCore/QScopedPointer>

namespace Core {
class IEditor;
}

namespace Debugger {
namespace Internal {

class QmlCppEnginePrivate;

class DEBUGGER_EXPORT QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT
public:
    explicit QmlCppEngine(const DebuggerStartParameters &sp);
    virtual ~QmlCppEngine();

    DebuggerLanguage activeEngine() const;
    void setActiveEngine(DebuggerLanguage language);

    virtual void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor * editor, int cursorPos);
    virtual void updateWatchData(const WatchData &data,
        const WatchUpdateFlags &flags);

    virtual void watchPoint(const QPoint &);
    virtual void fetchMemory(MemoryAgent *, QObject *,
            quint64 addr, quint64 length);
    virtual void fetchDisassembler(DisassemblerAgent *);
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

    virtual bool isSynchronous() const;
    virtual QByteArray qtNamespace() const;

    virtual void createSnapshot();
    virtual void updateAll();

    virtual void attemptBreakpointSynchronization();
    virtual bool acceptsBreakpoint(BreakpointId id) const;
    virtual void selectThread(int index);

    virtual void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);

    QAbstractItemModel *modulesModel() const;
    QAbstractItemModel *registerModel() const;
    QAbstractItemModel *stackModel() const;
    QAbstractItemModel *threadsModel() const;
    QAbstractItemModel *localsModel() const;
    QAbstractItemModel *watchersModel() const;
    QAbstractItemModel *returnModel() const;
    QAbstractItemModel *sourceFilesModel() const;

    DebuggerEngine *cppEngine() const;
    virtual void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    virtual void handleRemoteSetupFailed(const QString &message);

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
    void cppEngineStateChanged(const DebuggerState &state);
    void qmlEngineStateChanged(const DebuggerState &state);
    void setupSlaveEngine();
    void editorChanged(Core::IEditor *editor);

private:
    void initEngineShutdown();
    bool checkErrorState(const DebuggerState stateToCheck);
    void engineStateChanged(const DebuggerState &newState);

private:
    QScopedPointer<QmlCppEnginePrivate> d;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLGDBENGINE_H
