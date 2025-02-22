// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/filepath.h>

#include <QDebug>

namespace Ios::Internal {

class IosConfigurations;
class IosSimulatorFactory;

class IosDeviceType
{
public:
    enum Type {
        IosDevice,
        SimulatedDevice
    };
    IosDeviceType(Type type = IosDevice, const QString &identifier = QString(),
                  const QString &displayName = QString());

    bool fromMap(const Utils::Store &map);
    Utils::Store toMap() const;

    bool operator ==(const IosDeviceType &o) const;
    bool operator !=(const IosDeviceType &o) const { return !(*this == o); }
    bool operator <(const IosDeviceType &o) const;

    Type type;
    QString identifier;
    QString displayName;
};

QDebug operator <<(QDebug debug, const IosDeviceType &deviceType);

class IosSimulator final : public ProjectExplorer::IDevice
{
public:
    using ConstPtr = std::shared_ptr<const IosSimulator>;
    using Ptr = std::shared_ptr<IosSimulator>;
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;

    ProjectExplorer::IDeviceWidget *createWidget() override;

private:
    Tasking::ExecutableItem portsGatheringRecipe(
        const Tasking::Storage<Utils::PortsOutputData> &output) const override;
    QUrl toolControlChannel(const ControlChannelHint &) const override;

    friend class IosSimulatorFactory;
    friend class IosConfigurations;
    IosSimulator();
    IosSimulator(Utils::Id id);
};

class IosSimulatorFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    IosSimulatorFactory();
};

} // Ios::Internal

Q_DECLARE_METATYPE(Ios::Internal::IosDeviceType)
