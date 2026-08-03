// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosrunconfiguration.h"

#include "harmonyosconstants.h"
#include "harmonyosdevice.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QtTaskTree/qtasktree.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

// The application bundle name from the generated harmonyos-build/AppScope/app.json5.
static QString bundleName(const FilePath &buildDir)
{
    const FilePath appJson = buildDir.pathAppended("harmonyos-build/AppScope/app.json5");
    const Result<QByteArray> contents = appJson.fileContents();
    if (!contents)
        return {};
    // The file is JSON5, so drop the comments to not pick up a commented-out entry.
    static const QRegularExpression re("\"bundleName\"\\s*:\\s*\"([^\"]+)\"");
    const QString text = QString::fromUtf8(Utils::removeCommentsFromJson(*contents));
    const QRegularExpressionMatch match = re.match(text);
    return match.hasMatch() ? match.captured(1) : QString();
}

class HarmonyOsRunConfiguration final : public RunConfiguration
{
public:
    HarmonyOsRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {}
};

class HarmonyOsRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    HarmonyOsRunConfigurationFactory()
    {
        registerRunConfiguration<HarmonyOsRunConfiguration>(Constants::HARMONYOS_RUNCONFIG_ID);
        addSupportedTargetDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setDecorateDisplayNames(true);
    }
};

class HarmonyOsRunWorkerFactory final : public RunWorkerFactory
{
public:
    HarmonyOsRunWorkerFactory()
    {
        setId("HarmonyOsRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
            if (hdc.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("No HarmonyOS SDK is configured; cannot launch on the device."));
            }

            BuildConfiguration * const bc = runControl->buildConfiguration();
            QTC_ASSERT(bc, return runControl->errorTask(
                                Tr::tr("No build configuration; cannot launch on the device.")));

            const QString bundle = bundleName(bc->buildDirectory());
            if (bundle.isEmpty()) {
                return runControl->errorTask(
                    Tr::tr("Could not determine the application bundle name. "
                           "Build and deploy the package first."));
            }

            // Target the run device explicitly so the right one is used when several
            // are connected.
            QString serial;
            if (auto device = std::dynamic_pointer_cast<const HarmonyOsDevice>(runControl->device()))
                serial = device->serialNumber();

            const auto command = [hdc, serial](const QStringList &args) {
                CommandLine cmd{hdc};
                if (!serial.isEmpty())
                    cmd.addArgs({"-t", serial});
                cmd.addArgs(args);
                return cmd;
            };

            const auto forceStopTask = [command, bundle] {
                const auto onSetup = [command, bundle](Process &process) {
                    process.setCommand(command({"shell", "aa", "force-stop", bundle}));
                };
                return ProcessTask(onSetup) || successItem;
            };

            const auto onStartSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "aa", "start",
                                            "-a", Constants::HARMONYOS_ABILITY_NAME,
                                            "-b", bundle,
                                            "-m", Constants::HARMONYOS_MODULE_NAME}));
            };

            // "aa start" returns as soon as the ability was launched, so it cannot stand in
            // for the application's lifetime. Follow the application's hilog output instead:
            // that keeps the run alive, feeds the application output pane, and lets the run
            // be stopped. Polling for the process is still needed because hilog keeps
            // waiting once the process is gone.
            const Storage<QString> pidStorage;

            const auto onPidSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "pidof", "-s", bundle}));
            };
            const auto onPidDone = [pidStorage](const Process &process) {
                *pidStorage = process.cleanedStdOut().trimmed();
                return !pidStorage->isEmpty();
            };

            const auto onLogSetup = [command, pidStorage](Process &process) {
                process.setCommand(command({"shell", "hilog", "-P", *pidStorage}));
            };

            const auto onGoneSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "pidof", "-s", bundle}));
            };
            const auto onGoneDone = [](const Process &process) {
                // Succeeding ends the polling loop, and with it the run.
                return toDoneResult(process.cleanedStdOut().trimmed().isEmpty());
            };

            return Group {
                pidStorage,
                forceStopTask(),
                Group {
                    ProcessTask(onStartSetup),
                    ProcessTask(onPidSetup, onPidDone),
                    Group {
                        parallel,
                        stopOnSuccessOrError,
                        runControl->processRecipe(onLogSetup),
                        Forever {
                            stopOnSuccess,
                            ProcessTask(onGoneSetup, onGoneDone),
                            timeoutTask(1s)
                        }
                    }
                }.withCancel([runControl] {
                    return makeObjectSignal(runControl, &RunControl::canceled);
                }),
                forceStopTask(),
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::HARMONYOS_RUNCONFIG_ID);
        addSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
    }
};

void setupHarmonyOsRunSupport()
{
    static HarmonyOsRunConfigurationFactory theHarmonyOsRunConfigurationFactory;
    static HarmonyOsRunWorkerFactory theHarmonyOsRunWorkerFactory;
}

} // namespace HarmonyOs::Internal
