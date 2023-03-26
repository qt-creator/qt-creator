// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>
#include <utils/wizard.h>

namespace BareMetal::Internal {

class BareMetalDeviceConfigurationWizardSetupPage;

class BareMetalDeviceConfigurationWizard final : public Utils::Wizard
{
public:
    explicit BareMetalDeviceConfigurationWizard(QWidget *parent = nullptr);

    ProjectExplorer::IDevicePtr device() const;

private:
    BareMetalDeviceConfigurationWizardSetupPage *m_setupPage = nullptr;
};

} // BareMetal::Internal
