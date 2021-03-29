/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "dockerrunconfiguration.h"

#include "dockerconstants.h"
#include "dockerdevice.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/stringutils.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Docker {
namespace Internal {

class DockerContainerRunConfiguration : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerRunConfiguration)

public:
    DockerContainerRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        auto rmOption = addAspect<BoolAspect>();
        rmOption->setSettingsKey("Docker.RunConfiguration.RmOption");
        rmOption->setDefaultValue(true);
        rmOption->setLabelText(tr("Automatically remove the container when it exits"));

        auto ttyOption = addAspect<BoolAspect>();
        ttyOption->setSettingsKey("Docker.RunConfiguration.TtyOption");
        ttyOption->setLabelText(tr("Allocate a pseudo-TTY"));
        ttyOption->setVisible(false); // Not yet.

        auto interactiveOption = addAspect<BoolAspect>();
        interactiveOption->setSettingsKey("Docker.RunConfiguration.InteractiveOption");
        interactiveOption->setLabelText(tr("Keep STDIN open even if not attached"));
        interactiveOption->setVisible(false); // Not yet.

        auto effectiveCommand = addAspect<StringAspect>();
        effectiveCommand->setLabelText(tr("Effective command call:"));
        effectiveCommand->setDisplayStyle(StringAspect::TextEditDisplay);
        effectiveCommand->setReadOnly(true);

        setUpdater([this, effectiveCommand] {
            IDevice::ConstPtr device = DeviceKitAspect::device(kit());
            QTC_ASSERT(device, return);
            DockerDevice::ConstPtr dockerDevice = qSharedPointerCast<const DockerDevice>(device);
            QTC_ASSERT(dockerDevice, return);
            const DockerDeviceData &data = dockerDevice->data();

            const Runnable r = runnable();
            const QStringList dockerRunFlags = r.extraData[Constants::DOCKER_RUN_FLAGS].toStringList();

            CommandLine cmd("docker");
            cmd.addArg("run");
            cmd.addArgs(dockerRunFlags);
            cmd.addArg(data.imageId);

            // FIXME: the global one above is apparently not sufficient.
            effectiveCommand->setReadOnly(true);
            effectiveCommand->setValue(cmd.toUserOutput());
        });

        setRunnableModifier([rmOption, interactiveOption, ttyOption](Runnable &runnable) {
            QStringList runArgs;
            if (!rmOption->value())
                runArgs.append("--rm=false");
            if (interactiveOption->value())
                runArgs.append("--interactive");
            if (ttyOption->value())
                runArgs.append("--tty");
            runnable.extraData[Constants::DOCKER_RUN_FLAGS].toStringList();
        });

        setCommandLineGetter([] {
            return CommandLine();
        });

        update();
        connect(rmOption, &BaseAspect::changed, this, &RunConfiguration::update);
    }

private:
    bool isEnabled() const override { return true; }
};


DockerContainerRunConfigurationFactory::DockerContainerRunConfigurationFactory() :
    FixedRunConfigurationFactory(DockerContainerRunConfiguration::tr("Docker Container"))
{
    registerRunConfiguration<DockerContainerRunConfiguration>
        ("Docker.DockerContainerRunConfiguration");
    addSupportedTargetDeviceType(Constants::DOCKER_DEVICE_TYPE);
}

} // Internal
} // Docker
