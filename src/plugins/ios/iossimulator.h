/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/fileutils.h>

#include <QDebug>

namespace Ios {
namespace Internal {

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

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

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
    Q_DECLARE_TR_FUNCTIONS(Ios::Internal::IosSimulator)

public:
    using ConstPtr = QSharedPointer<const IosSimulator>;
    using Ptr = QSharedPointer<IosSimulator>;
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;

    ProjectExplorer::IDeviceWidget *createWidget() override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    Utils::Port nextPort() const;
    bool canAutoDetectPorts() const override;

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

} // namespace Internal
} // namespace Ios

Q_DECLARE_METATYPE(Ios::Internal::IosDeviceType)
