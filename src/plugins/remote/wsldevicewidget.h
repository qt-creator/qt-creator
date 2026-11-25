// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicewidget.h>

namespace Wsl::Internal {

class WslDeviceWidget final : public ProjectExplorer::IDeviceWidget
{
public:
    explicit WslDeviceWidget(const ProjectExplorer::IDevice::Ptr &device);

    void updateDeviceFromUi() final {}
};

} // namespace Wsl::Internal
