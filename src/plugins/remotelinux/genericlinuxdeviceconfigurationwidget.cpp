// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericlinuxdeviceconfigurationwidget.h"

#include "linuxdevice.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "sshkeycreationdialog.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QSpinBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

GenericLinuxDeviceConfigurationWidget::GenericLinuxDeviceConfigurationWidget(
    const IDevice::Ptr &device)
    : IDeviceWidget(device)
{
    auto createKeyButton = new QPushButton(Tr::tr("Create New..."));

    const QString machineType = device->machineType() == IDevice::Hardware
                                    ? Tr::tr("Physical Device")
                                    : Tr::tr("Emulator");
    auto linuxDevice = std::dynamic_pointer_cast<LinuxDevice>(device);
    QTC_ASSERT(linuxDevice, return);

    using namespace Layouting;

    auto portWarningLabel = new QLabel(
        QString("<font color=\"red\">%1</font>").arg(Tr::tr("You will need at least one port.")));

    auto updatePortWarningLabel = [portWarningLabel, device]() {
        portWarningLabel->setVisible(device->freePortsAspect.volatileValue().isEmpty());
    };

    updatePortWarningLabel();

    // clang-format off
    connect(&device->freePortsAspect, &PortListAspect::volatileValueChanged, this, updatePortWarningLabel);

    Form {
        Tr::tr("Machine type:"), machineType, st, br,
        device->sshParametersAspectContainer().host, device->sshParametersAspectContainer().port, device->sshParametersAspectContainer().hostKeyCheckingMode, st, br,
        device->freePortsAspect, portWarningLabel, device->sshParametersAspectContainer().timeout, st, br,
        device->sshParametersAspectContainer().userName, st, br,
        device->sshParametersAspectContainer().useKeyFile, st, br,
        device->sshParametersAspectContainer().privateKeyFile, createKeyButton, br,
        device->debugServerPathAspect, br,
        device->qmlRunCommandAspect, br,
        linuxDevice->sourceProfile, br,
        device->sshForwardDebugServerPort, br,
        device->linkDevice, br,
    }.attachTo(this);
    // clang-format on

    connect(
        createKeyButton,
        &QAbstractButton::clicked,
        this,
        &GenericLinuxDeviceConfigurationWidget::createNewKey);
}

GenericLinuxDeviceConfigurationWidget::~GenericLinuxDeviceConfigurationWidget() = default;

void GenericLinuxDeviceConfigurationWidget::createNewKey()
{
    SshKeyCreationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        device()->sshParametersAspectContainer().privateKeyFile.setValue(
            dialog.privateKeyFilePath());
    }
}

} // RemoteLinux::Internal
