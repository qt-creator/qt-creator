// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericlinuxdeviceconfigurationwizard.h"

#include "genericlinuxdeviceconfigurationwizardpages.h"

#include <coreplugin/icore.h>

namespace RemoteLinux {

GenericLinuxDeviceConfigurationWizard::GenericLinuxDeviceConfigurationWizard(
    const QString &title, const ProjectExplorer::IDevicePtr &device)
    : Utils::Wizard(Core::ICore::dialogParent())
{
    setWindowTitle(title);

    auto setupPage = new GenericLinuxDeviceConfigurationWizardSetupPage;
    auto keyDeploymentPage = new GenericLinuxDeviceConfigurationWizardKeyDeploymentPage;
    auto finalPage = new GenericLinuxDeviceConfigurationWizardFinalPage;

    addPage(setupPage);
    addPage(keyDeploymentPage);
    addPage(finalPage);

    finalPage->setCommitPage(true);
    setupPage->setDevice(device);
    keyDeploymentPage->setDevice(device);
}

} // namespace RemoteLinux
