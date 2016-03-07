/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "callgrindengine.h"

#include "callgrindtool.h"
#include "valgrindsettings.h"

#include <valgrind/callgrind/callgrindcontroller.h>
#include <valgrind/callgrind/callgrindparser.h>

#include <debugger/analyzer/analyzermanager.h>

#include <utils/qtcassert.h>

using namespace Debugger;
using namespace Valgrind;
using namespace Valgrind::Internal;

CallgrindRunControl::CallgrindRunControl(ProjectExplorer::RunConfiguration *runConfiguration, Core::Id runMode)
    : ValgrindRunControl(runConfiguration, runMode)
    , m_markAsPaused(false)
{
    connect(&m_runner, &Callgrind::CallgrindRunner::finished,
            this, &CallgrindRunControl::slotFinished);
    connect(m_runner.parser(), &Callgrind::Parser::parserDataReady,
            this, &CallgrindRunControl::slotFinished);
    connect(&m_runner, &Callgrind::CallgrindRunner::statusMessage,
            this, &Debugger::showPermanentStatusMessage);
}

QStringList CallgrindRunControl::toolArguments() const
{
    QStringList arguments;

    QTC_ASSERT(m_settings, return arguments);

    if (m_settings->enableCacheSim())
        arguments << QLatin1String("--cache-sim=yes");

    if (m_settings->enableBranchSim())
        arguments << QLatin1String("--branch-sim=yes");

    if (m_settings->collectBusEvents())
        arguments << QLatin1String("--collect-bus=yes");

    if (m_settings->collectSystime())
        arguments << QLatin1String("--collect-systime=yes");

    if (m_markAsPaused)
        arguments << QLatin1String("--instr-atstart=no");

    // add extra arguments
    if (!m_argumentForToggleCollect.isEmpty())
        arguments << m_argumentForToggleCollect;

    return arguments;
}

QString CallgrindRunControl::progressTitle() const
{
    return tr("Profiling");
}

ValgrindRunner * CallgrindRunControl::runner()
{
    return &m_runner;
}

void CallgrindRunControl::start()
{
    appendMessage(tr("Profiling %1").arg(executable()) + QLatin1Char('\n'), Utils::NormalMessageFormat);
    return ValgrindRunControl::start();
}

void CallgrindRunControl::dump()
{
    m_runner.controller()->run(Callgrind::CallgrindController::Dump);
}

void CallgrindRunControl::setPaused(bool paused)
{
    if (m_markAsPaused == paused)
        return;

    m_markAsPaused = paused;

    // call controller only if it is attached to a valgrind process
    if (m_runner.controller()->valgrindProcess()) {
        if (paused)
            pause();
        else
            unpause();
    }
}

void CallgrindRunControl::setToggleCollectFunction(const QString &toggleCollectFunction)
{
    if (toggleCollectFunction.isEmpty())
        return;

    m_argumentForToggleCollect = QLatin1String("--toggle-collect=") + toggleCollectFunction;
}

void CallgrindRunControl::reset()
{
    m_runner.controller()->run(Callgrind::CallgrindController::ResetEventCounters);
}

void CallgrindRunControl::pause()
{
    m_runner.controller()->run(Callgrind::CallgrindController::Pause);
}

void CallgrindRunControl::unpause()
{
    m_runner.controller()->run(Callgrind::CallgrindController::UnPause);
}

Callgrind::ParseData *CallgrindRunControl::takeParserData()
{
    return m_runner.parser()->takeData();
}

void CallgrindRunControl::slotFinished()
{
    emit parserDataReady(this);
}
