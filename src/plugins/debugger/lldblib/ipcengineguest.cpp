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

#include "ipcengineguest.h"
#include "ipcenginehost.h"
#include "breakpoint.h"
#include "stackframe.h"
#include "threaddata.h"
#include "debuggerstreamops.h"

#include <utils/qtcassert.h>

#include <QLocalSocket>

#include <QSysInfo>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define SET_NATIVE_BYTE_ORDER(x) x.setByteOrder(QDataStream::LittleEndian)
#else
#define SET_NATIVE_BYTE_ORDER(x) x.setByteOrder(QDataStream::BigEndian)
#endif

namespace Debugger {
namespace Internal {

IPCEngineGuest::IPCEngineGuest()
    : QObject()
    , m_local_host(0)
    , m_nextMessagePayloadSize(0)
    , m_cookie(1)
    , m_device(0)
{
}

IPCEngineGuest::~IPCEngineGuest()
{
}

void IPCEngineGuest::setLocalHost(IPCEngineHost *host)
{
    m_local_host = host;
}

void IPCEngineGuest::setHostDevice(QIODevice *device)
{
    if (m_device) {
        disconnect(m_device, SIGNAL(readyRead()), this, SLOT(readyRead()));
        delete m_device;
    }
    m_device = device;
    if (m_device)
        connect(m_device, SIGNAL(readyRead()), SLOT(readyRead()));
}

void IPCEngineGuest::rpcCall(Function f, QByteArray payload)
{
#if 0
    if (m_local_host) {
        QMetaObject::invokeMethod(m_local_host,
                "rpcCallback",
                Qt::QueuedConnection,
                Q_ARG(quint64, f),
                Q_ARG(QByteArray, payload));
    } else
#endif
    if (m_device) {
        {
            QDataStream s(m_device);
            SET_NATIVE_BYTE_ORDER(s);
            s << m_cookie++;
            s << quint64(f);
            s << quint64(payload.size());
        }
        m_device->write(payload);
        m_device->putChar('T');
        QLocalSocket *sock = qobject_cast<QLocalSocket *>(m_device);
        if (sock)
            sock->flush();
    }
}

void IPCEngineGuest::readyRead()
{
    if (!m_nextMessagePayloadSize) {
        if (quint64(m_device->bytesAvailable()) < 3 * sizeof(quint64))
            return;
        QDataStream s(m_device);
        SET_NATIVE_BYTE_ORDER(s);
        s >> m_nextMessageCookie;
        s >> m_nextMessageFunction;
        s >> m_nextMessagePayloadSize;
        m_nextMessagePayloadSize += 1; // terminator and "got header" marker
    }

    quint64 ba = m_device->bytesAvailable();
    if (ba < m_nextMessagePayloadSize)
        return;

    qint64 rrr = m_nextMessagePayloadSize;
    QByteArray payload = m_device->read(rrr);
    if (quint64(payload.size()) != m_nextMessagePayloadSize || !payload.endsWith('T')) {
        qDebug("IPC Error: corrupted frame");
        showMessage(QLatin1String("[guest] IPC Error: corrupted frame"), LogError);
        nuke();
        return;
    }
    payload.chop(1);
    rpcCallback(m_nextMessageFunction, payload);
    m_nextMessagePayloadSize = 0;

    if (quint64(m_device->bytesAvailable ()) >= 3 * sizeof(quint64))
        QTimer::singleShot(0, this, SLOT(readyRead()));
}

void IPCEngineGuest::rpcCallback(quint64 f, QByteArray payload)
{
    switch (f) {
        default:
            qDebug("IPC Error: unhandled id in host to guest call");
            showMessage(QLatin1String("IPC Error: unhandled id in host to guest call"), LogError);
            nuke();
            break;
        case IPCEngineHost::SetupIPC:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                int version;
                s >> version;
                Q_ASSERT(version == 1);
            }
            break;
        case IPCEngineHost::StateChanged:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 st;
                s >> st;
                m_state = (DebuggerState)st;
            }
            break;
        case IPCEngineHost::SetupEngine:
            setupEngine();
            break;
        case IPCEngineHost::SetupInferior:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                QString executable;
                QStringList arguments;
                QStringList environment;
                s >> executable;
                s >> arguments;
                s >> environment;
                setupInferior(executable, arguments, environment);
            }
            break;
        case IPCEngineHost::RunEngine:
            runEngine();
            break;
        case IPCEngineHost::ShutdownInferior:
            shutdownInferior();
            break;
        case IPCEngineHost::ShutdownEngine:
            shutdownEngine();
            break;
        case IPCEngineHost::DetachDebugger:
            detachDebugger();
            break;
        case IPCEngineHost::ExecuteStep:
            executeStep();
            break;
        case IPCEngineHost::ExecuteStepOut:
            executeStepOut();
            break;
        case IPCEngineHost::ExecuteNext:
            executeNext();
            break;
        case IPCEngineHost::ExecuteStepI:
            executeStepI();
            break;
        case IPCEngineHost::ExecuteNextI:
            executeNextI();
            break;
        case IPCEngineHost::ContinueInferior:
            continueInferior();
            break;
        case IPCEngineHost::InterruptInferior:
            interruptInferior();
            break;
        case IPCEngineHost::ExecuteRunToLine:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                ContextData data;
                s >> data.fileName;
                s >> data.lineNumber;
                executeRunToLine(data);
            }
            break;
        case IPCEngineHost::ExecuteRunToFunction:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                QString functionName;
                s >> functionName;
                executeRunToFunction(functionName);
            }
            break;
        case IPCEngineHost::ExecuteJumpToLine:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                ContextData data;
                s >> data.fileName;
                s >> data.lineNumber;
                executeJumpToLine(data);
            }
            break;
        case IPCEngineHost::ActivateFrame:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                activateFrame(id);
            }
            break;
        case IPCEngineHost::SelectThread:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 id;
                s >> id;
                selectThread(id);
            }
            break;
        case IPCEngineHost::Disassemble:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                quint64 pc;
                s >> pc;
                disassemble(pc);
            }
            break;
        case IPCEngineHost::AddBreakpoint:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                BreakpointModelId id;
                BreakpointParameters d;
                s >> id;
                s >> d;
                addBreakpoint(id, d);
            }
            break;
        case IPCEngineHost::RemoveBreakpoint:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                BreakpointModelId id;
                s >> id;
                removeBreakpoint(id);
            }
            break;
        case IPCEngineHost::ChangeBreakpoint:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                BreakpointModelId id;
                BreakpointParameters d;
                s >> id;
                s >> d;
                changeBreakpoint(id, d);
            }
            break;
        case IPCEngineHost::RequestUpdateWatchData:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                WatchData data;
                s >> data;
                requestUpdateWatchData(data);
            }
            break;
        case IPCEngineHost::FetchFrameSource:
            {
                QDataStream s(payload);
                SET_NATIVE_BYTE_ORDER(s);
                qint64 id;
                s >> id;
                fetchFrameSource(id);
            }
            break;
    };
}

