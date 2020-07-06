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

#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketool.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

static FilePath cmakeFilePath(const Target *target)
{
    const CMakeProjectManager::CMakeTool *tool =
            CMakeProjectManager::CMakeKitAspect::cmakeTool(target->kit());
    return tool->filePath();
}

static QStringList flashAndRunArgs(const Target *target)
{
    const QString projectName = target->project()->displayName();

    // TODO: Hack! Implement flash target name handling, properly
    const QString targetName = "flash_" + projectName;

    return {"--build", ".", "--target", targetName};
}

class FlashAndRunConfiguration final : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(McuSupport::Internal::FlashAndRunConfiguration)

public:
    FlashAndRunConfiguration(Target *target, Utils::Id id)
        : RunConfiguration(target, id)
    {
        auto flashAndRunParameters = addAspect<BaseStringAspect>();
        flashAndRunParameters->setLabelText(tr("Flash and run CMake parameters:"));
        flashAndRunParameters->setDisplayStyle(BaseStringAspect::TextEditDisplay);
        flashAndRunParameters->setSettingsKey("FlashAndRunConfiguration.Parameters");

        setUpdater([target, flashAndRunParameters] {
            flashAndRunParameters->setValue(flashAndRunArgs(target).join(' '));
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
            const CommandLine cmd(
                        cmakeFilePath(target),
                        runControl->runConfiguration()->aspect<BaseStringAspect>()->value(),
                        CommandLine::Raw);
            Runnable r;
            r.workingDirectory =
                    target->activeBuildConfiguration()->buildDirectory().toUserOutput();
            r.setCommandLine(cmd);
            r.environment = target->activeBuildConfiguration()->environment();
            SimpleTargetRunner::doStart(r, {});
        });
    }
};

RunWorkerFactory::WorkerCreator makeFlashAndRunWorker()
{
    return RunWorkerFactory::make<FlashAndRunWorker>();
}

McuSupportRunConfigurationFactory::McuSupportRunConfigurationFactory()
    : FixedRunConfigurationFactory(FlashAndRunConfiguration::tr("Flash and run"))
{
    registerRunConfiguration<FlashAndRunConfiguration>(Constants::RUNCONFIGURATION);
    addSupportedTargetDeviceType(Constants::DEVICE_TYPE);
}

} // namespace Internal
} // namespace McuSupport
