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

#include "remotegdbserveradapter.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>
#include <debugger/procinterrupt.h>

#include <coreplugin/messagebox.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFileInfo>

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
        m_gdbProc.setUseCtrlCStub(runParameters.useCtrlCStub); // This is only set for QNX/BlackBerry

    connect(&m_uploadProc, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &GdbRemoteServerEngine::uploadProcError);
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
    showMessage(_("TRYING TO START ADAPTER"));
    QString serverStartScript = runParameters().serverStartScript;
    if (!serverStartScript.isEmpty()) {

        // Provide script information about the environment
        QString arglist;
        QtcProcess::addArg(&arglist, serverStartScript);
        QtcProcess::addArg(&arglist, runParameters().executable);
        QtcProcess::addArg(&arglist, runParameters().remoteChannel);

        m_uploadProc.start(arglist);
        m_uploadProc.waitForStarted();
    }
    if (!runParameters().workingDirectory.isEmpty())
        m_gdbProc.setWorkingDirectory(runParameters().workingDirectory);

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
    const DebuggerRunParameters &rp = runParameters();
    QString executableFileName;
    if (!rp.executable.isEmpty()) {
        QFileInfo fi(rp.executable);
        executableFileName = fi.absoluteFilePath();
    }

    //const QByteArray sysroot = sp.sysroot.toLocal8Bit();
    //const QByteArray remoteArch = sp.remoteArchitecture.toLatin1();
    const QString args = isMasterEngine() ? runParameters().processArgs
                          : masterEngine()->runParameters().processArgs;

//    if (!remoteArch.isEmpty())
//        postCommand("set architecture " + remoteArch);
    const QString solibSearchPath = rp.solibSearchPath.join(HostOsInfo::pathListSeparator());
    if (!solibSearchPath.isEmpty())
        postCommand("set solib-search-path " + solibSearchPath.toLocal8Bit());

    if (!args.isEmpty())
        postCommand("-exec-arguments " + args.toLocal8Bit());

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
    if (boolSetting(TargetAsync))
        postCommand("set target-async on", NoFlags, CB(handleSetTargetAsync));

    if (executableFileName.isEmpty()) {
        showMessage(tr("No symbol file given."), StatusBar);
        callTargetRemote();
        return;
    }

    if (!executableFileName.isEmpty()) {
        postCommand("-file-exec-and-symbols \"" + executableFileName.toLocal8Bit() + '"',
            NoFlags, CB(handleFileExecAndSymbols));
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
        QByteArray reason = response.data["msg"].data();
        QString msg = tr("Reading debug information failed:");
        msg += QLatin1Char('\n');
        msg += QString::fromLocal8Bit(reason);
        if (reason.endsWith("No such file or directory.")) {
            showMessage(_("INFERIOR STARTUP: BINARY NOT FOUND"));
            showMessage(msg, StatusBar);
            callTargetRemote(); // Proceed nevertheless.
        } else {
            notifyInferiorSetupFailed(msg);
        }
    }
}

void GdbRemoteServerEngine::callTargetRemote()
{
    QByteArray rawChannel = runParameters().remoteChannel.toLatin1();
    QByteArray channel = rawChannel;

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
        postCommand("target qnx " + channel, NoFlags, CB(handleTargetQnx));
    else if (runParameters().multiProcess)
        postCommand("target extended-remote " + channel, NoFlags, CB(handleTargetExtendedRemote));
    else
        postCommand("target remote " + channel, NoFlags, CB(handleTargetRemote));
}

