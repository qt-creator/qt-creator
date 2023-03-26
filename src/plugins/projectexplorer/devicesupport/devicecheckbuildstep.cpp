// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicecheckbuildstep.h"

#include "../kitinformation.h"
#include "../projectexplorertr.h"

#include "devicemanager.h"
#include "idevice.h"
#include "idevicefactory.h"

#include <QMessageBox>

using namespace ProjectExplorer;

DeviceCheckBuildStep::DeviceCheckBuildStep(BuildStepList *bsl, Utils::Id id)
    : BuildStep(bsl, id)
{
    setWidgetExpandedByDefault(false);
}

bool DeviceCheckBuildStep::init()
{
    IDevice::ConstPtr device = DeviceKitAspect::device(kit());
    if (!device) {
        Utils::Id deviceTypeId = DeviceTypeKitAspect::deviceTypeId(kit());
        IDeviceFactory *factory = IDeviceFactory::find(deviceTypeId);
        if (!factory || !factory->canCreate()) {
            emit addOutput(Tr::tr("No device configured."), BuildStep::OutputFormat::ErrorMessage);
            return false;
        }

        QMessageBox msgBox(QMessageBox::Question, Tr::tr("Set Up Device"),
                              Tr::tr("There is no device set up for this kit. Do you want to add a device?"),
                              QMessageBox::Yes|QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::No) {
            emit addOutput(Tr::tr("No device configured."), BuildStep::OutputFormat::ErrorMessage);
            return false;
        }

        IDevice::Ptr newDevice = factory->create();
        if (newDevice.isNull()) {
            emit addOutput(Tr::tr("No device configured."), BuildStep::OutputFormat::ErrorMessage);
            return false;
        }

        DeviceManager *dm = DeviceManager::instance();
        dm->addDevice(newDevice);

        DeviceKitAspect::setDevice(kit(), newDevice);
    }

    return true;
}

void DeviceCheckBuildStep::doRun()
{
    emit finished(true);
}

Utils::Id DeviceCheckBuildStep::stepId()
{
    return "ProjectExplorer.DeviceCheckBuildStep";
}

QString DeviceCheckBuildStep::displayName()
{
    return Tr::tr("Check for a configured device");
}
