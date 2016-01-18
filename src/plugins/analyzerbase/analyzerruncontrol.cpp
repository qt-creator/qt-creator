/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "analyzericons.h"
#include "analyzerruncontrol.h"
#include "ianalyzertool.h"
#include "analyzermanager.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <QDebug>
#include <QAction>

using namespace ProjectExplorer;

//////////////////////////////////////////////////////////////////////////
//
// AnalyzerRunControl
//
//////////////////////////////////////////////////////////////////////////

namespace Analyzer {

AnalyzerRunControl::AnalyzerRunControl(const AnalyzerStartParameters &sp,
        RunConfiguration *runConfiguration, Core::Id runMode)
    : RunControl(runConfiguration, runMode)
{
    setIcon(Icons::ANALYZER_CONTROL_START);

    if (runConfiguration) {
        m_displayName = runConfiguration->displayName();
        if (auto aspect = runConfiguration->extraAspect<WorkingDirectoryAspect>())
            m_workingDirectory = aspect->workingDirectory().toString();
        if (m_workingDirectory.isEmpty())
            m_workingDirectory = runConfiguration->target()->project()->projectDirectory().toString();
    }

    setRunnable(AnalyzerRunnable(sp));
    setConnection(AnalyzerConnection(sp));

    connect(this, &AnalyzerRunControl::finished,
            this, &AnalyzerRunControl::runControlFinished);
    connect(AnalyzerManager::stopAction(), &QAction::triggered,
            this, &AnalyzerRunControl::stopIt);
}

const AnalyzerRunnable &AnalyzerRunControl::runnable() const
{
    return RunControl::runnable().as<AnalyzerRunnable>();
}

const AnalyzerConnection &AnalyzerRunControl::connection() const
{
    return RunControl::connection().as<AnalyzerConnection>();
}

void AnalyzerRunControl::stopIt()
{
    if (stop() == RunControl::StoppedSynchronously)
        AnalyzerManager::handleToolFinished();
}

void AnalyzerRunControl::runControlFinished()
{
    m_isRunning = false;
    AnalyzerManager::handleToolFinished();
}

QString AnalyzerRunControl::workingDirectory() const
{
    return m_workingDirectory;
}

void AnalyzerRunControl::start()
{
    AnalyzerManager::handleToolStarted();

    if (startEngine()) {
        m_isRunning = true;
        emit started();
    }
}

RunControl::StopResult AnalyzerRunControl::stop()
{
    if (!m_isRunning)
        return StoppedSynchronously;

    stopEngine();
    m_isRunning = false;
    return AsynchronousStop;
}

bool AnalyzerRunControl::isRunning() const
{
    return m_isRunning;
}

QString AnalyzerRunControl::displayName() const
{
    return m_displayName;
}

void AnalyzerRunControl::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void AnalyzerRunControl::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

} // namespace Analyzer
