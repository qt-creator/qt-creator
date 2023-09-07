// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetaldeviceconfigurationwizard.h"

#include "baremetalconstants.h"
#include "baremetaldevice.h"
#include "baremetaldeviceconfigurationwizardpages.h"
#include "baremetaltr.h"

namespace BareMetal::Internal {

enum PageId { SetupPageId };

BareMetalDeviceConfigurationWizard::BareMetalDeviceConfigurationWizard(QWidget *parent) :
   Utils::Wizard(parent),
   m_setupPage(new BareMetalDeviceConfigurationWizardSetupPage(this))
{
    setWindowTitle(Tr::tr("New Bare Metal Device Configuration Setup"));
    setPage(SetupPageId, m_setupPage);
    m_setupPage->setCommitPage(true);
}

ProjectExplorer::IDevice::Ptr BareMetalDeviceConfigurationWizard::device() const
{
    const auto dev = BareMetalDevice::create();
    dev->setupId(ProjectExplorer::IDevice::ManuallyAdded, Utils::Id());
    dev->settings()->displayName.setDefaultValue(m_setupPage->configurationName());
    dev->setType(Constants::BareMetalOsType);
    dev->setMachineType(ProjectExplorer::IDevice::Hardware);
    dev->setDebugServerProviderId(m_setupPage->debugServerProviderId());
    return dev;
}

} // BareMetal::Internal
