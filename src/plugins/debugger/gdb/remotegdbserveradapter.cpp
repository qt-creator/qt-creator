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

#include "remotegdbserveradapter.h"

#include "debuggeractions.h"
#include "debuggerstartparameters.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "gdbengine.h"
#include "gdbmi.h"

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <projectexplorer/abi.h>

#include <QFileInfo>
#include <QMessageBox>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&RemoteGdbServerAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

RemoteGdbServerAdapter::RemoteGdbServerAdapter(GdbEngine *engine)
    : AbstractGdbAdapter(engine)
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

AbstractGdbAdapter::DumperHandling RemoteGdbServerAdapter::dumperHandling() const
{
    using namespace ProjectExplorer;
    const Abi abi = startParameters().toolChainAbi;
    if (abi.os() == Abi::SymbianOS
            || abi.os() == Abi::WindowsOS
            || abi.binaryFormat() == Abi::ElfFormat)
        return DumperLoadedByGdb;
    return DumperLoadedByGdbPreload;
}

void RemoteGdbServerAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));
    if (startParameters().useServerStartScript) {
        if (startParameters().serverStartScript.isEmpty()) {
            showMessage(_("No server start script given. "), StatusBar);
        } else {
            m_uploadProc.start(_("/bin/sh ") + startParameters().serverStartScript);
            m_uploadProc.waitForStarted();
        }
    }
    if (startParameters().requestRemoteSetup)
        m_engine->notifyEngineRequestRemoteSetup();
    else
        m_engine->startGdb();
}

void RemoteGdbServerAdapter::uploadProcError(QProcess::ProcessError error)
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

void RemoteGdbServerAdapter::readUploadStandardOutput()
{
    const QByteArray ba = m_uploadProc.readAllStandardOutput();
    const QString msg = QString::fromLocal8Bit(ba, ba.length());
    showMessage(msg, LogOutput);
    showMessage(msg, AppOutput);
}

void RemoteGdbServerAdapter::readUploadStandardError()
{
    const QByteArray ba = m_uploadProc.readAllStandardError();
    const QString msg = QString::fromLocal8Bit(ba, ba.length());
    showMessage(msg, LogOutput);
    showMessage(msg, AppError);
}

void RemoteGdbServerAdapter::uploadProcFinished()
{
    if (m_uploadProc.exitStatus() == QProcess::NormalExit
        && m_uploadProc.exitCode() == 0)
        m_engine->startGdb();
    else
        handleRemoteSetupFailed(m_uploadProc.errorString());
}

void RemoteGdbServerAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    const DebuggerStartParameters &sp = startParameters();
#ifdef Q_OS_WIN
    #define PATHSEP ";"
#else
    #define PATHSEP ":"
#endif
    QString fileName;
    if (!sp.executable.isEmpty()) {
        QFileInfo fi(sp.executable);
        fileName = fi.absoluteFilePath();
    }
    const QByteArray sysroot = sp.sysroot.toLocal8Bit();
    const QByteArray remoteArch = sp.remoteArchitecture.toLatin1();
    const QByteArray gnuTarget = sp.gnuTarget.toLatin1();
    const QByteArray searchPath = startParameters().searchPath.toLocal8Bit();
    const QString args = sp.processArgs;
    const QString solibSearchPath = startParameters().solibSearchPath.join(QLatin1String(PATHSEP));

    if (!remoteArch.isEmpty())
        m_engine->postCommand("set architecture " + remoteArch);
    if (!gnuTarget.isEmpty())
        m_engine->postCommand("set gnutarget " + gnuTarget);
    if (!sysroot.isEmpty()) {
        m_engine->postCommand("set sysroot " + sysroot);
        // sysroot is not enough to correctly locate the sources, so explicitly
        // relocate the most likely place for the debug source
        m_engine->postCommand("set substitute-path /usr/src " + sysroot + "/usr/src");
    }
    if (!solibSearchPath.isEmpty())
        m_engine->postCommand("set solib-search-path " + solibSearchPath.toLocal8Bit());

    if (!args.isEmpty())
        m_engine->postCommand("-exec-arguments " + args.toLocal8Bit());

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
        m_engine->postCommand("set target-async on", CB(handleSetTargetAsync));

    if (fileName.isEmpty()) {
        showMessage(tr("No symbol file given."), StatusBar);
        callTargetRemote();
        return;
    }

    m_engine->postCommand("-file-exec-and-symbols \""
        + fileName.toLocal8Bit() + '"',
        CB(handleFileExecAndSymbols));
}

