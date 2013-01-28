/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "typespecificdeviceconfigurationlistmodel.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

TypeSpecificDeviceConfigurationListModel::TypeSpecificDeviceConfigurationListModel(Target *target)
    : QAbstractListModel(target)
{
    const DeviceManager * const devConfs = DeviceManager::instance();
    connect(devConfs, SIGNAL(updated()), this, SIGNAL(modelReset()));
    connect(target, SIGNAL(kitChanged()), this, SIGNAL(modelReset()));
}

int TypeSpecificDeviceConfigurationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int count = 0;
    const DeviceManager * const devConfs = DeviceManager::instance();
    const int devConfsCount = devConfs->deviceCount();
    for (int i = 0; i < devConfsCount; ++i) {
        if (deviceMatches(devConfs->deviceAt(i)))
            ++count;
    }
    return count;
}

QVariant TypeSpecificDeviceConfigurationListModel::data(const QModelIndex &index,
    int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const IDevice::ConstPtr &device = deviceAt(index.row());
    Q_ASSERT(device);
    QString displayedName = device->displayName();
    if (deviceMatches(device)
            && DeviceManager::instance()->defaultDevice(device->type()) == device) {
        displayedName = tr("%1 (default)").arg(displayedName);
    }
    return displayedName;
}

IDevice::ConstPtr TypeSpecificDeviceConfigurationListModel::deviceAt(int idx) const
{
    int currentRow = -1;
    const DeviceManager * const devConfs = DeviceManager::instance();
    const int devConfsCount = devConfs->deviceCount();
    for (int i = 0; i < devConfsCount; ++i) {
        const IDevice::ConstPtr device = devConfs->deviceAt(i);
        if (deviceMatches(device) && ++currentRow == idx)
            return device;
    }
    QTC_CHECK(false);
    return IDevice::ConstPtr();
}

IDevice::ConstPtr TypeSpecificDeviceConfigurationListModel::defaultDeviceConfig() const
{
    const DeviceManager * const deviceManager = DeviceManager::instance();
    const int deviceCount = deviceManager->deviceCount();
    for (int i = 0; i < deviceCount; ++i) {
        const IDevice::ConstPtr device = deviceManager->deviceAt(i);
        if (deviceMatches(device)
                && deviceManager->defaultDevice(device->type()) == device) {
            return device;
        }
    }
    return IDevice::ConstPtr();
}

IDevice::ConstPtr TypeSpecificDeviceConfigurationListModel::find(Core::Id id) const
{
    const IDevice::ConstPtr &device = DeviceManager::instance()->find(id);
    if (deviceMatches(device))
        return device;
    return defaultDeviceConfig();
}

int TypeSpecificDeviceConfigurationListModel::indexForId(Core::Id id) const
{
    const int count = rowCount();
    for (int i = 0; i < count; ++i) {
        if (deviceAt(i)->id() == id)
            return i;
    }
    return -1;
}

Target *TypeSpecificDeviceConfigurationListModel::target() const
{
    return qobject_cast<Target *>(QObject::parent());
}

bool TypeSpecificDeviceConfigurationListModel::deviceMatches(IDevice::ConstPtr dev) const
{
    if (dev.isNull())
        return false;
    Core::Id typeId = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    return dev->type() == typeId;
}

} // namespace Internal
} // namespace RemoteLinux
