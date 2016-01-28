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

#include "gdbplainengine.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Debugger {
namespace Internal {

#define CB(callback) [this](const DebuggerResponse &r) { callback(r); }

GdbPlainEngine::GdbPlainEngine(const DebuggerRunParameters &startParameters)
    : GdbEngine(startParameters)
{
    // Output
    connect(&m_outputCollector, &OutputCollector::byteDelivery,
            this, &GdbEngine::readDebugeeOutput);
}

void GdbPlainEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    setEnvironmentVariables();
    if (!runParameters().inferior.commandLineArguments.isEmpty()) {
        QString args = runParameters().inferior.commandLineArguments;
        runCommand({"-exec-arguments " + toLocalEncoding(args), NoFlags});
    }
    runCommand({"-file-exec-and-symbols \"" + execFilePath() + '"', NoFlags,
        CB(handleFileExecAndSymbols)});
}

void GdbPlainEngine::handleFileExecAndSymbols(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        handleInferiorPrepared();
    } else {
        QByteArray ba = response.data["msg"].data();
        QString msg = fromLocalEncoding(ba);
        // Extend the message a bit in unknown cases.
        if (!ba.endsWith("File format not recognized"))
            msg = tr("Starting executable failed:") + QLatin1Char('\n') + msg;
        notifyInferiorSetupFailed(msg);
    }
}

void GdbPlainEngine::runEngine()
{
    if (runParameters().useContinueInsteadOfRun)
        runCommand({"-exec-continue", RunRequest, CB(handleExecuteContinue)});
    else
        runCommand({"-exec-run", RunRequest, CB(handleExecRun)});
}
void GdbPlainEngine::handleExecRun(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == ResultRunning) {
        notifyEngineRunAndInferiorRunOk(); // For gdb < 7.0
        //showStatusMessage(tr("Running..."));
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgInferiorSetupOk(), StatusBar);
        // FIXME: That's the wrong place for it.
        if (boolSetting(EnableReverseDebugging))
            runCommand({"target record", NoFlags});
    } else {
        QString msg = fromLocalEncoding(response.data["msg"].data());
        //QTC_CHECK(status() == InferiorRunOk);
        //interruptInferior();
        showMessage(msg);
        notifyEngineRunFailed();
    }
}

void GdbPlainEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    if (!prepareCommand())
        return;

    QStringList gdbArgs;

    if (!m_outputCollector.listen()) {
        handleAdapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_outputCollector.errorString()));
        return;
    }
    gdbArgs.append(_("--tty=") + m_outputCollector.serverName());

    if (!runParameters().inferior.workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(runParameters().inferior.workingDirectory);

    startGdb(gdbArgs);
}

void GdbPlainEngine::handleGdbStartFailed()
{
    m_outputCollector.shutdown();
}

void GdbPlainEngine::interruptInferior2()
{
    interruptLocalInferior(inferiorPid());
}

void GdbPlainEngine::shutdownEngine()
{
    showMessage(_("PLAIN ADAPTER SHUTDOWN %1").arg(state()));
    m_outputCollector.shutdown();
    notifyAdapterShutdownOk();
}

QByteArray GdbPlainEngine::execFilePath() const
{
    return QFileInfo(runParameters().inferior.executable)
            .absoluteFilePath().toLocal8Bit();
}

QByteArray GdbPlainEngine::toLocalEncoding(const QString &s) const
{
    return s.toLocal8Bit();
}

QString GdbPlainEngine::fromLocalEncoding(const QByteArray &b) const
{
    return QString::fromLocal8Bit(b);
}

} // namespace Debugger
} // namespace Internal
