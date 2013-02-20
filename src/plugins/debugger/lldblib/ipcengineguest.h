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

#ifndef DEBUGGER_IPCENGINE_H
#define DEBUGGER_IPCENGINE_H

#include "breakhandler.h"
#include "debuggerengine.h"
#include "disassemblerlines.h"
#include "stackhandler.h"
#include "threadshandler.h"

#include <QQueue>
#include <QThread>
#include <QVariant>

namespace Debugger {
namespace Internal {

class IPCEngineHost;
class IPCEngineGuest : public QObject
{
    Q_OBJECT

public:
    IPCEngineGuest();
    virtual ~IPCEngineGuest();

    void setLocalHost(IPCEngineHost *);
    void setHostDevice(QIODevice *);

    virtual void nuke() = 0;
    virtual void setupEngine() = 0;
    virtual void setupInferior(const QString &executeable,
            const QStringList &arguments, const QStringList &environment) = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;
    virtual void detachDebugger() = 0;
    virtual void executeStep() = 0;
    virtual void executeStepOut() = 0;
    virtual void executeNext() = 0;
    virtual void executeStepI() = 0;
    virtual void executeNextI() = 0;
    virtual void continueInferior() = 0;
    virtual void interruptInferior() = 0;
    virtual void executeRunToLine(const ContextData &data) = 0;
    virtual void executeRunToFunction(const QString &functionName) = 0;
    virtual void executeJumpToLine(const ContextData &data) = 0;
    virtual void activateFrame(qint64 token) = 0;
    virtual void selectThread(qint64 token) = 0;
    virtual void disassemble(quint64 pc) = 0;
    virtual void addBreakpoint(BreakpointModelId id, const BreakpointParameters &bp) = 0;
    virtual void removeBreakpoint(BreakpointModelId id) = 0;
    virtual void changeBreakpoint(BreakpointModelId id, const BreakpointParameters &bp) = 0;
    virtual void requestUpdateWatchData(const WatchData &data,
            const WatchUpdateFlags & flags = WatchUpdateFlags()) = 0;
    virtual void fetchFrameSource(qint64 frame) = 0;

    enum Function
    {
        NotifyEngineSetupOk              = 1,
        NotifyEngineSetupFailed          = 2,
        NotifyEngineRunFailed            = 3,
        NotifyInferiorSetupOk            = 4,
        NotifyInferiorSetupFailed        = 5,
        NotifyEngineRunAndInferiorRunOk  = 6,
        NotifyEngineRunAndInferiorStopOk = 7,
        NotifyInferiorRunRequested       = 8,
        NotifyInferiorRunOk              = 9,
        NotifyInferiorRunFailed          = 10,
        NotifyInferiorStopOk             = 11,
        NotifyInferiorSpontaneousStop    = 12,
        NotifyInferiorStopFailed         = 13,
        NotifyInferiorExited             = 14,
        NotifyInferiorShutdownOk         = 15,
        NotifyInferiorShutdownFailed     = 16,
        NotifyEngineSpontaneousShutdown  = 17,
        NotifyEngineShutdownOk           = 18,
        NotifyEngineShutdownFailed       = 19,
        NotifyInferiorIll                = 20,
        NotifyEngineIll                  = 21,
        NotifyInferiorPid                = 22,
        ShowStatusMessage                = 23,
        ShowMessage                      = 24,
        CurrentFrameChanged              = 25,
        CurrentThreadChanged             = 26,
        ListFrames                       = 27,
        ListThreads                      = 28,
        Disassembled                     = 29,
        NotifyAddBreakpointOk            = 30,
        NotifyAddBreakpointFailed        = 31,
        NotifyRemoveBreakpointOk         = 32,
        NotifyRemoveBreakpointFailed     = 33,
        NotifyChangeBreakpointOk         = 34,
        NotifyChangeBreakpointFailed     = 35,
        NotifyBreakpointAdjusted         = 36,
        UpdateWatchData                  = 47,
        FrameSourceFetched               = 48
    };
    Q_ENUMS(Function)

    DebuggerState state() const;
    void notifyEngineSetupOk();
    void notifyEngineSetupFailed();
    void notifyEngineRunFailed();
    void notifyInferiorSetupOk();
    void notifyInferiorSetupFailed();
    void notifyEngineRunAndInferiorRunOk();
    void notifyEngineRunAndInferiorStopOk();
    void notifyInferiorRunRequested();
    void notifyInferiorRunOk();
    void notifyInferiorRunFailed();
    void notifyInferiorStopOk();
    void notifyInferiorSpontaneousStop();
    void notifyInferiorStopFailed();
    void notifyInferiorExited();
    void notifyInferiorShutdownOk();
    void notifyInferiorShutdownFailed();
    void notifyEngineSpontaneousShutdown();
    void notifyEngineShutdownOk();
    void notifyEngineShutdownFailed();
    void notifyInferiorIll();
    void notifyEngineIll();
    void notifyInferiorPid(qint64 pid);
    void showMessage(const QString &msg, quint16 channel = LogDebug, quint64 timeout = quint64(-1));
    void showStatusMessage(const QString &msg, quint64 timeout = quint64(-1));

    void currentFrameChanged(qint64 token);
    void currentThreadChanged(qint64 token);
    void listFrames(const StackFrames &);
    void listThreads(const Threads &);
    void disassembled(quint64 pc, const DisassemblerLines &da);

    void notifyAddBreakpointOk(BreakpointModelId id);
    void notifyAddBreakpointFailed(BreakpointModelId id);
    void notifyRemoveBreakpointOk(BreakpointModelId id);
    void notifyRemoveBreakpointFailed(BreakpointModelId id);
    void notifyChangeBreakpointOk(BreakpointModelId id);
    void notifyChangeBreakpointFailed(BreakpointModelId id);
    void notifyBreakpointAdjusted(BreakpointModelId id, const BreakpointParameters &bp);

    void updateWatchData(bool fullCycle, const QList<WatchData> &);

    void frameSourceFetched(qint64 frame, const QString &name, const QString &sourceCode);

    void rpcCall(Function f, QByteArray payload = QByteArray());
public slots:
    void rpcCallback(quint64 f, QByteArray payload = QByteArray());
private slots:
    void readyRead();
private:
    IPCEngineHost *m_local_host;
    quint64 m_nextMessageCookie;
    quint64 m_nextMessageFunction;
    quint64 m_nextMessagePayloadSize;
    quint64 m_cookie;
    QIODevice *m_device;
    DebuggerState m_state;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE_H
