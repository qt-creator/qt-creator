// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportrunconfiguration.h"

#include "mcusupportconstants.h"
#include "mcusupporttr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <cmakeprojectmanager/cmakekitaspect.h>
#include <cmakeprojectmanager/cmaketool.h>

#include <utils/aspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

static FilePath cmakeFilePath(const Target *target)
{
    const CMakeProjectManager::CMakeTool *tool = CMakeProjectManager::CMakeKitAspect::cmakeTool(
        target->kit());
    return tool->filePath();
}

static QStringList flashAndRunArgs(const RunConfiguration *rc, const Target *target)
{
    // Use buildKey if provided, fallback to projectName
    const QString targetName = QLatin1String("flash_%1")
                                   .arg(!rc->buildKey().isEmpty()
                                            ? rc->buildKey()
                                            : target->project()->displayName());

    return {"--build", ".", "--target", targetName};
}

class FlashAndRunConfiguration final : public RunConfiguration
{
public:
    FlashAndRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        flashAndRunParameters.setLabelText(Tr::tr("Flash and run CMake parameters:"));
        flashAndRunParameters.setDisplayStyle(StringAspect::TextEditDisplay);
        flashAndRunParameters.setSettingsKey("FlashAndRunConfiguration.Parameters");

        setUpdater([target, this] {
            flashAndRunParameters.setValue(flashAndRunArgs(this, target).join(' '));
        });

        update();

        connect(target->project(), &Project::displayNameChanged, this, &RunConfiguration::update);
    }

    bool isEnabled(Utils::Id runMode) const override
    {
        if (disabled)
            return false;

        return RunConfiguration::isEnabled(runMode);
    }

    static bool disabled;
    StringAspect flashAndRunParameters{this};
};

bool FlashAndRunConfiguration::disabled = false;

// Factories

McuSupportRunConfigurationFactory::McuSupportRunConfigurationFactory()
{
    registerRunConfiguration<FlashAndRunConfiguration>(Constants::RUNCONFIGURATION);
    addSupportedTargetDeviceType(Constants::DEVICE_TYPE);
}

FlashRunWorkerFactory::FlashRunWorkerFactory()
{
    setProducer([](RunControl *runControl) {
        auto worker = new SimpleTargetRunner(runControl);
        worker->setStartModifier([worker, runControl] {
            const Target *target = runControl->target();
            worker->setCommandLine({cmakeFilePath(target),
                                    runControl->aspectData<StringAspect>()->value, CommandLine::Raw});
            worker->setWorkingDirectory(target->activeBuildConfiguration()->buildDirectory());
            worker->setEnvironment(target->activeBuildConfiguration()->environment());
        });

        QObject::connect(runControl, &RunControl::started, runControl, [] {
            FlashAndRunConfiguration::disabled = true;
            ProjectExplorerPlugin::updateRunActions();
        });
        QObject::connect(runControl, &RunControl::stopped, runControl, [] {
            FlashAndRunConfiguration::disabled = false;
            ProjectExplorerPlugin::updateRunActions();
        });

        return worker;
    });
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(Constants::RUNCONFIGURATION);
}

} // McuSupport::Internal
