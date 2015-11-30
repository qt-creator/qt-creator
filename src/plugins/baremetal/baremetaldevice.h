/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2015 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BAREMETALDEVICE_H
#define BAREMETALDEVICE_H

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

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent);
    ProjectExplorer::IDevice::Ptr clone() const;

    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const;

    bool canCreateProcess() const { return true; }
    ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const;

    QString gdbServerProviderId() const;
    void setGdbServerProviderId(const QString &id);

    virtual void fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

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

#endif // BAREMETALDEVICE_H
