/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "valgrindengine.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"

#include <debugger/analyzer/analyzermanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>

#include <QApplication>

#define VALGRIND_DEBUG_OUTPUT 0

using namespace Debugger;
using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

ValgrindToolRunner::ValgrindToolRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
    setSupportsReRunning(false);

    if (IRunConfigurationAspect *aspect = runControl->runConfiguration()->extraAspect(ANALYZER_VALGRIND_SETTINGS))
        m_settings = qobject_cast<ValgrindBaseSettings *>(aspect->currentSettings());

    if (!m_settings)
        m_settings = ValgrindPlugin::globalSettings();
}

void ValgrindToolRunner::start()
{
    FutureProgress *fp = ProgressManager::addTimedTask(m_progress, progressTitle(), "valgrind", 100);
    fp->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(fp, &FutureProgress::canceled,
            this, &ValgrindToolRunner::handleProgressCanceled);
    connect(fp, &FutureProgress::finished,
            this, &ValgrindToolRunner::handleProgressFinished);
    m_progress.reportStarted();

#if VALGRIND_DEBUG_OUTPUT
    emit outputReceived(tr("Valgrind options: %1").arg(toolArguments().join(QLatin1Char(' '))), DebugFormat);
    emit outputReceived(tr("Working directory: %1").arg(runnable().workingDirectory), DebugFormat);
    emit outputReceived(tr("Command line arguments: %1").arg(runnable().debuggeeArgs), DebugFormat);
#endif

    m_runner.setValgrindExecutable(m_settings->valgrindExecutable());
    m_runner.setValgrindArguments(genericToolArguments() + toolArguments());
    m_runner.setDevice(device());
    QTC_ASSERT(runnable().is<StandardRunnable>(), reportFailure());
    m_runner.setDebuggee(runnable().as<StandardRunnable>());

    connect(&m_runner, &ValgrindRunner::processOutputReceived,
            this, &ValgrindToolRunner::receiveProcessOutput);
    connect(&m_runner, &ValgrindRunner::processErrorReceived,
            this, &ValgrindToolRunner::receiveProcessError);
    connect(&m_runner, &ValgrindRunner::finished,
            this, &ValgrindToolRunner::runnerFinished);

    if (!m_runner.start()) {
        m_progress.cancel();
        reportFailure();
        return;
    }

    reportStarted();
}

void ValgrindToolRunner::stop()
{
    m_isStopping = true;
    m_runner.stop();
}

QString ValgrindToolRunner::executable() const
{
    QTC_ASSERT(runnable().is<StandardRunnable>(), return QString());
    return runnable().as<StandardRunnable>().executable;
}

QStringList ValgrindToolRunner::genericToolArguments() const
{
    QTC_ASSERT(m_settings, return QStringList());
    QString smcCheckValue;
    switch (m_settings->selfModifyingCodeDetection()) {
    case ValgrindBaseSettings::DetectSmcNo:
        smcCheckValue = QLatin1String("none");
        break;
    case ValgrindBaseSettings::DetectSmcEverywhere:
        smcCheckValue = QLatin1String("all");
        break;
    case ValgrindBaseSettings::DetectSmcEverywhereButFile:
        smcCheckValue = QLatin1String("all-non-file");
        break;
    case ValgrindBaseSettings::DetectSmcStackOnly:
    default:
        smcCheckValue = QLatin1String("stack");
        break;
    }
    return {"--smc-check=" + smcCheckValue};
}

void ValgrindToolRunner::handleProgressCanceled()
{
    m_progress.reportCanceled();
    m_progress.reportFinished();
}

void ValgrindToolRunner::handleProgressFinished()
{
    QApplication::alert(ICore::mainWindow(), 3000);
}

void ValgrindToolRunner::runnerFinished()
{
    appendMessage(tr("Analyzing finished."), NormalMessageFormat);

    m_progress.reportFinished();

    disconnect(&m_runner, &ValgrindRunner::processOutputReceived,
               this, &ValgrindToolRunner::receiveProcessOutput);
    disconnect(&m_runner, &ValgrindRunner::finished,
               this, &ValgrindToolRunner::runnerFinished);

    reportStopped();
}

void ValgrindToolRunner::receiveProcessOutput(const QString &output, OutputFormat format)
{
    appendMessage(output, format);
}

void ValgrindToolRunner::receiveProcessError(const QString &message, QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        const QString valgrind = m_settings->valgrindExecutable();
        if (!valgrind.isEmpty())
            appendMessage(tr("Error: \"%1\" could not be started: %2").arg(valgrind, message), ErrorMessageFormat);
        else
            appendMessage(tr("Error: no Valgrind executable set."), ErrorMessageFormat);
    } else if (m_isStopping && error == QProcess::Crashed) { // process gets killed on stop
        appendMessage(tr("Process terminated."), ErrorMessageFormat);
    } else {
        appendMessage(tr("Process exited with return value %1\n").arg(message), NormalMessageFormat);
    }

    if (m_isStopping)
        return;

    QObject *obj = ExtensionSystem::PluginManager::getObjectByName(QLatin1String("AppOutputPane"));
    if (IOutputPane *pane = qobject_cast<IOutputPane *>(obj))
        pane->popup(IOutputPane::NoModeSwitch);
}

} // namespace Internal
} // namepsace Valgrind
