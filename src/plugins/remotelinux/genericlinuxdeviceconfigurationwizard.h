// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevicefwd.h>
#include <utils/wizard.h>

namespace RemoteLinux {
namespace Internal { class GenericLinuxDeviceConfigurationWizardPrivate; }

class REMOTELINUX_EXPORT GenericLinuxDeviceConfigurationWizard : public Utils::Wizard
{
    Q_OBJECT

public:
    GenericLinuxDeviceConfigurationWizard(QWidget *parent = nullptr);
    ~GenericLinuxDeviceConfigurationWizard() override;

    ProjectExplorer::IDevicePtr device();

private:
    Internal::GenericLinuxDeviceConfigurationWizardPrivate * const d;
};

} // namespace RemoteLinux
