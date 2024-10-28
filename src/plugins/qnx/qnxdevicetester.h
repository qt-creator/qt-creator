// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <remotelinux/linuxdevicetester.h>

namespace Qnx::Internal {

class QnxDeviceTester : public RemoteLinux::GenericLinuxDeviceTester
{
public:
    explicit QnxDeviceTester(const ProjectExplorer::IDevice::Ptr &device, QObject *parent = nullptr);

    void testDevice() override;
};

} // Qnx::Internal
