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

#include "remotegdbserveradapter.h"

#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "gdbengine.h"
#include "gdbmi.h"

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <projectexplorer/toolchaintype.h>

#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

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

RemoteGdbServerAdapter::RemoteGdbServerAdapter(GdbEngine *engine, int toolChainType, QObject *parent) :
    AbstractGdbAdapter(engine, parent),
    m_toolChainType(toolChainType)
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
    switch (m_toolChainType) {
    case ProjectExplorer::ToolChain_MinGW:
    case ProjectExplorer::ToolChain_MSVC:
    case ProjectExplorer::ToolChain_WINCE:
    case ProjectExplorer::ToolChain_WINSCW:
    case ProjectExplorer::ToolChain_GCCE:
    case ProjectExplorer::ToolChain_RVCT2_ARMV5:
    case ProjectExplorer::ToolChain_RVCT2_ARMV6:
    case ProjectExplorer::ToolChain_GCC_MAEMO:
        return DumperLoadedByGdb;
    default:
        break;
    }
    return DumperLoadedByGdbPreload;
}

void RemoteGdbServerAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));
    if (!startParameters().useServerStartScript) {
        handleSetupDone();
        return;
    }
    if (startParameters().serverStartScript.isEmpty()) {
        showMessage(_("No server start script given. "), StatusBar);
        m_engine->requestRemoteSetup();
    } else {
        m_uploadProc.start(_("/bin/sh ") + startParameters().serverStartScript);
        m_uploadProc.waitForStarted();
    }
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
        handleSetupDone();
    else
        handleSetupFailed(m_uploadProc.errorString());
}

void RemoteGdbServerAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    QString fileName;
    if (!startParameters().executable.isEmpty()) {
        QFileInfo fi(startParameters().executable);
        fileName = fi.absoluteFilePath();
    }
    const QByteArray sysRoot = startParameters().sysRoot.toLocal8Bit();
    const QByteArray remoteArch = startParameters().remoteArchitecture.toLatin1();
    const QByteArray gnuTarget = startParameters().gnuTarget.toLatin1();
    const QByteArray solibPath =
         QFileInfo(startParameters().dumperLibrary).path().toLocal8Bit();
    const QString args = startParameters().processArgs;

    if (!remoteArch.isEmpty())
        m_engine->postCommand("set architecture " + remoteArch);
    if (!gnuTarget.isEmpty())
        m_engine->postCommand("set gnutarget " + gnuTarget);
    if (!sysRoot.isEmpty())
        m_engine->postCommand("set sysroot " + sysRoot);
    if (!solibPath.isEmpty())
        m_engine->postCommand("set solib-search-path " + solibPath);
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
        QString msg = tr("Reading debug information failed:\n");
        msg += QString::fromLocal8Bit(response.data.findChild("msg").data());
        m_engine->notifyInferiorSetupFailed(msg);
    }
}

void RemoteGdbServerAdapter::callTargetRemote()
{
    //m_breakHandler->clearBreakMarkers();

    // "target remote" does three things:
    //     (1) connects to the gdb server
    //     (2) starts the remote application
    //     (3) stops the remote application (early, e.g. in the dynamic linker)
    QString channel = startParameters().remoteChannel;
    m_engine->postCommand("target remote " + channel.toLatin1(),
        CB(handleTargetRemote));
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

void RemoteGdbServerAdapter::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->notifyEngineRunAndInferiorStopOk();
    m_engine->continueInferiorInternal();
}

void RemoteGdbServerAdapter::interruptInferior()
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    m_engine->postCommand("-exec-interrupt", GdbEngine::Immediate,
        CB(handleInterruptInferior));
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

void RemoteGdbServerAdapter::shutdownInferior()
{
    m_engine->defaultInferiorShutdown("kill");
}

void RemoteGdbServerAdapter::shutdownAdapter()
{
    m_engine->notifyAdapterShutdownOk();
}

void RemoteGdbServerAdapter::handleSetupDone(int gdbServerPort, int qmlPort)
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
    handleSetupDone();
}

void RemoteGdbServerAdapter::handleSetupDone()
{
    if (m_engine->startGdb(QStringList(), startParameters().debuggerCommand))
        m_engine->handleAdapterStarted();
}

void RemoteGdbServerAdapter::handleSetupFailed(const QString &reason)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_engine->handleAdapterStartFailed(reason);
}

} // namespace Internal
} // namespace Debugger
