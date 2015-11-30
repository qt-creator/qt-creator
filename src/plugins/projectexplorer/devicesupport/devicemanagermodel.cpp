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
#include "devicemanagermodel.h"

#include "devicemanager.h"

#include <utils/qtcassert.h>

#include <QString>

namespace ProjectExplorer {
namespace Internal {
class DeviceManagerModelPrivate
{
public:
    const DeviceManager *deviceManager;
    QList<IDevice::ConstPtr> devices;
    QList<Core::Id> filter;
    Core::Id typeToKeep;
};
} // namespace Internal

DeviceManagerModel::DeviceManagerModel(const DeviceManager *deviceManager, QObject *parent) :
    QAbstractListModel(parent), d(new Internal::DeviceManagerModelPrivate)
{
    d->deviceManager = deviceManager;
    handleDeviceListChanged();
    connect(deviceManager, SIGNAL(deviceAdded(Core::Id)), SLOT(handleDeviceAdded(Core::Id)));
    connect(deviceManager, SIGNAL(deviceRemoved(Core::Id)), SLOT(handleDeviceRemoved(Core::Id)));
    connect(deviceManager, SIGNAL(deviceUpdated(Core::Id)), SLOT(handleDeviceUpdated(Core::Id)));
    connect(deviceManager, SIGNAL(deviceListReplaced()), SLOT(handleDeviceListChanged()));
}

DeviceManagerModel::~DeviceManagerModel()
{
    delete d;
}

void DeviceManagerModel::setFilter(const QList<Core::Id> &filter)
{
    d->filter = filter;
    handleDeviceListChanged();
}

void DeviceManagerModel::setTypeFilter(Core::Id type)
{
    if (d->typeToKeep == type)
        return;
    d->typeToKeep = type;
    handleDeviceListChanged();
}

void DeviceManagerModel::updateDevice(Core::Id id)
{
    handleDeviceUpdated(id);
}

IDevice::ConstPtr DeviceManagerModel::device(int pos) const
{
    if (pos < 0 || pos >= d->devices.count())
        return IDevice::ConstPtr();
    return d->devices.at(pos);
}

Core::Id DeviceManagerModel::deviceId(int pos) const
{
    IDevice::ConstPtr dev = device(pos);
    return dev ? dev->id() : Core::Id();
}

int DeviceManagerModel::indexOf(IDevice::ConstPtr dev) const
{
    if (dev.isNull())
        return -1;
    for (int i = 0; i < d->devices.count(); ++i) {
        IDevice::ConstPtr current = d->devices.at(i);
        if (current->id() == dev->id())
            return i;
    }
    return -1;
}

void DeviceManagerModel::handleDeviceAdded(Core::Id id)
{
    if (d->filter.contains(id))
        return;
    IDevice::ConstPtr dev = d->deviceManager->find(id);
    if (!matchesTypeFilter(dev))
        return;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    d->devices << dev;
    endInsertRows();
}

void DeviceManagerModel::handleDeviceRemoved(Core::Id id)
{
    const int idx = indexForId(id);
    QTC_ASSERT(idx != -1, return);
    beginRemoveRows(QModelIndex(), idx, idx);
    d->devices.removeAt(idx);
    endRemoveRows();
}

void DeviceManagerModel::handleDeviceUpdated(Core::Id id)
{
    const int idx = indexForId(id);
    if (idx < 0) // This occurs when a device not matching the type filter is updated
        return;
    d->devices[idx] = d->deviceManager->find(id);
    const QModelIndex changedIndex = index(idx, 0);
    emit dataChanged(changedIndex, changedIndex);
}

void DeviceManagerModel::handleDeviceListChanged()
{
    beginResetModel();
    d->devices.clear();

    for (int i = 0; i < d->deviceManager->deviceCount(); ++i) {
        IDevice::ConstPtr dev = d->deviceManager->deviceAt(i);
        if (d->filter.contains(dev->id()))
            continue;
        if (!matchesTypeFilter(dev))
            continue;
        d->devices << dev;
    }
    endResetModel();
}

int DeviceManagerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return d->devices.count();
}

QVariant DeviceManagerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();
    if (role != Qt::DisplayRole && role != Qt::UserRole)
        return QVariant();
    const IDevice::ConstPtr dev = device(index.row());
    if (role == Qt::UserRole)
        return dev->id().toSetting();
    QString name;
    if (d->deviceManager->defaultDevice(dev->type()) == dev)
        name = tr("%1 (default for %2)").arg(dev->displayName(), dev->displayType());
    else
        name = dev->displayName();
    return name;
}

bool DeviceManagerModel::matchesTypeFilter(const IDevice::ConstPtr &dev) const
{
    return !d->typeToKeep.isValid() || dev->type() == d->typeToKeep;
}

int DeviceManagerModel::indexForId(Core::Id id) const
{
    for (int i = 0; i < d->devices.count(); ++i) {
        if (d->devices.at(i)->id() == id)
            return i;
    }

    return -1;
}

} // namespace ProjectExplorer
