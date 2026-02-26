// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectdevicestep.h"

#include "abstractremotelinuxdeploystep.h"
#include "linuxdevice.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/async.h>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace RemoteLinux::Internal {

class ConnectDeviceStep final : public AbstractRemoteLinuxDeployStep
{
public:
    ConnectDeviceStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        setWidgetExpandedByDefault(false);

        setInternalInitializer([this]() -> Result<> {
            if (!deviceConfiguration()
                || !std::dynamic_pointer_cast<const LinuxDevice>(deviceConfiguration()))
                return make_unexpected(Tr::tr("No device configured for the run configuration."));
            return {};
        });
    }

private:
    GroupItem deployRecipe() final;
};

struct DeviceAndResult
{
    ConnectDeviceStep *step;
    LinuxDevice::ConstPtr device;
    Result<void> result;
};

class ConnectDeviceTaskAdapter final
{
public:
    void operator()(DeviceAndResult *task, QTaskInterface *iface)
    {
        auto device = task->device;
        device->tryToConnect({[task, iface](const Result<void> &r) {
            task->result = r;
            iface->reportDone(toDoneResult(r.has_value()));
        }});
    }
};

using ConnectDeviceTask = QCustomTask<DeviceAndResult, ConnectDeviceTaskAdapter>;

GroupItem ConnectDeviceStep::deployRecipe()
{
    const auto setup = [this](DeviceAndResult &task) {
        auto device = std::dynamic_pointer_cast<const LinuxDevice>(deviceConfiguration());
        if (!device->isDisconnected())
            return SetupResult::StopWithSuccess;

        task.device = device;
        task.step = this;
        task.step->addProgressMessage(Tr::tr("Connecting target device ..."));
        return SetupResult::Continue;
    };

    const auto onDone = [](const DeviceAndResult &task, DoneWith result) {
        if (result == DoneWith::Success)
            task.step->addProgressMessage(Tr::tr("Device connected."));
        else
            task.step->addErrorMessage(Tr::tr("failed: %1.").arg(task.result.error()));
    };

    return ConnectDeviceTask(setup, onDone);
}

// Factory

class ConnectDeviceStepFactory final : public BuildStepFactory
{
public:
    ConnectDeviceStepFactory()
    {
        registerStep<ConnectDeviceStep>(Constants::ConnectStepId);
        setDisplayName(Tr::tr("Connect to the target device"));
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    }
};

void setupConnectDeviceStep()
{
    static ConnectDeviceStepFactory theConnectDeviceStepFactory;
}

} // namespace RemoteLinux::Internal
