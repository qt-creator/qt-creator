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

#include "abstractplaingdbadapter.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerprotocol.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"

#include <QDir>
#include <QFile>
#include <QTemporaryFile>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::GdbCommandCallback>(&GdbAbstractPlainEngine::callback), \
    STRINGIFY(callback)

GdbAbstractPlainEngine::GdbAbstractPlainEngine(const DebuggerStartParameters &startParameters)
    : GdbEngine(startParameters)
{}

void GdbAbstractPlainEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (!startParameters().processArgs.isEmpty()) {
        QString args = startParameters().processArgs;
        postCommand("-exec-arguments " + toLocalEncoding(args));
    }
    postCommand("-file-exec-and-symbols \"" + execFilePath() + '"',
        CB(handleFileExecAndSymbols));
}

void GdbAbstractPlainEngine::handleFileExecAndSymbols(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        handleInferiorPrepared();
    } else {
        QByteArray ba = response.data.findChild("msg").data();
        QString msg = fromLocalEncoding(ba);
        // Extend the message a bit in unknown cases.
        if (!ba.endsWith("File format not recognized"))
            msg = tr("Starting executable failed:\n") + msg;
        notifyInferiorSetupFailed(msg);
    }
}

void GdbAbstractPlainEngine::runEngine()
{
    postCommand("-exec-run", GdbEngine::RunRequest, CB(handleExecRun));
}

void GdbAbstractPlainEngine::handleExecRun(const GdbResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        //notifyEngineRunOkAndInferiorRunRequested();
        notifyEngineRunAndInferiorRunOk(); // For gdb < 7.0
        //showStatusMessage(tr("Running..."));
        showMessage(_("INFERIOR STARTED"));
        showMessage(msgInferiorSetupOk(), StatusBar);
        // FIXME: That's the wrong place for it.
        if (debuggerCore()->boolSetting(EnableReverseDebugging))
            postCommand("target record");
    } else {
        QString msg = fromLocalEncoding(response.data.findChild("msg").data());
        //QTC_CHECK(status() == InferiorRunOk);
        //interruptInferior();
        showMessage(msg);
        notifyEngineRunFailed();
    }
}

} // namespace Debugger
} // namespace Internal
