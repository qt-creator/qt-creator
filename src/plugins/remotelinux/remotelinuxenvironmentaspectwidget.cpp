// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "remotelinuxenvironmentaspectwidget.h"

#include "linuxdevice.h"
#include "remotelinuxenvironmentaspect.h"
#include "remotelinuxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <utils/devicefileaccess.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

RemoteLinuxEnvironmentAspectWidget::RemoteLinuxEnvironmentAspectWidget
        (RemoteLinuxEnvironmentAspect *aspect, Target *target)
    : EnvironmentAspectWidget(aspect)
{
    auto fetchButton = new QPushButton(Tr::tr("Fetch Device Environment"));
    addWidget(fetchButton);

    connect(target, &ProjectExplorer::Target::kitChanged, [this, aspect] {
        aspect->setRemoteEnvironment({});
    });

    connect(fetchButton, &QPushButton::clicked, this, [this, aspect, target] {
        if (IDevice::ConstPtr device = DeviceKitAspect::device(target->kit())) {
            DeviceFileAccess *access = device->fileAccess();
            QTC_ASSERT(access, return);
            aspect->setRemoteEnvironment(access->deviceEnvironment());
        }
    });

    const EnvironmentWidget::OpenTerminalFunc openTerminalFunc
            = [target](const Environment &env) {
        IDevice::ConstPtr device = DeviceKitAspect::device(target->kit());
        if (!device) {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  Tr::tr("Cannot Open Terminal"),
                                  Tr::tr("Cannot open remote terminal: Current kit has no device."));
            return;
        }
        const auto linuxDevice = device.dynamicCast<const LinuxDevice>();
        QTC_ASSERT(linuxDevice, return);
        linuxDevice->openTerminal(env, FilePath());
    };

    envWidget()->setOpenTerminalFunc(openTerminalFunc);
}

} // RemoteLinux
