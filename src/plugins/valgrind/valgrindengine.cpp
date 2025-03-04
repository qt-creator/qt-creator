// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindengine.h"

#include "valgrindsettings.h"
#include "valgrindtr.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorericons.h>

#include <QApplication>

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace Valgrind::Internal {

ValgrindToolRunner::ValgrindToolRunner(RunControl *runControl, const QString &progressTitle)
    : RunWorker(runControl)
    , m_progressTitle(progressTitle)
{
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    m_settings.fromMap(runControl->settingsData(ANALYZER_VALGRIND_SETTINGS));

    connect(&m_runner, &ValgrindProcess::appendMessage, this,
            [runControl](const QString &msg, OutputFormat format) {
        runControl->postMessage(msg, format);
    });
    connect(&m_runner, &ValgrindProcess::processErrorReceived, this,
            [this, runControl](const QString &errorString, Utils::ProcessResult result) {
        switch (result) {
        case ProcessResult::StartFailed: {
            const FilePath valgrind = m_settings.valgrindExecutable();
            if (!valgrind.isEmpty()) {
                runControl->postMessage(Tr::tr("Error: \"%1\" could not be started: %2")
                                  .arg(valgrind.toUserOutput(), errorString), ErrorMessageFormat);
            } else {
                runControl->postMessage(Tr::tr("Error: no Valgrind executable set."),
                                        ErrorMessageFormat);
            }
            break;
        }
        case ProcessResult::Canceled:
            runControl->postMessage(Tr::tr("Process terminated."), ErrorMessageFormat);
            return; // Intentional.
        case ProcessResult::FinishedWithError:
            runControl->postMessage(Tr::tr("Process exited with return value %1\n").arg(errorString), NormalMessageFormat);
            break;
        default:
            break;
        }
        runControl->showOutputPane();

    });
    connect(&m_runner, &ValgrindProcess::done, this, [this, runControl] {
        runControl->postMessage(Tr::tr("Analyzing finished."), NormalMessageFormat);
        m_progress.reportFinished();
        reportStopped();
    });
}

static QString selfModifyingCodeDetectionToString(ValgrindSettings::SelfModifyingCodeDetection detection)
{
    switch (detection) {
    case ValgrindSettings::DetectSmcNo:                return "none";
    case ValgrindSettings::DetectSmcEverywhere:        return "all";
    case ValgrindSettings::DetectSmcEverywhereButFile: return "all-non-file";
    case ValgrindSettings::DetectSmcStackOnly:         return "stack";
    }
    return {};
}

void ValgrindToolRunner::start()
{
    FilePath valgrindExecutable = m_settings.valgrindExecutable();
    if (IDevice::ConstPtr dev = RunDeviceKitAspect::device(runControl()->kit()))
        valgrindExecutable = dev->filePath(valgrindExecutable.path());

    const FilePath found = valgrindExecutable.searchInPath();

    if (!found.isExecutableFile()) {
        reportFailure(Tr::tr("Valgrind executable \"%1\" not found or not executable.\n"
                             "Check settings or ensure Valgrind is installed and available in PATH.")
                      .arg(valgrindExecutable.toUserOutput()));
        return;
    }

    using namespace std::chrono_literals;
    FutureProgress *fp
        = ProgressManager::addTimedTask(m_progress, m_progressTitle, "valgrind", 100s);
    connect(fp, &FutureProgress::canceled, this, [this] {
        m_progress.reportCanceled();
        m_progress.reportFinished();
    });
    connect(fp, &FutureProgress::finished, this, [] {
        QApplication::alert(ICore::dialogParent(), 3000);
    });
    m_progress.reportStarted();

    CommandLine valgrind{valgrindExecutable};
    valgrind.addArgs(m_settings.valgrindArguments(), CommandLine::Raw);
    valgrind.addArg("--smc-check=" + selfModifyingCodeDetectionToString(m_settings.selfModifyingCodeDetection()));
    addToolArguments(valgrind);

    m_runner.setValgrindCommand(valgrind);
    m_runner.setDebuggee(runControl()->runnable());

    if (auto aspect = runControl()->aspectData<TerminalAspect>())
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
    m_runner.stop();
    runControl()->postMessage(Tr::tr("Terminating process..."), ErrorMessageFormat);
}

} // Valgrid::Internal
