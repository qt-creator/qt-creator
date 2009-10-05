/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "plaingdbadapter.h"

#include "debuggeractions.h"
#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <coreplugin/icore.h>

#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtGui/QMessageBox>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&PlainGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

PlainGdbAdapter::PlainGdbAdapter(GdbEngine *engine, QObject *parent)
    : AbstractGdbAdapter(engine, parent)
{
    QTC_ASSERT(state() == DebuggerNotReady, qDebug() << state());
    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(handleGdbError(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
        this, SIGNAL(readyReadStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
        this, SIGNAL(readyReadStandardError()));
    connect(&m_gdbProc, SIGNAL(started()),
        this, SLOT(handleGdbStarted()));
    connect(&m_gdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        this, SLOT(handleGdbFinished(int, QProcess::ExitStatus)));

    m_stubProc.setMode(Utils::ConsoleProcess::Debug);
#ifdef Q_OS_UNIX
    m_stubProc.setSettings(Core::ICore::instance()->settings());
#endif

    connect(&m_stubProc, SIGNAL(processError(QString)),
        this, SLOT(stubError(QString)));
    connect(&m_stubProc, SIGNAL(processStarted()),
        this, SLOT(stubStarted()));
// FIXME:
//    connect(&m_stubProc, SIGNAL(wrapperStopped()),
//        m_manager, SLOT(exitDebugger()));
}

void PlainGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    QStringList gdbArgs;
    gdbArgs.prepend(_("mi"));
    gdbArgs.prepend(_("-i"));

    if (startParameters().useTerminal) {
        m_stubProc.stop(); // We leave the console open, so recycle it now.

        m_stubProc.setWorkingDirectory(startParameters().workingDir);
        m_stubProc.setEnvironment(startParameters().environment);
        if (!m_stubProc.start(startParameters().executable,
                             startParameters().processArgs)) {
            // Error message for user is delivered via a signal.
            emitAdapterStartFailed(QString());
            return;
        }
    } else {
        if (!m_engine->m_outputCollector.listen()) {
            emitAdapterStartFailed(tr("Cannot set up communication with child process: %1")
                    .arg(m_engine->m_outputCollector.errorString()));
            return;
        }
        gdbArgs.prepend(_("--tty=") + m_engine->m_outputCollector.serverName());

        if (!startParameters().workingDir.isEmpty())
            m_gdbProc.setWorkingDirectory(startParameters().workingDir);
        if (!startParameters().environment.isEmpty())
            m_gdbProc.setEnvironment(startParameters().environment);
    }

    m_gdbProc.start(theDebuggerStringSetting(GdbLocation), gdbArgs);
}

void PlainGdbAdapter::handleGdbStarted()
{
    QTC_ASSERT(state() == AdapterStarting, qDebug() << state());
    setState(AdapterStarted);
    emit adapterStarted();
}

void PlainGdbAdapter::handleGdbError(QProcess::ProcessError error)
{
    debugMessage(_("PLAIN ADAPTER, HANDLE GDB ERROR"));
    emit adapterCrashed(m_engine->errorMessage(error));
}

void PlainGdbAdapter::prepareInferior()
{
    QTC_ASSERT(state() == AdapterStarted, qDebug() << state());
    setState(InferiorPreparing);
    if (!startParameters().processArgs.isEmpty())
        m_engine->postCommand(_("-exec-arguments ")
            + startParameters().processArgs.join(_(" ")));
    QFileInfo fi(startParameters().executable);
    m_engine->postCommand(_("-file-exec-and-symbols \"%1\"").arg(fi.absoluteFilePath()),
        CB(handleFileExecAndSymbols));
}

void PlainGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorPreparing, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        //m_breakHandler->clearBreakMarkers();
        setState(InferiorPrepared);
        emit inferiorPrepared();
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Starting executable failed:\n") +
            __(response.data.findChild("msg").data());
        setState(InferiorPreparationFailed);
        emit inferiorPreparationFailed(msg);
    }
}

void PlainGdbAdapter::handleExecRun(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        QTC_ASSERT(state() == InferiorRunning, qDebug() << state());
        debugMessage(_("INFERIOR STARTED"));
        showStatusMessage(tr("Inferior started."));
    } else {
        QTC_ASSERT(state() == InferiorRunningRequested, qDebug() << state());
        QTC_ASSERT(response.resultClass == GdbResultError, /**/);
        const QByteArray &msg = response.data.findChild("msg").data();
        //QTC_ASSERT(status() == InferiorRunning, /**/);
        //interruptInferior();
        setState(InferiorStartFailed);
        emit inferiorStartFailed(msg);
    }
}

void PlainGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    setState(InferiorRunningRequested);
    m_engine->postCommand(_("-exec-run"), CB(handleExecRun));
}

void PlainGdbAdapter::interruptInferior()
{
    debugMessage(_("TRYING TO INTERUPT INFERIOR"));
    const qint64 attachedPID = m_engine->inferiorPid();
    if (attachedPID <= 0) {
        debugMessage(_("TRYING TO INTERRUPT INFERIOR BEFORE PID WAS OBTAINED"));
        return;
    }

    if (!interruptProcess(attachedPID))
        debugMessage(_("CANNOT INTERRUPT %1").arg(attachedPID));
}

void PlainGdbAdapter::shutdown()
{
    debugMessage(_("PLAIN ADAPTER SHUTDOWN %1").arg(state()));
    switch (state()) {
    
    case InferiorRunningRequested:
    case InferiorRunning:
    case InferiorStopping:
    case InferiorStopped:
        setState(InferiorShuttingDown);
        m_engine->postCommand(_("kill"), CB(handleKill));
        return;

    case InferiorShuttingDown:
        // FIXME: How can we end up here?
        QTC_ASSERT(false, qDebug() << state());
        // Fall through.

    case InferiorShutDown:
        setState(AdapterShuttingDown);
        m_engine->postCommand(_("-gdb-exit"), CB(handleExit));
        return;

/*
    case InferiorShutdownFailed:
        m_gdbProc.terminate();
        // 20s can easily happen when loading webkit debug information
        m_gdbProc.waitForFinished(20000);
        setState(AdapterShuttingDown);
        debugMessage(_("FORCING TERMINATION: %1").arg(state()));
        if (state() != QProcess::NotRunning) {
            debugMessage(_("PROBLEM STOPPING DEBUGGER: STATE %1")
                .arg(state()));
            m_gdbProc.kill();
        }
        m_engine->postCommand(_("-gdb-exit"), CB(handleExit));
        return;
*/
    default:
        QTC_ASSERT(false, qDebug() << state());
    }
}

void PlainGdbAdapter::handleKill(const GdbResponse &response)
{
    debugMessage(_("PLAIN ADAPTER HANDLE KILL " + response.toString()));
    if (response.resultClass == GdbResultDone) {
        setState(InferiorShutDown);
        emit inferiorShutDown();
        shutdown(); // re-iterate...
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Inferior process could not be stopped:\n") +
            __(response.data.findChild("msg").data());
        setState(InferiorShutdownFailed);
        emit inferiorShutdownFailed(msg);
    }
}

void PlainGdbAdapter::handleExit(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // don't set state here, this will be handled in handleGdbFinished()
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Gdb process could not be stopped:\n") +
            __(response.data.findChild("msg").data());
        emit adapterShutdownFailed(msg);
    }
}

void PlainGdbAdapter::handleGdbFinished(int, QProcess::ExitStatus)
{
    debugMessage(_("GDB PROCESS FINISHED"));
    emit adapterShutDown();
}

void PlainGdbAdapter::stubStarted()
{
    const qint64 attachedPID = m_stubProc.applicationPID();
    emit inferiorPidChanged(attachedPID);
    m_engine->postCommand(_("attach %1").arg(attachedPID), CB(handleStubAttached));
}

void PlainGdbAdapter::handleStubAttached(const GdbResponse &)
{
    qDebug() << "STUB ATTACHED, FIXME";
    //qq->notifyInferiorStopped();
    //handleAqcuiredInferior();
}

void PlainGdbAdapter::stubError(const QString &msg)
{
    QMessageBox::critical(m_engine->mainWindow(), tr("Debugger Error"), msg);
}

void PlainGdbAdapter::emitAdapterStartFailed(const QString &msg)
{
    //  QMessageBox::critical(mainWindow(), tr("Debugger Startup Failure"),
    //    tr("Cannot start debugger: %1").arg(m_gdbAdapter->errorString()));
    bool blocked = m_stubProc.blockSignals(true);
    m_stubProc.stop();
    m_stubProc.blockSignals(blocked);
    emit adapterStartFailed(msg, QString());
}

} // namespace Internal
} // namespace Debugger
