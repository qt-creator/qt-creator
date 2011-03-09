/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "analyzerruncontrol.h"
#include "analyzerconstants.h"
#include "ianalyzerengine.h"
#include "ianalyzertool.h"
#include "analyzerplugin.h"
#include "analyzermanager.h"
#include "analyzerrunconfigwidget.h"
#include "analyzersettings.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <coreplugin/ioutputpane.h>

#include <QtCore/QDebug>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>

using namespace Analyzer;
using namespace Analyzer::Internal;

// AnalyzerRunControlFactory ////////////////////////////////////////////////////
AnalyzerRunControlFactory::AnalyzerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool AnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    if (!qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration))
        return false;
    return mode == Constants::MODE_ANALYZE;
}

ProjectExplorer::RunControl *AnalyzerRunControlFactory::create(RunConfiguration *runConfiguration,
                                                               const QString &mode)
{
    if (!qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration) ||
         mode != Constants::MODE_ANALYZE) {
        return 0;
    }
    AnalyzerRunControl *rc = new AnalyzerRunControl(runConfiguration);
    emit runControlCreated(rc);
    return rc;
}

QString AnalyzerRunControlFactory::displayName() const
{
    return tr("Analyzer");
}

ProjectExplorer::IRunConfigurationAspect *AnalyzerRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerProjectSettings;
}

ProjectExplorer::RunConfigWidget *AnalyzerRunControlFactory::createConfigurationWidget(RunConfiguration
                                                                                       *runConfiguration)
{
    ProjectExplorer::LocalApplicationRunConfiguration *localRc =
        qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration);
    if (!localRc)
        return 0;
    AnalyzerProjectSettings *settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    if (!settings)
        return 0;

    AnalyzerRunConfigWidget *ret = new AnalyzerRunConfigWidget;
    ret->setRunConfiguration(runConfiguration);
    return ret;
}

// AnalyzerRunControl ////////////////////////////////////////////////////
AnalyzerRunControl::AnalyzerRunControl(RunConfiguration *runConfiguration)
    : RunControl(runConfiguration, Constants::MODE_ANALYZE),
      m_isRunning(false),
      m_engine(0)
{
    IAnalyzerTool *tool = AnalyzerManager::instance()->currentTool();
    m_engine = tool->createEngine(runConfiguration);

    connect(m_engine, SIGNAL(standardErrorReceived(QString)),
            SLOT(receiveStandardError(QString)));
    connect(m_engine, SIGNAL(standardOutputReceived(QString)),
            SLOT(receiveStandardOutput(QString)));
    connect(m_engine, SIGNAL(taskToBeAdded(ProjectExplorer::Task::TaskType,QString,QString,int)),
            SLOT(addTask(ProjectExplorer::Task::TaskType,QString,QString,int)));
    connect(m_engine, SIGNAL(finished()),
            SLOT(engineFinished()));
}

AnalyzerRunControl::~AnalyzerRunControl()
{
    if (m_isRunning)
        stop();
}

void AnalyzerRunControl::start()
{
    // clear about-to-be-outdated tasks
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ProjectExplorer::TaskHub *hub = pm->getObject<ProjectExplorer::TaskHub>();
    hub->clearTasks(Constants::ANALYZERTASK_ID);

    m_isRunning = true;
    emit started();
    m_engine->start();
}

ProjectExplorer::RunControl::StopResult AnalyzerRunControl::stop()
{
    m_engine->stop();
    m_isRunning = false;
    return AsynchronousStop;
}

void AnalyzerRunControl::engineFinished()
{
    m_isRunning = false;
    emit finished();
}

bool AnalyzerRunControl::isRunning() const
{
    return m_isRunning;
}

QString AnalyzerRunControl::displayName() const
{
    return AnalyzerManager::instance()->currentTool()->displayName();
}

void AnalyzerRunControl::receiveStandardOutput(const QString &text)
{
    appendMessage(text, ProjectExplorer::StdOutFormat);
}

void AnalyzerRunControl::receiveStandardError(const QString &text)
{
    appendMessage(text, ProjectExplorer::StdErrFormat);
}

void AnalyzerRunControl::addTask(ProjectExplorer::Task::TaskType type, const QString &description,
                                 const QString &file, int line)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ProjectExplorer::TaskHub *hub = pm->getObject<ProjectExplorer::TaskHub>();
    hub->addTask(ProjectExplorer::Task(type, description, file, line, Constants::ANALYZERTASK_ID));

    ///FIXME: get a better API for this into Qt Creator
    QList<Core::IOutputPane *> panes = pm->getObjects<Core::IOutputPane>();
    foreach (Core::IOutputPane *pane, panes) {
        if (pane->displayName() == tr("Build Issues")) {
            pane->popup(false);
            break;
        }
    }
}
