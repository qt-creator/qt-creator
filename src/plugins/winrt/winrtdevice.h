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

namespace WinRt {
namespace Internal {

class WinRtDevice : public ProjectExplorer::IDevice
{
    friend class WinRtDeviceFactory;
public:
    typedef QSharedPointer<WinRtDevice> Ptr;
    typedef QSharedPointer<const WinRtDevice> ConstPtr;

    QString displayType() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;
    QList<Core::Id> actionIds() const override;
    QString displayNameForActionId(Core::Id actionId) const override;
    void executeAction(Core::Id actionId, QWidget *parent) override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    Utils::OsType osType() const override;
    ProjectExplorer::IDevice::Ptr clone() const override;

    static QString displayNameForType(Core::Id type);
    int deviceId() const { return m_deviceId; }

protected:
    WinRtDevice();
    WinRtDevice(Core::Id type, MachineType machineType, Core::Id internalId, int deviceId);
    WinRtDevice(const WinRtDevice &other);

private:
    void initFreePorts();

private:
    int m_deviceId;
};

} // Internal
} // WinRt
