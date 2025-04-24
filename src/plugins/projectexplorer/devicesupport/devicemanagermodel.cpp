// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicemanagermodel.h"

#include "devicemanager.h"
#include "../kitaspect.h"
#include "../projectexplorertr.h"

#include <utils/qtcassert.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class DeviceManagerModelPrivate
{
public:
    QList<IDevice::Ptr> devices;
    QList<Id> filter;
    Id typeToKeep;
};

} // namespace Internal

DeviceManagerModel::DeviceManagerModel(QObject *parent)
    : QAbstractListModel(parent), d(std::make_unique<Internal::DeviceManagerModelPrivate>())
{
    handleDeviceListChanged();
    connect(DeviceManager::instance(), &DeviceManager::deviceAdded,
            this, &DeviceManagerModel::handleDeviceAdded);
    connect(DeviceManager::instance(), &DeviceManager::deviceRemoved,
            this, &DeviceManagerModel::handleDeviceRemoved);
    connect(DeviceManager::instance(), &DeviceManager::deviceUpdated,
            this, &DeviceManagerModel::handleDeviceUpdated);
}

DeviceManagerModel::~DeviceManagerModel() = default;

void DeviceManagerModel::setFilter(const QList<Id> &filter)
{
    d->filter = filter;
    handleDeviceListChanged();
}

void DeviceManagerModel::setTypeFilter(Id type)
{
    if (d->typeToKeep == type)
        return;
    d->typeToKeep = type;
    handleDeviceListChanged();
}

void DeviceManagerModel::updateDevice(Id id)
{
    handleDeviceUpdated(id);
}

IDevice::Ptr DeviceManagerModel::device(int pos) const
{
    if (pos < 0 || pos >= d->devices.count())
        return nullptr;
    return d->devices.at(pos);
}

Id DeviceManagerModel::deviceId(int pos) const
{
    IDevice::ConstPtr dev = device(pos);
    return dev ? dev->id() : Id();
}

int DeviceManagerModel::indexOf(IDevice::ConstPtr dev) const
{
    if (!dev)
        return -1;
    for (int i = 0; i < d->devices.count(); ++i) {
        IDevice::ConstPtr current = d->devices.at(i);
        if (current->id() == dev->id())
            return i;
    }
    return -1;
}

void DeviceManagerModel::handleDeviceAdded(Id id)
{
    if (d->filter.contains(id))
        return;
    IDevice::Ptr dev = DeviceManager::find(id);
    if (!matchesTypeFilter(dev))
        return;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    d->devices << dev;
    endInsertRows();
}

void DeviceManagerModel::handleDeviceRemoved(Id id)
{
    const int idx = indexForId(id);
    QTC_ASSERT(idx != -1, return);
    beginRemoveRows(QModelIndex(), idx, idx);
    d->devices.removeAt(idx);
    endRemoveRows();
}

void DeviceManagerModel::handleDeviceUpdated(Id id)
{
    const int idx = indexForId(id);
    if (idx < 0) // This occurs when a device not matching the type filter is updated
        return;
    d->devices[idx] = DeviceManager::find(id);
    const QModelIndex changedIndex = index(idx, 0);
    emit dataChanged(changedIndex, changedIndex);
}

void DeviceManagerModel::handleDeviceListChanged()
{
    beginResetModel();
    d->devices.clear();

    for (int i = 0; i < DeviceManager::deviceCount(); ++i) {
        IDevice::Ptr dev = DeviceManager::deviceAt(i);
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
    Q_UNUSED(parent)
    return d->devices.count();
}

QVariant DeviceManagerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return {};
    const IDevice::ConstPtr dev = device(index.row());
    switch (role) {
    case Qt::DecorationRole:
        return dev->deviceStateIcon();
    case Qt::UserRole: // TODO: Any callers?
    case KitAspect::IdRole:
        return dev->id().toSetting();
    case Qt::DisplayRole:
        if (DeviceManager::defaultDevice(dev->type()) == dev)
            return Tr::tr("%1 (default for %2)").arg(dev->displayName(), dev->displayType());
        return dev->displayName();
    }
    return {};
}

bool DeviceManagerModel::matchesTypeFilter(const IDevice::ConstPtr &dev) const
{
    return !d->typeToKeep.isValid() || dev->type() == d->typeToKeep;
}

int DeviceManagerModel::indexForId(Id id) const
{
    for (int i = 0; i < d->devices.count(); ++i) {
        if (d->devices.at(i)->id() == id)
            return i;
    }

    return -1;
}

} // namespace ProjectExplorer
