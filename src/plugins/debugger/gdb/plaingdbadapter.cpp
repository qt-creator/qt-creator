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

#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>

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
    // Output
    connect(&m_outputCollector, SIGNAL(byteDelivery(QByteArray)),
        engine, SLOT(readDebugeeOutput(QByteArray)));
}

AbstractGdbAdapter::DumperHandling PlainGdbAdapter::dumperHandling() const
{
    // LD_PRELOAD fails for System-Qt on Mac.
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    return DumperLoadedByGdb;
#else
    return DumperLoadedByGdbPreload;
#endif
}

void PlainGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    QStringList gdbArgs;

    if (!m_outputCollector.listen()) {
        emit adapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_outputCollector.errorString()), QString());
        return;
    }
    gdbArgs.append(_("--tty=") + m_outputCollector.serverName());

    if (!startParameters().workingDir.isEmpty())
        m_engine->m_gdbProc.setWorkingDirectory(startParameters().workingDir);
    if (!startParameters().environment.isEmpty())
        m_engine->m_gdbProc.setEnvironment(startParameters().environment);

    if (!m_engine->startGdb(gdbArgs)) {
        m_outputCollector.shutdown();
        return;
    }

    emit adapterStarted();
}

void PlainGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (!startParameters().processArgs.isEmpty())
        m_engine->postCommand(_("-exec-arguments ")
            + startParameters().processArgs.join(_(" ")));
    QFileInfo fi(startParameters().executable);
    m_engine->postCommand(_("-file-exec-and-symbols \"%1\"").arg(fi.absoluteFilePath()),
        CB(handleFileExecAndSymbols));
}

void PlainGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
#ifdef Q_OS_LINUX
        // Old gdbs do not announce the PID for programs without pthreads.
        // Note that successfully preloading the debugging helpers will
        // automatically load pthreads, so this will be unnecessary.
        if (m_engine->m_gdbVersion < 70000)
            m_engine->postCommand(_("info target"), CB(handleInfoTarget));
#endif
        emit inferiorPrepared();
    } else {
        QString msg = tr("Starting executable failed:\n") +
            __(response.data.findChild("msg").data());
        emit inferiorStartFailed(msg);
    }
}

#ifdef Q_OS_LINUX
void PlainGdbAdapter::handleInfoTarget(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // [some leading stdout here]
        // >&"        Entry point: 0x80831f0  0x08048134 - 0x08048147 is .interp\n"
        // [some trailing stdout here]
        QString msg = _(response.data.findChild("consolestreamoutput").data());
        QRegExp needle(_("\\bEntry point: 0x([0-9a-f]+)\\b"));
        if (needle.indexIn(msg) != -1) {
            m_engine->m_entryPoint =
                    "0x" + needle.cap(1).toLatin1().rightJustified(sizeof(void *) * 2, '0');
            m_engine->postCommand(_("tbreak *0x") + needle.cap(1));
            // Do nothing here - inferiorPrepared handles the sequencing.
        } else {
            emit inferiorStartFailed(_("Parsing start address failed"));
        }
    } else if (response.resultClass == GdbResultError) {
        emit inferiorStartFailed(_("Fetching start address failed"));
    }
}
#endif

void PlainGdbAdapter::startInferiorPhase2()
{
    setState(InferiorRunningRequested);
    m_engine->postCommand(_("-exec-run"), GdbEngine::RunRequest, CB(handleExecRun));
}

void PlainGdbAdapter::handleExecRun(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        QTC_ASSERT(state() == InferiorRunning, qDebug() << state());
        debugMessage(_("INFERIOR STARTED"));
        showStatusMessage(msgInferiorStarted());
    } else {
        QTC_ASSERT(state() == InferiorRunningRequested, qDebug() << state());
        const QByteArray &msg = response.data.findChild("msg").data();
        //QTC_ASSERT(status() == InferiorRunning, /**/);
        //interruptInferior();
        emit inferiorStartFailed(msg);
    }
}

void PlainGdbAdapter::interruptInferior()
{
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
    m_outputCollector.shutdown();
}

} // namespace Internal
} // namespace Debugger
