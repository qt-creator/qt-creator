// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindengine.h"

#include "valgrindsettings.h"
#include "valgrindtr.h"

#include <debugger/analyzer/analyzermanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <QApplication>

using namespace Debugger;
using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace Valgrind::Internal {

ValgrindToolRunner::ValgrindToolRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
    setSupportsReRunning(false);

    m_settings.fromMap(runControl->settingsData(ANALYZER_VALGRIND_SETTINGS));

    connect(&m_runner,
            &ValgrindRunner::appendMessage,
            this,
            [this](const QString &msg, Utils::OutputFormat format) { appendMessage(msg, format); });
    connect(&m_runner, &ValgrindRunner::valgrindExecuted,
            this, [this](const QString &commandLine) {
        appendMessage(commandLine, NormalMessageFormat);
    });
    connect(&m_runner, &ValgrindRunner::processErrorReceived,
            this, &ValgrindToolRunner::receiveProcessError);
    connect(&m_runner, &ValgrindRunner::finished,
            this, &ValgrindToolRunner::runnerFinished);
}

void ValgrindToolRunner::start()
{
    FilePath valgrindExecutable = m_settings.valgrindExecutable();
    if (IDevice::ConstPtr dev = DeviceKitAspect::device(runControl()->kit()))
        valgrindExecutable = dev->filePath(valgrindExecutable.path());
    if (!valgrindExecutable.isExecutableFile()) {
        reportFailure(Tr::tr("Valgrind executable \"%1\" not found or not executable.\n"
                             "Check settings or ensure valgrind is installed and available in PATH.")
                      .arg(valgrindExecutable.toUserOutput()));
        return;
    }

    FutureProgress *fp = ProgressManager::addTimedTask(m_progress, progressTitle(), "valgrind", 100);
    connect(fp, &FutureProgress::canceled,
            this, &ValgrindToolRunner::handleProgressCanceled);
    connect(fp, &FutureProgress::finished,
            this, &ValgrindToolRunner::handleProgressFinished);
    m_progress.reportStarted();

    CommandLine valgrind{valgrindExecutable};
    valgrind.addArgs(m_settings.valgrindArguments.value(), CommandLine::Raw);
    valgrind.addArgs(genericToolArguments());
    valgrind.addArgs(toolArguments());

    m_runner.setValgrindCommand(valgrind);
    m_runner.setDebuggee(runControl()->runnable());

    if (auto aspect = runControl()->aspect<TerminalAspect>())
        m_runner.setUseTerminal(aspect->useTerminal);

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

QStringList ValgrindToolRunner::genericToolArguments() const
{
    QString smcCheckValue;

    switch (m_settings.selfModifyingCodeDetection.value()) {
    case ValgrindBaseSettings::DetectSmcNo:
        smcCheckValue = "none";
        break;
    case ValgrindBaseSettings::DetectSmcEverywhere:
        smcCheckValue = "all";
        break;
    case ValgrindBaseSettings::DetectSmcEverywhereButFile:
        smcCheckValue = "all-non-file";
        break;
    case ValgrindBaseSettings::DetectSmcStackOnly:
    default:
        smcCheckValue = "stack";
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
    QApplication::alert(ICore::dialogParent(), 3000);
}

void ValgrindToolRunner::runnerFinished()
{
    appendMessage(Tr::tr("Analyzing finished."), NormalMessageFormat);

    m_progress.reportFinished();

    reportStopped();
}

void ValgrindToolRunner::receiveProcessError(const QString &message, QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        const QString valgrind = m_settings.valgrindExecutable.value();
        if (!valgrind.isEmpty())
            appendMessage(Tr::tr("Error: \"%1\" could not be started: %2").arg(valgrind, message), ErrorMessageFormat);
        else
            appendMessage(Tr::tr("Error: no Valgrind executable set."), ErrorMessageFormat);
    } else if (m_isStopping && error == QProcess::Crashed) { // process gets killed on stop
        appendMessage(Tr::tr("Process terminated."), ErrorMessageFormat);
    } else {
        appendMessage(Tr::tr("Process exited with return value %1\n").arg(message), NormalMessageFormat);
    }

    if (m_isStopping)
        return;

    QObject *obj = ExtensionSystem::PluginManager::getObjectByName("AppOutputPane");
    if (auto pane = qobject_cast<IOutputPane *>(obj))
        pane->popup(IOutputPane::NoModeSwitch);
}

} // Valgrid::Internal
