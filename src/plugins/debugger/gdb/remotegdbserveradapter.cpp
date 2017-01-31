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

#include "remotegdbserveradapter.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/procinterrupt.h>

#include <coreplugin/messagebox.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QAbstractButton>
#include <QFileInfo>
#include <QMessageBox>

using namespace Utils;

namespace Debugger {
namespace Internal {

#define CB(callback) [this](const DebuggerResponse &r) { callback(r); }

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

GdbRemoteServerEngine::GdbRemoteServerEngine(const DebuggerRunParameters &runParameters)
    : GdbEngine(runParameters), m_startAttempted(false)
{
    if (HostOsInfo::isWindowsHost())
        m_gdbProc.setUseCtrlCStub(runParameters.useCtrlCStub); // This is only set for QNX

    connect(&m_uploadProc, &QProcess::errorOccurred, this, &GdbRemoteServerEngine::uploadProcError);
    connect(&m_uploadProc, &QProcess::readyReadStandardOutput,
            this, &GdbRemoteServerEngine::readUploadStandardOutput);
    connect(&m_uploadProc, &QProcess::readyReadStandardError,
            this, &GdbRemoteServerEngine::readUploadStandardError);
    connect(&m_uploadProc, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
            this, &GdbRemoteServerEngine::uploadProcFinished);
}

void GdbRemoteServerEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage("TRYING TO START ADAPTER");
    QString serverStartScript = runParameters().serverStartScript;
    if (!serverStartScript.isEmpty()) {

        // Provide script information about the environment
        QString arglist;
        QtcProcess::addArg(&arglist, serverStartScript);
        QtcProcess::addArg(&arglist, runParameters().inferior.executable);
        QtcProcess::addArg(&arglist, runParameters().remoteChannel);

        m_uploadProc.start(arglist);
        m_uploadProc.waitForStarted();
    }

    if (runParameters().remoteSetupNeeded) {
        notifyEngineRequestRemoteSetup();
    } else {
        m_startAttempted = true;
        startGdb();
    }
}

void GdbRemoteServerEngine::uploadProcError(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
        case QProcess::FailedToStart:
            msg = tr("The upload process failed to start. Shell missing?");
            break;
        case QProcess::Crashed:
            msg = tr("The upload process crashed some time after starting "
                "successfully.");
            break;
        case QProcess::Timedout:
            msg = tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
            break;
        case QProcess::WriteError:
            msg = tr("An error occurred when attempting to write "
                "to the upload process. For example, the process may not be running, "
                "or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            msg = tr("An error occurred when attempting to read from "
                "the upload process. For example, the process may not be running.");
            break;
        default:
            msg = tr("An unknown error in the upload process occurred. "
                "This is the default return value of error().");
    }

    showMessage(msg, StatusBar);
    Core::AsynchronousMessageBox::critical(tr("Error"), msg);
}

void GdbRemoteServerEngine::readUploadStandardOutput()
{
    const QByteArray ba = m_uploadProc.readAllStandardOutput();
    const QString msg = QString::fromLocal8Bit(ba, ba.length());
    showMessage(msg, LogOutput);
    showMessage(msg, AppOutput);
}

void GdbRemoteServerEngine::readUploadStandardError()
{
    const QByteArray ba = m_uploadProc.readAllStandardError();
    const QString msg = QString::fromLocal8Bit(ba, ba.length());
    showMessage(msg, LogOutput);
    showMessage(msg, AppError);
}

void GdbRemoteServerEngine::uploadProcFinished()
{
    if (m_uploadProc.exitStatus() == QProcess::NormalExit && m_uploadProc.exitCode() == 0) {
        if (!m_startAttempted)
            startGdb();
    } else {
        RemoteSetupResult result;
        result.success = false;
        result.reason = m_uploadProc.errorString();
        notifyEngineRemoteSetupFinished(result);
    }
}

void GdbRemoteServerEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    setLinuxOsAbi();
    const DebuggerRunParameters &rp = runParameters();
    QString symbolFile;
    if (!rp.symbolFile.isEmpty()) {
        QFileInfo fi(rp.symbolFile);
        symbolFile = fi.absoluteFilePath();
    }

    //const QByteArray sysroot = sp.sysroot.toLocal8Bit();
    //const QByteArray remoteArch = sp.remoteArchitecture.toLatin1();
    const QString args = isMasterEngine() ? runParameters().inferior.commandLineArguments
                          : masterEngine()->runParameters().inferior.commandLineArguments;

