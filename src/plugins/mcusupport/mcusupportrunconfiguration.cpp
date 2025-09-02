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
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

static FilePath cmakeFilePath(const Kit *k)
{
    const CMakeProjectManager::CMakeTool *tool = CMakeProjectManager::CMakeKitAspect::cmakeTool(k);
    return tool->filePath();
}

static QStringList flashAndRunArgs(const RunConfiguration *rc)
{
    // Use buildKey if provided, fallback to projectName
    const QString targetName = QLatin1String("flash_%1")
                                   .arg(!rc->buildKey().isEmpty()
                                            ? rc->buildKey()
                                            : rc->project()->displayName());

    return {"--build", ".", "--target", targetName};
}

class FlashAndRunConfiguration final : public RunConfiguration
{
public:
    FlashAndRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        flashAndRunParameters.setLabelText(Tr::tr("Flash and run CMake parameters:"));
        flashAndRunParameters.setDisplayStyle(StringAspect::TextEditDisplay);
        flashAndRunParameters.setSettingsKey("FlashAndRunConfiguration.Parameters");

        setUpdater([this] { flashAndRunParameters.setValue(flashAndRunArgs(this).join(' ')); });
        update();
        connect(project(), &Project::displayNameChanged, this, &RunConfiguration::update);
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
        const auto modifier = [runControl](Process &process) {
            const BuildConfiguration *bc = runControl->buildConfiguration();
            process.setCommand({cmakeFilePath(bc->kit()),
                                runControl->aspectData<StringAspect>()->value, CommandLine::Raw});
            process.setWorkingDirectory(bc->buildDirectory());
            process.setEnvironment(bc->environment());
        };

        QObject::connect(runControl, &RunControl::started, runControl, [] {
            FlashAndRunConfiguration::disabled = true;
            ProjectExplorerPlugin::updateRunActions();
        });
        QObject::connect(runControl, &RunControl::stopped, runControl, [] {
            FlashAndRunConfiguration::disabled = false;
            ProjectExplorerPlugin::updateRunActions();
        });
        return createProcessWorker(runControl, modifier);
    });
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(Constants::RUNCONFIGURATION);
}

} // McuSupport::Internal
