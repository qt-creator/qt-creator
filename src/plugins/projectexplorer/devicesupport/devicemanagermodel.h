// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "idevicefwd.h"

#include <QAbstractListModel>

#include <memory>

namespace Utils { class Id; }

namespace ProjectExplorer {
namespace Internal { class DeviceManagerModelPrivate; }
class IDevice;
class DeviceManager;

class PROJECTEXPLORER_EXPORT DeviceManagerModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit DeviceManagerModel(const DeviceManager *deviceManager, QObject *parent = nullptr);
    ~DeviceManagerModel() override;

    void setFilter(const QList<Utils::Id> &filter);
    void setTypeFilter(Utils::Id type);

    IDeviceConstPtr device(int pos) const;
    Utils::Id deviceId(int pos) const;
    int indexOf(IDeviceConstPtr dev) const;
    int indexForId(Utils::Id id) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void updateDevice(Utils::Id id);

private:
    void handleDeviceAdded(Utils::Id id);
    void handleDeviceRemoved(Utils::Id id);
    void handleDeviceUpdated(Utils::Id id);
    void handleDeviceListChanged();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool matchesTypeFilter(const IDeviceConstPtr &dev) const;

    const std::unique_ptr<Internal::DeviceManagerModelPrivate> d;
};

} // namespace ProjectExplorer