//    if (!remoteArch.isEmpty())
//        postCommand("set architecture " + remoteArch);
    const QString solibSearchPath = rp.solibSearchPath.join(HostOsInfo::pathListSeparator());
    if (!solibSearchPath.isEmpty())
        runCommand({"set solib-search-path " + solibSearchPath});

    if (!args.isEmpty())
        runCommand({"-exec-arguments " + args});

    setEnvironmentVariables();

    // This has to be issued before 'target remote'. On pre-7.0 the
    // command is not present and will result in ' No symbol table is
    // loaded.  Use the "file" command.' as gdb tries to set the
    // value of a variable with name 'target-async'.
    //
    // Testing with -list-target-features which was introduced at
    // the same time would not work either, as this need an existing
    // target.
    //
    // Using it even without a target and having it fail might still
    // be better as:
    // Some external comment: '[but] "set target-async on" with a native
    // windows gdb will work, but then fail when you actually do
    // "run"/"attach", I think..


    // gdb/mi/mi-main.c:1958: internal-error:
    // mi_execute_async_cli_command: Assertion `is_running (inferior_ptid)'
    // failed.\nA problem internal to GDB has been detected,[...]
    if (usesTargetAsync())
        runCommand({"set target-async on", CB(handleSetTargetAsync)});

    if (symbolFile.isEmpty()) {
        showMessage(tr("No symbol file given."), StatusBar);
        callTargetRemote();
        return;
    }

    if (!symbolFile.isEmpty()) {
        runCommand({"-file-exec-and-symbols \"" + symbolFile + '"',
                    CB(handleFileExecAndSymbols)});
    }
}

void GdbRemoteServerEngine::handleSetTargetAsync(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultError)
        qDebug() << "Adapter too old: does not support asynchronous mode.";
}

void GdbRemoteServerEngine::handleFileExecAndSymbols(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        callTargetRemote();
    } else {
        QString reason = response.data["msg"].data();
        QString msg = tr("Reading debug information failed:") + '\n' + reason;
        if (reason.endsWith("No such file or directory.")) {
            showMessage("INFERIOR STARTUP: BINARY NOT FOUND");
            showMessage(msg, StatusBar);
            callTargetRemote(); // Proceed nevertheless.
        } else {
            notifyInferiorSetupFailed(msg);
        }
    }
}

void GdbRemoteServerEngine::callTargetRemote()
{
    QString channel = runParameters().remoteChannel;

    // Don't touch channels with explicitly set protocols.
    if (!channel.startsWith("tcp:") && !channel.startsWith("udp:")
            && !channel.startsWith("file:") && channel.contains(':')
            && !channel.startsWith('|'))
    {
        // "Fix" the IPv6 case with host names without '['...']'
        if (!channel.startsWith('[') && channel.count(':') >= 2) {
            channel.insert(0, '[');
            channel.insert(channel.lastIndexOf(':'), ']');
        }
        channel = "tcp:" + channel;
    }

    if (m_isQnxGdb)
        runCommand({"target qnx " + channel, CB(handleTargetQnx)});
    else if (runParameters().useExtendedRemote)
        runCommand({"target extended-remote " + channel, CB(handleTargetExtendedRemote)});
    else
        runCommand({"target remote " + channel, CB(handleTargetRemote)});
}

void GdbRemoteServerEngine::handleTargetRemote(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        showMessage("INFERIOR STARTED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        QString commands = expand(stringSetting(GdbPostAttachCommands));
        if (!commands.isEmpty())
            runCommand({commands, NativeCommand});
        handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        notifyInferiorSetupFailed(msgConnectRemoteServerFailed(response.data["msg"].data()));
    }
}

void GdbRemoteServerEngine::handleTargetExtendedRemote(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        showMessage("ATTACHED TO GDB SERVER STARTED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        QString commands = expand(stringSetting(GdbPostAttachCommands));
        if (!commands.isEmpty())
            runCommand({commands, NativeCommand});
        if (runParameters().attachPID > 0) { // attach to pid if valid
            // gdb server will stop the remote application itself.
            runCommand({"attach " + QString::number(runParameters().attachPID),
                        CB(handleTargetExtendedAttach)});
        } else if (!runParameters().inferior.executable.isEmpty()) {
            runCommand({"-gdb-set remote exec-file " + runParameters().inferior.executable,
                        CB(handleTargetExtendedAttach)});
        } else {
            const QString title = tr("No Remote Executable or Process ID Specified");
            const QString msg = tr(
                "No remote executable could be determined from your build system files.<p>"
                "In case you use qmake, consider adding<p>"
                "&nbsp;&nbsp;&nbsp;&nbsp;target.path = /tmp/your_executable # path on device<br>"
                "&nbsp;&nbsp;&nbsp;&nbsp;INSTALLS += target</p>"
                "to your .pro file.");
            QMessageBox *mb = showMessageBox(QMessageBox::Critical, title, msg,
                QMessageBox::Ok | QMessageBox::Cancel);
            mb->button(QMessageBox::Cancel)->setText(tr("Continue Debugging"));
            mb->button(QMessageBox::Ok)->setText(tr("Stop Debugging"));
            if (mb->exec() == QMessageBox::Ok) {
                showMessage("KILLING DEBUGGER AS REQUESTED BY USER");
                notifyInferiorSetupFailed(title);
            } else {
                showMessage("CONTINUE DEBUGGER AS REQUESTED BY USER");
                handleInferiorPrepared(); // This will likely fail.
            }
        }
    } else {
        notifyInferiorSetupFailed(msgConnectRemoteServerFailed(response.data["msg"].data()));
    }
}

