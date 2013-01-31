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

#ifndef DEBUGGER_IPCENGINE_HOST_H
#define DEBUGGER_IPCENGINE_HOST_H

#include "debuggerengine.h"
#include "threadshandler.h"
#include "stackhandler.h"
#include "breakhandler.h"
#include "sourceagent.h"

#include <QQueue>
#include <QVariant>
#include <QThread>

namespace Debugger {
namespace Internal {

class IPCEngineGuest;
class IPCEngineHost : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit IPCEngineHost(const DebuggerStartParameters &startParameters);
    ~IPCEngineHost();

    // use either one
    void setLocalGuest(IPCEngineGuest *);
    void setGuestDevice(QIODevice *);

    enum Function
    {
        SetupIPC               = 1,
        StateChanged           = 2,
        SetupEngine            = 3,
        SetupInferior          = 4,
        RunEngine              = 5,
        ShutdownInferior       = 6,
        ShutdownEngine         = 7,
        DetachDebugger         = 8,
        ExecuteStep            = 9,
        ExecuteStepOut         = 10,
        ExecuteNext            = 11,
        ExecuteStepI           = 12,
        ExecuteNextI           = 13,
        ContinueInferior       = 14,
        InterruptInferior      = 15,
        ExecuteRunToLine       = 16,
        ExecuteRunToFunction   = 17,
        ExecuteJumpToLine      = 18,
        ActivateFrame          = 19,
        SelectThread           = 20,
        Disassemble            = 21,
        AddBreakpoint          = 22,
        RemoveBreakpoint       = 23,
        ChangeBreakpoint       = 24,
        RequestUpdateWatchData = 25,
        FetchFrameSource       = 26
    };
    Q_ENUMS(Function)

    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();
    void detachDebugger();
    void executeStep();
    void executeStepOut() ;
    void executeNext();
    void executeStepI();
    void executeNextI();
    void continueInferior();
    void interruptInferior();
    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);
    void activateFrame(int index);
    void selectThread(ThreadId index);
    void fetchDisassembler(DisassemblerAgent *);
    bool acceptsBreakpoint(BreakpointModelId) const { return true; } // FIXME
    void insertBreakpoint(BreakpointModelId id);
    void removeBreakpoint(BreakpointModelId id);
    void changeBreakpoint(BreakpointModelId id);
    void updateWatchData(const WatchData &data,
            const WatchUpdateFlags &flags = WatchUpdateFlags());
    void fetchFrameSource(qint64 id);
    bool hasCapability(unsigned) const { return false; }

    void rpcCall(Function f, QByteArray payload = QByteArray());
protected:
    virtual void nuke() = 0;
public slots:
    void rpcCallback(quint64 f, QByteArray payload = QByteArray());
private slots:
    void m_stateChanged(Debugger::DebuggerState state);
    void readyRead();
private:
    IPCEngineGuest *m_localGuest;
    quint64 m_nextMessageCookie;
    quint64 m_nextMessageFunction;
    quint64 m_nextMessagePayloadSize;
    quint64 m_cookie;
    QIODevice *m_device;
    QHash<quint64, DisassemblerAgent *> m_frameToDisassemblerAgent;
    QHash<QString, SourceAgent *> m_sourceAgents;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE_H
