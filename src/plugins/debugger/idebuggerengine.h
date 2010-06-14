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

#ifndef DEBUGGER_IDEBUGGERENGINE_H
#define DEBUGGER_IDEBUGGERENGINE_H

#include "debuggerconstants.h"

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QPoint;
class QString;
QT_END_NAMESPACE

namespace TextEditor {
class ITextEditor;
}

namespace Core {
class IOptionsPage;
}

namespace Debugger {

class DebuggerManager;
class DebuggerRunControl;

namespace Internal {

class DisassemblerViewAgent;
class MemoryViewAgent;
class Symbol;
class WatchData;

class IDebuggerEngine : public QObject
{
    Q_OBJECT

public:
    IDebuggerEngine(DebuggerManager *manager, QObject *parent = 0)
        : QObject(parent), m_manager(manager), m_runControl()
    {}

    // FIXME: Move this to DebuggerEngineFactory::create(); ?
    void setRunControl(DebuggerRunControl *runControl)
        { m_runControl = runControl; }
    DebuggerRunControl *runControl() const
        { return m_runControl; }

    virtual void shutdown() = 0;
    virtual void setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos) = 0;
    virtual void startDebugger() = 0;
    virtual void exitDebugger() = 0;
    virtual void abortDebugger() { exitDebugger(); }
    virtual void detachDebugger() {}
    virtual void updateWatchData(const WatchData &data) = 0;

    virtual void executeStep() = 0;
    virtual void executeStepOut() = 0;
    virtual void executeNext() = 0;
    virtual void executeStepI() = 0;
    virtual void executeNextI() = 0;
    virtual void executeReturn() {}

    virtual void continueInferior() = 0;
    virtual void interruptInferior() = 0;

    virtual void executeRunToLine(const QString &fileName, int lineNumber) = 0;
    virtual void executeRunToFunction(const QString &functionName) = 0;
    virtual void executeJumpToLine(const QString &fileName, int lineNumber) = 0;
    virtual void assignValueInDebugger(const QString &expr, const QString &value) = 0;
    virtual void executeDebuggerCommand(const QString &command) = 0;

    virtual void activateFrame(int index) = 0;
    virtual void selectThread(int index) = 0;

    virtual void makeSnapshot() {}
    virtual void activateSnapshot(int index) { Q_UNUSED(index); }

    virtual void attemptBreakpointSynchronization() = 0;

    virtual void reloadModules() = 0;
    virtual void loadSymbols(const QString &moduleName) = 0;
    virtual void loadAllSymbols() = 0;
    virtual void requestModuleSymbols(const QString &moduleName) = 0;

    virtual void reloadRegisters() = 0;

    virtual void reloadSourceFiles() = 0;
    virtual void reloadFullStack() = 0;

    virtual void watchPoint(const QPoint &) {}
    virtual void fetchMemory(MemoryViewAgent *, QObject *,
            quint64 addr, quint64 length);
    virtual void fetchDisassembler(DisassemblerViewAgent *) {}
    virtual void setRegisterValue(int regnr, const QString &value);
    virtual void addOptionPages(QList<Core::IOptionsPage*> *) const {}
    virtual unsigned debuggerCapabilities() const { return 0; }

    virtual bool checkConfiguration(int toolChain,
        QString *errorMessage, QString *settingsPage = 0) const;

    virtual bool isSynchroneous() const { return false; }
    virtual QString qtNamespace() const { return QString(); }

    // Convenience
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1) const;
    void showStatusMessage(const QString &msg, int timeout = -1) const
        { showMessage(msg, StatusBar, timeout); }
    DebuggerManager *manager() const { return m_manager; }

protected:
    DebuggerState state() const;
    void setState(DebuggerState state, bool forced = false);
    DebuggerManager *m_manager;
    DebuggerRunControl *m_runControl;

signals:
    void startSuccessful();
    void startFailed();
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_IDEBUGGERENGINE_H