void GdbRemoteServerEngine::handleTargetExtendedAttach(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        handleInferiorPrepared();
    } else {
        notifyInferiorSetupFailed(msgConnectRemoteServerFailed(response.data["msg"].data()));
    }
}

void GdbRemoteServerEngine::handleTargetQnx(const DebuggerResponse &response)
{
    QTC_ASSERT(m_isQnxGdb, qDebug() << m_isQnxGdb);
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        showMessage("INFERIOR STARTED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);

        const DebuggerRunParameters &rp = isMasterEngine() ? runParameters() : masterEngine()->runParameters();
        const qint64 pid = rp.attachPID;
        const QString remoteExecutable = rp.inferior.executable;
        if (pid > -1)
            runCommand({"attach " + QString::number(pid), CB(handleAttach)});
        else if (!remoteExecutable.isEmpty())
            runCommand({"set nto-executable " + remoteExecutable, CB(handleSetNtoExecutable)});
        else
            handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        notifyInferiorSetupFailed(response.data["msg"].data());
    }
}

void GdbRemoteServerEngine::handleAttach(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning: {
        showMessage("INFERIOR ATTACHED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        handleInferiorPrepared();
        break;
    }
    case ResultError:
        if (response.data["msg"].data() == "ptrace: Operation not permitted.") {
            notifyInferiorSetupFailed(msgPtraceError(runParameters().startMode));
            break;
        }
        // if msg != "ptrace: ..." fall through
    default:
        notifyInferiorSetupFailed(response.data["msg"].data());
    }
}

void GdbRemoteServerEngine::handleSetNtoExecutable(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning: {
        showMessage("EXECUTABLE SET");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        handleInferiorPrepared();
        break;
    }
    case ResultError:
    default:
        notifyInferiorSetupFailed(response.data["msg"].data());
    }
}

void GdbRemoteServerEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    if (runParameters().useContinueInsteadOfRun) {
        notifyEngineRunAndInferiorStopOk();
        continueInferiorInternal();
    } else {
        runCommand({"-exec-run", DebuggerCommand::RunRequest, CB(handleExecRun)});
    }
}

void GdbRemoteServerEngine::handleExecRun(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == ResultRunning) {
        notifyEngineRunAndInferiorRunOk();
        showMessage("INFERIOR STARTED");
        showMessage(msgInferiorSetupOk(), StatusBar);
    } else {
        showMessage(response.data["msg"].data());
        notifyEngineRunFailed();
    }
}

void GdbRemoteServerEngine::interruptInferior2()
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    if (usesTargetAsync()) {
        runCommand({"-exec-interrupt", CB(handleInterruptInferior)});
    } else if (m_isQnxGdb && HostOsInfo::isWindowsHost()) {
        m_gdbProc.interrupt();
    } else {
        qint64 pid = m_gdbProc.processId();
        bool ok = interruptProcess(pid, GdbEngineType, &m_errorString);
        if (!ok) {
            // FIXME: Extra state needed?
            showMessage("NOTE: INFERIOR STOP NOT POSSIBLE");
            showStatusMessage(tr("Interrupting not possible"));
            notifyInferiorRunOk();
        }
    }
}

void GdbRemoteServerEngine::handleInterruptInferior(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        // The gdb server will trigger extra output that we will pick up
        // to do a proper state transition.
    } else {
        // FIXME: On some gdb versions like git 170ffa5d7dd this produces
        // >810^error,msg="mi_cmd_exec_interrupt: Inferior not executing."
        notifyInferiorStopOk();
    }
}

void GdbRemoteServerEngine::shutdownEngine()
{
    notifyAdapterShutdownOk();
}

void GdbRemoteServerEngine::notifyEngineRemoteServerRunning
    (const QString &serverChannel, int inferiorPid)
{
    // Currently only used by Android support.
    runParameters().attachPID = inferiorPid;
    runParameters().remoteChannel = serverChannel;
    runParameters().useExtendedRemote = true;
    showMessage("NOTE: REMOTE SERVER RUNNING IN MULTIMODE");
    m_startAttempted = true;
    startGdb();
}

void GdbRemoteServerEngine::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    GdbEngine::notifyEngineRemoteSetupFinished(result);

    if (result.success) {
        if (!m_startAttempted)
            startGdb();
    } else {
        handleAdapterStartFailed(result.reason);
    }
}

} // namespace Internal
} // namespace Debugger
