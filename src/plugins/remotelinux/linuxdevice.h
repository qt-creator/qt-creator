// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

namespace RemoteLinux {

class REMOTELINUX_EXPORT LinuxDevice : public ProjectExplorer::IDevice
{
public:
    using Ptr = QSharedPointer<LinuxDevice>;
    using ConstPtr = QSharedPointer<const LinuxDevice>;

    ~LinuxDevice();

    static Ptr create() { return Ptr(new LinuxDevice); }

    ProjectExplorer::IDeviceWidget *createWidget() override;

    bool canAutoDetectPorts() const override;
    ProjectExplorer::PortsGatheringMethod portsGatheringMethod() const override;
    bool canCreateProcessModel() const override { return true; }
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const override;
    bool hasDeviceTester() const override { return true; }
    ProjectExplorer::DeviceTester *createDeviceTester() const override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    bool usableAsBuildDevice() const override;

    QString userAtHost() const;

    Utils::FilePath rootPath() const override;

    bool handlesFile(const Utils::FilePath &filePath) const override;

    Utils::ProcessInterface *createProcessInterface() const override;
    ProjectExplorer::FileTransferInterface *createFileTransferInterface(
            const ProjectExplorer::FileTransferSetupData &setup) const override;

protected:
    LinuxDevice();

    class LinuxDevicePrivate *d;
    friend class SshProcessInterface;
    friend class SshTransferInterface;
};

namespace Internal {

class LinuxDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    LinuxDeviceFactory();
};

} // namespace Internal
} // namespace RemoteLinux
