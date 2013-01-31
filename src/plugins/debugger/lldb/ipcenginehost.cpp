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

#include "ipcenginehost.h"

#include "ipcengineguest.h"
#include "debuggerstartparameters.h"
#include "breakhandler.h"
#include "breakpoint.h"
#include "disassemblerlines.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "threadshandler.h"
#include "disassembleragent.h"
#include "memoryagent.h"
#include "debuggerstreamops.h"
#include "debuggercore.h"

#include <utils/qtcassert.h>

#include <QSysInfo>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>
#include <QLocalSocket>

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define SET_NATIVE_BYTE_ORDER(x) x.setByteOrder(QDataStream::LittleEndian)
#else
#define SET_NATIVE_BYTE_ORDER(x) x.setByteOrder(QDataStream::BigEndian)
#endif

namespace Debugger {
namespace Internal {

IPCEngineHost::IPCEngineHost (const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
    , m_localGuest(0)
    , m_nextMessagePayloadSize(0)
    , m_cookie(1)
    , m_device(0)
{
    connect(this, SIGNAL(stateChanged(Debugger::DebuggerState)), SLOT(m_stateChanged(Debugger::DebuggerState)));
}

IPCEngineHost::~IPCEngineHost()
{
    delete m_device;
}

void IPCEngineHost::setLocalGuest(IPCEngineGuest *guest)
{
    m_localGuest = guest;
}

void IPCEngineHost::setGuestDevice(QIODevice *device)
{
    if (m_device) {
        disconnect(m_device, SIGNAL(readyRead()), this, SLOT(readyRead()));
        delete m_device;
    }
    m_device = device;
    if (m_device)
        connect(m_device, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

void IPCEngineHost::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    rpcCall(SetupEngine);
}

void IPCEngineHost::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << QFileInfo(startParameters().executable).absoluteFilePath();
        s << startParameters().processArgs;
        s << startParameters().environment.toStringList();
    }
    rpcCall(SetupInferior, p);
}

void IPCEngineHost::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    rpcCall(RunEngine);
}

void IPCEngineHost::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    rpcCall(ShutdownInferior);
}

void IPCEngineHost::shutdownEngine()
{
    rpcCall(ShutdownEngine);
}

void IPCEngineHost::detachDebugger()
{
    rpcCall(DetachDebugger);
}

void IPCEngineHost::executeStep()
{
    rpcCall(ExecuteStep);
}

void IPCEngineHost::executeStepOut()
{
    rpcCall(ExecuteStepOut);
}

void IPCEngineHost::executeNext()
{
    rpcCall(ExecuteNext);
}

void IPCEngineHost::executeStepI()
{
    rpcCall(ExecuteStepI);
}

void IPCEngineHost::executeNextI()
{
    rpcCall(ExecuteNextI);
}

void IPCEngineHost::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    resetLocation();
    rpcCall(ContinueInferior);
}

void IPCEngineHost::interruptInferior()
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    rpcCall(InterruptInferior);
}

void IPCEngineHost::executeRunToLine(const ContextData &data)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << data.fileName;
        s << quint64(data.lineNumber);
    }
    rpcCall(ExecuteRunToLine, p);
}

void IPCEngineHost::executeRunToFunction(const QString &functionName)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << functionName;
    }
    rpcCall(ExecuteRunToFunction, p);
}

void IPCEngineHost::executeJumpToLine(const ContextData &data)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << data.fileName;
        s << quint64(data.lineNumber);
    }
    rpcCall(ExecuteJumpToLine, p);
}

void IPCEngineHost::activateFrame(int index)
{
    resetLocation();
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << quint64(index);
    }
    rpcCall(ActivateFrame, p);
}

void IPCEngineHost::selectThread(ThreadId id)
{
    resetLocation();
    QTC_ASSERT(id.isValid(), return);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id.raw();
    }
    rpcCall(SelectThread, p);
}

void IPCEngineHost::fetchDisassembler(DisassemblerAgent *v)
{
    quint64 address = v->location().address();
    m_frameToDisassemblerAgent.insert(address, v);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << address;
    }
    rpcCall(Disassemble, p);
}

void IPCEngineHost::insertBreakpoint(BreakpointModelId id)
{
    breakHandler()->notifyBreakpointInsertProceeding(id);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
        s << breakHandler()->breakpointData(id);
    }
    rpcCall(AddBreakpoint, p);
}

void IPCEngineHost::removeBreakpoint(BreakpointModelId id)
{
    breakHandler()->notifyBreakpointRemoveProceeding(id);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(RemoveBreakpoint, p);
}

void IPCEngineHost::changeBreakpoint(BreakpointModelId id)
{
    breakHandler()->notifyBreakpointChangeProceeding(id);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
        s << breakHandler()->breakpointData(id);
    }
    rpcCall(RemoveBreakpoint, p);
}

void IPCEngineHost::updateWatchData(const WatchData &data,
            const WatchUpdateFlags &flags)
{
    Q_UNUSED(flags);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << data;
    }
    rpcCall(RequestUpdateWatchData, p);
}

