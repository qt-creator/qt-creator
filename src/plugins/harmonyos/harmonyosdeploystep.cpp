// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdeploystep.h"

#include "harmonyosconstants.h"
#include "harmonyostr.h"

#include <cmakeprojectmanager/cmakekitaspect.h>

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
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

class HarmonyOsDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    HarmonyOsDeployConfigurationFactory()
    {
        setConfigBaseId(Constants::HARMONYOS_DEPLOY_CONFIG_ID);
        addSupportedTargetDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setDefaultDisplayName(Tr::tr("Build HarmonyOS package"));
        addInitialStep(Constants::HARMONYOS_MAKE_HAP_STEP_ID);
    }
};

void setupHarmonyOsDeployStep()
{
    static MakeHapStepFactory theMakeHapStepFactory;
}

void setupHarmonyOsDeployConfiguration()
{
    static HarmonyOsDeployConfigurationFactory theHarmonyOsDeployConfigurationFactory;
}

} // namespace HarmonyOs::Internal
