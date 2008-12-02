/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "debuggerrunner.h"

#include "assert.h"
#include "debuggermanager.h"

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

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
    Q_UNUSED(mode);
    Q_ASSERT(mode == ProjectExplorer::Constants::DEBUGMODE);
    ApplicationRunConfigurationPtr rc =
        qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration);
    Q_ASSERT(rc);
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
            this, SLOT(debuggingFinished()));
    connect(m_manager, SIGNAL(applicationOutputAvailable(QString, QString)),
            this, SLOT(slotAddToOutputWindow(QString, QString)));
    connect(m_manager, SIGNAL(inferiorPidChanged(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)));
}

void DebuggerRunControl::start()
{
    m_running = true;
    ApplicationRunConfigurationPtr rc =
        qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration());
    QWB_ASSERT(rc, return);
    ProjectExplorer::Project *project = rc->project();
    QWB_ASSERT(project, return);

    m_manager->m_executable = rc->executable();
    m_manager->m_environment = rc->environment().toStringList();
    m_manager->m_workingDir = rc->workingDirectory();
    m_manager->m_processArgs = rc->commandLineArguments();
    m_manager->m_buildDir =
        project->buildDirectory(project->activeBuildConfiguration());
    //<daniel> andre: + "\qtc-gdbmacros\"

    //emit addToOutputWindow(this, tr("Debugging %1").arg(m_executable));
    if (m_manager->startNewDebugger(DebuggerManager::startInternal))
        emit started();
    else
        debuggingFinished();
}

void DebuggerRunControl::slotAddToOutputWindow(const QString &prefix, const QString &line)
{  
    Q_UNUSED(prefix);
    foreach (const QString &l, line.split('\n'))
        emit addToOutputWindow(this, prefix + l);
    //emit addToOutputWindow(this, prefix + line);
}

void DebuggerRunControl::stop()
{
    m_manager->exitDebugger();
}

void DebuggerRunControl::debuggingFinished()
{
    m_running = false;
    //emit addToOutputWindow(this, tr("Debugging %1 finished").arg(m_executable));
    emit finished();
}

bool DebuggerRunControl::isRunning() const
{
    return m_running;
}