void IPCEngineHost::fetchFrameSource(qint64 id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(FetchFrameSource, p);
}

void IPCEngineHost::rpcCallback(quint64 f, QByteArray payload)
{
    switch (f) {
        default: {
            showMessage(QLatin1String("IPC Error: unhandled id in guest to host call"));
            const QString logMessage = tr("Fatal engine shutdown. Incompatible binary or IPC error.");
            showMessage(logMessage, LogError);
            showStatusMessage(logMessage);
    }
            nuke();
            break;
        case IPCEngineGuest::NotifyEngineSetupOk:
            notifyEngineSetupOk();
            break;
        case IPCEngineGuest::NotifyEngineSetupFailed:
            notifyEngineSetupFailed();
            break;
        case IPCEngineGuest::NotifyEngineRunFailed:
            notifyEngineRunFailed();
            break;
        case IPCEngineGuest::NotifyInferiorSetupOk:
            attemptBreakpointSynchronization();
            notifyInferiorSetupOk();
            break;
        case IPCEngineGuest::NotifyInferiorSetupFailed:
            notifyInferiorSetupFailed();
            break;
        case IPCEngineGuest::NotifyEngineRunAndInferiorRunOk:
            notifyEngineRunAndInferiorRunOk();
            break;
        case IPCEngineGuest::NotifyEngineRunAndInferiorStopOk:
            notifyEngineRunAndInferiorStopOk();
            break;
        case IPCEngineGuest::NotifyInferiorRunRequested:
            notifyInferiorRunRequested();
            break;
        case IPCEngineGuest::NotifyInferiorRunOk:
            notifyInferiorRunOk();
            break;
        case IPCEngineGuest::NotifyInferiorRunFailed:
            notifyInferiorRunFailed();
            break;
        case IPCEngineGuest::NotifyInferiorStopOk:
            notifyInferiorStopOk();
            break;
        case IPCEngineGuest::NotifyInferiorSpontaneousStop:
            notifyInferiorSpontaneousStop();
            break;
        case IPCEngineGuest::NotifyInferiorStopFailed:
            notifyInferiorStopFailed();
            break;
        case IPCEngineGuest::NotifyInferiorExited:
            notifyInferiorExited();
            break;
        case IPCEngineGuest::NotifyInferiorShutdownOk:
            notifyInferiorShutdownOk();
            break;
        case IPCEngineGuest::NotifyInferiorShutdownFailed:
            notifyInferiorShutdownFailed();
            break;
        case IPCEngineGuest::NotifyEngineSpontaneousShutdown:
            notifyEngineSpontaneousShutdown();
            break;
        case IPCEngineGuest::NotifyEngineShutdownOk:
            notifyEngineShutdownOk();
            break;
        case IPCEngineGuest::NotifyEngineShutdownFailed:
            notifyEngineShutdownFailed();
            break;
        case IPCEngineGuest::NotifyInferiorIll:
            notifyInferiorIll();
            break;
        case IPCEngineGuest::NotifyEngineIll:
            notifyEngineIll();
            break;
        case IPCEngineGuest::NotifyInferiorPid:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 pid;
                s >> pid;
                notifyInferiorPid(pid);
            }
            break;
        case IPCEngineGuest::ShowStatusMessage:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                QString msg;
                qint64 timeout;
                s >> msg;
                s >> timeout;
                showStatusMessage(msg, timeout);
            }
            break;
        case IPCEngineGuest::ShowMessage:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                QString msg;
                qint16 channel;
                qint64 timeout;
                s >> msg;
                s >> channel;
                s >> timeout;
                showMessage(msg, channel, timeout);
            }
            break;
        case IPCEngineGuest::CurrentFrameChanged:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 token;
                s >> token;

                resetLocation();
                StackHandler *sh = stackHandler();
                sh->setCurrentIndex(token);
                if (!sh->currentFrame().isUsable() || QFileInfo(sh->currentFrame().file).exists())
                    gotoLocation(Location(sh->currentFrame(), true));
                else if (!m_sourceAgents.contains(sh->currentFrame().file))
                    fetchFrameSource(token);
                foreach (SourceAgent *agent, m_sourceAgents.values())
                    agent->updateLocationMarker();
            }
            break;
        case IPCEngineGuest::CurrentThreadChanged:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 token;
                s >> token;
                threadsHandler()->setCurrentThread(ThreadId(token));
            }
            break;
        case IPCEngineGuest::ListFrames:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                StackFrames frames;
                s >> frames;
                stackHandler()->setFrames(frames);
            }
            break;
        case IPCEngineGuest::ListThreads:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                Threads threads;
                s >> threads;
                threadsHandler()->setThreads(threads);
            }
            break;
        case IPCEngineGuest::Disassembled:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 pc;
                DisassemblerLines lines;
                s >> pc;
                s >> lines;
                DisassemblerAgent *view = m_frameToDisassemblerAgent.take(pc);
                if (view)
                    view->setContents(lines);
            }
            break;
        case IPCEngineGuest::UpdateWatchData:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                bool fullCycle;
                qint64 count;
                QList<WatchData> wd;
                s >> fullCycle;
                s >> count;
                for (qint64 i = 0; i < count; ++i) {
                    WatchData d;
                    s >> d;
                    wd.append(d);
                }
                WatchHandler *wh = watchHandler();
                if (!wh)
                    break;
                wh->insertData(wd);
            }
            break;
        case IPCEngineGuest::NotifyAddBreakpointOk:
            {
                attemptBreakpointSynchronization();
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 d;
                s >> d;
                BreakpointModelId id = BreakpointModelId::fromInternalId(d);
                breakHandler()->notifyBreakpointInsertOk(id);
            }
            break;
        case IPCEngineGuest::NotifyAddBreakpointFailed:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 d;
                s >> d;
                BreakpointModelId id = BreakpointModelId::fromInternalId(d);
                breakHandler()->notifyBreakpointInsertFailed(id);
            }
            break;
        case IPCEngineGuest::NotifyRemoveBreakpointOk:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 d;
                s >> d;
                BreakpointModelId id = BreakpointModelId::fromInternalId(d);
                breakHandler()->notifyBreakpointRemoveOk(id);
            }
            break;
        case IPCEngineGuest::NotifyRemoveBreakpointFailed:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 d;
                s >> d;
                BreakpointModelId id = BreakpointModelId::fromInternalId(d);
                breakHandler()->notifyBreakpointRemoveFailed(id);
            }
            break;
        case IPCEngineGuest::NotifyChangeBreakpointOk:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 d;
                s >> d;
                BreakpointModelId id = BreakpointModelId::fromInternalId(d);
                breakHandler()->notifyBreakpointChangeOk(id);
            }
            break;
        case IPCEngineGuest::NotifyChangeBreakpointFailed:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 d;
                s >> d;
                BreakpointModelId id = BreakpointModelId::fromInternalId(d);
                breakHandler()->notifyBreakpointChangeFailed(id);
            }
            break;
        case IPCEngineGuest::NotifyBreakpointAdjusted:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 dd;
                BreakpointParameters d;
                s >> dd >> d;
                BreakpointModelId id = BreakpointModelId::fromInternalId(dd);
                breakHandler()->notifyBreakpointAdjusted(id, d);
            }
            break;
        case IPCEngineGuest::FrameSourceFetched:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                qint64 token;
                QString path;
                QString source;
                s >> token >> path >> source;
                SourceAgent *agent = new SourceAgent(this);
                agent->setSourceProducerName(startParameters().connParams.host);
                agent->setContent(path, source);
                m_sourceAgents.insert(path, agent);
                agent->updateLocationMarker();
            }
            break;
    }
}

