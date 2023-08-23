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
    using ConstPtr = QSharedPointer<const IosSimulator>;
    using Ptr = QSharedPointer<IosSimulator>;
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;

    ProjectExplorer::IDeviceWidget *createWidget() override;
    Utils::Port nextPort() const;

protected:
    friend class IosSimulatorFactory;
    friend class IosConfigurations;
    IosSimulator();
    IosSimulator(Utils::Id id);

private:
    mutable quint16 m_lastPort;
};

class IosSimulatorFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    IosSimulatorFactory();
};

} // Ios::Internal

Q_DECLARE_METATYPE(Ios::Internal::IosDeviceType)
