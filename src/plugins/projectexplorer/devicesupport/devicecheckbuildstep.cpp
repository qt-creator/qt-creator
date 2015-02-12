/**************************************************************************
**
** Copyright (C) 2011 - 2014 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

bool DeviceCheckBuildStep::init()
{
    IDevice::ConstPtr device = DeviceKitInformation::device(target()->kit());
    if (!device) {
        Core::Id deviceTypeId = DeviceTypeKitInformation::deviceTypeId(target()->kit());
        IDeviceFactory *factory = IDeviceFactory::find(deviceTypeId);
        if (!factory || !factory->canCreate()) {
            emit addOutput(tr("No device configured."), BuildStep::ErrorMessageOutput);
            return false;
        }

        QMessageBox msgBox(QMessageBox::Question, tr("Set Up Device"),
                              tr("There is no device set up for this kit. Do you want to add a device?"),
                              QMessageBox::Yes|QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::No) {
            emit addOutput(tr("No device configured."), BuildStep::ErrorMessageOutput);
            return false;
        }

        IDevice::Ptr newDevice = factory->create(deviceTypeId);
        if (newDevice.isNull()) {
            emit addOutput(tr("No device configured."), BuildStep::ErrorMessageOutput);
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
    fi.reportResult(true);
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