void IPCEngineHost::m_stateChanged(Debugger::DebuggerState state)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << (qint64)state;
    }
    rpcCall(StateChanged, p);

}

void IPCEngineHost::rpcCall(Function f, QByteArray payload)
{
    if (m_localGuest) {
        QMetaObject::invokeMethod(m_localGuest,
                "rpcCallback",
                Qt::QueuedConnection,
                Q_ARG(quint64, f),
                Q_ARG(QByteArray, payload));
    } else if (m_device) {
        QByteArray header;
        {
            QDataStream s(&header, QIODevice::WriteOnly);
            SET_NATIVE_BYTE_ORDER(s);
            s << m_cookie++;
            s << (quint64) f;
            s << (quint64) payload.size();
        }
        m_device->write(header);
        m_device->write(payload);
        m_device->putChar('T');
        QLocalSocket *sock = qobject_cast<QLocalSocket *>(m_device);
        if (sock)
            sock->flush();
    }
}

void IPCEngineHost::readyRead()
{
    QDataStream s(m_device);
    SET_NATIVE_BYTE_ORDER(s);
    if (!m_nextMessagePayloadSize) {
        if (quint64(m_device->bytesAvailable ()) < 3 * sizeof(quint64))
            return;
        s >> m_nextMessageCookie;
        s >> m_nextMessageFunction;
        s >> m_nextMessagePayloadSize;
        m_nextMessagePayloadSize += 1; // Terminator and "got header" marker.
    }

    quint64 ba = m_device->bytesAvailable();
    if (ba < m_nextMessagePayloadSize)
        return;

    QByteArray payload = m_device->read(m_nextMessagePayloadSize - 1);

    char terminator;
    m_device->getChar(&terminator);
    if (terminator != 'T') {
        showStatusMessage(tr("Fatal engine shutdown. Incompatible binary or IPC error."));
        showMessage(QLatin1String("IPC Error: terminator missing"));
        nuke();
        return;
    }
    rpcCallback(m_nextMessageFunction, payload);
    m_nextMessagePayloadSize = 0;
    if (quint64(m_device->bytesAvailable()) >= 3 * sizeof(quint64))
        QTimer::singleShot(0, this, SLOT(readyRead()));
}

} // namespace Internal
} // namespace Debugger


