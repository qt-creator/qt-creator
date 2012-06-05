/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "attachgdbadapter.h"
#include "gdbmi.h"
#include "debuggerstartparameters.h"

#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>


namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&AttachGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// AttachGdbAdapter
//
///////////////////////////////////////////////////////////////////////

AttachGdbAdapter::AttachGdbAdapter(GdbEngine *engine)
    : AbstractGdbAdapter(engine)
{
}

void AttachGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));
    m_engine->startGdb();
}

void AttachGdbAdapter::handleGdbStartDone()
{
    m_engine->handleAdapterStarted();
}

void AttachGdbAdapter::handleGdbStartFailed()
{
}

void AttachGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    const qint64 pid = startParameters().attachPID;
    m_engine->postCommand("attach " + QByteArray::number(pid), CB(handleAttach));
    // Task 254674 does not want to remove them
    //qq->breakHandler()->removeAllBreakpoints();
}

void AttachGdbAdapter::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->showStatusMessage(tr("Attached to process %1.")
        .arg(m_engine->inferiorPid()));
    m_engine->notifyEngineRunAndInferiorStopOk();
    GdbMi data;
    m_engine->handleStop1(data);
}

void AttachGdbAdapter::handleAttach(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    switch (response.resultClass) {
    case GdbResultDone:
    case GdbResultRunning:
        showMessage(_("INFERIOR ATTACHED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        m_engine->handleInferiorPrepared();
        break;
    case GdbResultError:
        if (response.data.findChild("msg").data() == "ptrace: Operation not permitted.") {
            m_engine->notifyInferiorSetupFailed(DumperHelper::msgPtraceError(startParameters().startMode));
            break;
        }
        // if msg != "ptrace: ..." fall through
    default:
        QString msg = QString::fromLocal8Bit(response.data.findChild("msg").data());
        m_engine->notifyInferiorSetupFailed(msg);
    }
}

void AttachGdbAdapter::interruptInferior()
{
    interruptLocalInferior(startParameters().attachPID);
}

void AttachGdbAdapter::shutdownAdapter()
{
    m_engine->notifyAdapterShutdownOk();
}

} // namespace Internal
} // namespace Debugger
