/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "analyzerruncontrol.h"
#include "ianalyzertool.h"
#include "analyzermanager.h"
#include "analyzerstartparameters.h"

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
        ProjectExplorer::RunConfiguration *runConfiguration)
    : RunControl(runConfiguration, sp.runMode)
{
    setIcon(QLatin1String(":/images/analyzer_start_small.png"));

    m_runConfig = runConfiguration;
    m_sp = sp;

    connect(this, SIGNAL(finished()), SLOT(runControlFinished()));
    connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), SLOT(stopIt()));
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
    return m_runConfig ? m_runConfig->displayName() : m_sp.displayName;
}

} // namespace Analyzer
