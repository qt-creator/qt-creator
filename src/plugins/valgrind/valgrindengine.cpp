/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "valgrindplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <analyzerbase/analyzermanager.h>

#include <QApplication>
#include <QMainWindow>

#define VALGRIND_DEBUG_OUTPUT 0

using namespace Analyzer;
using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

ValgrindRunControl::ValgrindRunControl(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration)
    : AnalyzerRunControl(sp, runConfiguration),
      m_settings(0),
      m_progress(new QFutureInterface<void>()),
      m_isStopping(false)
{
    if (runConfiguration)
        if (IRunConfigurationAspect *aspect = runConfiguration->extraAspect(ANALYZER_VALGRIND_SETTINGS))
            m_settings = qobject_cast<ValgrindBaseSettings *>(aspect->currentSettings());

    if (!m_settings)
        m_settings = ValgrindPlugin::globalSettings();

}

ValgrindRunControl::~ValgrindRunControl()
{
    delete m_progress;
}

bool ValgrindRunControl::startEngine()
{
    emit starting(this);

    FutureProgress *fp = ProgressManager::addTimedTask(m_progress, progressTitle(), "valgrind", 100);
    fp->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(fp, SIGNAL(canceled()), this, SLOT(handleProgressCanceled()));
    connect(fp, SIGNAL(finished()), this, SLOT(handleProgressFinished()));
    m_progress->reportStarted();

    const AnalyzerStartParameters &sp = startParameters();
#if VALGRIND_DEBUG_OUTPUT
    emit outputReceived(tr("Valgrind options: %1").arg(toolArguments().join(QLatin1Char(' '))), DebugFormat);
    emit outputReceived(tr("Working directory: %1").arg(sp.workingDirectory), DebugFormat);
    emit outputReceived(tr("Command line arguments: %1").arg(sp.debuggeeArgs), DebugFormat);
#endif

    ValgrindRunner *run = runner();
    run->setWorkingDirectory(sp.workingDirectory);
    QString valgrindExe = m_settings->valgrindExecutable();
    if (!sp.analyzerCmdPrefix.isEmpty())
        valgrindExe = sp.analyzerCmdPrefix + QLatin1Char(' ') + valgrindExe;
    run->setValgrindExecutable(valgrindExe);
    run->setValgrindArguments(genericToolArguments() + toolArguments());
    run->setDebuggeeExecutable(sp.debuggee);
    run->setDebuggeeArguments(sp.debuggeeArgs);
    run->setEnvironment(sp.environment);
    run->setConnectionParameters(sp.connParams);
    run->setStartMode(sp.startMode);
    run->setLocalRunMode(sp.localRunMode);

    connect(run, SIGNAL(processOutputReceived(QString,Utils::OutputFormat)),
            SLOT(receiveProcessOutput(QString,Utils::OutputFormat)));
    connect(run, SIGNAL(processErrorReceived(QString,QProcess::ProcessError)),
            SLOT(receiveProcessError(QString,QProcess::ProcessError)));
    connect(run, SIGNAL(finished()), SLOT(runnerFinished()));

    if (!run->start()) {
        m_progress->cancel();
        return false;
    }
    return true;
}

void ValgrindRunControl::stopEngine()
{
    m_isStopping = true;
    runner()->stop();
}

QString ValgrindRunControl::executable() const
{
    return startParameters().debuggee;
}

QStringList ValgrindRunControl::genericToolArguments() const
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
    return QStringList() << QLatin1String("--smc-check=") + smcCheckValue;
}

void ValgrindRunControl::handleProgressCanceled()
{
    AnalyzerManager::stopTool();
    m_progress->reportCanceled();
    m_progress->reportFinished();
}

void ValgrindRunControl::handleProgressFinished()
{
    QApplication::alert(ICore::mainWindow(), 3000);
}

void ValgrindRunControl::runnerFinished()
{
    appendMessage(tr("Analyzing finished.") + QLatin1Char('\n'), NormalMessageFormat);
    emit finished();

    m_progress->reportFinished();

    disconnect(runner(), SIGNAL(processOutputReceived(QString,Utils::OutputFormat)),
               this, SLOT(receiveProcessOutput(QString,Utils::OutputFormat)));
    disconnect(runner(), SIGNAL(finished()),
               this, SLOT(runnerFinished()));
}

void ValgrindRunControl::receiveProcessOutput(const QString &output, OutputFormat format)
{
    appendMessage(output, format);
}

void ValgrindRunControl::receiveProcessError(const QString &message, QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        const QString valgrind = m_settings->valgrindExecutable();
        if (!valgrind.isEmpty())
            appendMessage(tr("Error: \"%1\" could not be started: %2").arg(valgrind, message) + QLatin1Char('\n'), ErrorMessageFormat);
        else
            appendMessage(tr("Error: no Valgrind executable set.") + QLatin1Char('\n'), ErrorMessageFormat);
    } else if (m_isStopping && error == QProcess::Crashed) { // process gets killed on stop
        appendMessage(tr("Process terminated.") + QLatin1Char('\n'), ErrorMessageFormat);
    } else {
        appendMessage(QString::fromLatin1("** %1 **\n").arg(message), ErrorMessageFormat);
    }

    if (m_isStopping)
        return;

    QObject *obj = ExtensionSystem::PluginManager::getObjectByName(QLatin1String("AppOutputPane"));
    if (IOutputPane *pane = qobject_cast<IOutputPane *>(obj))
        pane->popup(IOutputPane::NoModeSwitch);
}

} // namespace Internal
} // namepsace Valgrind
