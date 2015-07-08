/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
        RunConfiguration *runConfiguration)
    : AnalyzerRunControl(sp, runConfiguration),
      m_settings(0),
      m_isStopping(false)
{
    m_isCustomStart = false;

    if (runConfiguration)
        if (IRunConfigurationAspect *aspect = runConfiguration->extraAspect(ANALYZER_VALGRIND_SETTINGS))
            m_settings = qobject_cast<ValgrindBaseSettings *>(aspect->currentSettings());

    if (!m_settings)
        m_settings = ValgrindPlugin::globalSettings();
}

ValgrindRunControl::~ValgrindRunControl()
{
}

bool ValgrindRunControl::startEngine()
{
    emit starting(this);

    FutureProgress *fp = ProgressManager::addTimedTask(m_progress, progressTitle(), "valgrind", 100);
    fp->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(fp, &FutureProgress::canceled,
            this, &ValgrindRunControl::handleProgressCanceled);
    connect(fp, &FutureProgress::finished,
            this, &ValgrindRunControl::handleProgressFinished);
    m_progress.reportStarted();

    const AnalyzerStartParameters &sp = startParameters();
#if VALGRIND_DEBUG_OUTPUT
    emit outputReceived(tr("Valgrind options: %1").arg(toolArguments().join(QLatin1Char(' '))), DebugFormat);
    emit outputReceived(tr("Working directory: %1").arg(sp.workingDirectory), DebugFormat);
    emit outputReceived(tr("Command line arguments: %1").arg(sp.debuggeeArgs), DebugFormat);
#endif

    ValgrindRunner *run = runner();
    run->setWorkingDirectory(sp.workingDirectory);
    run->setValgrindExecutable(m_settings->valgrindExecutable());
    run->setValgrindArguments(genericToolArguments() + toolArguments());
    run->setDebuggeeExecutable(sp.debuggee);
    run->setDebuggeeArguments(sp.debuggeeArgs);
    run->setEnvironment(sp.environment);
    run->setConnectionParameters(sp.connParams);
    run->setUseStartupProject(!m_isCustomStart);
    run->setLocalRunMode(sp.localRunMode);

    connect(run, &ValgrindRunner::processOutputReceived,
            this, &ValgrindRunControl::receiveProcessOutput);
    connect(run, &ValgrindRunner::processErrorReceived,
            this, &ValgrindRunControl::receiveProcessError);
    connect(run, &ValgrindRunner::finished,
            this, &ValgrindRunControl::runnerFinished);

    if (!run->start()) {
        m_progress.cancel();
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
    m_progress.reportCanceled();
    m_progress.reportFinished();
}

void ValgrindRunControl::handleProgressFinished()
{
    QApplication::alert(ICore::mainWindow(), 3000);
}

void ValgrindRunControl::runnerFinished()
{
    appendMessage(tr("Analyzing finished.") + QLatin1Char('\n'), NormalMessageFormat);
    emit finished();

    m_progress.reportFinished();

    disconnect(runner(), &ValgrindRunner::processOutputReceived,
               this, &ValgrindRunControl::receiveProcessOutput);
    disconnect(runner(), &ValgrindRunner::finished,
               this, &ValgrindRunControl::runnerFinished);
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
