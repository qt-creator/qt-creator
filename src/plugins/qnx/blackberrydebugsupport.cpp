/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydebugsupport.h"
#include "blackberryapplicationrunner.h"

#include <debugger/debuggerrunner.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryDebugSupport::BlackBerryDebugSupport(BlackBerryRunConfiguration *runConfig,
                                 Debugger::DebuggerRunControl *runControl)
    : QObject(runControl->engine())
    , m_engine(runControl->engine())
{
    BlackBerryApplicationRunner::LaunchFlags launchFlags;
    if (m_engine->startParameters().languages & Debugger::CppLanguage)
        launchFlags |= BlackBerryApplicationRunner::CppDebugLaunch;
    if (m_engine->startParameters().languages & Debugger::QmlLanguage)
        launchFlags |= BlackBerryApplicationRunner::QmlDebugLaunch;
    m_runner = new BlackBerryApplicationRunner(launchFlags, runConfig, this);

    connect(m_engine, SIGNAL(requestRemoteSetup()), this, SLOT(launchRemoteApplication()));
    connect(m_engine, SIGNAL(stateChanged(Debugger::DebuggerState)),
            this, SLOT(handleDebuggerStateChanged(Debugger::DebuggerState)));

    connect(m_runner, SIGNAL(started()), this, SLOT(handleStarted()));
    connect(m_runner, SIGNAL(startFailed(QString)), this, SLOT(handleStartFailed(QString)));
    connect(m_runner, SIGNAL(output(QString,Utils::OutputFormat)),
            this, SLOT(handleApplicationOutput(QString,Utils::OutputFormat)));

    connect(this, SIGNAL(output(QString,Utils::OutputFormat)),
            runControl, SLOT(appendMessage(QString,Utils::OutputFormat)));
}

void BlackBerryDebugSupport::launchRemoteApplication()
{
    m_runner->start();
}

void BlackBerryDebugSupport::handleStarted()
{
    m_engine->startParameters().attachPID = m_runner->pid();
    m_engine->notifyEngineRemoteSetupDone(8000, -1);
}

void BlackBerryDebugSupport::handleStartFailed(const QString &message)
{
    m_engine->notifyEngineRemoteSetupFailed(message);
}

void BlackBerryDebugSupport::handleDebuggerStateChanged(Debugger::DebuggerState state)
{
    if (state == Debugger::EngineShutdownOk || state == Debugger::DebuggerFinished) {
        if (m_runner->isRunning())
            m_runner->stop();
    }
}

void BlackBerryDebugSupport::handleApplicationOutput(const QString &msg, Utils::OutputFormat format)
{
    Q_UNUSED(format)
    m_engine->showMessage(msg, Debugger::AppOutput);
}
