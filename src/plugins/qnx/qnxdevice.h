// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qnx_export.h"

#include <remotelinux/linuxdevice.h>

namespace Qnx::Internal {

class QnxDevice final : public RemoteLinux::LinuxDevice
{
public:
    using Ptr = QSharedPointer<QnxDevice>;
    using ConstPtr = QSharedPointer<const QnxDevice>;

    static Ptr create() { return Ptr(new QnxDevice); }

    ProjectExplorer::PortsGatheringMethod portsGatheringMethod() const override;
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;

    ProjectExplorer::DeviceTester *createDeviceTester() const override;
    Utils::ProcessInterface *createProcessInterface() const override;

    int qnxVersion() const;

protected:
    void fromMap(const QVariantMap &map) final;
    QVariantMap toMap() const final;

    QString interruptProcessByNameCommandLine(const QString &filePath) const;
    QString killProcessByNameCommandLine(const QString &filePath) const;

private:
    QnxDevice();

    void updateVersionNumber() const;

    mutable int m_versionNumber = 0;
};

class QnxDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    QnxDeviceFactory();
};

} // Qnx::Internal
