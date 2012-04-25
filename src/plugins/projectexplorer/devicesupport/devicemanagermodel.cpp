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
#include "devicemanagermodel.h"

#include "devicemanager.h"

#include <QString>

namespace ProjectExplorer {
namespace Internal {
class DeviceManagerModelPrivate
{
public:
    const DeviceManager *deviceManager;
    QList<IDevice::ConstPtr> devices;
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
    connect(deviceManager, SIGNAL(deviceListChanged()), SLOT(handleDeviceListChanged()));
}

DeviceManagerModel::~DeviceManagerModel()
{
    delete d;
}

void DeviceManagerModel::updateDevice(Core::Id id)
{
    handleDeviceUpdated(id);
}

void DeviceManagerModel::handleDeviceAdded(Core::Id id)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    d->devices << d->deviceManager->find(id);
    endInsertRows();
}

void DeviceManagerModel::handleDeviceRemoved(Core::Id id)
{
    const int idx = indexForId(id);
    beginRemoveRows(QModelIndex(), idx, idx);
    d->devices.removeAt(idx);
    endRemoveRows();
}

void DeviceManagerModel::handleDeviceUpdated(Core::Id id)
{
    const int idx = indexForId(id);
    d->devices[idx] = d->deviceManager->deviceAt(idx);
    const QModelIndex changedIndex = index(idx, 0);
    emit dataChanged(changedIndex, changedIndex);
}

void DeviceManagerModel::handleDeviceListChanged()
{
    beginResetModel();
    d->devices.clear();
    for (int i = 0; i < d->deviceManager->deviceCount(); ++i)
        d->devices << d->deviceManager->deviceAt(i);
    endResetModel();
}

int DeviceManagerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return d->devices.count();
}

QVariant DeviceManagerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const IDevice::ConstPtr device = d->devices.at(index.row());
    QString name = device->displayName();
    if (d->deviceManager->defaultDevice(device->type()) == device)
        name = tr("%1 (default for %2)").arg(name, device->displayType());
    return name;
}

int DeviceManagerModel::indexForId(Core::Id id) const
{
    for (int i = 0; i < d->devices.count(); ++i) {
        if (d->devices.at(i)->id() == id)
            return i;
    }

    qWarning("%s: Invalid id %s.", Q_FUNC_INFO, qPrintable(id.toString()));
    return -1;
}

} // namespace ProjectExplorer
