/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDDEVICE_H
#define ANDROIDDEVICE_H

#include <projectexplorer/devicesupport/idevice.h>

namespace Android {
class AndroidConfigurations; // needed for friend declaration

namespace Internal {

class AndroidDevice : public ProjectExplorer::IDevice
{
public:
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const;

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent = 0);
    bool canAutoDetectPorts() const;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const;

    ProjectExplorer::IDevice::Ptr clone() const;
    QString qmlProfilerHost() const;

protected:
    friend class AndroidDeviceFactory;
    friend class Android::AndroidConfigurations;
    AndroidDevice();
    AndroidDevice(const AndroidDevice &other);
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEVICE_H
