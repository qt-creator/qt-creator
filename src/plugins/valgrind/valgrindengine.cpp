/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "valgrindengine.h"
#include "valgrindsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <analyzerbase/analyzermanager.h>

#include <QApplication>
#include <QMainWindow>

#define VALGRIND_DEBUG_OUTPUT 0

using namespace Analyzer;
using namespace Core;
using namespace Utils;

namespace Valgrind {
namespace Internal {

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

    FutureProgress *fp = ICore::progressManager()->addTask(m_progress->future(),
                                                        progressTitle(), QLatin1String("valgrind"));
    fp->setKeepOnFinish(FutureProgress::HideOnFinish);
    m_progress->setProgressRange(0, progressMaximum);
    m_progress->reportStarted();
    m_progressWatcher->setFuture(m_progress->future());
    m_progress->setProgressValue(progressMaximum / 10);

    const AnalyzerStartParameters &sp = startParameters();
#if VALGRIND_DEBUG_OUTPUT
    emit outputReceived(tr("Valgrind options: %1").arg(toolArguments().join(QLatin1Char(' '))), DebugFormat);
    emit outputReceived(tr("Working directory: %1").arg(sp.workingDirectory), DebugFormat);
    emit outputReceived(tr("Commandline arguments: %1").arg(sp.debuggeeArgs), DebugFormat);
#endif

    runner()->setWorkingDirectory(sp.workingDirectory);
    QString valgrindExe = m_settings->subConfig<ValgrindBaseSettings>()->valgrindExecutable();
    if (!sp.analyzerCmdPrefix.isEmpty())
        valgrindExe = sp.analyzerCmdPrefix + QLatin1Char(' ') + valgrindExe;
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
    QApplication::alert(ICore::mainWindow(), 3000);
}

void ValgrindEngine::runnerFinished()
{
    emit outputReceived(tr("** Analyzing finished **\n"), NormalMessageFormat);
    emit finished();

    m_progress->reportFinished();

    disconnect(runner(), SIGNAL(processOutputReceived(QByteArray,Utils::OutputFormat)),
               this, SLOT(receiveProcessOutput(QByteArray,Utils::OutputFormat)));
    disconnect(runner(), SIGNAL(finished()),
               this, SLOT(runnerFinished()));
}

void ValgrindEngine::receiveProcessOutput(const QByteArray &output, OutputFormat format)
{
    int progress = m_progress->progressValue();
    if (progress < 5 * progressMaximum / 10)
        progress += progress / 100;
    else if (progress < 9 * progressMaximum / 10)
        progress += progress / 1000;
    m_progress->setProgressValue(progress);
    emit outputReceived(QString::fromLocal8Bit(output), format);
}

void ValgrindEngine::receiveProcessError(const QString &message, QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        const QString &valgrind = m_settings->subConfig<ValgrindBaseSettings>()->valgrindExecutable();
        if (!valgrind.isEmpty())
            emit outputReceived(tr("** Error: \"%1\" could not be started: %2 **\n").arg(valgrind).arg(message), ErrorMessageFormat);
        else
            emit outputReceived(tr("** Error: no valgrind executable set **\n"), ErrorMessageFormat);
    } else if (m_isStopping && error == QProcess::Crashed) { // process gets killed on stop
        emit outputReceived(tr("** Process Terminated **\n"), ErrorMessageFormat);
    } else {
        emit outputReceived(QString::fromLatin1("** %1 **\n").arg(message), ErrorMessageFormat);
    }

    if (m_isStopping)
        return;

    QObject *obj = ExtensionSystem::PluginManager::getObjectByName(QLatin1String("AppOutputPane"));
    if (IOutputPane *pane = qobject_cast<IOutputPane *>(obj))
        pane->popup(IOutputPane::NoModeSwitch);
}

} // namespace Internal
} // namepsace Valgrind
