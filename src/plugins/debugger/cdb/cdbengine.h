/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_CDBENGINE_H
#define DEBUGGER_CDBENGINE_H

#include "debuggerengine.h"

#include <QtCore/QSharedPointer>

namespace Debugger {
namespace Internal {

class DisassemblerViewAgent;
class CdbDebugEventCallback;
class CdbDebugOutput;
class CdbEnginePrivate;
struct CdbOptions;

class CdbEngine : public DebuggerEngine
{
    Q_OBJECT
    explicit CdbEngine(const DebuggerStartParameters &sp);

public:
    ~CdbEngine();

    // Factory function that returns 0 if the debug engine library cannot be found.
    static DebuggerEngine *create(const DebuggerStartParameters &sp,
                                   QString *errorMessage);

    virtual void setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);
    virtual void setupEngine();
    virtual void setupInferior();
    virtual void runEngine();
    virtual void shutdownInferior();
    virtual void shutdownEngine();
    virtual void detachDebugger();
    virtual void updateWatchData(const WatchData &data, const WatchUpdateFlags &flags);
    virtual unsigned debuggerCapabilities() const;

    virtual void executeStep();
    virtual void executeStepOut();
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();

    virtual void continueInferior();
    virtual void interruptInferior();

    virtual void executeRunToLine(const QString &fileName, int lineNumber);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const QString &fileName, int lineNumber);
    virtual void assignValueInDebugger(const WatchData *w, const QString &expr, const QVariant &value);
    virtual void executeDebuggerCommand(const QString &command);

    virtual void activateFrame(int index);
    virtual void selectThread(int index);

    virtual bool stateAcceptsBreakpointChanges() const;
    virtual bool acceptsBreakpoint(BreakpointId id) const;
    virtual void attemptBreakpointSynchronization();

    virtual void setRegisterValue(int regnr, const QString &value);
    virtual void fetchDisassembler(DisassemblerViewAgent *agent);
    virtual void fetchMemory(MemoryViewAgent *, QObject *, quint64 addr, quint64 length);

    virtual void reloadModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack() {}

public slots:
    void syncDebuggerPaths();

private slots:
    void slotConsoleStubStarted();
    void slotConsoleStubMessage(const QString &msg, bool);
    void slotConsoleStubTerminated();
    void slotBreakAttachToCrashed();
    void warning(const QString &w);

private:
    inline bool startAttachDebugger(qint64 pid, DebuggerStartMode sm, QString *errorMessage);
    void processTerminated(unsigned long exitCode);
    void evaluateWatcher(WatchData *wd);
    QString editorToolTip(const QString &exp, const QString &function);
    bool step(unsigned long executionStatus);
    bool attemptBreakpointSynchronizationI(QString *errorMessage);

    CdbEnginePrivate *m_d;

    friend class CdbEnginePrivate;
    friend class CdbDebugEventCallback;
    friend class CdbDebugOutput;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINE_H
