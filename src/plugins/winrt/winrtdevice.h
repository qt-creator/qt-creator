/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef WINRTDEVICE_H
#define WINRTDEVICE_H

#include <QList>
#include <QUuid>
#include <QSharedPointer>

#include <projectexplorer/devicesupport/idevice.h>

namespace Debugger {
    class DebuggerStartParameters;
} // Debugger

namespace WinRt {
namespace Internal {

class WinRtDevice : public ProjectExplorer::IDevice
{
    friend class WinRtDeviceFactory;
public:
    typedef QSharedPointer<WinRtDevice> Ptr;
    typedef QSharedPointer<const WinRtDevice> ConstPtr;

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent);
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const;
    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;
    ProjectExplorer::IDevice::Ptr clone() const;

    static QString displayNameForType(Core::Id type);
    int deviceId() const { return m_deviceId; }

protected:
    WinRtDevice();
    WinRtDevice(Core::Id type, MachineType machineType, Core::Id internalId, int deviceId);
    WinRtDevice(const WinRtDevice &other);

private:
    int m_deviceId;
};

} // Internal
} // WinRt

#endif // WINRTDEVICE_H
