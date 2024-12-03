// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <utils/wizard.h>

namespace ProjectExplorer { class DeviceRef; }

namespace RemoteLinux {

class REMOTELINUX_EXPORT SshDeviceWizard : public Utils::Wizard
{
public:
    SshDeviceWizard(const QString &title, const ProjectExplorer::DeviceRef &device);
};

} // namespace RemoteLinux
