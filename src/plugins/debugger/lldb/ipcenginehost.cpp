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

#include "ipcenginehost.h"
#include "ipcengineguest.h"
#include "breakhandler.h"
#include "breakpoint.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "threadshandler.h"
#include "debuggeragents.h"
#include "debuggerstreamops.h"

#include <QSysInfo>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>
#include <utils/qtcassert.h>

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define SET_NATIVE_BYTE_ORDER(x) x.setByteOrder(QDataStream::LittleEndian)
#else
#define SET_NATIVE_BYTE_ORDER(x) x.setByteOrder(QDataStream::BigEndian)
#endif

namespace Debugger {
namespace Internal {

IPCEngineHost::IPCEngineHost (const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
    , m_local_guest(0)
    , m_nextMessagePayloadSize(0)
    , m_cookie(1)
    , m_device(0)
{
    connect(this, SIGNAL(stateChanged(DebuggerState)), this, SLOT(m_stateChanged(DebuggerState)));
}

IPCEngineHost::~IPCEngineHost()
{
    delete m_device;
}

void IPCEngineHost::setLocalGuest(IPCEngineGuest *g)
{
    m_local_guest = g;
}

void IPCEngineHost::setGuestDevice(QIODevice *d)
{
    if (m_device) {
        disconnect(m_device, SIGNAL(readyRead()), this, SLOT(readyRead()));
        delete m_device;
    }
    m_device = d;
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
        s << startParameters().environment;
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

void IPCEngineHost::executeRunToLine(const QString &fileName, int lineNumber)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << fileName;
        s << quint64(lineNumber);
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

void IPCEngineHost::executeJumpToLine(const QString &fileName, int lineNumber)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << fileName;
        s << quint64(lineNumber);
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

void IPCEngineHost::selectThread(int index)
{
    resetLocation();
    Threads threads = threadsHandler()->threads();
    QTC_ASSERT(index < threads.size(), return);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << quint64(threads.at(index).id);
    }
    rpcCall(SelectThread, p);
}

void IPCEngineHost::fetchDisassembler(Internal::DisassemblerViewAgent *v)
{
    quint64 address = v->frame().address;
    m_frameToDisassemblerAgent.insert(address, v);
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << address;
    }
    rpcCall(Disassemble, p);
}


void IPCEngineHost::addBreakpoint(const Internal::BreakpointData &bp)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << bp;
    }
    rpcCall(AddBreakpoint, p);
}

void IPCEngineHost::removeBreakpoint(quint64 id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(RemoveBreakpoint, p);
}

void IPCEngineHost::changeBreakpoint(const Internal::BreakpointData &bp)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << bp;
    }
    rpcCall(RemoveBreakpoint, p);
}

void IPCEngineHost::updateWatchData(const Internal::WatchData &data,
            const Internal::WatchUpdateFlags &flags)
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

void IPCEngineHost::rpcCallback(quint64 f, QByteArray payload)
{
    switch (f) {
        default:
            showMessage(QLatin1String("IPC Error: unhandled id in guest to host call"));
            showMessage(tr("Fatal engine shutdown. Incompatible binary or ipc error."), LogError);
            showStatusMessage(tr("Fatal engine shutdown. Incompatible binary or ipc error."));
            notifyEngineSpontaneousShutdown();
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
                gotoLocation(sh->currentFrame(), true);
            }
            break;
        case IPCEngineGuest::CurrentThreadChanged:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 token;
                s >> token;
                threadsHandler()->setCurrentThreadId(token);
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
                QString da;
                s >> pc;
                s >> da;
                Internal::DisassemblerViewAgent *view = m_frameToDisassemblerAgent.take(pc);
                if (view)
                    view->setContents(da);
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
                for (qint64 i = 0; i < count; i++) {
                    WatchData d;
                    s >> d;
                    wd.append(d);
                }
                WatchHandler *wh = watchHandler();
                if (!wh)
                    break;
                wh->beginCycle(fullCycle);
                wh->insertBulkData(wd);
                wh->endCycle(fullCycle);
            }
            break;
        case IPCEngineGuest::NotifyAddBreakpointOk:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                notifyBreakpointInsertOk(id);
            }
        case IPCEngineGuest::NotifyAddBreakpointFailed:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                notifyBreakpointInsertFailed(id);
            }
        case IPCEngineGuest::NotifyRemoveBreakpointOk:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                notifyBreakpointRemoveOk(id);
            }
        case IPCEngineGuest::NotifyRemoveBreakpointFailed:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                notifyBreakpointRemoveFailed(id);
            }
        case IPCEngineGuest::NotifyChangeBreakpointOk:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                notifyBreakpointChangeOk(id);
            }
        case IPCEngineGuest::NotifyChangeBreakpointFailed:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                notifyBreakpointChangeFailed(id);
            }
        case IPCEngineGuest::NotifyBreakpointAdjusted:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                BreakpointData d;
                s >> d;
                qDebug() << "FIXME";
                //notifyBreakpointAdjusted(d);
            }
    }
}

void IPCEngineHost::m_stateChanged(const DebuggerState &state)
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
    if (m_local_guest) {
        QMetaObject::invokeMethod(m_local_guest,
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
    }
}

void IPCEngineHost::readyRead()
{
    QDataStream s(m_device);
    SET_NATIVE_BYTE_ORDER(s);
    if (!m_nextMessagePayloadSize) {
        if (quint64(m_device->bytesAvailable ()) < (sizeof(quint64) * 3))
            return;
        s >> m_nextMessageCookie;
        s >> m_nextMessageFunction;
        s >> m_nextMessagePayloadSize;
        m_nextMessagePayloadSize += 1; // terminator and "got header" marker
    }

    quint64 ba = m_device->bytesAvailable();
    if (ba < m_nextMessagePayloadSize)
        return;

    QByteArray payload = m_device->read(m_nextMessagePayloadSize - 1);

    char terminator;
    m_device->getChar(&terminator);
    if (terminator != 'T') {
        showStatusMessage(tr("Fatal engine shutdown. Incompatible binary or ipc error."));
        showMessage(QLatin1String("IPC Error: terminator missing"));
        notifyEngineSpontaneousShutdown();
        return;
    }
    rpcCallback(m_nextMessageFunction, payload);
    m_nextMessagePayloadSize = 0;
    if (quint64(m_device->bytesAvailable()) >= (sizeof(quint64) * 3))
        QTimer::singleShot(0, this, SLOT(readyRead()));
}


} // namespace Internal
} // namespace Debugger


