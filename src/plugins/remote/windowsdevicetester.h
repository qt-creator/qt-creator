// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace Remote {

class REMOTELINUX_EXPORT WindowsDeviceTester : public ProjectExplorer::DeviceTester
{
    Q_OBJECT

public:
    explicit WindowsDeviceTester(const ProjectExplorer::IDevice::Ptr &device,
                                 QObject *parent = nullptr);

    void testDevice() override;
    void stopTest() override;
};

} // namespace Remote
