// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicecheckbuildstep.h"

#include "../buildstep.h"
#include "../kitaspects.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"

#include "devicemanager.h"
#include "idevicefactory.h"

#include <solutions/tasking/tasktree.h>

#include <QMessageBox>

namespace ProjectExplorer {

class DeviceCheckBuildStep final : public BuildStep
{
public:
    DeviceCheckBuildStep(BuildStepList *bsl, Utils::Id id)
        : BuildStep(bsl, id)
    {
        setWidgetExpandedByDefault(false);
    }

    bool init() final
    {
        IDevice::ConstPtr device = DeviceKitAspect::device(kit());
        if (device)
            return true;

        Utils::Id deviceTypeId = DeviceTypeKitAspect::deviceTypeId(kit());
        IDeviceFactory *factory = IDeviceFactory::find(deviceTypeId);
        if (!factory || !factory->canCreate()) {
            emit addOutput(Tr::tr("No device configured."), OutputFormat::ErrorMessage);
            return false;
        }

        QMessageBox msgBox(QMessageBox::Question, Tr::tr("Set Up Device"),
                           Tr::tr("There is no device set up for this kit. Do you want to add a device?"),
                           QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::No) {
            emit addOutput(Tr::tr("No device configured."), OutputFormat::ErrorMessage);
            return false;
        }

        IDevice::Ptr newDevice = factory->create();
        if (!newDevice) {
            emit addOutput(Tr::tr("No device configured."), OutputFormat::ErrorMessage);
            return false;
        }

        DeviceManager *dm = DeviceManager::instance();
        dm->addDevice(newDevice);
        DeviceKitAspect::setDevice(kit(), newDevice);
        return true;
    }

private:
    Tasking::GroupItem runRecipe() final { return Tasking::Group{}; }
};

// Factory

class DeviceCheckBuildStepFactory final : public BuildStepFactory
{
public:
    DeviceCheckBuildStepFactory()
    {
        registerStep<DeviceCheckBuildStep>(Constants::DEVICE_CHECK_STEP);
        setDisplayName(Tr::tr("Check for a configured device"));
    }
};

void setupDeviceCheckBuildStep()
{
    static DeviceCheckBuildStepFactory theDeviceCheckBuildStepFactory;
}

} // ProjectExplorer
