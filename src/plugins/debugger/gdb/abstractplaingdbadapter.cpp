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

#include "abstractplaingdbadapter.h"

#include "debuggeractions.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&AbstractPlainGdbAdapter::callback), \
    STRINGIFY(callback)

AbstractPlainGdbAdapter::AbstractPlainGdbAdapter(GdbEngine *engine,
                                                 QObject *parent)
    : AbstractGdbAdapter(engine, parent)
{
}

void AbstractPlainGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSettingUp, qDebug() << state());
    if (!startParameters().processArgs.isEmpty()) {
        QString args = startParameters().processArgs.join(_(" "));
        m_engine->postCommand("-exec-arguments " + toLocalEncoding(args));
    }
    m_engine->postCommand("-file-exec-and-symbols \"" + execFilePath() + '"',
        CB(handleFileExecAndSymbols));
}

void AbstractPlainGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSettingUp, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        if (infoTargetNecessary()) {
            // Old gdbs do not announce the PID for programs without pthreads.
            // Note that successfully preloading the debugging helpers will
            // automatically load pthreads, so this will be unnecessary.
            if (m_engine->m_gdbVersion < 70000)
                m_engine->postCommand("info target", CB(handleInfoTarget));
        }
        m_engine->handleInferiorPrepared();
    } else {
        QByteArray ba = response.data.findChild("msg").data();
        QString msg = fromLocalEncoding(ba);
        // Extend the message a bit in unknown cases.
        if (!ba.endsWith("File format not recognized"))
            msg = tr("Starting executable failed:\n") + msg;
        m_engine->handleInferiorSetupFailed(msg);
    }
}

void AbstractPlainGdbAdapter::runAdapter()
{
    setState(InferiorRunningRequested);
    m_engine->postCommand("-exec-run", GdbEngine::RunRequest, CB(handleExecRun));
}

void AbstractPlainGdbAdapter::handleExecRun(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        QTC_ASSERT(state() == InferiorRunning, qDebug() << state());
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgInferiorSetupOk(), StatusBar);
        // FIXME: That's the wrong place for it.
        if (theDebuggerBoolSetting(EnableReverseDebugging))
            m_engine->postCommand("target record");
    } else {
        QTC_ASSERT(state() == InferiorRunningRequested, qDebug() << state());
        QString msg = fromLocalEncoding(response.data.findChild("msg").data());
        //QTC_ASSERT(status() == InferiorRunning, /**/);
        //interruptInferior();
        m_engine->handleInferiorSetupFailed(msg);
    }
}

void AbstractPlainGdbAdapter::handleInfoTarget(const GdbResponse &response)
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
            m_engine->postCommand("tbreak *0x" + needle.cap(1).toAscii());
            // Do nothing here - inferiorPrepared handles the sequencing.
        } else {
            m_engine->handleInferiorSetupFailed(_("Parsing start address failed"));
        }
    } else if (response.resultClass == GdbResultError) {
        m_engine->handleInferiorSetupFailed(_("Fetching start address failed"));
    }
}

} // namespace Debugger
} // namespace Internal
