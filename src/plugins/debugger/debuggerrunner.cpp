/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "debuggerrunner.h"

#include "debuggermanager.h"

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtGui/QTextDocument>

using namespace Debugger::Internal;

using ProjectExplorer::RunConfiguration;
using ProjectExplorer::RunControl;
using ProjectExplorer::ApplicationRunConfiguration;


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunner
//
////////////////////////////////////////////////////////////////////////

DebuggerRunner::DebuggerRunner(DebuggerManager *manager)
  : m_manager(manager)
{}

bool DebuggerRunner::canRun(RunConfigurationPtr runConfiguration, const QString &mode)
{
    return mode == ProjectExplorer::Constants::DEBUGMODE 
       && !qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration).isNull();
}

QString DebuggerRunner::displayName() const
{
    return QObject::tr("Debug");
}

RunControl* DebuggerRunner::run(RunConfigurationPtr runConfiguration, const QString &mode)
{
    QTC_ASSERT(mode == ProjectExplorer::Constants::DEBUGMODE, return 0);
    ApplicationRunConfigurationPtr rc =
        qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration);
    QTC_ASSERT(rc, return 0);
    //qDebug() << "***** Debugging" << rc->name() << rc->executable();
    return new DebuggerRunControl(m_manager, rc);
}

QWidget *DebuggerRunner::configurationWidget(RunConfigurationPtr runConfiguration)
{
    // NBS TODO: Add GDB-specific configuration widget
    Q_UNUSED(runConfiguration);
    return 0;
}



////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControl
//
////////////////////////////////////////////////////////////////////////


DebuggerRunControl::DebuggerRunControl(DebuggerManager *manager,
        QSharedPointer<ApplicationRunConfiguration> runConfiguration)
  : RunControl(runConfiguration), m_manager(manager), m_running(false)
{
    connect(m_manager, SIGNAL(debuggingFinished()),
            this, SLOT(debuggingFinished()),
            Qt::QueuedConnection);
    connect(m_manager, SIGNAL(applicationOutputAvailable(QString)),
            this, SLOT(slotAddToOutputWindowInline(QString)),
            Qt::QueuedConnection);
    connect(m_manager, SIGNAL(inferiorPidChanged(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(stopRequested()),
            m_manager, SLOT(exitDebugger()));
}

void DebuggerRunControl::start()
{
    m_running = true;
    ApplicationRunConfigurationPtr rc =
        qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration());
    QTC_ASSERT(rc, return);
    ProjectExplorer::Project *project = rc->project();
    QTC_ASSERT(project, return);

    m_manager->m_executable = rc->executable();
    m_manager->m_environment = rc->environment().toStringList();
    m_manager->m_workingDir = rc->workingDirectory();
    m_manager->m_processArgs = rc->commandLineArguments();
    m_manager->m_buildDir =
        project->buildDirectory(project->activeBuildConfiguration());
    //<daniel> andre: + "\qtc-gdbmacros\"

    //emit addToOutputWindow(this, tr("Debugging %1").arg(m_executable));
    if (m_manager->startNewDebugger(DebuggerManager::StartInternal))
        emit started();
    else
        debuggingFinished();
}

void DebuggerRunControl::slotAddToOutputWindowInline(const QString &data)
{  
    emit addToOutputWindowInline(this, data);
}

void DebuggerRunControl::stop()
{
    //qDebug() << "DebuggerRunControl::stop";
    m_running = false;
    emit stopRequested();
}

void DebuggerRunControl::debuggingFinished()
{
    m_running = false;
    //qDebug() << "DebuggerRunControl::finished";
    //emit addToOutputWindow(this, tr("Debugging %1 finished").arg(m_executable));
    emit finished();
}

bool DebuggerRunControl::isRunning() const
{
    //qDebug() << "DebuggerRunControl::isRunning" << m_running;
    return m_running;
}
