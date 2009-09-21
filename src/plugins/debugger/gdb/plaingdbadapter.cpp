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

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

namespace Debugger {
namespace Internal {

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)
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
    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)),
        this, SIGNAL(error(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
        this, SIGNAL(readyReadStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
        this, SIGNAL(readyReadStandardError()));
    connect(&m_gdbProc, SIGNAL(started()),
        this, SIGNAL(adapterStarted()));
    connect(&m_gdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        this, SLOT(handleFinished(int, QProcess::ExitStatus)));

    m_stubProc.setMode(Core::Utils::ConsoleProcess::Debug);
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

void PlainGdbAdapter::startAdapter(const DebuggerStartParametersPtr &sp)
{
    debugMessage(_("TRYING TO START ADAPTER"));
    m_startParameters = sp;

    QStringList gdbArgs;
    gdbArgs.prepend(_("mi"));
    gdbArgs.prepend(_("-i"));

    if (m_startParameters->useTerminal) {
        m_stubProc.stop(); // We leave the console open, so recycle it now.

        m_stubProc.setWorkingDirectory(m_startParameters->workingDir);
        m_stubProc.setEnvironment(m_startParameters->environment);
        if (!m_stubProc.start(m_startParameters->executable,
                             m_startParameters->processArgs)) {
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

        if (!m_startParameters->workingDir.isEmpty())
            setWorkingDirectory(m_startParameters->workingDir);
        if (!m_startParameters->environment.isEmpty())
            setEnvironment(m_startParameters->environment);
    }

    QString location = theDebuggerStringSetting(GdbLocation);
    //showStatusMessage(tr("Starting Debugger: ") + loc + _c(' ') + gdbArgs.join(_(" ")));
    m_gdbProc.start(location, gdbArgs);
}

void PlainGdbAdapter::prepareInferior()
{
    if (!m_startParameters->processArgs.isEmpty())
        m_engine->postCommand(_("-exec-arguments ") + m_startParameters->processArgs.join(_(" ")));
    QFileInfo fi(m_engine->startParameters().executable);
    m_engine->postCommand(_("-file-exec-and-symbols \"%1\"").arg(fi.absoluteFilePath()),
        CB(handleFileExecAndSymbols));
}

void PlainGdbAdapter::handleFileExecAndSymbols(const GdbResultRecord &response, const QVariant &)
{
    if (response.resultClass == GdbResultDone) {
        //m_breakHandler->clearBreakMarkers();
        emit inferiorPrepared();
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Starting executable failed:\n") +
            __(response.data.findChild("msg").data());
        emit inferiorPreparationFailed(msg);
    }
}

void PlainGdbAdapter::startInferior()
{
    m_engine->postCommand(_("-exec-run"), CB(handleExecRun));
/*
    #ifdef Q_OS_MAC        
    m_engine->postCommand(_("sharedlibrary apply-load-rules all"));
    // On MacOS, breaking in at the entry point wreaks havoc.
    m_engine->postCommand(_("tbreak main"));
    m_waitingForFirstBreakpointToBeHit = true;
    m_engine->postCommand(_("-exec-run"), CB(handleExecRun));
    #else
// FIXME:
//    if (!m_dumperInjectionLoad)
//        m_engine->postCommand(_("set auto-solib-add off"));
    m_engine->postCommand(_("info target"), CB(handleInfoTarget));
    #endif
*/
}

void PlainGdbAdapter::handleInfoTarget(const GdbResultRecord &response, const QVariant &)
{
#if defined(Q_OS_MAC)
    Q_UNUSED(response)
#else
    if (response.resultClass == GdbResultDone) {
        // [some leading stdout here]
        // >&"        Entry point: 0x80831f0  0x08048134 - 0x08048147 is .interp\n"
        // [some trailing stdout here]
        QString msg = _(response.data.findChild("consolestreamoutput").data());
        QRegExp needle(_("\\bEntry point: (0x[0-9a-f]+)\\b"));
        if (needle.indexIn(msg) != -1) {
            //debugMessage(_("STREAM: ") + msg + " " + needle.cap(1));
            m_engine->postCommand(_("tbreak *") + needle.cap(1));
// FIXME:            m_waitingForFirstBreakpointToBeHit = true;
            m_engine->postCommand(_("-exec-run"), CB(handleExecRun));
        } else {
            debugMessage(_("PARSING START ADDRESS FAILED: ") + msg);
            emit inferiorStartFailed(_("Parsing start address failed"));
        }
    } else if (response.resultClass == GdbResultError) {
        debugMessage(_("FETCHING START ADDRESS FAILED: " + response.toString()));
        emit inferiorStartFailed(_("Fetching start address failed"));
    }
#endif
}

void PlainGdbAdapter::handleExecRun(const GdbResultRecord &response, const QVariant &)
{
    if (response.resultClass == GdbResultRunning) {
        emit inferiorStarted();
    } else {
        QTC_ASSERT(response.resultClass == GdbResultError, /**/);
        const QByteArray &msg = response.data.findChild("msg").data();
        //QTC_ASSERT(status() == DebuggerInferiorRunning, /**/);
        //interruptInferior();
        emit inferiorStartFailed(msg);
    }
}

void PlainGdbAdapter::interruptInferior()
{
    debugMessage(_("TRYING TO INTERUPT INFERIOR"));
    if (m_engine->startMode() == StartRemote) {
        m_engine->postCommand(_("-exec-interrupt"));
        return;
    }

    const qint64 attachedPID = m_engine->inferiorPid();
    if (attachedPID <= 0) {
        debugMessage(_("TRYING TO INTERRUPT INFERIOR BEFORE PID WAS OBTAINED"));
        return;
    }

    if (!interruptProcess(attachedPID))
        debugMessage(_("CANNOT INTERRUPT %1").arg(attachedPID));
}

void PlainGdbAdapter::shutdownAdapter()
{
    m_engine->postCommand(_("-gdb-exit"), CB(handleExit));
    // 20s can easily happen when loading webkit debug information
    if (!m_gdbProc.waitForFinished(20000)) {
        debugMessage(_("FORCING TERMINATION: %1")
            .arg(state()));
        m_gdbProc.terminate();
        m_gdbProc.waitForFinished(20000);
    }

    if (state() != QProcess::NotRunning) {
        debugMessage(_("PROBLEM STOPPING DEBUGGER: STATE %1")
            .arg(state()));
        m_gdbProc.kill();
    }
}

void PlainGdbAdapter::handleExit(const GdbResultRecord &response, const QVariant &)
{
    if (response.resultClass == GdbResultDone) {
        emit adapterShutDown();
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Gdb process could not be stopped:\n") +
            __(response.data.findChild("msg").data());
        emit adapterShutdownFailed(msg);
    }
}

void PlainGdbAdapter::handleFinished(int, QProcess::ExitStatus)
{
     debugMessage(_("GDB PROESS FINISHED"));
}

void PlainGdbAdapter::shutdownInferior()
{
    m_engine->postCommand(_("kill"));
}

void PlainGdbAdapter::stubStarted()
{
    const qint64 attachedPID = m_stubProc.applicationPID();
    emit inferiorPidChanged(attachedPID);
    m_engine->postCommand(_("attach %1").arg(attachedPID), CB(handleStubAttached));
}

void PlainGdbAdapter::handleStubAttached(const GdbResultRecord &, const QVariant &)
{
    qDebug() << "STUB ATTACHED, FIXME";
    //qq->notifyInferiorStopped();
    //handleAqcuiredInferior();
    // FIXME: m_autoContinue = true;
}

void PlainGdbAdapter::stubError(const QString &msg)
{
    QMessageBox::critical(m_engine->mainWindow(), tr("Debugger Error"), msg);
}

void PlainGdbAdapter::emitAdapterStartFailed(const QString &msg)
{
    //  QMessageBox::critical(mainWindow(), tr("Debugger Startup Failure"),
    //    tr("Cannot start debugger: %1").arg(m_gdbAdapter->errorString()));
    m_stubProc.blockSignals(true);
    m_stubProc.stop();
    m_stubProc.blockSignals(false);
    emit adapterStartFailed(msg);
}
} // namespace Internal
} // namespace Debugger
