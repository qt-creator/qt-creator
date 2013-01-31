/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "analyzerconstants.h"
#include "ianalyzerengine.h"
#include "ianalyzertool.h"
#include "analyzermanager.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <coreplugin/ioutputpane.h>

#include <QDebug>

using namespace ProjectExplorer;

//////////////////////////////////////////////////////////////////////////
//
// AnalyzerRunControl::Private
//
//////////////////////////////////////////////////////////////////////////

namespace Analyzer {

class AnalyzerRunControl::Private
{
public:
    Private();

    bool m_isRunning;
    IAnalyzerEngine *m_engine;
};

AnalyzerRunControl::Private::Private()
   : m_isRunning(false), m_engine(0)
{}


//////////////////////////////////////////////////////////////////////////
//
// AnalyzerRunControl
//
//////////////////////////////////////////////////////////////////////////

AnalyzerRunControl::AnalyzerRunControl(IAnalyzerTool *tool,
        const AnalyzerStartParameters &sp, RunConfiguration *runConfiguration)
    : RunControl(runConfiguration, tool->runMode()),
      d(new Private)
{
    d->m_engine = tool->createEngine(sp, runConfiguration);

    if (!d->m_engine)
        return;

    connect(d->m_engine, SIGNAL(outputReceived(QString,Utils::OutputFormat)),
            SLOT(receiveOutput(QString,Utils::OutputFormat)));
    connect(d->m_engine, SIGNAL(taskToBeAdded(ProjectExplorer::Task::TaskType,QString,QString,int)),
            SLOT(addTask(ProjectExplorer::Task::TaskType,QString,QString,int)));
    connect(d->m_engine, SIGNAL(finished()),
            SLOT(engineFinished()));
}

AnalyzerRunControl::~AnalyzerRunControl()
{
    if (d->m_isRunning)
        stop();

    delete d->m_engine;
    d->m_engine = 0;
    delete d;
}

void AnalyzerRunControl::start()
{
    if (!d->m_engine) {
        emit finished();
        return;
    }

    AnalyzerManager::handleToolStarted();

    // Clear about-to-be-outdated tasks.
    ProjectExplorerPlugin::instance()->taskHub()
        ->clearTasks(Core::Id(Constants::ANALYZERTASK_ID));

    if (d->m_engine->start()) {
        d->m_isRunning = true;
        emit started();
    }
}

RunControl::StopResult AnalyzerRunControl::stop()
{
    if (!d->m_engine || !d->m_isRunning)
        return StoppedSynchronously;

    d->m_engine->stop();
    d->m_isRunning = false;
    return AsynchronousStop;
}

void AnalyzerRunControl::stopIt()
{
    if (stop() == RunControl::StoppedSynchronously)
        AnalyzerManager::handleToolFinished();
}

void AnalyzerRunControl::engineFinished()
{
    d->m_isRunning = false;
    AnalyzerManager::handleToolFinished();
    emit finished();
}

bool AnalyzerRunControl::isRunning() const
{
    return d->m_isRunning;
}

QString AnalyzerRunControl::displayName() const
{
    if (!d->m_engine)
        return QString();
    if (d->m_engine->runConfiguration())
        return d->m_engine->runConfiguration()->displayName();
    else
        return d->m_engine->startParameters().displayName;
}

QIcon AnalyzerRunControl::icon() const
{
    return QIcon(QLatin1String(":/images/analyzer_start_small.png"));
}

void AnalyzerRunControl::receiveOutput(const QString &text, Utils::OutputFormat format)
{
    appendMessage(text, format);
}

void AnalyzerRunControl::addTask(Task::TaskType type, const QString &description,
                                 const QString &file, int line)
{
    TaskHub *hub = ProjectExplorerPlugin::instance()->taskHub();
    hub->addTask(Task(type, description, Utils::FileName::fromUserInput(file), line,
                      Core::Id(Constants::ANALYZERTASK_ID)));
    hub->requestPopup();
}

} // namespace Analyzer
