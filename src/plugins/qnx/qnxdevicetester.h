// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <remotelinux/linuxdevicetester.h>

namespace Qnx::Internal {

class QnxDeviceTester : public ProjectExplorer::DeviceTester
{
public:
    explicit QnxDeviceTester(QObject *parent = nullptr);

    void testDevice(const ProjectExplorer::IDevice::Ptr &device) override;
    void stopTest() override;

private:
    RemoteLinux::GenericLinuxDeviceTester *m_genericTester = nullptr;
    ProjectExplorer::IDevice::ConstPtr m_device;
};

} // Qnx::Internal
