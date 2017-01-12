/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: KDAB (info@kdab.com)
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

#include "devicecheckbuildstep.h"

#include "../kitinformation.h"
#include "../target.h"
#include "devicemanager.h"
#include "idevice.h"
#include "idevicefactory.h"

#include <QMessageBox>

using namespace ProjectExplorer;

DeviceCheckBuildStep::DeviceCheckBuildStep(BuildStepList *bsl, Core::Id id)
    : BuildStep(bsl, id)
{
    setDefaultDisplayName(stepDisplayName());
}

DeviceCheckBuildStep::DeviceCheckBuildStep(BuildStepList *bsl, DeviceCheckBuildStep *bs)
    : BuildStep(bsl, bs)
{
    setDefaultDisplayName(stepDisplayName());
}

bool DeviceCheckBuildStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    IDevice::ConstPtr device = DeviceKitInformation::device(target()->kit());
    if (!device) {
        Core::Id deviceTypeId = DeviceTypeKitInformation::deviceTypeId(target()->kit());
        IDeviceFactory *factory = IDeviceFactory::find(deviceTypeId);
        if (!factory || !factory->canCreate()) {
            emit addOutput(tr("No device configured."), BuildStep::OutputFormat::ErrorMessage);
            return false;
        }

        QMessageBox msgBox(QMessageBox::Question, tr("Set Up Device"),
                              tr("There is no device set up for this kit. Do you want to add a device?"),
                              QMessageBox::Yes|QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::No) {
            emit addOutput(tr("No device configured."), BuildStep::OutputFormat::ErrorMessage);
            return false;
        }

        IDevice::Ptr newDevice = factory->create(deviceTypeId);
        if (newDevice.isNull()) {
            emit addOutput(tr("No device configured."), BuildStep::OutputFormat::ErrorMessage);
            return false;
        }

        DeviceManager *dm = DeviceManager::instance();
        dm->addDevice(newDevice);

        DeviceKitInformation::setDevice(target()->kit(), newDevice);
    }

    return true;
}

void DeviceCheckBuildStep::run(QFutureInterface<bool> &fi)
{
    reportRunResult(fi, true);
}

BuildStepConfigWidget *DeviceCheckBuildStep::createConfigWidget()
{
    return new SimpleBuildStepConfigWidget(this);
}

Core::Id DeviceCheckBuildStep::stepId()
{
    return "ProjectExplorer.DeviceCheckBuildStep";
}

QString DeviceCheckBuildStep::stepDisplayName()
{
    return tr("Check for a configured device");
}
