/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "valgrindengine.h"
#include "valgrindsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/applicationrunconfiguration.h>

#define VALGRIND_DEBUG_OUTPUT 0

using namespace Analyzer;
using namespace Analyzer::Internal;
using namespace Utils;

ValgrindEngine::ValgrindEngine(ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(runConfiguration),
      m_settings(0),
      m_progress(new QFutureInterface<void>()) ,
      m_isStopping(false)
{
    ProjectExplorer::LocalApplicationRunConfiguration *localAppConfig =
            qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration);

    m_settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    if (!localAppConfig || !m_settings)
        return;

    m_workingDirectory = localAppConfig->workingDirectory();
    m_executable = localAppConfig->executable();
    m_commandLineArguments = localAppConfig->commandLineArguments();
    m_environment = localAppConfig->environment();
}

ValgrindEngine::~ValgrindEngine()
{
    delete m_progress;
}

void ValgrindEngine::start()
{
    emit starting(this);

    Core::FutureProgress* fp = Core::ICore::instance()->progressManager()->addTask(m_progress->future(),
                                                        progressTitle(), "valgrind");
    fp->setKeepOnFinish(Core::FutureProgress::DontKeepOnFinish);
    m_progress->reportStarted();

#if VALGRIND_DEBUG_OUTPUT
    emit standardOutputReceived(tr("Valgrind options: %1").arg(toolArguments().join(" ")));
    emit standardOutputReceived(tr("Working directory: %1").arg(m_workingDirectory));
    emit standardOutputReceived(tr("Command-line arguments: %1").arg(m_commandLineArguments));
#endif

    runner()->setWorkingDirectory(m_workingDirectory);
    runner()->setValgrindExecutable(m_settings->subConfig<ValgrindSettings>()->valgrindExecutable());
    runner()->setValgrindArguments(toolArguments());
    runner()->setDebuggeeExecutable(m_executable);
    // note that m_commandLineArguments may contain several arguments in one string
    runner()->setDebuggeeArguments(m_commandLineArguments);
    runner()->setEnvironment(m_environment);

    connect(runner(), SIGNAL(standardOutputReceived(QByteArray)),
            SLOT(receiveStandardOutput(QByteArray)));
    connect(runner(), SIGNAL(standardErrorReceived(QByteArray)),
            SLOT(receiveStandardError(QByteArray)));
    connect(runner(), SIGNAL(processErrorReceived(QString, QProcess::ProcessError)),
            SLOT(receiveProcessError(QString, QProcess::ProcessError)));
    connect(runner(), SIGNAL(finished()),
            SLOT(runnerFinished()));

    runner()->start();
}

void ValgrindEngine::stop()
{
    m_isStopping = true;
    runner()->stop();
}

QString ValgrindEngine::executable() const
{
    return m_executable;
}

void ValgrindEngine::runnerFinished()
{
    emit standardOutputReceived(tr("** Analysing finished **"));
    emit finished();

    m_progress->reportFinished();

    disconnect(runner(), SIGNAL(standardOutputReceived(QByteArray)),
               this, SLOT(receiveStandardOutput(QByteArray)));
    disconnect(runner(), SIGNAL(standardErrorReceived(QByteArray)),
               this, SLOT(receiveStandardError(QByteArray)));
    disconnect(runner(), SIGNAL(processErrorReceived(QString, QProcess::ProcessError)),
               this, SLOT(receiveProcessError(QString, QProcess::ProcessError)));
    disconnect(runner(), SIGNAL(finished()),
               this, SLOT(runnerFinished()));
}

void ValgrindEngine::receiveStandardOutput(const QByteArray &b)
{
    emit standardOutputReceived(QString::fromLocal8Bit(b));
}

void ValgrindEngine::receiveStandardError(const QByteArray &b)
{
    emit standardErrorReceived(QString::fromLocal8Bit(b));
}

void ValgrindEngine::receiveProcessError(const QString &error, QProcess::ProcessError e)
{
    if (e == QProcess::FailedToStart) {
        const QString &valgrind = m_settings->subConfig<ValgrindSettings>()->valgrindExecutable();
        if (!valgrind.isEmpty()) {
            emit standardErrorReceived(tr("** Error: \"%1\" could not be started: %2 **").arg(valgrind).arg(error));
        } else {
            emit standardErrorReceived(tr("** Error: no valgrind executable set **"));
        }
    } else if (m_isStopping && e == QProcess::Crashed) { // process gets killed on stop
        emit standardErrorReceived(tr("** Process Terminated **"));
    } else {
        emit standardErrorReceived(QString("** %1 **").arg(error));
    }

    if (m_isStopping)
        return;

    ///FIXME: get a better API for this into Qt Creator
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QList< Core::IOutputPane *> panes = pm->getObjects<Core::IOutputPane>();
    foreach(Core::IOutputPane *pane, panes) {
        if (pane->displayName() == tr("Application Output")) {
            pane->popup(false);
            break;
        }
    }
}
