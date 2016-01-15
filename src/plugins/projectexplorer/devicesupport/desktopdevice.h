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

#ifndef DESKTOPDEVICE_H
#define DESKTOPDEVICE_H

#include "../projectexplorer_export.h"

#include "idevice.h"
#include "idevicefactory.h"

namespace ProjectExplorer {
class ProjectExplorerPlugin;

namespace Internal { class DesktopDeviceFactory; }

class PROJECTEXPLORER_EXPORT DesktopDevice : public IDevice
{
public:
    IDevice::DeviceInfo deviceInformation() const;

    QString displayType() const;
    IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent = 0);
    bool canAutoDetectPorts() const;
    bool canCreateProcessModel() const;
    DeviceProcessList *createProcessListModel(QObject *parent) const;
    bool canCreateProcess() const { return true; }
    DeviceProcess *createProcess(QObject *parent) const;
    DeviceProcessSignalOperation::Ptr signalOperation() const;
    QString qmlProfilerHost() const;

    IDevice::Ptr clone() const;

protected:
    DesktopDevice();
    DesktopDevice(const DesktopDevice &other);

    friend class ProjectExplorerPlugin;
    friend class Internal::DesktopDeviceFactory;
};

} // namespace ProjectExplorer

#endif // DESKTOPDEVICE_H
