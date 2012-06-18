/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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
#include <analyzerbase/analyzermanager.h>

#include <QApplication>
#include <QMainWindow>

#define VALGRIND_DEBUG_OUTPUT 0

using namespace Analyzer;
using namespace Valgrind::Internal;
using namespace Utils;

const int progressMaximum  = 1000000;

ValgrindEngine::ValgrindEngine(IAnalyzerTool *tool, const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(tool, sp, runConfiguration),
      m_settings(0),
      m_progress(new QFutureInterface<void>()),
      m_progressWatcher(new QFutureWatcher<void>()),
      m_isStopping(false)
{
    if (runConfiguration)
        m_settings = runConfiguration->extraAspect<AnalyzerRunConfigurationAspect>();

    if  (!m_settings)
        m_settings = AnalyzerGlobalSettings::instance();

    connect(m_progressWatcher, SIGNAL(canceled()),
            this, SLOT(handleProgressCanceled()));
    connect(m_progressWatcher, SIGNAL(finished()),
            this, SLOT(handleProgressFinished()));
}

ValgrindEngine::~ValgrindEngine()
{
    delete m_progress;
}

bool ValgrindEngine::start()
{
    emit starting(this);

    Core::FutureProgress *fp = Core::ICore::progressManager()->addTask(m_progress->future(),
                                                        progressTitle(), "valgrind");
    fp->setKeepOnFinish(Core::FutureProgress::HideOnFinish);
    m_progress->setProgressRange(0, progressMaximum);
    m_progress->reportStarted();
    m_progressWatcher->setFuture(m_progress->future());
    m_progress->setProgressValue(progressMaximum / 10);

#if VALGRIND_DEBUG_OUTPUT
    emit outputReceived(tr("Valgrind options: %1").arg(toolArguments().join(" ")), Utils::DebugFormat);
    emit outputReceived(tr("Working directory: %1").arg(m_workingDirectory), Utils::DebugFormat);
    emit outputReceived(tr("Command-line arguments: %1").arg(m_commandLineArguments), Utils::DebugFormat);
#endif

    const AnalyzerStartParameters &sp = startParameters();
    runner()->setWorkingDirectory(sp.workingDirectory);
    QString valgrindExe = m_settings->subConfig<ValgrindBaseSettings>()->valgrindExecutable();
    if (!sp.analyzerCmdPrefix.isEmpty())
        valgrindExe = sp.analyzerCmdPrefix + ' ' + valgrindExe;
    runner()->setValgrindExecutable(valgrindExe);
    runner()->setValgrindArguments(toolArguments());
    runner()->setDebuggeeExecutable(sp.debuggee);
    runner()->setDebuggeeArguments(sp.debuggeeArgs);
    runner()->setEnvironment(sp.environment);
    runner()->setConnectionParameters(sp.connParams);
    runner()->setStartMode(sp.startMode);

    connect(runner(), SIGNAL(processOutputReceived(QByteArray,Utils::OutputFormat)),
            SLOT(receiveProcessOutput(QByteArray,Utils::OutputFormat)));
    connect(runner(), SIGNAL(processErrorReceived(QString,QProcess::ProcessError)),
            SLOT(receiveProcessError(QString,QProcess::ProcessError)));
    connect(runner(), SIGNAL(finished()),
            SLOT(runnerFinished()));

    if (!runner()->start()) {
        m_progress->cancel();
        return false;
    }
    return true;
}

void ValgrindEngine::stop()
{
    m_isStopping = true;
    runner()->stop();
}

QString ValgrindEngine::executable() const
{
    return startParameters().debuggee;
}

void ValgrindEngine::handleProgressCanceled()
{
    AnalyzerManager::stopTool();
    m_progress->reportCanceled();
    m_progress->reportFinished();
}

void ValgrindEngine::handleProgressFinished()
{
    QApplication::alert(Core::ICore::mainWindow(), 3000);
}

void ValgrindEngine::runnerFinished()
{
    emit outputReceived(tr("** Analyzing finished **\n"), Utils::NormalMessageFormat);
    emit finished();

    m_progress->reportFinished();

    disconnect(runner(), SIGNAL(processOutputReceived(QByteArray,Utils::OutputFormat)),
               this, SLOT(receiveProcessOutput(QByteArray,Utils::OutputFormat)));
    disconnect(runner(), SIGNAL(finished()),
               this, SLOT(runnerFinished()));
}

void ValgrindEngine::receiveProcessOutput(const QByteArray &b, Utils::OutputFormat format)
{
    int progress = m_progress->progressValue();
    if (progress < 5 * progressMaximum / 10)
        progress += progress / 100;
    else if (progress < 9 * progressMaximum / 10)
        progress += progress / 1000;
    m_progress->setProgressValue(progress);
    emit outputReceived(QString::fromLocal8Bit(b), format);
}

void ValgrindEngine::receiveProcessError(const QString &error, QProcess::ProcessError e)
{
    if (e == QProcess::FailedToStart) {
        const QString &valgrind = m_settings->subConfig<ValgrindBaseSettings>()->valgrindExecutable();
        if (!valgrind.isEmpty()) {
            emit outputReceived(tr("** Error: \"%1\" could not be started: %2 **\n").arg(valgrind).arg(error), Utils::ErrorMessageFormat);
        } else {
            emit outputReceived(tr("** Error: no valgrind executable set **\n"), Utils::ErrorMessageFormat);
        }
    } else if (m_isStopping && e == QProcess::Crashed) { // process gets killed on stop
        emit outputReceived(tr("** Process Terminated **\n"), Utils::ErrorMessageFormat);
    } else {
        emit outputReceived(QString("** %1 **\n").arg(error), Utils::ErrorMessageFormat);
    }

    if (m_isStopping)
        return;

    ///FIXME: get a better API for this into Qt Creator
    QList<Core::IOutputPane *> panes = ExtensionSystem::PluginManager::getObjects<Core::IOutputPane>();
    foreach (Core::IOutputPane *pane, panes) {
        if (pane->displayName() == tr("Application Output")) {
            pane->popup(false);
            break;
        }
    }
}
