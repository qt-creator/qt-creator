/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "abstractplaingdbadapter.h"
#include "gdbmi.h"
#include "gdbengine.h"
#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&AbstractPlainGdbAdapter::callback), \
    STRINGIFY(callback)

AbstractPlainGdbAdapter::AbstractPlainGdbAdapter(GdbEngine *engine)
    : AbstractGdbAdapter(engine)
{
}

void AbstractPlainGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (!startParameters().processArgs.isEmpty()) {
        QString args = startParameters().processArgs;
        m_engine->postCommand("-exec-arguments " + toLocalEncoding(args));
    }
    m_engine->postCommand("-file-exec-and-symbols \"" + execFilePath() + '"',
        CB(handleFileExecAndSymbols));
}

void AbstractPlainGdbAdapter::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
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
        m_engine->notifyInferiorSetupFailed(msg);
    }
}

void AbstractPlainGdbAdapter::runEngine()
{
    m_engine->postCommand("-exec-run", GdbEngine::RunRequest, CB(handleExecRun));
}

void AbstractPlainGdbAdapter::handleExecRun(const GdbResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        m_engine->notifyEngineRunAndInferiorRunOk();
        //showStatusMessage(tr("Running..."));
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgInferiorSetupOk(), StatusBar);
        // FIXME: That's the wrong place for it.
        if (debuggerCore()->boolSetting(EnableReverseDebugging))
            m_engine->postCommand("target record");
    } else {
        QString msg = fromLocalEncoding(response.data.findChild("msg").data());
        //QTC_ASSERT(status() == InferiorRunOk, /**/);
        //interruptInferior();
        showMessage(msg);
        m_engine->notifyEngineRunFailed();
    }
}

void AbstractPlainGdbAdapter::handleInfoTarget(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // [some leading stdout here]
        // >&"        Entry point: 0x80831f0  0x08048134 - 0x08048147 is .interp\n"
        // [some trailing stdout here]
        QString msg = _(response.consoleStreamOutput);
        QRegExp needle(_("\\bEntry point: 0x([0-9a-f]+)\\b"));
        if (needle.indexIn(msg) != -1) {
            m_engine->m_entryPoint =
                    "0x" + needle.cap(1).toLatin1().rightJustified(sizeof(void *) * 2, '0');
            m_engine->postCommand("tbreak *0x" + needle.cap(1).toAscii());
            // Do nothing here - inferiorPrepared handles the sequencing.
        } else {
            m_engine->notifyInferiorSetupFailed(_("Parsing start address failed"));
        }
    } else if (response.resultClass == GdbResultError) {
        m_engine->notifyInferiorSetupFailed(_("Fetching start address failed"));
    }
}

} // namespace Debugger
} // namespace Internal
