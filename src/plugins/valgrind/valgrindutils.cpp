// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindutils.h"

#include "valgrindprocess.h"
#include "valgrindsettings.h"
#include "valgrindtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

#include <utils/checkablemessagebox.h>

#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Valgrind::Internal {

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

ExecutableItem initValgrindRecipe(const Storage<ValgrindSettings> &storage, RunControl *runControl)
{
    const auto onSetup = [storage, runControl] {
        storage->fromMap(runControl->settingsData(ANALYZER_VALGRIND_SETTINGS));
        if (storage->valgrindExecutable().searchInPath().isExecutableFile()) {
            runControl->reportStarted();
            return DoneResult::Success;
        }
        runControl->postMessage(Tr::tr("Valgrind executable \"%1\" not found or not executable.\n"
            "Check settings or ensure Valgrind is installed and available in PATH.")
            .arg(storage->valgrindExecutable().toUserOutput()), ErrorMessageFormat);
        return DoneResult::Error;
    };
    return QSyncTask(onSetup);
}

void setupValgrindProcess(ValgrindProcess *process, RunControl *runControl,
                          const CommandLine &valgrindCommand)
{
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    QObject::connect(process, &ValgrindProcess::appendMessage, runControl,
                     [runControl](const QString &msg, OutputFormat format) {
        runControl->postMessage(msg, format);
    });
    QObject::connect(process, &ValgrindProcess::processErrorReceived, runControl,
                     [runControl, valgrindCommand](const QString &errorString, ProcessResult result) {
        switch (result) {
        case ProcessResult::StartFailed: {
            const FilePath valgrind = valgrindCommand.executable();
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
    QObject::connect(runControl, &RunControl::canceled, process, &ValgrindProcess::stop);
    process->setValgrindCommand(valgrindCommand);
    process->setDebuggee(runControl->runnable());
    if (auto aspect = runControl->aspectData<TerminalAspect>())
        process->setUseTerminal(aspect->useTerminal);
}

CommandLine defaultValgrindCommand(RunControl *runControl, const ValgrindSettings &settings)
{
    FilePath valgrindExecutable = settings.valgrindExecutable();
    if (IDevice::ConstPtr dev = RunDeviceKitAspect::device(runControl->kit()))
        valgrindExecutable = dev->filePath(valgrindExecutable.path());

    CommandLine valgrindCommand{valgrindExecutable};
    valgrindCommand.addArgs(settings.valgrindArguments(), CommandLine::Raw);
    valgrindCommand.addArg("--smc-check=" + selfModifyingCodeDetectionToString(
                               settings.selfModifyingCodeDetection()));
    return valgrindCommand;
}

static bool buildTypeAccepted(QFlags<ToolMode> toolMode, BuildConfiguration::BuildType buildType)
{
    if (buildType == BuildConfiguration::Unknown)
        return true;
    if (buildType == BuildConfiguration::Debug && (toolMode & DebugMode))
        return true;
    if (buildType == BuildConfiguration::Release && (toolMode & ReleaseMode))
        return true;
    if (buildType == BuildConfiguration::Profile && (toolMode & ProfileMode))
        return true;
    return false;
}

static BuildConfiguration::BuildType startupBuildType()
{
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
    if (RunConfiguration *runConfig = activeRunConfigForActiveProject()) {
        if (const BuildConfiguration *buildConfig = runConfig->buildConfiguration())
            buildType = buildConfig->buildType();
    }
    return buildType;
}

bool wantRunTool(ToolMode toolMode, const QString &toolName)
{
    BuildConfiguration::BuildType buildType = startupBuildType();
    if (!buildTypeAccepted(toolMode, buildType)) {
        QString currentMode;
        switch (buildType) {
            case BuildConfiguration::Debug:
                currentMode = msgBuildConfigurationDebug();
                break;
            case BuildConfiguration::Profile:
                currentMode = msgBuildConfigurationProfile();
                break;
            case BuildConfiguration::Release:
                currentMode = msgBuildConfigurationRelease();
                break;
            default:
                QTC_CHECK(false);
        }

        QString toolModeString;
        switch (toolMode) {
            case DebugMode:
                toolModeString = Tr::tr("in Debug mode");
                break;
            case ProfileMode:
                toolModeString = Tr::tr("in Profile mode");
                break;
            case ReleaseMode:
                toolModeString = Tr::tr("in Release mode");
                break;
            case SymbolsMode:
                toolModeString = Tr::tr("with debug symbols (Debug or Profile mode)");
                break;
            case OptimizedMode:
                toolModeString = Tr::tr("on optimized code (Profile or Release mode)");
                break;
            default:
                QTC_CHECK(false);
        }
        const QString title = Tr::tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
        const QString message = Tr::tr("<html><head/><body><p>You are trying "
            "to run the tool \"%1\" on an application in %2 mode. "
            "The tool is designed to be used %3.</p><p>"
            "Run-time characteristics differ significantly between "
            "optimized and non-optimized binaries. Analytical "
            "findings for one mode may or may not be relevant for "
            "the other.</p><p>"
            "Running tools that need debug symbols on binaries that "
            "don't provide any may lead to missing function names "
            "or otherwise insufficient output.</p><p>"
            "Do you want to continue and run the tool in %2 mode?</p></body></html>")
                .arg(toolName).arg(currentMode).arg(toolModeString);
        if (Utils::CheckableMessageBox::question(title,
                                                 message,
                                                 Key("AnalyzerCorrectModeWarning"))
            != QMessageBox::Yes)
                return false;
    }

    return true;
}

} // Valgrind::Internal