void RemoteGdbServerAdapter::handleSetTargetAsync(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultError)
        qDebug() << "Adapter too old: does not support asynchronous mode.";
}

void RemoteGdbServerAdapter::handleFileExecAndSymbols(const GdbResponse &response)
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
            m_engine->notifyInferiorSetupFailed(msg);
        }
    }
}

void RemoteGdbServerAdapter::callTargetRemote()
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

    if (m_engine->m_isQnxGdb)
        m_engine->postCommand("target qnx " + channel, CB(handleTargetQnx));
    else
        m_engine->postCommand("target remote " + channel, CB(handleTargetRemote));
}

void RemoteGdbServerAdapter::handleTargetRemote(const GdbResponse &record)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (record.resultClass == GdbResultDone) {
        // gdb server will stop the remote application itself.
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        m_engine->handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(record.data.findChild("msg").data()));
        m_engine->notifyInferiorSetupFailed(msg);
    }
}

void RemoteGdbServerAdapter::handleTargetQnx(const GdbResponse &response)
{
    QTC_ASSERT(m_engine->m_isQnxGdb, qDebug() << m_engine->m_isQnxGdb);
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        // gdb server will stop the remote application itself.
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);

        const qint64 pid = startParameters().attachPID;
        if (pid > -1) {
            m_engine->postCommand("attach " + QByteArray::number(pid), CB(handleAttach));
        } else {
            m_engine->handleInferiorPrepared();
        }
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        QString msg = msgConnectRemoteServerFailed(
            QString::fromLocal8Bit(response.data.findChild("msg").data()));
        m_engine->notifyInferiorSetupFailed(msg);
    }
}

void RemoteGdbServerAdapter::handleAttach(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    switch (response.resultClass) {
    case GdbResultDone:
    case GdbResultRunning: {
        showMessage(_("INFERIOR ATTACHED"));
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        m_engine->handleInferiorPrepared();
        break;
    }
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

void RemoteGdbServerAdapter::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->notifyEngineRunAndInferiorStopOk();
    m_engine->continueInferiorInternal();
}

void RemoteGdbServerAdapter::interruptInferior()
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    if (debuggerCore()->boolSetting(TargetAsync)) {
        m_engine->postCommand("-exec-interrupt", GdbEngine::Immediate,
            CB(handleInterruptInferior));
    } else {
        bool ok = m_gdbProc.interrupt();
        if (!ok) {
            // FIXME: Extra state needed?
            m_engine->showMessage(_("NOTE: INFERIOR STOP NOT POSSIBLE"));
            m_engine->showStatusMessage(tr("Interrupting not possible"));
            m_engine->notifyInferiorRunOk();
        }
    }
}

void RemoteGdbServerAdapter::handleInterruptInferior(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // The gdb server will trigger extra output that we will pick up
        // to do a proper state transition.
    } else {
        // FIXME: On some gdb versions like git 170ffa5d7dd this produces
        // >810^error,msg="mi_cmd_exec_interrupt: Inferior not executing."
        m_engine->notifyInferiorStopOk();
    }
}

void RemoteGdbServerAdapter::shutdownAdapter()
{
    m_engine->notifyAdapterShutdownOk();
}

void RemoteGdbServerAdapter::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

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
    m_engine->startGdb();
}

void RemoteGdbServerAdapter::handleGdbStartDone()
{
    m_engine->handleAdapterStarted();
}

void RemoteGdbServerAdapter::handleGdbStartFailed()
{
}

void RemoteGdbServerAdapter::handleRemoteSetupFailed(const QString &reason)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_engine->handleAdapterStartFailed(reason);
}

} // namespace Internal
} // namespace Debugger
