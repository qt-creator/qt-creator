/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

namespace BareMetal {
namespace Internal {

class BareMetalDevice : public ProjectExplorer::IDevice
{
public:
    typedef QSharedPointer<BareMetalDevice> Ptr;
    typedef QSharedPointer<const BareMetalDevice> ConstPtr;

    static Ptr create();
    static Ptr create(const QString &name, Core::Id type, MachineType machineType,
                      Origin origin = ManuallyAdded, Core::Id id = Core::Id());
    static Ptr create(const BareMetalDevice &other);

    QString displayType() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;
    QList<Core::Id> actionIds() const override;
    QString displayNameForActionId(Core::Id actionId) const override;
    void executeAction(Core::Id actionId, QWidget *parent) override;
    ProjectExplorer::IDevice::Ptr clone() const override;

    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;

    bool canCreateProcess() const override { return true; }
    ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const override;

    QString gdbServerProviderId() const;
    void setGdbServerProviderId(const QString &id);

    virtual void fromMap(const QVariantMap &map) override;
    virtual QVariantMap toMap() const override;

protected:
    BareMetalDevice() {}
    BareMetalDevice(const QString &name, Core::Id type,
                    MachineType machineType, Origin origin, Core::Id id);
    BareMetalDevice(const BareMetalDevice &other);

private:
    BareMetalDevice &operator=(const BareMetalDevice &);
    QString m_gdbServerProviderId;
};

} //namespace Internal
} //namespace BareMetal