DebuggerState IPCEngineGuest::state() const
{
    return m_state;
}

void IPCEngineGuest::notifyEngineSetupOk()
{
    rpcCall(NotifyEngineSetupOk);
}

void IPCEngineGuest::notifyEngineSetupFailed()
{
    rpcCall(NotifyEngineSetupFailed);
}

void IPCEngineGuest::notifyEngineRunFailed()
{
    rpcCall(NotifyEngineRunFailed);
}

void IPCEngineGuest::notifyInferiorSetupOk()
{
    rpcCall(NotifyInferiorSetupOk);
}

void IPCEngineGuest::notifyInferiorSetupFailed()
{
    rpcCall(NotifyInferiorSetupFailed);
}

void IPCEngineGuest::notifyEngineRunAndInferiorRunOk()
{
    rpcCall(NotifyEngineRunAndInferiorRunOk);
}

void IPCEngineGuest::notifyEngineRunAndInferiorStopOk()
{
    rpcCall(NotifyEngineRunAndInferiorStopOk);
}

void IPCEngineGuest::notifyInferiorRunRequested()
{
    rpcCall(NotifyInferiorRunRequested);
}

void IPCEngineGuest::notifyInferiorRunOk()
{
    rpcCall(NotifyInferiorRunOk);
}

void IPCEngineGuest::notifyInferiorRunFailed()
{
    rpcCall(NotifyInferiorRunFailed);
}

void IPCEngineGuest::notifyInferiorStopOk()
{
    rpcCall(NotifyInferiorStopOk);
}

void IPCEngineGuest::notifyInferiorSpontaneousStop()
{
    rpcCall(NotifyInferiorSpontaneousStop);
}