void GdbRemoteServerEngine::handleTargetRemote(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        QString postAttachCommands = stringSetting(GdbPostAttachCommands);
        if (!postAttachCommands.isEmpty()) {
            foreach (const QString &cmd, postAttachCommands.split(QLatin1Char('\n')))
                postCommand(cmd.toLatin1());
        }
        handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(response.data["msg"].data()));
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::handleTargetExtendedRemote(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        showMessage(_("ATTACHED TO GDB SERVER STARTED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        QString postAttachCommands = stringSetting(GdbPostAttachCommands);
        if (!postAttachCommands.isEmpty()) {
            foreach (const QString &cmd, postAttachCommands.split(QLatin1Char('\n')))
                postCommand(cmd.toLatin1());
        }
        if (runParameters().attachPID > 0) { // attach to pid if valid
            // gdb server will stop the remote application itself.
            postCommand("attach " + QByteArray::number(runParameters().attachPID),
                        NoFlags, CB(handleTargetExtendedAttach));
        } else {
            postCommand("-gdb-set remote exec-file " + runParameters().remoteExecutable.toLatin1(),
                        NoFlags, CB(handleTargetExtendedAttach));
        }
    } else {
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(response.data["msg"].data()));
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::handleTargetExtendedAttach(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        handleInferiorPrepared();
    } else {
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(response.data["msg"].data()));
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::handleTargetQnx(const DebuggerResponse &response)
{
    QTC_ASSERT(m_isQnxGdb, qDebug() << m_isQnxGdb);
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);

        const qint64 pid = isMasterEngine() ? runParameters().attachPID : masterEngine()->runParameters().attachPID;
        const QString remoteExecutable = isMasterEngine() ? runParameters().remoteExecutable : masterEngine()->runParameters().remoteExecutable;
        if (pid > -1)
            postCommand("attach " + QByteArray::number(pid), NoFlags, CB(handleAttach));
        else if (!remoteExecutable.isEmpty())
            postCommand("set nto-executable " + remoteExecutable.toLatin1(), NoFlags, CB(handleSetNtoExecutable));
        else
            handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(response.data["msg"].data()));
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::handleAttach(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning: {
        showMessage(_("INFERIOR ATTACHED"));
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
        QString msg = QString::fromLocal8Bit(response.data["msg"].data());
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::handleSetNtoExecutable(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning: {
        showMessage(_("EXECUTABLE SET"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        handleInferiorPrepared();
        break;
    }
    case ResultError:
    default:
        QString msg = QString::fromLocal8Bit(response.data["msg"].data());
        notifyInferiorSetupFailed(msg);
    }

}

void GdbRemoteServerEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    const QString remoteExecutable = runParameters().remoteExecutable;
    if (!remoteExecutable.isEmpty()) {
        postCommand("-exec-run", GdbEngine::RunRequest, CB(handleExecRun));
    } else {
        notifyEngineRunAndInferiorStopOk();
        continueInferiorInternal();
    }
}

void GdbRemoteServerEngine::handleExecRun(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == ResultRunning) {
        notifyEngineRunAndInferiorRunOk();
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgInferiorSetupOk(), StatusBar);
    } else {
        QString msg = QString::fromLocal8Bit(response.data["msg"].data());
        showMessage(msg);
        notifyEngineRunFailed();
    }
}

void GdbRemoteServerEngine::interruptInferior2()
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    if (boolSetting(TargetAsync)) {
        postCommand("-exec-interrupt", GdbEngine::Immediate,
            CB(handleInterruptInferior));
    } else if (m_isQnxGdb && HostOsInfo::isWindowsHost()) {
        m_gdbProc.interrupt();
    } else {
        long pid = Utils::qPidToPid(m_gdbProc.pid());
        bool ok = interruptProcess(pid, GdbEngineType, &m_errorString);
        if (!ok) {
            // FIXME: Extra state needed?
            showMessage(_("NOTE: INFERIOR STOP NOT POSSIBLE"));
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
    (const QByteArray &serverChannel, int inferiorPid)
{
    // Currently only used by Android support.
    runParameters().attachPID = inferiorPid;
    runParameters().remoteChannel = QString::fromLatin1(serverChannel);
    runParameters().multiProcess = true;
    showMessage(_("NOTE: REMOTE SERVER RUNNING IN MULTIMODE"));
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
