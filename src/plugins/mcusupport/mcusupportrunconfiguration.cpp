/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "mcusupportrunconfiguration.h"
#include "mcusupportconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
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
        setStarter([this, runControl] {
            const Target *target = runControl->target();
            Runnable r;
            r.command = {cmakeFilePath(target),
                         runControl->runConfiguration()->aspect<StringAspect>()->value(),
                         CommandLine::Raw};
            r.workingDirectory = target->activeBuildConfiguration()->buildDirectory();
            r.environment = target->activeBuildConfiguration()->environment();
            SimpleTargetRunner::doStart(r);
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