void IPCEngineGuest::notifyInferiorStopFailed()
{
    rpcCall(NotifyInferiorStopFailed);
}

void IPCEngineGuest::notifyInferiorExited()
{
    rpcCall(NotifyInferiorExited);
}

void IPCEngineGuest::notifyInferiorShutdownOk()
{
    rpcCall(NotifyInferiorShutdownOk);
}

void IPCEngineGuest::notifyInferiorShutdownFailed()
{
    rpcCall(NotifyInferiorShutdownFailed);
}

void IPCEngineGuest::notifyEngineSpontaneousShutdown()
{
    rpcCall(NotifyEngineSpontaneousShutdown);
}

void IPCEngineGuest::notifyEngineShutdownOk()
{
    rpcCall(NotifyEngineShutdownOk);
}

void IPCEngineGuest::notifyEngineShutdownFailed()
{
    rpcCall(NotifyEngineShutdownFailed);
}

void IPCEngineGuest::notifyInferiorIll()
{
    rpcCall(NotifyInferiorIll);
}

void IPCEngineGuest::notifyEngineIll()
{
    rpcCall(NotifyEngineIll);
}

void IPCEngineGuest::notifyInferiorPid(qint64 pid)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << pid;
    }
    rpcCall(NotifyInferiorPid, p);
}

void IPCEngineGuest::showStatusMessage(const QString &msg, quint64 timeout)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << msg;
        s << (qint64)timeout;
    }
    rpcCall(ShowStatusMessage, p);
}

void IPCEngineGuest::showMessage(const QString &msg, quint16 channel, quint64 timeout)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << msg;
        s << (qint64)channel;
        s << (qint64)timeout;
    }
    rpcCall(ShowMessage, p);
}

void IPCEngineGuest::currentFrameChanged(qint64 osid)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << osid;
    }
    rpcCall(CurrentFrameChanged, p);
}

void IPCEngineGuest::currentThreadChanged(qint64 osid)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << osid;
    }
    rpcCall(CurrentThreadChanged, p);
}

void IPCEngineGuest::listFrames(const StackFrames &frames)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << frames;
    }
    rpcCall(ListFrames, p);
}

void IPCEngineGuest::listThreads(const Threads &threads)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << threads;
    }
    rpcCall(ListThreads, p);
}

void IPCEngineGuest::disassembled(quint64 pc, const DisassemblerLines &da)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << pc;
        s << da;
    }
    rpcCall(Disassembled, p);
}

void IPCEngineGuest::notifyAddBreakpointOk(BreakpointModelId id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(NotifyAddBreakpointOk, p);
}

void IPCEngineGuest::notifyAddBreakpointFailed(BreakpointModelId id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(NotifyAddBreakpointFailed, p);
}

void IPCEngineGuest::notifyRemoveBreakpointOk(BreakpointModelId id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(NotifyRemoveBreakpointOk, p);
}

void IPCEngineGuest::notifyRemoveBreakpointFailed(BreakpointModelId id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(NotifyRemoveBreakpointFailed, p);
}

void IPCEngineGuest::notifyChangeBreakpointOk(BreakpointModelId id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(NotifyChangeBreakpointOk, p);
}

void IPCEngineGuest::notifyChangeBreakpointFailed(BreakpointModelId id)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
    }
    rpcCall(NotifyChangeBreakpointFailed, p);
}

void IPCEngineGuest::notifyBreakpointAdjusted(BreakpointModelId id,
    const BreakpointParameters &bp)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id << bp;
    }
    rpcCall(NotifyBreakpointAdjusted, p);
}

void IPCEngineGuest::updateWatchData(bool fullCycle, const QList<WatchData> &wd)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << fullCycle;
        s << quint64(wd.count());
        for (int i = 0; i < wd.count(); ++i)
            s << wd.at(i);
    }
    rpcCall(UpdateWatchData, p);
}

void IPCEngineGuest::frameSourceFetched(qint64 id, const QString &name, const QString &source)
{
    QByteArray p;
    {
        QDataStream s(&p, QIODevice::WriteOnly);
        SET_NATIVE_BYTE_ORDER(s);
        s << id;
        s << name;
        s << source;
    }
    rpcCall(FrameSourceFetched, p);
}

} // namespace Internal
} // namespace Debugger


