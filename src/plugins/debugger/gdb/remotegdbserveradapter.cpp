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

#include "remotegdbserveradapter.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerprotocol.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"

#include <projectexplorer/abi.h>
#include <utils/fancymainwindow.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFileInfo>
#include <QMessageBox>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::GdbCommandCallback>(&GdbRemoteServerEngine::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

GdbRemoteServerEngine::GdbRemoteServerEngine(const DebuggerStartParameters &startParameters)
    : GdbEngine(startParameters)
{
    connect(&m_uploadProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(uploadProcError(QProcess::ProcessError)));
    connect(&m_uploadProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readUploadStandardOutput()));
    connect(&m_uploadProc, SIGNAL(readyReadStandardError()),
        SLOT(readUploadStandardError()));
    connect(&m_uploadProc, SIGNAL(finished(int)),
        SLOT(uploadProcFinished()));
}

GdbEngine::DumperHandling GdbRemoteServerEngine::dumperHandling() const
{
    using namespace ProjectExplorer;
    const Abi abi = startParameters().toolChainAbi;
    if (abi.os() == Abi::WindowsOS
            || abi.binaryFormat() == Abi::ElfFormat)
        return DumperLoadedByGdb;
    return DumperLoadedByGdbPreload;
}

void GdbRemoteServerEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));
    if (!startParameters().serverStartScript.isEmpty()) {

        // Provide script information about the environment
        QString arglist;
        Utils::QtcProcess::addArg(&arglist, startParameters().serverStartScript);
        Utils::QtcProcess::addArg(&arglist, startParameters().executable);
        Utils::QtcProcess::addArg(&arglist, startParameters().remoteChannel);

        m_uploadProc.start(_("/bin/sh ") + arglist);
        m_uploadProc.waitForStarted();
    }
    if (startParameters().remoteSetupNeeded)
        notifyEngineRequestRemoteSetup();
    else
        startGdb();
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
    showMessageBox(QMessageBox::Critical, tr("Error"), msg);
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
    if (m_uploadProc.exitStatus() == QProcess::NormalExit
        && m_uploadProc.exitCode() == 0)
        startGdb();
    else
        notifyEngineRemoteSetupFailed(m_uploadProc.errorString());
}

void GdbRemoteServerEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    const DebuggerStartParameters &sp = startParameters();
    QString executableFileName;
    if (!sp.executable.isEmpty()) {
        QFileInfo fi(sp.executable);
        executableFileName = fi.absoluteFilePath();
    }
    QString symbolFileName;
    if (!sp.symbolFileName.isEmpty()) {
        QFileInfo fi(sp.symbolFileName);
        symbolFileName = fi.absoluteFilePath();
    }

    //const QByteArray sysroot = sp.sysroot.toLocal8Bit();
    //const QByteArray remoteArch = sp.remoteArchitecture.toLatin1();
    const QString args = sp.processArgs;

//    if (!remoteArch.isEmpty())
//        postCommand("set architecture " + remoteArch);
    const QString solibSearchPath
            = sp.solibSearchPath.join(QString(Utils::HostOsInfo::pathListSeparator()));
    if (!solibSearchPath.isEmpty())
        postCommand("set solib-search-path " + solibSearchPath.toLocal8Bit());

    if (!args.isEmpty())
        postCommand("-exec-arguments " + args.toLocal8Bit());

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
    if (debuggerCore()->boolSetting(TargetAsync))
        postCommand("set target-async on", CB(handleSetTargetAsync));

    if (executableFileName.isEmpty() && symbolFileName.isEmpty()) {
        showMessage(tr("No symbol file given."), StatusBar);
        callTargetRemote();
        return;
    }

    if (!symbolFileName.isEmpty()) {
        postCommand("-file-symbol-file \""
                    + symbolFileName.toLocal8Bit() + '"',
                    CB(handleFileExecAndSymbols));
    }
    if (!executableFileName.isEmpty()) {
        postCommand("-file-exec-and-symbols \"" + executableFileName.toLocal8Bit() + '"',
            CB(handleFileExecAndSymbols));
    }
}

void GdbRemoteServerEngine::handleSetTargetAsync(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultError)
        qDebug() << "Adapter too old: does not support asynchronous mode.";
}

