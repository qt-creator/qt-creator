/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
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

#include <QtGui/QApplication>
#include <QtGui/QMainWindow>

#define VALGRIND_DEBUG_OUTPUT 0

using namespace Analyzer;
using namespace Valgrind::Internal;
using namespace Utils;

ValgrindEngine::ValgrindEngine(const AnalyzerStartParameters &sp,
                               ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(sp, runConfiguration),
      m_settings(0),
      m_progress(new QFutureInterface<void>()),
      m_progressWatcher(new QFutureWatcher<void>()),
      m_isStopping(false)
{
    if (runConfiguration)
        m_settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();

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

void ValgrindEngine::start()
{
    emit starting(this);

    Core::FutureProgress *fp = Core::ICore::instance()->progressManager()->addTask(m_progress->future(),
                                                        progressTitle(), "valgrind");
    fp->setKeepOnFinish(Core::FutureProgress::DontKeepOnFinish);
    m_progress->reportStarted();
    m_progressWatcher->setFuture(m_progress->future());

#if VALGRIND_DEBUG_OUTPUT
    emit standardOutputReceived(tr("Valgrind options: %1").arg(toolArguments().join(" ")));
    emit standardOutputReceived(tr("Working directory: %1").arg(m_workingDirectory));
    emit standardOutputReceived(tr("Command-line arguments: %1").arg(m_commandLineArguments));
#endif

    const AnalyzerStartParameters &sp = startParameters();
    runner()->setWorkingDirectory(sp.workingDirectory);
    QString valgrindExe = m_settings->subConfig<ValgrindSettings>()->valgrindExecutable();
    if (!sp.analyzerCmdPrefix.isEmpty())
        valgrindExe = sp.analyzerCmdPrefix + ' ' + valgrindExe;
    runner()->setValgrindExecutable(valgrindExe);
    runner()->setValgrindArguments(toolArguments());
    runner()->setDebuggeeExecutable(sp.debuggee);
    runner()->setDebuggeeArguments(sp.debuggeeArgs);
    runner()->setEnvironment(sp.environment);

    connect(runner(), SIGNAL(standardOutputReceived(QByteArray)),
            SLOT(receiveStandardOutput(QByteArray)));
    connect(runner(), SIGNAL(standardErrorReceived(QByteArray)),
            SLOT(receiveStandardError(QByteArray)));
    connect(runner(), SIGNAL(processErrorReceived(QString, QProcess::ProcessError)),
            SLOT(receiveProcessError(QString, QProcess::ProcessError)));
    connect(runner(), SIGNAL(finished()),
            SLOT(runnerFinished()));

    if (sp.startMode == StartRemote)
        runner()->startRemotely(sp.connParams);
    else
        runner()->start();
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
    AnalyzerManager::instance()->stopTool();
}

void ValgrindEngine::handleProgressFinished()
{
    QApplication::alert(Core::ICore::instance()->mainWindow(), 3000);
}

void ValgrindEngine::runnerFinished()
{
    emit standardOutputReceived(tr("** Analyzing finished **"));
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
    foreach (Core::IOutputPane *pane, panes) {
        if (pane->displayName() == tr("Application Output")) {
            pane->popup(false);
            break;
        }
    }
}
