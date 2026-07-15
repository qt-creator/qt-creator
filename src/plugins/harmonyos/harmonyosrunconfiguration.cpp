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

#include <utils/qtcprocess.h>

#include <QtTaskTree/qtasktree.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace HarmonyOs::Internal {

// The application bundle name from the generated harmonyos-build/AppScope/app.json5.
static QString bundleName(const FilePath &buildDir)
{
    const FilePath appJson = buildDir.pathAppended("harmonyos-build/AppScope/app.json5");
    const Result<QByteArray> contents = appJson.fileContents();
    if (!contents)
        return {};
    const QRegularExpression re("\"bundleName\"\\s*:\\s*\"([^\"]+)\"");
    const QRegularExpressionMatch match = re.match(QString::fromUtf8(*contents));
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

            const FilePath buildDir = runControl->buildConfiguration()->buildDirectory();
            const QString bundle = bundleName(buildDir);
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

            const auto onForceStopSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "aa", "force-stop", bundle}));
            };
            const auto onStartSetup = [command, bundle](Process &process) {
                process.setCommand(command({"shell", "aa", "start",
                                            "-a", Constants::HARMONYOS_ABILITY_NAME,
                                            "-b", bundle,
                                            "-m", Constants::HARMONYOS_MODULE_NAME}));
            };

            return Group {
                ProcessTask(onForceStopSetup) || successItem,
                runControl->processRecipe(onStartSetup),
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
