/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "typespecificdeviceconfigurationlistmodel.h"

#include "abstractembeddedlinuxtarget.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

TypeSpecificDeviceConfigurationListModel::TypeSpecificDeviceConfigurationListModel(AbstractEmbeddedLinuxTarget *target)
    : QAbstractListModel(target)
{
    const DeviceManager * const devConfs = DeviceManager::instance();
    connect(devConfs, SIGNAL(updated()), this, SIGNAL(modelReset()));
    connect(target, SIGNAL(supportedDevicesChanged()), this, SIGNAL(modelReset()));
}

TypeSpecificDeviceConfigurationListModel::~TypeSpecificDeviceConfigurationListModel()
{
}

int TypeSpecificDeviceConfigurationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int count = 0;
    const DeviceManager * const devConfs = DeviceManager::instance();
    const int devConfsCount = devConfs->deviceCount();
    for (int i = 0; i < devConfsCount; ++i) {
        if (target()->supportsDevice(devConfs->deviceAt(i)))
            ++count;
    }
    return count;
}

QVariant TypeSpecificDeviceConfigurationListModel::data(const QModelIndex &index,
    int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const LinuxDeviceConfiguration::ConstPtr &devConf = deviceAt(index.row());
    Q_ASSERT(devConf);
    QString displayedName = devConf->displayName();
    if (target()->supportsDevice(devConf) && DeviceManager::instance()
            ->defaultDevice(devConf->type()) == devConf) {
        displayedName = tr("%1 (default)").arg(displayedName);
    }
    return displayedName;
}

LinuxDeviceConfiguration::ConstPtr TypeSpecificDeviceConfigurationListModel::deviceAt(int idx) const
{
    int currentRow = -1;
    const DeviceManager * const devConfs = DeviceManager::instance();
    const int devConfsCount = devConfs->deviceCount();
    for (int i = 0; i < devConfsCount; ++i) {
        const IDevice::ConstPtr device = devConfs->deviceAt(i);
        if (target()->supportsDevice(device) && ++currentRow == idx)
            return device.staticCast<const LinuxDeviceConfiguration>();
    }
    QTC_CHECK(false);
    return LinuxDeviceConfiguration::ConstPtr();
}

LinuxDeviceConfiguration::ConstPtr TypeSpecificDeviceConfigurationListModel::defaultDeviceConfig() const
{
    const DeviceManager * const deviceManager = DeviceManager::instance();
    const int deviceCount = deviceManager->deviceCount();
    for (int i = 0; i < deviceCount; ++i) {
        const IDevice::ConstPtr device = deviceManager->deviceAt(i);
        if (target()->supportsDevice(device)
                && deviceManager->defaultDevice(device->type()) == device) {
            return device.staticCast<const LinuxDeviceConfiguration>();
        }
    }
    return LinuxDeviceConfiguration::ConstPtr();
}

LinuxDeviceConfiguration::ConstPtr TypeSpecificDeviceConfigurationListModel::find(Core::Id id) const
{
    const IDevice::ConstPtr &devConf = DeviceManager::instance()->find(id);
    if (devConf && target()->supportsDevice(devConf))
        return devConf.staticCast<const LinuxDeviceConfiguration>();
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

AbstractEmbeddedLinuxTarget *TypeSpecificDeviceConfigurationListModel::target() const
{
    return qobject_cast<AbstractEmbeddedLinuxTarget *>(QObject::parent());
}

} // namespace Internal
} // namespace RemoteLinux
