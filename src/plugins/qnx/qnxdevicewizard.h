// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qnxdevice.h"

#include <utils/wizard.h>

namespace RemoteLinux {
class GenericLinuxDeviceConfigurationWizardSetupPage;
class GenericLinuxDeviceConfigurationWizardKeyDeploymentPage;
class GenericLinuxDeviceConfigurationWizardFinalPage;
}

namespace Qnx::Internal {

class QnxDeviceWizard : public Utils::Wizard
{
public:
    explicit QnxDeviceWizard(QWidget *parent = nullptr);

    ProjectExplorer::IDevice::Ptr device();

private:
    enum PageId {
        SetupPageId,
        KeyDeploymenPageId,
        FinalPageId
    };

    RemoteLinux::GenericLinuxDeviceConfigurationWizardSetupPage *m_setupPage;
    RemoteLinux::GenericLinuxDeviceConfigurationWizardKeyDeploymentPage *m_keyDeploymentPage;
    RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage *m_finalPage;
    QnxDevice::Ptr m_device;
};

} // Qnx::Internal
