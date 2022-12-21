// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportrunconfiguration.h"
#include "mcusupportconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketool.h>

#include <utils/aspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

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
    Q_DECLARE_TR_FUNCTIONS(McuSupport::Internal::FlashAndRunConfiguration)

public:
    FlashAndRunConfiguration(Target *target, Utils::Id id)
        : RunConfiguration(target, id)
    {
        auto flashAndRunParameters = addAspect<StringAspect>();
        flashAndRunParameters->setLabelText(tr("Flash and run CMake parameters:"));
        flashAndRunParameters->setDisplayStyle(StringAspect::TextEditDisplay);
        flashAndRunParameters->setSettingsKey("FlashAndRunConfiguration.Parameters");

        setUpdater([target, flashAndRunParameters, this] {
            flashAndRunParameters->setValue(flashAndRunArgs(this, target).join(' '));
        });

        update();

        connect(target->project(), &Project::displayNameChanged, this, &RunConfiguration::update);
    }
};

class FlashAndRunWorker : public SimpleTargetRunner
{
public:
    FlashAndRunWorker(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setStartModifier([this, runControl] {
            const Target *target = runControl->target();
            setCommandLine({cmakeFilePath(target), runControl->aspect<StringAspect>()->value,
                            CommandLine::Raw});
            setWorkingDirectory(target->activeBuildConfiguration()->buildDirectory());
            setEnvironment(target->activeBuildConfiguration()->environment());
        });
    }
};

RunWorkerFactory::WorkerCreator makeFlashAndRunWorker()
{
    return RunWorkerFactory::make<FlashAndRunWorker>();
}

McuSupportRunConfigurationFactory::McuSupportRunConfigurationFactory()
    : RunConfigurationFactory()
{
    registerRunConfiguration<FlashAndRunConfiguration>(Constants::RUNCONFIGURATION);
    addSupportedTargetDeviceType(Constants::DEVICE_TYPE);
}

} // namespace Internal
} // namespace McuSupport
