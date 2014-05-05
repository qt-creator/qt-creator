/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

CallgrindRunControl::CallgrindRunControl(const AnalyzerStartParameters &sp,
         ProjectExplorer::RunConfiguration *runConfiguration)
    : ValgrindRunControl(sp, runConfiguration)
    , m_markAsPaused(false)
{
    connect(&m_runner, SIGNAL(finished()), this, SLOT(slotFinished()));
    connect(m_runner.parser(), SIGNAL(parserDataReady()), this, SLOT(slotFinished()));
    connect(&m_runner, SIGNAL(statusMessage(QString)), SLOT(showStatusMessage(QString)));
}

void CallgrindRunControl::showStatusMessage(const QString &msg)
{
    AnalyzerManager::showStatusMessage(msg);
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

Valgrind::ValgrindRunner * CallgrindRunControl::runner()
{
    return &m_runner;
}

bool CallgrindRunControl::startEngine()
{
    appendMessage(tr("Profiling %1").arg(executable()) + QLatin1Char('\n'), Utils::NormalMessageFormat);
    return ValgrindRunControl::startEngine();
}

void CallgrindRunControl::dump()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::Dump);
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
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::ResetEventCounters);
}

void CallgrindRunControl::pause()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::Pause);
}

void CallgrindRunControl::unpause()
{
    m_runner.controller()->run(Valgrind::Callgrind::CallgrindController::UnPause);
}

Valgrind::Callgrind::ParseData *CallgrindRunControl::takeParserData()
{
    return m_runner.parser()->takeData();
}

void CallgrindRunControl::slotFinished()
{
    emit parserDataReady(this);
}
