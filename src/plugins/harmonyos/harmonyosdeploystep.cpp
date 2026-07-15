// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdeploystep.h"

#include "harmonyosconstants.h"
#include "harmonyosdevice.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <cmakeprojectmanager/cmakekitaspect.h>

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/commandline.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace HarmonyOs::Internal {

// Builds the .hap via the Qt-generated CMake "<target>_make_hap" target.
class MakeHapStep final : public AbstractProcessStep
{
public:
    MakeHapStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Build HarmonyOS package (.hap)"));
        setSummaryUpdater([this] {
            const QString target = makeHapTarget();
            return Tr::tr("<b>Build HarmonyOS package:</b> %1")
                .arg(target.isEmpty() ? Tr::tr("(no target)") : target);
        });
    }

private:
    QString makeHapTarget() const
    {
        const QString buildKey = buildConfiguration()->activeBuildKey();
        return buildKey.isEmpty() ? QString() : buildKey + "_make_hap";
    }

    bool init() final
    {
        if (!AbstractProcessStep::init())
            return false;

        BuildConfiguration *const bc = buildConfiguration();
        QTC_ASSERT(bc, return false);

        const FilePath cmake = CMakeProjectManager::CMakeKitAspect::cmakeExecutable(kit());
        if (cmake.isEmpty()) {
            emit addOutput(Tr::tr("No CMake tool is set in the kit."), OutputFormat::ErrorMessage);
            return false;
        }

        const QString target = makeHapTarget();
        if (target.isEmpty()) {
            emit addOutput(Tr::tr("No active build target."), OutputFormat::ErrorMessage);
            return false;
        }

        const FilePath buildDir = bc->buildDirectory();
        processParameters()->setCommandLine(
            {cmake, {"--build", buildDir.path(), "--target", target}});
        processParameters()->setWorkingDirectory(buildDir);
        return true;
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }
};

class MakeHapStepFactory final : public BuildStepFactory
{
public:
    MakeHapStepFactory()
    {
        registerStep<MakeHapStep>(Constants::HARMONYOS_MAKE_HAP_STEP_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setRepeatable(false);
        setDisplayName(Tr::tr("Build HarmonyOS package (.hap)"));
    }
};

// Installs the built .hap onto the connected device with hdc.
class InstallHapStep final : public AbstractProcessStep
{
public:
    InstallHapStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Install HarmonyOS package with hdc"));
        setSummaryUpdater([] { return Tr::tr("<b>Install .hap on device with hdc</b>"); });
    }

private:
    bool init() final
    {
        if (!AbstractProcessStep::init())
            return false;

        BuildConfiguration *const bc = buildConfiguration();
        QTC_ASSERT(bc, return false);

        const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
        if (hdc.isEmpty()) {
            emit addOutput(Tr::tr("hdc was not found in the HarmonyOS SDK."),
                           OutputFormat::ErrorMessage);
            return false;
        }

        const QString buildKey = bc->activeBuildKey();
        if (buildKey.isEmpty()) {
            emit addOutput(Tr::tr("No active build target."), OutputFormat::ErrorMessage);
            return false;
        }

        const FilePath hap = bc->buildDirectory().pathAppended(
            "harmonyos-build/entry/build/default/outputs/default/" + buildKey + ".hap");

        // Target the run device explicitly so the right one is used when several
        // are connected.
        CommandLine cmd{hdc};
        if (const auto device = std::dynamic_pointer_cast<const HarmonyOsDevice>(
                RunDeviceKitAspect::device(kit()))) {
            if (const QString serial = device->serialNumber(); !serial.isEmpty())
                cmd.addArgs({"-t", serial});
        }
        cmd.addArgs({"install", hap.nativePath()});
        processParameters()->setCommandLine(cmd);
        processParameters()->setWorkingDirectory(bc->buildDirectory());
        return true;
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }
};

class InstallHapStepFactory final : public BuildStepFactory
{
public:
    InstallHapStepFactory()
    {
        registerStep<InstallHapStep>(Constants::HARMONYOS_INSTALL_HAP_STEP_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setRepeatable(false);
        setDisplayName(Tr::tr("Install HarmonyOS package with hdc"));
    }
};

class HarmonyOsDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    HarmonyOsDeployConfigurationFactory()
    {
        setConfigBaseId(Constants::HARMONYOS_DEPLOY_CONFIG_ID);
        addSupportedTargetDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setDefaultDisplayName(Tr::tr("Deploy to HarmonyOS device"));
        addInitialStep(Constants::HARMONYOS_MAKE_HAP_STEP_ID);
        addInitialStep(Constants::HARMONYOS_INSTALL_HAP_STEP_ID);
    }
};

void setupHarmonyOsDeployStep()
{
    static MakeHapStepFactory theMakeHapStepFactory;
    static InstallHapStepFactory theInstallHapStepFactory;
}

void setupHarmonyOsDeployConfiguration()
{
    static HarmonyOsDeployConfigurationFactory theHarmonyOsDeployConfigurationFactory;
}

} // namespace HarmonyOs::Internal