void GdbRemoteServerEngine::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        callTargetRemote();
    } else {
        QByteArray reason = response.data.findChild("msg").data();
        QString msg = tr("Reading debug information failed:\n");
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
    //m_breakHandler->clearBreakMarkers();

    // "target remote" does three things:
    //     (1) connects to the gdb server
    //     (2) starts the remote application
    //     (3) stops the remote application (early, e.g. in the dynamic linker)
    QByteArray channel = startParameters().remoteChannel.toLatin1();

    // Don't touch channels with explicitly set protocols.
    if (!channel.startsWith("tcp:") && !channel.startsWith("udp:")
            && !channel.startsWith("file:") && channel.contains(':'))
    {
        // "Fix" the IPv6 case with host names without '['...']'
        if (!channel.startsWith('[') && channel.count(':') >= 2) {
            channel.insert(0, '[');
            channel.insert(channel.lastIndexOf(':'), ']');
        }
        channel = "tcp:" + channel;
    }

    if (m_isQnxGdb)
        postCommand("target qnx " + channel, CB(handleTargetQnx));
    else
        postCommand("target remote " + channel, CB(handleTargetRemote));
}

void GdbRemoteServerEngine::handleTargetRemote(const GdbResponse &record)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (record.resultClass == GdbResultDone) {
        // gdb server will stop the remote application itself.
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        QString postAttachCommands = debuggerCore()->stringSetting(GdbPostAttachCommands);
        if (!postAttachCommands.isEmpty()) {
            foreach (const QString &cmd, postAttachCommands.split(QLatin1Char('\n')))
                postCommand(cmd.toLatin1());
        }
        handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(record.data.findChild("msg").data()));
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::handleTargetQnx(const GdbResponse &response)
{
    QTC_ASSERT(m_isQnxGdb, qDebug() << m_isQnxGdb);
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        // gdb server will stop the remote application itself.
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);

        const qint64 pid = isMasterEngine() ? startParameters().attachPID : masterEngine()->startParameters().attachPID;
        if (pid > -1)
            postCommand("attach " + QByteArray::number(pid), CB(handleAttach));
        else
            handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(response.data.findChild("msg").data()));
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::handleAttach(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    switch (response.resultClass) {
    case GdbResultDone:
    case GdbResultRunning: {
        showMessage(_("INFERIOR ATTACHED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        handleInferiorPrepared();
        break;
    }
    case GdbResultError:
        if (response.data.findChild("msg").data() == "ptrace: Operation not permitted.") {
            notifyInferiorSetupFailed(DumperHelper::msgPtraceError(startParameters().startMode));
            break;
        }
        // if msg != "ptrace: ..." fall through
    default:
        QString msg = QString::fromLocal8Bit(response.data.findChild("msg").data());
        notifyInferiorSetupFailed(msg);
    }
}

void GdbRemoteServerEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    const QString remoteExecutable = startParameters().remoteExecutable;
    if (!remoteExecutable.isEmpty()) {
        // Cannot use -exec-run for QNX gdb 7.4 as it does not support path parameter for the MI call
        const bool useRun = m_isQnxGdb && m_gdbVersion > 70300;
        const QByteArray command = useRun ? "run" : "-exec-run";
        postCommand(command + " " + remoteExecutable.toLocal8Bit(), GdbEngine::RunRequest, CB(handleExecRun));
    } else {
        notifyEngineRunAndInferiorStopOk();
        continueInferiorInternal();
    }
}

void GdbRemoteServerEngine::handleExecRun(const GdbResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        notifyEngineRunAndInferiorRunOk();
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgInferiorSetupOk(), StatusBar);
    } else {
        QString msg = QString::fromLocal8Bit(response.data.findChild("msg").data());
        showMessage(msg);
        notifyEngineRunFailed();
    }
}

void GdbRemoteServerEngine::interruptInferior2()
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    if (debuggerCore()->boolSetting(TargetAsync)) {
        postCommand("-exec-interrupt", GdbEngine::Immediate,
            CB(handleInterruptInferior));
    } else {
        bool ok = m_gdbProc.interrupt();
        if (!ok) {
            // FIXME: Extra state needed?
            showMessage(_("NOTE: INFERIOR STOP NOT POSSIBLE"));
            showStatusMessage(tr("Interrupting not possible"));
            notifyInferiorRunOk();
        }
    }
}

void GdbRemoteServerEngine::handleInterruptInferior(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
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

void GdbRemoteServerEngine::notifyEngineRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    DebuggerEngine::notifyEngineRemoteSetupDone(gdbServerPort, qmlPort);

    if (qmlPort != -1)
        startParameters().qmlServerPort = qmlPort;
    if (gdbServerPort != -1) {
        QString &rc = startParameters().remoteChannel;
        const int sepIndex = rc.lastIndexOf(QLatin1Char(':'));
        if (sepIndex != -1) {
            rc.replace(sepIndex + 1, rc.count() - sepIndex - 1,
                       QString::number(gdbServerPort));
        }
    }
    startGdb();
}

void GdbRemoteServerEngine::notifyEngineRemoteSetupFailed(const QString &reason)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    DebuggerEngine::notifyEngineRemoteSetupFailed(reason);
    handleAdapterStartFailed(reason);
}

} // namespace Internal
} // namespace Debugger
