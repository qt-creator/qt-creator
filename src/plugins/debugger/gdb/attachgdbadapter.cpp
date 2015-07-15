/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "attachgdbadapter.h"

#include <coreplugin/messagebox.h>

#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstringutils.h>
#include <debugger/debuggerstartparameters.h>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// AttachGdbAdapter
//
///////////////////////////////////////////////////////////////////////

GdbAttachEngine::GdbAttachEngine(const DebuggerRunParameters &startParameters)
    : GdbEngine(startParameters)
{
}

void GdbAttachEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    if (!runParameters().workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(runParameters().workingDirectory);

    startGdb();
}

void GdbAttachEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    // Task 254674 does not want to remove them
    //qq->breakHandler()->removeAllBreakpoints();
    handleInferiorPrepared();
}

void GdbAttachEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    const qint64 pid = runParameters().attachPID;
    postCommand("attach " + QByteArray::number(pid), NoFlags,
                [this](const DebuggerResponse &r) { handleAttach(r); });
    showStatusMessage(tr("Attached to process %1.").arg(inferiorPid()));
}

void GdbAttachEngine::handleAttach(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested || state() == InferiorStopOk,
               qDebug() << state());
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning:
        showMessage(_("INFERIOR ATTACHED"));
        if (state() == EngineRunRequested) {
            // Happens e.g. for "Attach to unstarted application"
            // We will get a '*stopped' later that we'll interpret as 'spontaneous'
            // So acknowledge the current state and put a delayed 'continue' in the pipe.
            showMessage(tr("Attached to running application"), StatusBar);
            notifyEngineRunAndInferiorRunOk();
        } else {
            // InferiorStopOk, e.g. for "Attach to running application".
            // The *stopped came in between sending the 'attach' and
            // receiving its '^done'.
            if (runParameters().continueAfterAttach)
                continueInferiorInternal();
        }
        break;
    case ResultError:
        if (response.data["msg"].data() == "ptrace: Operation not permitted.") {
            QString msg = msgPtraceError(runParameters().startMode);
            showStatusMessage(tr("Failed to attach to application: %1").arg(msg));
            Core::AsynchronousMessageBox::warning(tr("Debugger Error"), msg);
            notifyEngineIll();
            break;
        }
        // if msg != "ptrace: ..." fall through
    default:
        showStatusMessage(tr("Failed to attach to application: %1")
                          .arg(QString::fromLocal8Bit(response.data["msg"].data())));
        notifyEngineIll();
    }
}

void GdbAttachEngine::interruptInferior2()
{
    interruptLocalInferior(runParameters().attachPID);
}

void GdbAttachEngine::shutdownEngine()
{
    notifyAdapterShutdownOk();
}

} // namespace Internal
} // namespace Debugger
