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

#include "callgrindengine.h"

#include "valgrindsettings.h"

#include <valgrind/callgrind/callgrindcontroller.h>
#include <valgrind/callgrind/callgrindparser.h>

#include <analyzerbase/analyzermanager.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace Valgrind;
using namespace Valgrind::Internal;

CallgrindEngine::CallgrindEngine(IAnalyzerTool *tool, const AnalyzerStartParameters &sp,
         ProjectExplorer::RunConfiguration *runConfiguration)
    : ValgrindEngine(tool, sp, runConfiguration)
    , m_markAsPaused(false)
{
    connect(&m_runner, SIGNAL(finished()), this, SLOT(slotFinished()));
    connect(&m_runner, SIGNAL(started()), this, SLOT(slotStarted()));
    connect(m_runner.parser(), SIGNAL(parserDataReady()), this, SLOT(slotFinished()));
    connect(&m_runner, SIGNAL(statusMessage(QString)), SLOT(showStatusMessage(QString)));

    m_progress->setProgressRange(0, 2);
}

void CallgrindEngine::showStatusMessage(const QString &msg)
{
    AnalyzerManager::showStatusMessage(msg);
}

QStringList CallgrindEngine::toolArguments() const
{
    QStringList arguments;

    ValgrindBaseSettings *callgrindSettings = m_settings->subConfig<ValgrindBaseSettings>();
    QTC_ASSERT(callgrindSettings, return arguments);

    if (callgrindSettings->enableCacheSim())
        arguments << QLatin1String("--cache-sim=yes");

    if (callgrindSettings->enableBranchSim())
        arguments << QLatin1String("--branch-sim=yes");

    if (callgrindSettings->collectBusEvents())
        arguments << QLatin1String("--collect-bus=yes");

    if (callgrindSettings->collectSystime())
        arguments << QLatin1String("--collect-systime=yes");

    if (m_markAsPaused)
        arguments << QLatin1String("--instr-atstart=no");

    // add extra arguments
    if (!m_argumentForToggleCollect.isEmpty())
        arguments << m_argumentForToggleCollect;

    return arguments;
}

QString CallgrindEngine::progressTitle() const
{
    return tr("Profiling");
}

Valgrind::ValgrindRunner * CallgrindEngine::runner()
{
    return &m_runner;
}

bool CallgrindEngine::start()
{
    emit outputReceived(tr("Profiling %1\n").arg(executable()), Utils::NormalMessageFormat);
    return ValgrindEngine::start();
}

void CallgrindEngine::dump()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::Dump);
}

void CallgrindEngine::setPaused(bool paused)
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

void CallgrindEngine::setToggleCollectFunction(const QString &toggleCollectFunction)
{
    if (toggleCollectFunction.isEmpty())
        return;

    m_argumentForToggleCollect = QLatin1String("--toggle-collect=") + toggleCollectFunction;
}

void CallgrindEngine::reset()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::ResetEventCounters);
}

void CallgrindEngine::pause()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::Pause);
}

void CallgrindEngine::unpause()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::UnPause);
}

Valgrind::Callgrind::ParseData *CallgrindEngine::takeParserData()
{
    return m_runner.parser()->takeData();
}

void CallgrindEngine::slotFinished()
{
    emit parserDataReady(this);
}

void CallgrindEngine::slotStarted()
{
    m_progress->setProgressValue(1);;
}
