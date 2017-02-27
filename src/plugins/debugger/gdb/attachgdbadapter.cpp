/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "attachgdbadapter.h"

#include <coreplugin/messagebox.h>

#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

GdbAttachEngine::GdbAttachEngine(const DebuggerRunParameters &startParameters)
    : GdbEngine(startParameters)
{
}

void GdbAttachEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage("TRYING TO START ADAPTER");

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
    const qint64 pid = runParameters().attachPID.pid();
    showStatusMessage(tr("Attaching to process %1.").arg(pid));
    runCommand({"attach " + QString::number(pid),
                [this](const DebuggerResponse &r) { handleAttach(r); }});
    // In some cases we get only output like
    //   "Could not attach to process.  If your uid matches the uid of the target\n"
    //   "process, check the setting of /proc/sys/kernel/yama/ptrace_scope, or try\n"
    //   " again as the root user.  For more details, see /etc/sysctl.d/10-ptrace.conf\n"
    //   " ptrace: Operation not permitted.\n"
    // but no(!) ^ response. Use a second command to force *some* output
    runCommand({"print 24"});
}

void GdbAttachEngine::handleAttach(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested || state() == InferiorStopOk,
               qDebug() << state());
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning:
        showMessage("INFERIOR ATTACHED");
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
                          .arg(QString(response.data["msg"].data())));
        notifyEngineIll();
    }
}

void GdbAttachEngine::interruptInferior2()
{
    interruptLocalInferior(runParameters().attachPID.pid());
}

void GdbAttachEngine::shutdownEngine()
{
    notifyAdapterShutdownOk();
}

} // namespace Internal
} // namespace Debugger
