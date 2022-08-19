// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>
#include <utils/wizard.h>

namespace BareMetal {
namespace Internal {

class BareMetalDeviceConfigurationWizardSetupPage;

// BareMetalDeviceConfigurationWizard

class BareMetalDeviceConfigurationWizard final : public Utils::Wizard
{
    Q_OBJECT

public:
    explicit BareMetalDeviceConfigurationWizard(QWidget *parent = nullptr);

    ProjectExplorer::IDevicePtr device() const;

private:
    BareMetalDeviceConfigurationWizardSetupPage *m_setupPage = nullptr;
};

} // namespace Internal
} // namespace BareMetal
